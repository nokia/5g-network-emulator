/**********************************************
* Copyright 2022 Nokia
* Licensed under the BSD 3-Clause Clear License
* SPDX-License-Identifier: BSD-3-Clause-Clear
**********************************************/

#pragma once

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <fstream>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <vector>

#include <utils/control/command.h>
#include <utils/control/control_config.h>
#include <utils/control/control_transport.h>

class ue;

//--------------------------------------------------------------------------------------------------
// control_manager(): the runtime control plane. Member of simulator, never a singleton.
//
// tick() is the only place where control state is written, and it runs as the first
// statement of simulator::step(), before mac_l.step(). At that instant both thread pools
// are parked on their condition variables (mac_layer::step and ue_handler::step both end
// in wait_threads()), so mutating UE state there needs no atomics on the mutated fields
// and no locks on the readers.
//
// Applying a command anywhere else -- from the transport thread, or from inside a
// parallel phase -- would race against grid_dl and grid_ul, which walk the same ue_list
// concurrently.
//--------------------------------------------------------------------------------------------------
class control_manager
{
public:
    control_manager();
    ~control_manager();

    // period_ms is the .ini value: > 0 means real time. metric_type is only used to warn
    // that priority does nothing under round robin.
    void init(const control_config &cfg, std::vector<ue> *ue_list, float period_ms, int metric_type);

    void tick(double sim_t, std::int64_t tti);

    void stop();

    bool is_enabled() const { return enabled_; }
    bool stop_requested() const { return stopping_; }
    std::int64_t credit_until_tti() const { return credit_until_tti_; }
    bool barrier_mode() const { return mode_ == mode_t::barrier; }

private:
    enum class mode_t { async, barrier };
    enum class on_timeout_t { cont, abort };

    void wait_for_credit(std::int64_t tti);
    void apply_grant(const command &c, ack &a);
    void write_journal(const command &c, double sim_t, std::int64_t tti);
    void drain_transport();
    void apply_due(double sim_t, std::int64_t tti);
    void apply(const command &c, double sim_t, std::int64_t tti);
    void refill_rate_buckets();

    // Validates every key of the message before touching anything, so that a ue/* with
    // one bad value does not leave the system half applied.
    bool validate(const command &c, std::vector<ue *> &targets, ack &a);
    bool resolve_target(const std::string &target, std::vector<ue *> &out, ack &a);
    std::string read_state(const std::vector<ue *> &targets) const;
    void warn_priority_under_rr();

private:
    struct scheduled
    {
        std::int64_t at_tti;
        std::uint64_t seq;   // keeps FIFO order among commands due at the same TTI
        command cmd;
    };
    struct scheduled_later
    {
        bool operator()(const scheduled &a, const scheduled &b) const
        {
            if (a.at_tti != b.at_tti) return a.at_tti > b.at_tti;
            return a.seq > b.seq;
        }
    };

private:
    bool enabled_ = false;
    std::vector<ue *> ue_index_;          // by ue id, stable after ue_handler::init()
    std::vector<ue> *ue_list_ = nullptr;
    std::unique_ptr<control_transport> transport_;
    bool transport_open_ = false;

    std::priority_queue<scheduled, std::vector<scheduled>, scheduled_later> sched_;
    std::uint64_t seq_ = 0;

    // Set as soon as a rate cap is configured on any UE, so that a run without caps does
    // not pay for the refill walk.
    bool any_rate_cap_ = false;
    bool ranks_dirty_ = false;

    mode_t mode_ = mode_t::async;
    on_timeout_t on_timeout_ = on_timeout_t::cont;
    std::chrono::milliseconds timeout_{30000};
    std::int64_t credit_until_tti_ = -1;
    bool stopping_ = false;
    std::mutex mtx_;
    std::condition_variable cv_;

    std::ofstream journal_;

    int max_cmds_per_tick_ = 256;
    bool warned_rr_priority_ = false;
    int metric_type_ = -1;
};
