/**********************************************
* Copyright 2022 Nokia
* Licensed under the BSD 3-Clause Clear License
* SPDX-License-Identifier: BSD-3-Clause-Clear
**********************************************/

#pragma once

#include <memory>

#include <pdcp_layer/packet_handler.h>
#include <traffic_models/traffic_model.h>

class simulated_packet_handler : public packet_handler
{
public:
    simulated_packet_handler(int ue_id, traffic_config traffic_c, pdcp_config pdcp_c, int verbosity = 0);

    float ingest(int tx_dir, float current_t) override;

private:
    std::unique_ptr<traffic_model> traffic_m;
    int current_id = 0;
};
