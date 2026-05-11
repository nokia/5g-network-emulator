/**********************************************
* Copyright 2022 Nokia
* Licensed under the BSD 3-Clause Clear License
* SPDX-License-Identifier: BSD-3-Clause-Clear
**********************************************/

#pragma once

#include <chrono>
#include <deque>
#include <memory>
#include <random>

#include <pdcp_layer/pdcp_config.h>
#include <pdcp_layer/pdcp_queue_status.h>
#include <pkts/pkts.h>
#include <traffic_models/traffic_config.h>
#include <utils/logging/mean_handler.h>

class packet_handler
{
public:
    explicit packet_handler(pdcp_config pdcp_c, int verbosity = 0);
    virtual ~packet_handler();

    virtual void init();
    virtual void quit();
    virtual void step(float t);
    virtual float ingest(int tx_dir, float current_t) = 0;
    bool has_ingress_pkts() const;
    ip_pkt pop_ingress_pkt();
    virtual void drop_ingress_pkt(ip_pkt pkt);
    virtual void push(harq_pkt pkt);
    virtual void drop(harq_pkt pkt);
    virtual float release();
    virtual void fill_queue_status(pdcp_queue_status& status, float current_t) const;

    float get_generated(bool partial = true);
    float get_error(bool partial = true);
    float get_ip_latency(bool elapsed = true);
    float get_latency(bool elapsed = true);
    float get_tp(bool elapsed = true);
    void record_error(float bits);

protected:
    void push_ingress_pkt(ip_pkt pkt);

protected:
    std::deque<ip_pkt> ingress_pkts;
    std::deque<harq_pkt> pkt_list;
    float current_t = 0;
    float bh_d = 0;
    float bh_d_var = 0;
    int verbosity = 0;

    mean_handler<float> tp_mean;
    mean_handler<float> l_mean;
    mean_handler<float> ipl_mean;
    mean_handler<float> g_mean;
    mean_handler<float> e_mean;

    std::mt19937 gauss_dist_gen;
    std::uniform_real_distribution<float> gauss_dist{-1,1};
};

struct packet_handler_config
{
    int ue_type;
    int tx_dir;
    int queue_num;
    int ue_id;
    std::chrono::microseconds *init_t;
    traffic_config traffic_c;
    pdcp_config pdcp_c;
    bool log_traffic;
    bool log_quality;
};

std::unique_ptr<packet_handler> make_packet_handler(packet_handler_config cfg);
