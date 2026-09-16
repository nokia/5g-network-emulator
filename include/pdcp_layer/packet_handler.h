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

enum class final_packet_verdict
{
    ACCEPT,
    ACCEPT_CE,
    DROP
};

class packet_handler
{
public:
    packet_handler(pdcp_config pdcp_c, unsigned int seed, int verbosity = 0);
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

    // Runtime control hooks. Only a simulated source has a target rate to change; a
    // captured one answers false and the registry turns that into an explicit error.
    virtual bool set_traffic_target(int tx_dir, float bps) { (void)tx_dir; (void)bps; return false; }
    virtual bool get_traffic_target(int tx_dir, float &bps) const { (void)tx_dir; (void)bps; return false; }

    // Client driven injection: the runtime twin of the file driven traffic_generator.
    // The caller decides when and how much; the emulator only packetizes and queues.
    virtual bool inject_bits(float bits) { (void)bits; return false; }
    virtual float injected_bits_total() const { return 0.0f; }
    virtual int get_pkt_size() const { return 0; }

    // Cumulative, monotonic counters. The client diffs two reads, so a lost read loses
    // nothing and the emulator keeps no "since last time" state.
    float delivered_bits_total() const { return delivered_bits_total_; }
    float expired_bits_total() const { return expired_bits_total_; }
    float dropped_bits_total() const { return dropped_bits_total_; }
    int ce_packets_total() const { return ce_packets_total_; }

    // Expiry by delay budget means the client is overfeeding; an AQM drop or an
    // exhausted HARQ means the radio is struggling. Merging them would make the reading
    // useless, so the caller states which one it is.
    void record_error(float bits, bool expired);

protected:
    void push_ingress_pkt(ip_pkt pkt);
    virtual void verdict(const ip_pkt& pkt, final_packet_verdict verdict);

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
    float delivered_bits_total_ = 0.0f;
    float expired_bits_total_ = 0.0f;
    float dropped_bits_total_ = 0.0f;
    int ce_packets_total_ = 0;
    int final_accept_packets_interval = 0;
    int final_accept_ce_packets_interval = 0;
    int final_drop_packets_interval = 0;

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
    dualpi2_config l4s_c;
    bool log_traffic;
    bool log_quality;
};

std::unique_ptr<packet_handler> make_packet_handler(packet_handler_config cfg);
