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
    simulated_packet_handler(int ue_id, traffic_config traffic_c, pdcp_config pdcp_c, int verbosity = 0);

    float ingest(int tx_dir, float current_t) override;
    void drop(harq_pkt pkt) override;
    float release() override;

private:
    struct pending_packet_result
    {
        float original_size = 0.0f;
        float accounted_bits = 0.0f;
        bool dropped = false;
        bool congestion_signal = false;
    };

    void update_pending_packet(const ip_pkt& pkt, bool dropped);

private:
    std::unique_ptr<traffic_model> traffic_m;
    int current_id = 0;
    std::unordered_map<uint32_t, pending_packet_result> pending_results;
};
