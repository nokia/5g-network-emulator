/**********************************************
* Copyright 2022 Nokia
* Licensed under the BSD 3-Clause Clear License
* SPDX-License-Identifier: BSD-3-Clause-Clear
**********************************************/

#pragma once

#include <math.h>

#include <mac_layer/harq_handler.h>
#include <pdcp_layer/dualpi2_queue.h>
#include <utils/conversions.h>
#include <utils/logging/mean_handler.h>
#include <pkts/pkts.h>

//--------------------------------------------------------------------------------------------------
// ip_buffer(): this class stores the simulated (coming from purely virtual or actual IP packets)
// IP packets. It's a custom buffer implementation which include some statistics tracking, such 
// as drop, release or generation rates. 
// Input: 
//      _verbosity: Enable/disable verbosity
//--------------------------------------------------------------------------------------------------
class ip_buffer
{
public: 
    ip_buffer(int _verbosity = 0);
private: 
    std::deque<ip_pkt> pkt_list; 
    float current_t; 
    int current_id = 0; 
    float oldest_t = 0; 
    int verbosity = 0; 

private: 
    float max_size = 65500.0f*8.0f*20000.0f;
    float current_size = 0; 

private: 
    mean_handler<float> g_mean; 
    mean_handler<float> e_mean; 

public: 
    float get_oldest_timestamp();
    bool has_pkts();
    void generate(float bits, float pkt_size, float t, float bh_d, float bh_d_var);
    float drop_pkt(int bits);
    void step(float _current_t);
    float get_pkts(float _bits, harq_pkt& pkt);
    bool pop_oldest_pkt(harq_pkt& pkt);
    const ip_pkt* peek_oldest_pkt() const;
    int size() const { return l4s_cfg.enabled ? l4s_queue.size() : (int)pkt_list.size(); }
    float get_generated(bool partial = true);
    float get_error(bool partial = true);
    bool add_pkt(ip_pkt pkt);
    void configure_l4s(dualpi2_config cfg);
    bool using_dualpi2() const { return l4s_cfg.enabled; }
    dualpi2_stats get_l4s_stats() const;
    dualpi2_stats get_l4s_interval_stats();
    bool pop_aqm_dropped_pkt(harq_pkt& pkt);

public:
    int debug_queue_num = -1; // Queue ID -- just for debug

private:
    dualpi2_config l4s_cfg;
    dualpi2_queue l4s_queue;
};
