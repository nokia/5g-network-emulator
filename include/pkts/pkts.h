/**********************************************
* Copyright 2022 Nokia
* Licensed under the BSD 3-Clause Clear License
* SPDX-License-Identifier: BSD-3-Clause-Clear
**********************************************/

#pragma once

#include <deque>
#include <cstdint>
#include <vector>

#define BIT_ROUND_MARGIN 0.99f

enum ecn_codepoint
{
    ECN_NOT_ECT = 0,
    ECN_ECT1 = 1,
    ECN_ECT0 = 2,
    ECN_CE = 3
};

struct ip_pkt
{
    ip_pkt(float _current_t, float _size, float _original_size, int _prev_uid, int _uid, float _backhaul_d, float _backhaul_d_var)
    {
        current_t = _current_t; 
        ip_t = _current_t; 
        size = _size; 
        original_size = _original_size; 
        uid = _uid; 
        prev_uid = _prev_uid; 
        is_fragment = size != original_size; 
        backhaul_d = _backhaul_d; 
        backhaul_d_var = _backhaul_d_var; 
        t_out = current_t;  
    };

    ip_pkt(float _current_t, float _size, float _original_size, int _uid, float _backhaul_d, float _backhaul_d_var)
    {
        current_t = _current_t; 
        ip_t = _current_t; 
        size = _size; 
        original_size = _original_size; 
        uid = _uid; 
        prev_uid = uid - 1; 
        is_fragment = size != original_size; 
        backhaul_d = _backhaul_d; 
        backhaul_d_var = _backhaul_d_var; 
        t_out = current_t; 
    };

    ip_pkt(const ip_pkt &cpy_pkt)
    {
        current_t = cpy_pkt.current_t; 
        size = cpy_pkt.size; 
        original_size = cpy_pkt.original_size; 
        uid = cpy_pkt.uid;
        prev_uid = cpy_pkt.prev_uid;  
        is_fragment = cpy_pkt.is_fragment; 
        frags_created = cpy_pkt.frags_created;
        frags_recovered = cpy_pkt.frags_recovered;
        backhaul_d = cpy_pkt.backhaul_d; 
        backhaul_d_var = cpy_pkt.backhaul_d_var; 
        t_out = cpy_pkt.t_out; 
        is_head = cpy_pkt.is_head;
        erase = cpy_pkt.erase; 
        ip_t = cpy_pkt.ip_t; 
        ecn = cpy_pkt.ecn;
        original_ecn = cpy_pkt.original_ecn;
        ce_marked = cpy_pkt.ce_marked;
        force_ect1_applied = cpy_pkt.force_ect1_applied;
        aqm_dropped = cpy_pkt.aqm_dropped;
        payload = cpy_pkt.payload;
    };

    bool is_ready()
    {
        return fabs(size - original_size) <= BIT_ROUND_MARGIN;  
    }

    float current_t; 
    float ip_t; 
    float t_out; 
    float size; 
    float original_size; 
    uint32_t uid; 
    uint32_t prev_uid; 
    bool is_fragment;
    int frags_created = 0;
    int frags_recovered = 0;
    float backhaul_d; 
    float backhaul_d_var; 
    bool is_head = true; 
    bool erase = false; 
    uint8_t ecn = ECN_NOT_ECT;
    uint8_t original_ecn = ECN_NOT_ECT;
    bool ce_marked = false;
    bool force_ect1_applied = false;
    bool aqm_dropped = false;
    std::vector<uint8_t> payload;
};


static bool sorter(ip_pkt & lval, ip_pkt & rval) 
{
    return (lval.uid < rval.uid);
} 

// HARQ packet which includes full and/or fragments of IP packets
struct harq_pkt
{
    harq_pkt(int _id, float _ip_t, float c_t, float mcs, int _distance, float _bits, float _backhaul_d, float _backhaul_d_var)
    {
        id = _id; 
        current_t = c_t; 
        ip_t = _ip_t;
        t_out = c_t; 
        distance = _distance; 
        mcs_i = mcs; 
        n_tx = 0; 
        dropped = false; 
        bits = _bits;
        backhaul_d = _backhaul_d; 
        backhaul_d_var = _backhaul_d_var; 
        pkts.clear(); 
    }

    harq_pkt()
    {
        pkts.clear();
    }
    
    void set_delay(float delay)
    {
        t_out = current_t + delay;
    }
    float ip_t = 0.0;
    float current_t = 0.0; 
    float distance = 0.0;
    bool dropped = false; 
    int n_tx = 0; 
    float t_out = 0.0; 
    int mcs_i = -1; 
    float bits = 0.0; 
    float backhaul_d = 0.0; 
    float backhaul_d_var = 0.0; 
    int id; 
    std::deque<ip_pkt> pkts;
};
