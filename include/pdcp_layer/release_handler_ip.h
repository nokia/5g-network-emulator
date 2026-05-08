/**********************************************
* Copyright 2022 Nokia
* Licensed under the BSD 3-Clause Clear License
* SPDX-License-Identifier: BSD-3-Clause-Clear
**********************************************/

#pragma once

#include <deque>
#include <memory>

#include <pdcp_layer/release_handler.h>

class pkt_capture;

class release_handler_ip: public release_handler
{
public: 
    release_handler_ip(bool _do_check);

public: 
    void set_pkt_cptr(const std::shared_ptr<pkt_capture> &_pkt_cptr) override;
    void push(harq_pkt pkt) override;
    void drop(harq_pkt pkt) override;
    void fill_queue_status(pdcp_queue_status& status, float current_t) const override;
    float release() override;

private: 
    void sort_by_id();
    bool add_data(ip_pkt *recv_pkt);
    bool remove_data(int id, float bits);
    bool check_order(int id);
    void update_order(int id);

private: 
    bool do_check = true; 
    std::deque<ip_pkt> out_pkts; 
    std::shared_ptr<pkt_capture> pkt_cptr; 
};
