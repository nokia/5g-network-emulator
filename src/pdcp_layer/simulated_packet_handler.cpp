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

void simulated_packet_handler::drop(harq_pkt pkt)
{
    for(std::deque<ip_pkt>::const_iterator it = pkt.pkts.begin(); it != pkt.pkts.end(); ++it)
    {
        update_pending_packet(*it, true);
    }
}

float simulated_packet_handler::release()
{
    int count = 0;
    float bits = 0.0f;
    float latency = 0.0f;
    float ip_latency = 0.0f;
    for(std::deque<harq_pkt>::iterator it = pkt_list.begin(); it != pkt_list.end();)
    {
        if(it->t_out <= current_t)
        {
            bits += it->bits;
            latency += current_t - it->current_t;
            ip_latency += current_t - it->ip_t;
            count++;
            for(std::deque<ip_pkt>::const_iterator pkt_it = it->pkts.begin(); pkt_it != it->pkts.end(); ++pkt_it)
            {
                update_pending_packet(*pkt_it, false);
            }
            it = pkt_list.erase(it);
        }
        else it++;
    }
    if(count > 0)
    {
        l_mean.add(latency/count);
        ipl_mean.add(ip_latency/count);
        l_mean.step();
        ipl_mean.step();
    }
    tp_mean.add(bits);
    return bits;
}

void simulated_packet_handler::update_pending_packet(const ip_pkt& pkt, bool dropped)
{
    pending_packet_result& state = pending_results[pkt.uid];
    if(state.original_size <= 0.0f) state.original_size = pkt.original_size;
    state.accounted_bits += pkt.size;
    state.dropped = state.dropped || dropped;
    state.congestion_signal = state.congestion_signal || pkt.ce_marked;

    if(state.original_size > 0.0f && state.accounted_bits + BIT_ROUND_MARGIN >= state.original_size)
    {
        if(state.dropped) verdict(pkt, final_packet_verdict::DROP);
        else if(state.congestion_signal) verdict(pkt, final_packet_verdict::ACCEPT_CE);
        else verdict(pkt, final_packet_verdict::ACCEPT);
        pending_results.erase(pkt.uid);
    }
}
