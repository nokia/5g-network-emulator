/**********************************************
* Copyright 2022 Nokia
* Licensed under the BSD 3-Clause Clear License
* SPDX-License-Identifier: BSD-3-Clause-Clear
**********************************************/

#include <iostream>
#include <chrono>
#include <pdcp_layer/ip_buffer.h>
#include <utils/terminal_logging.h>

ip_buffer::ip_buffer(int _verbosity)
{
    verbosity = _verbosity; 
}

float ip_buffer::get_oldest_timestamp()
{
    if(l4s_cfg.enabled) return l4s_queue.oldest_timestamp(current_t);
    if(pkt_list.size() > 0) return oldest_t; 
    else return current_t; 
}

bool ip_buffer::has_pkts()
{
    if(l4s_cfg.enabled) return l4s_queue.has_pkts();
    return pkt_list.size() > 0;
}

void ip_buffer::generate(float bits, float pkt_size, float t, float bh_d, float bh_d_var)
{
    if(current_size < max_size && bits > 0)
    {
        int pkts = ceil(bits/pkt_size);
        if(pkt_list.size() == 0) oldest_t = t; 
        for(int i = 0; i < pkts - 1; i++)
        {
            pkt_list.push_back(ip_pkt(t, pkt_size, pkt_size, current_id, bh_d, bh_d_var));
            current_id++;
        }
        float bits_left = bits - (pkts - 1)*pkt_size;
        if(bits_left > 0) 
        {
            pkt_list.push_back(ip_pkt(t, bits_left, bits_left, current_id, bh_d, bh_d_var));
            current_id++;
        }
        current_size += bits; 
    }
    else
    {
        if(bits > 0 && verbosity > 0)
        {
            e_mean.add(bits);
        }
        bits = 0; 
        
    }
    if(verbosity > 0) g_mean.add(bits);
}

float ip_buffer::drop_pkt(int bits)
{
    if(verbosity > 0) e_mean.add(bits);
    return bits; 
}

bool ip_buffer::add_pkt(ip_pkt pkt)
{
    if(current_size + pkt.size > max_size)
    {
        if(verbosity > 0) e_mean.add(pkt.size);
        return false;
    }

    current_size += pkt.size; 
    if(verbosity > 0) g_mean.add(pkt.size);
    if(l4s_cfg.enabled)
    {
        if(l4s_queue.size() == 0) oldest_t = pkt.current_t;
        l4s_queue.enqueue(std::move(pkt));
    }
    else
    {
        if(pkt_list.size() == 0) oldest_t = pkt.current_t; 
        pkt_list.push_back(std::move(pkt));
    }
    return true;
}

void ip_buffer::step(float _current_t){
    if(verbosity > 0) g_mean.step();
    if(verbosity > 0) e_mean.step();
    current_t = _current_t;
    if(l4s_cfg.enabled) l4s_queue.step(current_t);
}

float ip_buffer::get_pkts(float _bits, harq_pkt& out_pkt)
{
    float bits = floorf(_bits); 
    int n_out_pkts = 0; 
    out_pkt.bits = 0; 
    if(l4s_cfg.enabled)
    {
        while(bits > 0 && l4s_queue.has_pkts())
        {
            ip_pkt pkt(0.0f, 0.0f, 0.0f, 0, 0.0f, 0.0f);
            if(!l4s_queue.pop_next(pkt)) break;
            if(bits >= pkt.size)
            {
                oldest_t = pkt.current_t;
                bits -= pkt.size;
                out_pkt.bits += pkt.size;
                pkt.is_fragment = false;
                pkt.frags_created++;
                out_pkt.pkts.push_back(std::move(pkt));
                n_out_pkts++;
                if(bits <= BIT_ROUND_MARGIN) break;
            }
            else
            {
                ip_pkt pkt_f(pkt);
                pkt.size -= bits;
                pkt_f.size = bits;
                pkt_f.is_fragment = true;
                pkt_f.frags_created++;
                pkt.is_fragment = true;
                pkt.frags_created++;
                out_pkt.bits += bits;
                out_pkt.pkts.push_back(std::move(pkt_f));
                l4s_queue.enqueue(std::move(pkt));
                bits = 0;
                n_out_pkts++;
            }
        }
        current_size -= out_pkt.bits;
        if(current_size < 0.0f) current_size = 0.0f;
        return out_pkt.bits;
    }
    while(bits > 0 && pkt_list.size() > 0)
    {
        ip_pkt *pkt = &pkt_list.front();
        if(bits >= pkt->size)
        {
            // LOG_INFO_I("PKT_FRAG") << "(" << debug_queue_num << ")" << "UID [" << pkt->uid << "." << out_pkt.id << "] with bits [" << pkt->size << ":" << pkt->size << "/" << pkt->original_size << "] (" << n_out_pkts << ") FULL" << END(); 

            oldest_t = pkt->current_t;
            bits -= pkt->size; 
            out_pkt.bits += pkt->size; 
            //pkt->size = pkt->original_size; 
            pkt->is_fragment = false;
            pkt->frags_created++;
            out_pkt.pkts.push_back(std::move(*pkt));
            pkt_list.pop_front();
            n_out_pkts++;

            
            // Prevent starting a new IP packet with 0.1 bits or less (it creates blocks donwstream)
            if(bits <= BIT_ROUND_MARGIN) {
                // LOG_INFO_I("PKT_FRAG") << "(" << debug_queue_num << ")" <<"UID [" << pkt->uid << "." << out_pkt.id << "] FEW BITS REMAINING " << bits << ": skip"<< END();
                break; 
            }
                
        }
        else
        {    
            // LOG_INFO_I("PKT_FRAG") << "(" << debug_queue_num << ")" <<"UID [" << pkt->uid << "." << out_pkt.id << "] with bits [" << bits << ":" << pkt->size << "/" << pkt->original_size << "] (" << n_out_pkts << ") FRAG" << END();

            ip_pkt pkt_f(*pkt);
            pkt->size -= bits;
            out_pkt.bits += bits; 
            pkt_f.size = bits;
            pkt_f.is_fragment = true; 
            pkt_f.frags_created++;
            out_pkt.pkts.push_back(std::move(pkt_f));
            bits = 0; 
            pkt->is_fragment = true; 
            pkt->frags_created++;
            n_out_pkts++;
        } 
    }
    current_size -= out_pkt.bits; 
    return out_pkt.bits; 
}

float ip_buffer::get_generated(bool partial)
{ 
    if(verbosity > 0) 
    {
        if(!partial)return BIT2MBIT*g_mean.get_total()*S2MS;
        else return BIT2MBIT*g_mean.get()*S2MS;
    }
    else return -1; 
}

float ip_buffer::get_error(bool partial)
{ 
    if(verbosity > 0) 
    {
        if(!partial)return BIT2MBIT*e_mean.get_total()*S2MS;
        else return BIT2MBIT*e_mean.get()*S2MS;
    }
    else return -1; 
}

bool ip_buffer::pop_oldest_pkt(harq_pkt& out_pkt)
{
    if(!l4s_cfg.enabled && pkt_list.empty()) return false;
    if(l4s_cfg.enabled && !l4s_queue.has_pkts()) return false;

    out_pkt.pkts.clear();
    out_pkt.bits = 0.0f;

    ip_pkt pkt(0.0f, 0.0f, 0.0f, 0, 0.0f, 0.0f);
    if(l4s_cfg.enabled)
    {
        l4s_queue.pop_oldest(pkt);
    }
    else
    {
        pkt = std::move(pkt_list.front());
        pkt_list.pop_front();
    }

    out_pkt.bits = pkt.size;
    out_pkt.pkts.push_back(std::move(pkt));

    current_size -= out_pkt.bits;
    if(current_size < 0) current_size = 0;

    if(l4s_cfg.enabled && l4s_queue.has_pkts()) oldest_t = l4s_queue.oldest_timestamp(current_t);
    else if(!l4s_cfg.enabled && !pkt_list.empty()) oldest_t = pkt_list.front().current_t;
    else oldest_t = current_t;

    return true;
}

const ip_pkt* ip_buffer::peek_oldest_pkt() const
{
    if(l4s_cfg.enabled) return l4s_queue.peek_oldest();
    if(pkt_list.empty()) return nullptr;
    return &pkt_list.front();
}

void ip_buffer::configure_l4s(dualpi2_config cfg)
{
    l4s_cfg = cfg;
    l4s_queue.configure(cfg);
}

dualpi2_stats ip_buffer::get_l4s_stats() const
{
    if(!l4s_cfg.enabled) return dualpi2_stats();
    return l4s_queue.get_stats();
}

dualpi2_stats ip_buffer::get_l4s_interval_stats()
{
    if(!l4s_cfg.enabled) return dualpi2_stats();
    return l4s_queue.get_interval_stats();
}

bool ip_buffer::pop_aqm_dropped_pkt(harq_pkt& out_pkt)
{
    if(!l4s_cfg.enabled || !l4s_queue.has_dropped_pkts()) return false;
    ip_pkt pkt(0.0f, 0.0f, 0.0f, 0, 0.0f, 0.0f);
    if(!l4s_queue.pop_dropped_pkt(pkt)) return false;
    out_pkt.pkts.clear();
    out_pkt.bits = pkt.size;
    out_pkt.ip_t = pkt.ip_t;
    out_pkt.current_t = current_t;
    out_pkt.t_out = current_t;
    out_pkt.pkts.push_back(std::move(pkt));
    current_size -= out_pkt.bits;
    if(current_size < 0.0f) current_size = 0.0f;
    return true;
}
