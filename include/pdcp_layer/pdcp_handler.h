/**********************************************
* Copyright 2022 Nokia
* Licensed under the BSD 3-Clause Clear License
* SPDX-License-Identifier: BSD-3-Clause-Clear
**********************************************/

#pragma once
#include <pdcp_layer/pdcp_layer_ip.h>
#include <pdcp_layer/pdcp_config.h>

#ifndef T_DL
#define T_DL 0
#endif
#ifndef T_UL
#define T_UL 1
#endif

//--------------------------------------------------------------------------------------------------
// pdcp_handler(): this class interfaces the different PDCP/RLC processes with other emulator's 
// modules. Within this class there are two instances of the PDCPLayer() class, one for each TX 
// direction. This last class is the one orquestrating the pkt flow between the different queues,
// according to the instructions coming from other modules.

// Input: 
//      _queue_num_ul/_queue_num_dl: Netfilter Queues IDs (only if real traffic is being used)
//      _init_t: Initial timestamp (only for real IP traffic)
//      pdcp_c_ul/pdcp_c_dl: PDCP configuration struct. 
//      verbosity: Enable/disable verbosity
//--------------------------------------------------------------------------------------------------
class pdcp_handler
{
public: 
    pdcp_handler( pdcp_config pdcp_c_ul, pdcp_config pdcp_c_dl, bool verbosity = true);
    pdcp_handler( int _queue_num_ul, int _queue_num_dl, std::chrono::microseconds * _init_t, pdcp_config pdcp_c_ul, pdcp_config pdcp_c_dl, bool verbosity = true);

public: 
    void exit();
    void init(int _mod_i, int _layers, int _logic_units);
    void release(); 
    void release(float &ul_bits, float &dl_bits);
    void step(int tx, float t);
    void step(float t);
    void drop_pkt(int tx, harq_pkt pkt);
    float get_ip_pkts(int tx);
    void init_pkt_capture();
    void generate_pkts(int tx, float bits, float pkt_size, float t);
    bool has_pkts(int tx);
    float get_oldest_timestamp(int tx);
    float handle_pkt(int tx, float bits, int mcs, float sinr, float distance);
    float get_error(int tx, bool elapsed = true);
    float get_generated(int tx, bool elapsed = true);
    float get_tp(int tx, bool elapsed = true);
    float get_latency(int tx, bool elapsed = true);
    float get_ip_latency(int tx, bool elapsed = true);

protected: 
    std::shared_ptr<pdcp_layer> pdcp_ul;  
    std::shared_ptr<pdcp_layer> pdcp_dl;  
protected: 
    float current_t = 0;
protected: 
    int verbosity = 0; 
};
