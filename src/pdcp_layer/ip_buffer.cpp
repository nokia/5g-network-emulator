/**********************************************
* Copyright 2022 Nokia
* Licensed under the BSD 3-Clause Clear License
* SPDX-License-Identifier: BSD-3-Clause-Clear
**********************************************/

#include <iostream>
#include <chrono>
#include <pdcp_layer/ip_buffer.h>

ip_buffer::ip_buffer(int _verbosity)
{
    verbosity = _verbosity; 
}

float ip_buffer::get_oldest_timestamp()
{
    if(pkt_list.size() > 0) return oldest_t; 
    else return current_t; 
}

bool ip_buffer::has_pkts()
{
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

void ip_buffer::add_pkt(ip_pkt pkt)
{
    current_size += pkt.size; 
    if(verbosity > 0) g_mean.add(pkt.size);
    if(pkt_list.size() == 0) oldest_t = pkt.current_t; 
    pkt_list.push_back(std::move(pkt));
}

void ip_buffer::step(float _current_t){
    if(verbosity > 0) g_mean.step();
    if(verbosity > 0) e_mean.step();
    current_t = _current_t;
}

float ip_buffer::get_pkts(float _bits, harq_pkt& out_pkt)
{
    float bits = _bits; 
    int n_out_pkts = 0; 
    out_pkt.bits = 0; 
    while(bits > 0 && pkt_list.size() > 0)
    {
        ip_pkt *pkt = &pkt_list.front();
        if(bits > pkt->size)
        {
            oldest_t = pkt->current_t;
            bits -= pkt->size; 
            out_pkt.bits += pkt->size; 
            //pkt->size = pkt->original_size; 
            pkt->is_fragment = false; 
            out_pkt.pkts.push_back(std::move(*pkt));
            pkt_list.pop_front();
            n_out_pkts++; 
        }
        else
        {    
            ip_pkt pkt_f(*pkt);
            pkt->size -= bits;
            out_pkt.bits += bits; 
            pkt_f.size = bits;
            pkt_f.is_fragment = true; 
            out_pkt.pkts.push_back(std::move(pkt_f));
            bits = 0; 
            pkt->is_fragment = true; 
            n_out_pkts++;
        } 
    }
    current_size -= out_pkt.bits; 
    return out_pkt.bits; 
}

float ip_buffer::get_pkts(float _bits)
{
    float bits = _bits; 
    float bits_c = 0; 
    int n_out_pkts = 0; 
    while(bits > 0 && pkt_list.size() > 0)
    {
        ip_pkt *pkt = &pkt_list.front();                
        if(bits > pkt->size)
        {
            bits_c += pkt->size; 
            bits -= pkt->size; 
            pkt_list.pop_front();
            n_out_pkts++; 
        
        }
        else
        {    
            bits_c += bits; 
            pkt->size -= bits;
            bits = 0; 
            pkt->is_fragment = true; 
            n_out_pkts++;
        } 
    }
    oldest_t = pkt_list.front().current_t;
    current_size -= bits_c; 
    return bits_c; 
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