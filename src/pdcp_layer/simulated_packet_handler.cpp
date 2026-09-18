/**********************************************
* Copyright 2022 Nokia
* Licensed under the BSD 3-Clause Clear License
* SPDX-License-Identifier: BSD-3-Clause-Clear
**********************************************/

#include <cmath>

#include <pdcp_layer/simulated_packet_handler.h>

simulated_packet_handler::simulated_packet_handler(int ue_id, traffic_config traffic_c, pdcp_config pdcp_c, unsigned int seed, int verbosity)
    : packet_handler(pdcp_c, seed, verbosity),
      traffic_m(new traffic_model(ue_id, traffic_c))
{
}

bool simulated_packet_handler::set_traffic_target(int tx_dir, float bps)
{
    traffic_m->set_target(tx_dir, bps);
    return true;
}

bool simulated_packet_handler::get_traffic_target(int tx_dir, float &bps) const
{
    bps = traffic_m->get_target(tx_dir);
    return true;
}

bool simulated_packet_handler::inject_bits(float bits, std::uint32_t tag)
{
    if(bits <= 0.0f) return true;
    for(size_t i = 0; i < pending_injections_.size(); i++)
    {
        if(pending_injections_[i].first == tag)
        {
            pending_injections_[i].second += bits;
            injected_bits_total_ += bits;
            return true;
        }
    }
    pending_injections_.push_back(std::make_pair(tag, bits));
    injected_bits_total_ += bits;
    if(tag != 0) objects_[tag];
    return true;
}

void simulated_packet_handler::packetize(float bits, float current_t, std::uint32_t tag)
{
    const float pkt_size = traffic_m->get_pkt_size(0);
    const int pkts = (int)ceil(bits / pkt_size);
    for(int i = 0; i < pkts - 1; i++)
    {
        ip_pkt pkt(current_t, pkt_size, pkt_size, current_id, bh_d, bh_d_var);
        pkt.tag = tag;
        push_ingress_pkt(std::move(pkt));
        current_id++;
    }

    const float bits_left = bits - (pkts - 1) * pkt_size;
    if(bits_left > 0)
    {
        ip_pkt pkt(current_t, bits_left, bits_left, current_id, bh_d, bh_d_var);
        pkt.tag = tag;
        push_ingress_pkt(std::move(pkt));
        current_id++;
    }
}

float simulated_packet_handler::ingest(int tx_dir, float current_t)
{
    const float generated = traffic_m->generate(tx_dir, current_t);
    if(generated > 0) packetize(generated, current_t, 0);

    // Injected bits are packetized on their own, one object at a time, so that an
    // injection of N bytes always yields the same packets regardless of what the
    // generator produced in the same step and of what the other objects injected.
    // Injection adds to the configured traffic, it does not replace it.
    float injected = 0.0f;
    for(size_t i = 0; i < pending_injections_.size(); i++)
    {
        injected += pending_injections_[i].second;
        packetize(pending_injections_[i].second, current_t, pending_injections_[i].first);
    }
    pending_injections_.clear();

    return generated + injected;
}

void simulated_packet_handler::drop(harq_pkt pkt, bool expired)
{
    for(std::deque<ip_pkt>::const_iterator it = pkt.pkts.begin(); it != pkt.pkts.end(); ++it)
    {
        update_pending_packet(*it, expired ? bit_fate::expired : bit_fate::dropped);
    }
}

void simulated_packet_handler::drop_ingress_pkt(ip_pkt pkt)
{
    update_pending_packet(pkt, bit_fate::dropped);
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
                update_pending_packet(*pkt_it, bit_fate::delivered);
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
    delivered_bits_total_ += bits;
    return bits;
}

void simulated_packet_handler::update_pending_packet(const ip_pkt& pkt, bit_fate fate)
{
    // Per object accounting is per fragment: a packet half delivered and half dropped
    // contributes to both counters, and the three add up to what was injected. It does
    // not wait for the packet to be whole again, which is what the verdict below needs.
    if(pkt.tag != 0)
    {
        object_counters& o = objects_[pkt.tag];
        if(fate == bit_fate::delivered) o.delivered_bits += pkt.size;
        else if(fate == bit_fate::expired) o.expired_bits += pkt.size;
        else o.dropped_bits += pkt.size;
    }

    const bool dropped = fate != bit_fate::delivered;
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
