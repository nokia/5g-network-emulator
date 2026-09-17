/**********************************************
* Copyright 2022 Nokia
* Licensed under the BSD 3-Clause Clear License
* SPDX-License-Identifier: BSD-3-Clause-Clear
**********************************************/

#include <algorithm>
#include <cstdlib>

#include <mac_layer/mac_definitions.h>
#include <utils/monitoring/monitoring_manager.h>
#include <ue/ue.h>
#include <ue/ue_handler.h>
#include <nlohmann/json.hpp>
#include <utils/control/control_manager.h>
#include <utils/control/ndjson.h>
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

void control_manager::init(const control_config &cfg, std::vector<ue> *ue_list, const cell_info &cell)
{
    ue_list_ = ue_list;
    max_cmds_per_tick_ = cfg.max_cmds_per_tick;
    cell_ = cell;
    const float period_ms = cell.period_ms;

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

    transport_->set_grant_sink([this](const command &c, ack &a) { apply_grant(c, a); });

    // Barrier in real time would mean blocking the wall clock, which defeats the mode.
    mode_ = (cfg.sync_mode == "barrier") ? mode_t::barrier : mode_t::async;
    if (mode_ == mode_t::barrier && period_ms > 0)
    {
        mode_ = mode_t::async;
        LOG_WARNING_I("control_manager::init")
            << " sync_mode: barrier is not available with period > 0; degrading to async" << END();
    }
    // A file has no peer to grant credit, so a barrier over it would never advance.
    if (mode_ == mode_t::barrier && cfg.transport == "file")
    {
        mode_ = mode_t::async;
        LOG_WARNING_I("control_manager::init")
            << " sync_mode: barrier needs a socket transport; degrading to async" << END();
    }

    on_timeout_ = (cfg.on_timeout == "abort") ? on_timeout_t::abort : on_timeout_t::cont;
    timeout_ = std::chrono::milliseconds(cfg.credit_timeout_ms);

    if (cfg.journal_file != "none" && !cfg.journal_file.empty())
    {
        journal_.open(cfg.journal_file, std::ios::out | std::ios::trunc);
        if (!journal_.is_open())
            LOG_ERROR_I("control_manager::init") << " cannot open journal_file: " << cfg.journal_file << END();
    }

    transport_open_ = true;
    enabled_ = true;

    LOG_INFO_I("control_manager::init")
        << "Runtime control enabled"
        << " transport=" << cfg.transport
        << " sync_mode=" << (mode_ == mode_t::barrier ? "barrier" : "async")
        << " ues=" << ue_list_->size()
        << " realtime=" << (period_ms > 0 ? "yes" : "no")
        << END();
}

void control_manager::stop()
{
    {
        std::lock_guard<std::mutex> lk(mtx_);
        stopping_ = true;
    }
    cv_.notify_all();
    if (transport_) transport_->stop();
    transport_open_ = false;
    if (journal_.is_open()) journal_.close();
}

// Absolute and monotonic: until_tti is the last TTI the emulator may run. Credit never
// moves backwards, so a grant into the past is a no-op answered with ok and the credit in
// force, which is exactly what a client retrying after a timeout needs.
void control_manager::apply_grant(const command &c, ack &a)
{
    {
        std::lock_guard<std::mutex> lk(mtx_);
        if (c.until_tti > credit_until_tti_) credit_until_tti_ = c.until_tti;
        a.credit_until_tti = credit_until_tti_;
    }
    a.ok = true;
    cv_.notify_all();
}

void control_manager::wait_for_credit(std::int64_t tti)
{
    if (mode_ != mode_t::barrier) return;

    const std::chrono::steady_clock::time_point wait_start = std::chrono::steady_clock::now();
    std::unique_lock<std::mutex> lk(mtx_);
    const bool granted = cv_.wait_for(lk, timeout_, [&] {
        return tti <= credit_until_tti_ || stopping_
            || (transport_->peer_ever_connected() && !transport_->peer_alive());
    });

    const double waited_ms = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(
        std::chrono::steady_clock::now() - wait_start).count();
    if (waited_ms > 0.0)
    {
        blocked_ttis_++;
        blocked_ms_ += waited_ms;
    }

    if (stopping_) return;

    if (transport_->peer_ever_connected() && !transport_->peer_alive())
    {
        // Fail-open, and irreversible for the rest of the run: an unattended run that
        // loses its controller finishes instead of hanging, and does not pretend to be
        // synchronised again if someone reconnects.
        mode_ = mode_t::async;
        LOG_WARNING_I("control_manager")
            << " control peer lost at tti " << tti << "; degrading to async for the rest of the run" << END();
        return;
    }

    if (!granted && on_timeout_ == on_timeout_t::abort)
    {
        stopping_ = true;
        LOG_ERROR_I("control_manager")
            << " no credit for tti " << tti << " after " << timeout_.count() << " ms; aborting" << END();
    }
}

void control_manager::write_journal(const command &c, double sim_t, std::int64_t tti)
{
    if (!journal_.is_open()) return;

    const std::int64_t wall_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    // A journal line is a valid script line: replaying it by at_tti reproduces the
    // session step by step, with no conversion in between.
    journal_ << ndjson::serialize_journal_entry(c, sim_t, tti, wall_ns);
    journal_.flush();
}

void control_manager::tick(double sim_t, std::int64_t tti)
{
    if (!enabled_) return;

    wait_for_credit(tti);

    drain_transport();
    apply_due(sim_t, tti);

    if (ranks_dirty_)
    {
        ue_handler::refresh_enabled_ranks(*ue_list_);
        ranks_dirty_ = false;
    }

    refill_rate_buckets();
    publish_metrics(tti);
}

void control_manager::publish_metrics(std::int64_t tti)
{
    // Only when something happened: a point per TTI would put the aggregator's mutex on
    // the path of every step for no information at all.
    if (applied_in_tick_ == 0 && rejected_in_tick_ == 0 && blocked_ttis_ == 0) return;

    monitoring_manager &monitoring = monitoring_manager::instance();
    if (monitoring.is_enabled() && monitoring.get_config().emit_control)
    {
        metric_point point;
        point.measurement = "control";
        point.ts_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

        metric_field applied;   applied.value = applied_in_tick_;      applied.aggregation = field_aggregation::sum;
        metric_field rejected;  rejected.value = rejected_in_tick_;    rejected.aggregation = field_aggregation::sum;
        metric_field blocked;   blocked.value = blocked_ttis_;         blocked.aggregation = field_aggregation::sum;
        metric_field blocked_t; blocked_t.value = blocked_ms_;         blocked_t.aggregation = field_aggregation::sum;
        metric_field last_tti;  last_tti.value = (double)last_applied_tti_; last_tti.aggregation = field_aggregation::last;
        metric_field credit;    credit.value = (double)credit_until_tti_;   credit.aggregation = field_aggregation::last;

        point.fields["applied_sum"] = applied;
        point.fields["rejected_sum"] = rejected;
        point.fields["blocked_ttis_sum"] = blocked;
        point.fields["blocked_ms_sum"] = blocked_t;
        point.fields["last_applied_tti_last"] = last_tti;
        point.fields["credit_until_tti_last"] = credit;
        monitoring.publish(point);
    }

    (void)tti;
    applied_in_tick_ = 0;
    rejected_in_tick_ = 0;
    blocked_ttis_ = 0;
    blocked_ms_ = 0.0;
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
        write_journal(c, sim_t, tti);
        last_applied_tti_ = tti;
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
        if (c.target == "cell")
        {
            a.payload = read_cell_state();
            a.ok = true;
            transport_->reply(a);
            return;
        }

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
        rejected_in_tick_++;
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
    if (a.ok) applied_in_tick_++;
    else rejected_in_tick_++;
    transport_->reply(a);
}

void control_manager::warn_priority_under_rr()
{
    if (warned_rr_priority_ || cell_.metric_type != METRIC_RR) return;
    warned_rr_priority_ = true;
    LOG_WARNING_I("control_manager")
        << " priority was set while metric_type is round robin: get_metric derives to the"
        << " round robin rotation before applying priority, so the knob has no effect"
        << END();
}

namespace
{
// Everything a client driving its own injection needs in order to pace: what is still
// queued, what came out, and what was lost and why. Cumulative where it makes sense, so
// two reads can be diffed and a lost read costs nothing.
nlohmann::json direction_state(ue &u, int tx_dir)
{
    pdcp_layer &p = u.pdcp_state(tx_dir);
    const pdcp_queue_status q = p.get_queue_status();

    nlohmann::json j;
    j["injected_bytes_total"] = p.injected_bits_total() / 8.0;
    j["delivered_bytes_total"] = p.delivered_bits_total() / 8.0;
    j["expired_bytes_total"] = p.expired_bits_total() / 8.0;
    j["dropped_bytes_total"] = p.dropped_bits_total() / 8.0;
    j["ce_packets_total"] = p.ce_packets_total();
    j["pending_packets"] = q.ip_buffer_size;
    j["pending_bytes"] = p.pending_bits() / 8.0;
    j["oldest_age_s"] = q.ip_oldest_age;
    j["latency_s"] = p.get_latency(false);
    return j;
}
}

std::string control_manager::read_cell_state() const
{
    nlohmann::json j;
    j["scenario_type"] = cell_.scenario_type;
    j["frequency_hz"] = cell_.frequency_hz;
    j["bandwidth_hz"] = cell_.bandwidth_hz;
    j["numerology"] = cell_.numerology;
    j["n_freq_rbg"] = cell_.n_freq_rbg;
    j["metric_type"] = cell_.metric_type;
    j["period_ms"] = cell_.period_ms;
    j["duration_s"] = cell_.duration_s;
    j["map_file"] = cell_.map_file;
    j["realtime"] = cell_.period_ms > 0.0f;

    // Read live rather than stored: UEs come and go with the enabled knob.
    j["n_ues"] = (int)(ue_list_ != nullptr ? ue_list_->size() : 0);
    int enabled = 0;
    for (size_t i = 0; ue_list_ != nullptr && i < ue_list_->size(); i++)
        if ((*ue_list_)[i].is_enabled()) enabled++;
    j["n_ues_enabled"] = enabled;
    // The apothem is a property of the scenario map, held by every UE's MapHandler.
    j["apothem_m"] = (ue_list_ != nullptr && !ue_list_->empty()) ? (*ue_list_)[0].get_apothem() : 0.0f;

    return j.dump();
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

        nlohmann::json state;
        state["dl"] = direction_state(*targets[t], TX_DL);
        state["ul"] = direction_state(*targets[t], TX_UL);
        state["pkt_size_bits"] = targets[t]->get_pkt_size(TX_DL);
        j["state"] = state;

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
