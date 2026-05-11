/**********************************************
* Copyright 2022 Nokia
* Licensed under the BSD 3-Clause Clear License
* SPDX-License-Identifier: BSD-3-Clause-Clear
**********************************************/

#include <cmath>

#include <pdcp_layer/simulated_packet_handler.h>

simulated_packet_handler::simulated_packet_handler(int ue_id, traffic_config traffic_c, pdcp_config pdcp_c, int verbosity)
    : packet_handler(pdcp_c, verbosity),
      traffic_m(new traffic_model(ue_id, traffic_c))
{
}

float simulated_packet_handler::ingest(int tx_dir, float current_t)
{
    float bits = traffic_m->generate(tx_dir, current_t);
    if(bits <= 0) return 0.0f;

    float pkt_size = traffic_m->get_pkt_size(tx_dir);
    int pkts = (int)ceil(bits / pkt_size);
    for(int i = 0; i < pkts - 1; i++)
    {
        push_ingress_pkt(ip_pkt(current_t, pkt_size, pkt_size, current_id, bh_d, bh_d_var));
        current_id++;
    }

    float bits_left = bits - (pkts - 1) * pkt_size;
    if(bits_left > 0)
    {
        push_ingress_pkt(ip_pkt(current_t, bits_left, bits_left, current_id, bh_d, bh_d_var));
        current_id++;
    }
    return bits;
}
