/**********************************************
* Copyright 2022 Nokia
* Licensed under the BSD 3-Clause Clear License
* SPDX-License-Identifier: BSD-3-Clause-Clear
**********************************************/

#include <algorithm>
#include <cstdlib>

#include <mac_layer/mac_definitions.h>
#include <ue/ue.h>
#include <ue/ue_handler.h>
#include <nlohmann/json.hpp>
#include <utils/control/control_manager.h>
#include <utils/control/param_registry.h>
#include <utils/control/transport_file.h>
#include <utils/control/transport_socket.h>
#include <utils/terminal_logging.h>

namespace
{
// One TTI of simulated time. Deliberately the nominal value and not the wall clock, so
// that a rate cap behaves identically in fast mode and in real time.
const float TTI_S = 0.001f;

}

control_manager::control_manager() {}

control_manager::~control_manager()
{
    stop();
}

void control_manager::init(const control_config &cfg, std::vector<ue> *ue_list, float period_ms, int metric_type)
{
    ue_list_ = ue_list;
    max_cmds_per_tick_ = cfg.max_cmds_per_tick;
    metric_type_ = metric_type;

    if (!cfg.enabled || cfg.transport == "none")
    {
        enabled_ = false;
        return;
    }

    if (cfg.transport == "file")
    {
        if (cfg.timeline_file == "none" || cfg.timeline_file.empty())
        {
            LOG_ERROR_I("control_manager::init") << " transport: file needs a timeline_file" << END();
            enabled_ = false;
            return;
        }
        std::unique_ptr<transport_file> t(new transport_file(cfg.timeline_file));
        if (!t->ok())
        {
            enabled_ = false;
            return;
        }
        transport_.reset(t.release());
    }
    else if (cfg.transport == "unix" || cfg.transport == "tcp")
    {
        std::unique_ptr<transport_socket> t(new transport_socket(cfg));
        if (!t->ok())
        {
            enabled_ = false;
            return;
        }
        transport_.reset(t.release());
    }
    else
    {
        LOG_ERROR_I("control_manager::init") << " unknown transport: " << cfg.transport << END();
        enabled_ = false;
        return;
    }

    // Index by UE id. ue_handler::init() has already run, so both the vector and the
    // addresses inside it are stable for the rest of the run.
    ue_index_.clear();
    for (size_t i = 0; i < ue_list_->size(); i++)
    {
        ue &u = (*ue_list_)[i];
        const int id = u.get_id();
        if ((int)ue_index_.size() <= id) ue_index_.resize(id + 1, nullptr);
        ue_index_[id] = &u;
    }

    transport_open_ = true;
    enabled_ = true;

    LOG_INFO_I("control_manager::init")
        << "Runtime control enabled"
        << " transport=" << cfg.transport
        << " sync_mode=" << cfg.sync_mode
        << " ues=" << ue_list_->size()
        << " realtime=" << (period_ms > 0 ? "yes" : "no")
        << END();
}

void control_manager::stop()
{
    if (transport_) transport_->stop();
    transport_open_ = false;
}

void control_manager::tick(double sim_t, std::int64_t tti)
{
    if (!enabled_) return;

    drain_transport();
    apply_due(sim_t, tti);

    if (ranks_dirty_)
    {
        ue_handler::refresh_enabled_ranks(*ue_list_);
        ranks_dirty_ = false;
    }

    refill_rate_buckets();
}

void control_manager::drain_transport()
{
    if (!transport_open_) return;

    std::vector<command> batch;
    transport_open_ = transport_->poll(batch);

    for (size_t i = 0; i < batch.size(); i++)
    {
        scheduled s;
        s.at_tti = batch[i].at_tti;
        s.seq = seq_++;
        s.cmd = batch[i];
        sched_.push(s);
    }
}

void control_manager::apply_due(double sim_t, std::int64_t tti)
{
    int applied = 0;
    while (!sched_.empty() && sched_.top().at_tti <= tti && applied < max_cmds_per_tick_)
    {
        command c = sched_.top().cmd;
        sched_.pop();
        apply(c, sim_t, tti);
        applied++;
    }
}

bool control_manager::resolve_target(const std::string &target, std::vector<ue *> &out, ack &a)
{
    if (target == "ue/*")
    {
        for (size_t i = 0; i < ue_list_->size(); i++) out.push_back(&(*ue_list_)[i]);
        return true;
    }

    if (target.compare(0, 3, "ue/") == 0)
    {
        const int id = std::atoi(target.c_str() + 3);
        if (id < 0 || id >= (int)ue_index_.size() || ue_index_[id] == nullptr)
        {
            ack_error e; e.key = target; e.reason = "unknown ue";
            a.errors.push_back(e);
            return false;
        }
        out.push_back(ue_index_[id]);
        return true;
    }

    ack_error e; e.key = target; e.reason = "unknown target";
    a.errors.push_back(e);
    return false;
}

bool control_manager::validate(const command &c, std::vector<ue *> &targets, ack &a)
{
    if (!resolve_target(c.target, targets, a)) return false;

    // Whole message first, nothing applied yet: a ue/* carrying one bad value must not
    // leave half the UEs updated.
    const param_registry &reg = param_registry::instance();
    for (size_t i = 0; i < c.sets.size(); i++)
    {
        const param_entry *e = reg.find(c.sets[i].first);
        if (e == nullptr)
        {
            ack_error err; err.key = c.sets[i].first; err.reason = "unknown";
            a.errors.push_back(err);
            continue;
        }

        std::string reason;
        if (!reg.check(*e, c.sets[i].second, reason))
        {
            ack_error err; err.key = c.sets[i].first; err.reason = reason;
            a.errors.push_back(err);
        }
    }

    return a.errors.empty();
}

void control_manager::apply(const command &c, double sim_t, std::int64_t tti)
{
    ack a;
    a.id = c.id;
    a.tti = tti;
    a.t = sim_t;

    if (c.op == command_op::ping)
    {
        a.ok = true;
        transport_->reply(a);
        return;
    }

    if (c.op == command_op::describe)
    {
        a.ok = true;
        a.payload = param_registry::instance().describe_json();
        transport_->reply(a);
        return;
    }

    if (c.op == command_op::get)
    {
        std::vector<ue *> targets;
        if (!resolve_target(c.target, targets, a))
        {
            a.ok = false;
            transport_->reply(a);
            return;
        }
        a.payload = read_state(targets);
        a.ok = true;
        transport_->reply(a);
        return;
    }

    std::vector<ue *> targets;
    if (!validate(c, targets, a))
    {
        a.ok = false;
        transport_->reply(a);
        return;
    }

    const param_registry &reg = param_registry::instance();
    for (size_t t = 0; t < targets.size(); t++)
    {
        ue *u = targets[t];
        for (size_t i = 0; i < c.sets.size(); i++)
        {
            const param_entry *e = reg.find(c.sets[i].first);
            std::string reason;
            if (!e->apply(*u, c.sets[i].second, reason))
            {
                ack_error err; err.key = c.sets[i].first; err.reason = reason;
                a.errors.push_back(err);
                continue;
            }

            if (c.sets[i].first == "enabled") ranks_dirty_ = true;
            if (c.sets[i].first == "dl.rmax_mbps" || c.sets[i].first == "ul.rmax_mbps") any_rate_cap_ = true;
            if (c.sets[i].first == "priority") warn_priority_under_rr();
        }
    }

    a.ok = a.errors.empty();
    transport_->reply(a);
}

void control_manager::warn_priority_under_rr()
{
    if (warned_rr_priority_ || metric_type_ != METRIC_RR) return;
    warned_rr_priority_ = true;
    LOG_WARNING_I("control_manager")
        << " priority was set while metric_type is round robin: get_metric derives to the"
        << " round robin rotation before applying priority, so the knob has no effect"
        << END();
}

std::string control_manager::read_state(const std::vector<ue *> &targets) const
{
    const param_registry &reg = param_registry::instance();
    nlohmann::json out = nlohmann::json::array();
    for (size_t t = 0; t < targets.size(); t++)
    {
        nlohmann::json j;
        j["target"] = std::string("ue/") + std::to_string(targets[t]->get_id());
        const std::vector<param_entry> &entries = reg.entries();
        for (size_t i = 0; i < entries.size(); i++)
        {
            if (!entries[i].read) continue;
            if (entries[i].type == param_type::boolean) j[entries[i].name] = entries[i].read(*targets[t]) != 0.0;
            else j[entries[i].name] = entries[i].read(*targets[t]);
        }
        out.push_back(j);
    }
    return out.dump();
}

void control_manager::refill_rate_buckets()
{
    if (!any_rate_cap_) return;

    for (size_t i = 0; i < ue_list_->size(); i++)
    {
        ue_overrides &c = (*ue_list_)[i].overrides();
        for (int d = 0; d < 2; d++)
        {
            if (c.rmax_bps[d] <= 0.0f) continue;
            const float cap = c.rmax_bps[d] * TTI_S;
            c.rmax_tokens[d] = std::min(c.rmax_tokens[d] + cap, cap);
        }
    }
}
