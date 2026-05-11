/**********************************************
* Copyright 2022 Nokia
* Licensed under the BSD 3-Clause Clear License
* SPDX-License-Identifier: BSD-3-Clause-Clear
**********************************************/

#pragma once

#include <chrono>
#include <cstdint>
#include <deque>
#include <memory>

#include <netfilter/packet_capture_interface.h>
#include <pdcp_layer/packet_handler.h>

class captured_packet_handler : public packet_handler
{
public:
    captured_packet_handler(int queue_num, std::chrono::microseconds *init_t, pdcp_config pdcp_c, int verbosity = 0);
    captured_packet_handler(std::unique_ptr<packet_capture_interface> capture, std::chrono::microseconds *init_t, pdcp_config pdcp_c, int verbosity = 0);
    ~captured_packet_handler() override;

    void init() override;
    void quit() override;
    float ingest(int tx_dir, float current_t) override;
    void drop_ingress_pkt(ip_pkt pkt) override;
    void push(harq_pkt pkt) override;
    void drop(harq_pkt pkt) override;
    float release() override;
    void fill_queue_status(pdcp_queue_status& status, float current_t) const override;

private:
    void cb(void* handler, netfilter_interface_t *nfiface, const nfq_packet_metadata& meta);
    float get_current_ts() const;
    void sort_by_id();
    bool add_data(ip_pkt *recv_pkt);
    bool remove_data(int id, float bits);
    bool check_order(int id);
    void update_order(int id);

private:
    bool do_check = true;
    std::chrono::microseconds *init_t = nullptr;
    uint32_t prev_uid = static_cast<uint32_t>(-1);
    int prev_released_id = -1;
    std::unique_ptr<packet_capture_interface> pkt_cptr;
    std::deque<ip_pkt> captured_pkts;
    std::deque<ip_pkt> out_pkts;
};
