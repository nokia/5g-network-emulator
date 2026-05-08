/**********************************************
* Copyright 2022 Nokia
* Licensed under the BSD 3-Clause Clear License
* SPDX-License-Identifier: BSD-3-Clause-Clear
**********************************************/

#include <cassert>
#include <iostream>
#include <pdcp_layer/pdcp_handler.h>

pdcp_handler::pdcp_handler( pdcp_config pdcp_c_ul, pdcp_config pdcp_c_dl, bool _verbosity)
             {
                 layers[T_UL] = std::shared_ptr<pdcp_layer>(new pdcp_layer(pdcp_c_ul, _verbosity));
                 layers[T_DL] = std::shared_ptr<pdcp_layer>(new pdcp_layer(pdcp_c_dl, _verbosity));
                 verbosity = _verbosity; 
             }

pdcp_handler::pdcp_handler( int _queue_num_ul, int _queue_num_dl, std::chrono::microseconds * _init_t, pdcp_config pdcp_c_ul, pdcp_config pdcp_c_dl, bool _verbosity)
             {
                 layers[T_UL] = std::shared_ptr<pdcp_layer>(new pdcp_layer(_queue_num_ul, _init_t, pdcp_c_ul, _verbosity));
                 layers[T_DL] = std::shared_ptr<pdcp_layer>(new pdcp_layer(_queue_num_dl, _init_t, pdcp_c_dl, _verbosity));
                 verbosity = _verbosity; 
             }

pdcp_layer& pdcp_handler::layer(int tx)
{
    assert(tx==T_DL||tx==T_UL);
    return *layers[tx];
}

const pdcp_layer& pdcp_handler::layer(int tx) const
{
    assert(tx==T_DL||tx==T_UL);
    return *layers[tx];
}

void pdcp_handler::exit()
{
    layer(T_UL).exit();
    layer(T_DL).exit();
}

void pdcp_handler::init(int _mod_i, int _layers, int _logic_units)
{
    layer(T_UL).init(_mod_i, _layers, _logic_units);
    layer(T_DL).init(_mod_i, _layers, _logic_units);
}

void pdcp_handler::release()
{
    layer(T_UL).release();
    layer(T_DL).release();
}

void pdcp_handler::release(float &ul_bits, float &dl_bits)
{
    ul_bits = layer(T_UL).release();
    dl_bits = layer(T_DL).release();
}

void pdcp_handler::step(int tx, float t)
{
    layer(tx).step(t);
}

void pdcp_handler::step(float t)
{
    layer(T_DL).step(t);
    layer(T_UL).step(t);
}

void pdcp_handler::init_pkt_capture()
{
    layer(T_UL).init_pkt_capture();
    layer(T_DL).init_pkt_capture();
} 

float pdcp_handler::get_ip_pkts(int tx)
{
    return layer(tx).get_ip_pkts();
}

void pdcp_handler::generate_pkts(int tx, float bits, float pkt_size, float t)
{
    layer(tx).generate_pkts(bits, pkt_size, t);
}

bool pdcp_handler::has_pkts(int tx)
{
    return layer(tx).has_pkts();
}

float pdcp_handler::get_oldest_timestamp(int tx)
{
    return layer(tx).get_oldest_timestamp();
}

float pdcp_handler::handle_pkt(int tx, float bits, int mcs, float sinr, float distance)
{
    return layer(tx).handle_pkt(bits,  mcs, sinr, distance);
}

float pdcp_handler::get_generated(int tx, bool elapsed)
{
    return layer(tx).get_generated(elapsed);
}

float pdcp_handler::get_error(int tx, bool elapsed)
{
    return layer(tx).get_error(elapsed);
}

float pdcp_handler::get_tp(int tx, bool elapsed)
{
    return layer(tx).get_tp(elapsed);
}

float pdcp_handler::get_latency(int tx, bool elapsed)
{
    return layer(tx).get_latency(elapsed);
}

float pdcp_handler::get_ip_latency(int tx, bool elapsed)
{
    return layer(tx).get_ip_latency(elapsed);
}

pdcp_queue_status pdcp_handler::get_queue_status(int tx) const
{
    return layer(tx).get_queue_status();
}

void pdcp_handler::set_pkt_delay_budget(float budget_s)
{
    layer(T_UL).set_pkt_delay_budget(budget_s);
    layer(T_DL).set_pkt_delay_budget(budget_s);
}
