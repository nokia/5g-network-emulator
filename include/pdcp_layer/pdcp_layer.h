/**********************************************
* Copyright 2022 Nokia
* Licensed under the BSD 3-Clause Clear License
* SPDX-License-Identifier: BSD-3-Clause-Clear
**********************************************/

#pragma once

#include <memory>

#include <mac_layer/harq_handler.h>
#include <pdcp_layer/ip_buffer.h>
#include <pdcp_layer/packet_handler.h>
#include <pdcp_layer/pdcp_config.h>
#include <pdcp_layer/pdcp_queue_status.h>

// Orchestrates the common packet flow between ingress, HARQ, and packet release.
// Traffic source and final release behavior live in packet_handler implementations.
class pdcp_layer
{
public: 
    pdcp_layer(packet_handler_config handler_cfg, int _verbosity = 0);
public: 
    void exit();
    void init(int _mod_i, int _layers, int _logic_units);
    float release(); 
    void step(float t);
    bool has_pkts();
    float get_oldest_timestamp();
    virtual float handle_pkt(float bits, int mcs, float sinr, float distance);
    float get_generated(bool partial = true);
    float get_error(bool partial = true);
    float get_ip_latency(bool partial = true);
    float get_latency(bool partial = true);
    float get_tp(bool partial = true);
    pdcp_queue_status get_queue_status() const;
    dualpi2_stats get_l4s_interval_stats();
    void set_pkt_delay_budget(float budget_s) { pkt_delay_budget_s = budget_s; }
    float get_pkt_delay_budget() const { return pkt_delay_budget_s; }

private:
    void release_pkts(harq_pkt pkt);
    void drain_ingress_pkts();
    void cleanup_expired_pkts();
    void cleanup_expired_ip_pkts();
    void cleanup_expired_harq_pkts();
    void drop_harq_pkt(harq_pkt pkt);
    bool is_expired(float ip_t) const;
    bool is_expired(const harq_pkt& pkt) const;
    float oldest_allowed_ip_t() const;

protected: 
    ip_buffer _ip_buffer;
    harq_handler _harq_buffer;
    std::unique_ptr<packet_handler> _packet_h;
    int verbosity = 0;
    int tx_dir;
    
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
};
