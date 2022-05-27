/**********************************************
* Copyright 2022 Nokia
* Licensed under the BSD 3-Clause Clear License
* SPDX-License-Identifier: BSD-3-Clause-Clear
**********************************************/

#pragma once

#include <vector>
#include <random>
#include <iostream>
#include <memory>
#include <chrono>
#include <mac_layer/mac_definitions.h>
#include <utils/conversions.h>
#include <pkts/pkts.h>
#include <assert.h>     /* assert */

#define TARGET_BLER 0.1f
#define ERROR_RED_HARQ 0.2f

//--------------------------------------------------------------------------------------------------
// harq_handler(): it implements a buffer to queue the packets that have to be retransmitted. Wether
// a packet has to be retransmitted is also estimated by this class using a simple HARQ statistical
//model
// Input: 
//      * _max_rtx: max number of retransmissions
//      * _air_delay_var: added variance to the delay comming from the air propagation.
//      * _rtx_period: ack/nack delay in seconds
//      * _rtx_period_var: white noise spread around the ack/nack delay in seconds
//      * _rtx_proc_delay: ack/nack processing delay in seconds
//      * _rtx_proc_delay_var: white noise spread around the ack/nack processing delay in seconds
//      * _verbosity: enable/disable verbosity
//--------------------------------------------------------------------------------------------------
class harq_handler
{
public: 
    harq_handler(int _max_rtx, float _air_delay, 
                 float _rtx_period, float _rtx_period_var, 
                 float _rtx_p_delay, float _rtx_p_delay_var, int _verbosity = 0);
private: 
    int max_rtx; 
    float air_delay; 
    bool stchstc_air; 
    bool stchstc_delay; 
    float rtx_p_delay; 
    float rtx_p_delay_var;
    float rtx_period; 
    float rtx_period_var; 
    float oldest_t = -1; 
    float t_out = -1; 
    std::deque<harq_pkt> harq_buffer; 
    int rtx_mbit = 0; 
    float current_t = 0.0;
    int verbosity = 0; 

    int mod_i; 
    int rbg_i; 
    int l_i; 
    int logic_units; 

    std::mt19937 generator;
    std::uniform_real_distribution<float> p_delay_dist = std::uniform_real_distribution<float>(-1.0f, 1.0f);
    std::uniform_real_distribution<float> rtx_period_dist = std::uniform_real_distribution<float>(-1.0f, 1.0f);
    std::uniform_real_distribution<float> rtx_prob = std::uniform_real_distribution<float>(0.0f, 1.0f);
public: 
    void step(float t);
    int get_rtx_mbits();
    void queue(harq_pkt pkt, float distance);
    void add_pkts(harq_pkt pkt);
    bool is_pkt_ready();
    harq_pkt get_pkt();
    float get_oldest_t();
    bool get_rtx(int mcs, float sinr, int n_tx);
    bool get_rtx();
    void init(int _mod_i, int _layers, int _logic_units);

private: 
    float emulate_ack_delay(float distance);
};
