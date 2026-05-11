/**********************************************
* Copyright 2022 Nokia
* Licensed under the BSD 3-Clause Clear License
* SPDX-License-Identifier: BSD-3-Clause-Clear
**********************************************/

#include <chrono>
#include <utility>

#include <pdcp_layer/captured_packet_handler.h>
#include <pdcp_layer/packet_handler.h>
#include <pdcp_layer/simulated_packet_handler.h>
#include <simulator/configuration_loader.h>
#include <utils/conversions.h>

packet_handler::packet_handler(pdcp_config pdcp_c, int _verbosity)
    : gauss_dist_gen(std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::system_clock::now().time_since_epoch()).count())
{
    bh_d = pdcp_c.bh_d;
    bh_d_var = pdcp_c.bh_d_var;
    verbosity = _verbosity;
    force_ect1 = false;
}

packet_handler::~packet_handler()
{
}

void packet_handler::init()
{
}

void packet_handler::quit()
{
}

void packet_handler::step(float t)
{
    tp_mean.step();
    g_mean.step();
    e_mean.step();
    current_t = t;
}

bool packet_handler::has_ingress_pkts() const
{
    return !ingress_pkts.empty();
}

ip_pkt packet_handler::pop_ingress_pkt()
{
    ip_pkt pkt = std::move(ingress_pkts.front());
    ingress_pkts.pop_front();
    return pkt;
}

void packet_handler::drop_ingress_pkt(ip_pkt pkt)
{
    (void)pkt;
}

void packet_handler::push(harq_pkt pkt)
{
    pkt.t_out += pkt.backhaul_d + gauss_dist(gauss_dist_gen)*pkt.backhaul_d_var;
    pkt_list.push_back(std::move(pkt));
}

void packet_handler::drop(harq_pkt pkt)
{
    (void)pkt;
}

float packet_handler::release()
{
    int count = 0;
    float bits = 0;
    float latency = 0;
    float ip_latency = 0;
    for(std::deque<harq_pkt>::iterator it=pkt_list.begin(); it!=pkt_list.end();)
    {
        if(it->t_out <= current_t)
        {
            bits += it->bits;
            latency += current_t - it->current_t;
            ip_latency += current_t - it->ip_t;
            count++;
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

void packet_handler::fill_queue_status(pdcp_queue_status& status, float current_t) const
{
    status.release_size = (int)pkt_list.size();
    if(!pkt_list.empty())
    {
        const harq_pkt &pkt = pkt_list.front();
        status.release_oldest_uid = pkt.id;
        status.release_oldest_age = current_t - pkt.ip_t;
    }
}

float packet_handler::get_generated(bool partial)
{
    if(verbosity <= 0) return -1;
    if(!partial) return BIT2MBIT * g_mean.get_total() * S2MS;
    return BIT2MBIT * g_mean.get() * S2MS;
}

float packet_handler::get_error(bool partial)
{
    if(verbosity <= 0) return -1;
    if(!partial) return BIT2MBIT * e_mean.get_total() * S2MS;
    return BIT2MBIT * e_mean.get() * S2MS;
}

float packet_handler::get_latency(bool elapsed)
{
    if(!elapsed)return l_mean.get_total();
    else return l_mean.get();
}

float packet_handler::get_ip_latency(bool elapsed)
{
    if(!elapsed)return ipl_mean.get_total();
    else return ipl_mean.get();
}

float packet_handler::get_tp(bool elapsed)
{
    if(!elapsed)return BIT2MBIT*tp_mean.get_total()*S2MS;
    else return BIT2MBIT*tp_mean.get()*S2MS;
}

void packet_handler::record_error(float bits)
{
    if(verbosity > 0) e_mean.add(bits);
}

void packet_handler::push_ingress_pkt(ip_pkt pkt)
{
    if(force_ect1 && pkt.ecn != ECN_ECT1 && pkt.ecn != ECN_CE)
    {
        pkt.original_ecn = pkt.ecn;
        pkt.ecn = ECN_ECT1;
        pkt.force_ect1_applied = true;
    }
    if(verbosity > 0) g_mean.add(pkt.size);
    ingress_pkts.push_back(std::move(pkt));
}

std::unique_ptr<packet_handler> make_packet_handler(packet_handler_config cfg)
{
    if(cfg.ue_type == SIM_UE)
    {
        std::unique_ptr<packet_handler> handler(new simulated_packet_handler(cfg.ue_id, cfg.traffic_c, cfg.pdcp_c, cfg.log_traffic));
        handler->set_force_ect1(cfg.l4s_c.force_ect1);
        return handler;
    }

    assert(cfg.queue_num >= 0);
    std::unique_ptr<packet_handler> handler(new captured_packet_handler(cfg.queue_num, cfg.init_t, cfg.pdcp_c, cfg.log_quality));
    handler->set_force_ect1(cfg.l4s_c.force_ect1);
    return handler;
}
