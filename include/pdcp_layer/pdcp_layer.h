/**********************************************
* Copyright 2022 Nokia
* Licensed under the BSD 3-Clause Clear License
* SPDX-License-Identifier: BSD-3-Clause-Clear
**********************************************/

#pragma once

#include <chrono>
#include <deque>
#include <iostream>
#include <memory>
#include <utility>
#include <pdcp_layer/pdcp_queue_status.h>
#include <mac_layer/harq_handler.h>
#include <netfilter/netfilter_interface.h>
#include <pdcp_layer/release_handler.h>
#include <pdcp_layer/pdcp_config.h>

class pkt_capture;

//--------------------------------------------------------------------------------------------------
// pdcp_layer(): this class is the one orquestrating the pkt flow between the different queues,
// according to the instructions coming from other modules. While most of the implemented methods 
// are just interfaces between other modules and the main methods from the different buffers, 
// there is key method which is actually handling the flow between the IP, Release and HARQ buffers. 
// This method, called handle_pkt(), takes the size to be transmitted in bits, along with some 
// arbitrary Channel State Indicators.

// Input: 
//      _queue_num_ul/_queue_num_dl: Netfilter Queues IDs (only if real traffic is being used)
//      _init_t: Initial timestamp (only for real IP traffic)
//      pdcp_c_ul/pdcp_c_dl: configuration struct. Includes: 
//          * _max_rtx: max number of retransmissions
//          * _air_delay_var: added variance to the delay comming from the air propagation.
//          * _rtx_period: ack/nack delay in seconds
//          * _rtx_period_var: white noise spread around the ack/nack delay in seconds
//          * _rtx_proc_delay: ack/nack processing delay in seconds
//          * _rtx_proc_delay_var: white noise spread around the ack/nack processing delay in seconds
//          * _bh_d: backhauling delay, use for Release buffer
//          * _bh_d_var: white noise spread around the backhauling delay
//          * _order_pkts: enable/disable RLC reordering
//          * _verbosity: enable/disable verbosity
//      verbosity: Enable/disable verbosity
//--------------------------------------------------------------------------------------------------
class pdcp_layer
{
public: 
    pdcp_layer( int is_ip, int _max_rtx, float _air_delay_var, 
                float _rtx_period, float _rtx_period_var, float _rtx_proc_delay, 
                float _rtx_proc_delay_var, float _bh_d, float _bh_d_var, bool _order_pkts, int _verbosity = 0);

    pdcp_layer(int is_ip, pdcp_config pdcp_c, int _verbosity = 0);
    pdcp_layer( int _max_rtx, float _air_delay_var, 
                float _rtx_period, float _rtx_period_var, float _rtx_proc_delay, 
                float _rtx_proc_delay_var, float _bh_d, float _bh_d_var, int _verbosity = 0);

    pdcp_layer(pdcp_config pdcp_c, int _verbosity = 0);
    pdcp_layer(int _queue_num, std::chrono::microseconds *_init_t, pdcp_config pdcp_c, int _verbosity = 0);
public: 
    void exit();
    void init(int _mod_i, int _layers, int _logic_units);
    float release(); 
    void step(float t);
    float request_pkts(float bits);
    void drop_pkt(int bits);
    void generate_pkts(float bits, float pkt_size, float t);
    void enqueue_ip_pkt(ip_pkt pkt);
    void release_pkts(harq_pkt pkt);
    bool has_pkts();
    float get_oldest_timestamp();
    virtual float handle_pkt(float bits, int mcs, float sinr, float distance);
    float get_generated(bool partial = true);
    float get_error(bool partial = true);
    float get_ip_pkts();
    void init_pkt_capture();
    float get_ip_latency(bool partial = true);
    float get_latency(bool partial = true);
    float get_tp(bool partial = true);
    pdcp_queue_status get_queue_status() const;
    void set_pkt_delay_budget(float budget_s) { pkt_delay_budget_s = budget_s; }
    float get_pkt_delay_budget() const { return pkt_delay_budget_s; }

protected:
    void cleanup_old_pkts();

private:
    void init_ip_capture(int _queue_num, std::chrono::microseconds *_init_t);
    void cb(void* handler, netfilter_interface_t *nfiface, uint64_t timestamp_sec, uint64_t timestamp_usec,
            uint64_t bytes, uint32_t pkt_id);
    float get_current_ts() const;
    void drop_harq_pkt(harq_pkt pkt);
    bool is_pkt_too_old(const harq_pkt& pkt) const;

protected: 
    ip_buffer _ip_buffer;
    harq_handler _harq_buffer;
    std::shared_ptr<release_handler> _release_h; 
    int verbosity = 0;
    
protected: 
    int current_id = 0; 
    
protected: 
    float bh_d; 
    float bh_d_var; 

public: 
    float current_t = 0;

protected:
    float pkt_delay_budget_s = 0.350f;

private:
    bool is_ip = false;
    std::chrono::microseconds *init_t = nullptr;
    uint32_t prev_uid = static_cast<uint32_t>(-1);
    std::shared_ptr<pkt_capture> pkt_cptr;
    int queue_num = -1;
    std::deque<ip_pkt> captured_pkts;
};
