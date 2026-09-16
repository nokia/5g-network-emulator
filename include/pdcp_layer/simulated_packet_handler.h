/**********************************************
* Copyright 2022 Nokia
* Licensed under the BSD 3-Clause Clear License
* SPDX-License-Identifier: BSD-3-Clause-Clear
**********************************************/

#pragma once

#include <memory>
#include <unordered_map>

#include <pdcp_layer/packet_handler.h>
#include <traffic_models/traffic_model.h>

class simulated_packet_handler : public packet_handler
{
public:
    simulated_packet_handler(int ue_id, traffic_config traffic_c, pdcp_config pdcp_c, unsigned int seed, int verbosity = 0);

    float ingest(int tx_dir, float current_t) override;
    void drop(harq_pkt pkt) override;
    float release() override;
    bool set_traffic_target(int tx_dir, float bps) override;
    bool get_traffic_target(int tx_dir, float &bps) const override;
    bool inject_bits(float bits) override;
    float injected_bits_total() const override { return injected_bits_total_; }
    int get_pkt_size() const override { return traffic_m->get_pkt_size(0); }

private:
    struct pending_packet_result
    {
        float original_size = 0.0f;
        float accounted_bits = 0.0f;
        bool dropped = false;
        bool congestion_signal = false;
    };

    void update_pending_packet(const ip_pkt& pkt, bool dropped);
    void packetize(float bits, float current_t);

private:
    std::unique_ptr<traffic_model> traffic_m;
    // Bits handed over by the client and not yet turned into IP packets. They go out on
    // the next ingest, in full: the pacing is the client's job, not the emulator's.
    float pending_injected_bits_ = 0.0f;
    float injected_bits_total_ = 0.0f;
    int current_id = 0;
    std::unordered_map<uint32_t, pending_packet_result> pending_results;
};
