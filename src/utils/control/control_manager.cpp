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
#include <utils/control/control_manager.h>
#include <utils/control/transport_file.h>
#include <utils/terminal_logging.h>

namespace
{
// One TTI of simulated time. Deliberately the nominal value and not the wall clock, so
// that a rate cap behaves identically in fast mode and in real time.
const float TTI_S = 0.001f;

bool as_double(const param_value &v, double &out)
{
    if (const double *d = std::get_if<double>(&v)) { out = *d; return true; }
    if (const bool *b = std::get_if<bool>(&v)) { out = *b ? 1.0 : 0.0; return true; }
    return false;
}

bool as_bool(const param_value &v, bool &out)
{
    if (const bool *b = std::get_if<bool>(&v)) { out = *b; return true; }
    if (const double *d = std::get_if<double>(&v)) { out = (*d != 0.0); return true; }
    return false;
}
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
    else
    {
        LOG_ERROR_I("control_manager::init") << " transport not available yet: " << cfg.transport << END();
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

    for (size_t i = 0; i < c.sets.size(); i++)
    {
        const std::string &key = c.sets[i].first;
        double d = 0.0;
        bool b = false;

        if (key == "priority")
        {
            if (!as_double(c.sets[i].second, d) || d < 0.0)
            {
                ack_error e; e.key = key; e.reason = "expected a number >= 0";
                a.errors.push_back(e);
            }
        }
        else if (key == "enabled")
        {
            if (!as_bool(c.sets[i].second, b))
            {
                ack_error e; e.key = key; e.reason = "expected a boolean";
                a.errors.push_back(e);
            }
        }
        else
        {
            ack_error e; e.key = key; e.reason = "unknown";
            a.errors.push_back(e);
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

    if (c.op != command_op::set)
    {
        ack_error e; e.key = "op"; e.reason = "only set is available in this build";
        a.errors.push_back(e);
        a.ok = false;
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

    for (size_t t = 0; t < targets.size(); t++)
    {
        ue *u = targets[t];
        for (size_t i = 0; i < c.sets.size(); i++)
        {
            const std::string &key = c.sets[i].first;
            if (key == "priority")
            {
                double d = 0.0;
                as_double(c.sets[i].second, d);
                u->overrides().priority = (float)d;

                if (!warned_rr_priority_ && metric_type_ == METRIC_RR)
                {
                    warned_rr_priority_ = true;
                    LOG_WARNING_I("control_manager::apply")
                        << " priority set while metric_type is round robin: the scheduler"
                        << " ignores priority under RR, so this knob will have no effect"
                        << END();
                }
            }
            else if (key == "enabled")
            {
                bool b = true;
                as_bool(c.sets[i].second, b);
                u->set_enabled(b);
                ranks_dirty_ = true;
            }
        }
    }

    a.ok = true;
    transport_->reply(a);
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
