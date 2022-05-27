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
                 pdcp_ul = std::shared_ptr<pdcp_layer>(new pdcp_layer(pdcp_c_ul, _verbosity));
                 pdcp_dl = std::shared_ptr<pdcp_layer>(new pdcp_layer(pdcp_c_dl, _verbosity));
                 verbosity = _verbosity; 
             }

pdcp_handler::pdcp_handler( int _queue_num_ul, int _queue_num_dl, std::chrono::microseconds * _init_t, pdcp_config pdcp_c_ul, pdcp_config pdcp_c_dl, bool _verbosity)
             {
                 pdcp_ul = std::shared_ptr<pdcp_layer>(new pdcp_layer_ip(_queue_num_ul, _init_t, pdcp_c_ul, _verbosity));
                 pdcp_dl = std::shared_ptr<pdcp_layer>(new pdcp_layer_ip(_queue_num_dl, _init_t, pdcp_c_dl, _verbosity));
                 verbosity = _verbosity; 
             }

void pdcp_handler::exit()
{
    pdcp_ul->exit(); 
    pdcp_dl->exit(); 
}

void pdcp_handler::init(int _mod_i, int _layers, int _logic_units)
{
    pdcp_ul->init(_mod_i, _layers, _logic_units); 
    pdcp_dl->init(_mod_i, _layers, _logic_units); 
}

void pdcp_handler::release()
{
    pdcp_ul->release(); 
    pdcp_dl->release(); 
}

void pdcp_handler::release(float &ul_bits, float &dl_bits)
{
    ul_bits = pdcp_ul->release(); 
    dl_bits = pdcp_dl->release(); 
}

void pdcp_handler::step(int tx, float t)
{
	assert(tx==T_DL||tx==T_UL); 
    if(tx == T_DL) pdcp_dl->step(t);
    else pdcp_ul->step(t);
}

void pdcp_handler::step(float t)
{
    pdcp_dl->step(t);
    pdcp_ul->step(t);
}

void pdcp_handler::init_pkt_capture()
{
    pdcp_ul->init_pkt_capture(); 
    pdcp_dl->init_pkt_capture(); 
} 

float pdcp_handler::get_ip_pkts(int tx)
{
    assert(tx==T_DL||tx==T_UL); 
    if(tx == T_DL) return pdcp_dl->get_ip_pkts();
    else return pdcp_ul->get_ip_pkts();
}
/*
void pdcp_handler::drop_pkt(int tx, harq_pkt pkt)
{
    if(tx == T_DL) pdcp_dl->drop_pkt(pkt);
    if(tx == T_UL) pdcp_ul->drop_pkt(pkt);
}
*/

void pdcp_handler::generate_pkts(int tx, float bits, float pkt_size, float t)
{
    assert(tx==T_DL||tx==T_UL); 
    if(tx == T_DL) pdcp_dl->generate_pkts(bits, pkt_size, t);
    else pdcp_ul->generate_pkts(bits, pkt_size, t);
}

/*float pdcp_handler::release_pkts(int tx, ip_pkt pkt)
{
    if(tx == T_DL) return pdcp_dl->release_pkts(pkt);
    if(tx == T_UL) return pdcp_ul->release_pkts(pkt);
}

float pdcp_handler::release_pkts(int tx, ip_pkt pkt)
{
    if(tx == T_DL) return pdcp_dl->release_pkts(pkt);
    if(tx == T_UL) return pdcp_ul->release_pkts(pkt);
}

float pdcp_handler::release_pkts(int tx, std::deque<ip_pkt> *pkts)
{
    if(tx == T_DL) return pdcp_dl->release_pkts(pkts);
    if(tx == T_UL) return pdcp_ul->release_pkts(pkts);
}*/

bool pdcp_handler::has_pkts(int tx)
{
    assert(tx==T_DL||tx==T_UL); 
    if(tx == T_DL) return pdcp_dl->has_pkts();
    else return pdcp_ul->has_pkts();
}

float pdcp_handler::get_oldest_timestamp(int tx)
{
    assert(tx==T_DL||tx==T_UL); 
    if(tx == T_DL) return pdcp_dl->get_oldest_timestamp();
    else return pdcp_ul->get_oldest_timestamp(); 
}

float pdcp_handler::handle_pkt(int tx, float bits, int mcs, float sinr, float distance)
{
    assert(tx==T_DL||tx==T_UL); 
    if(tx == T_DL) return pdcp_dl->handle_pkt(bits,  mcs, sinr, distance);
    else return pdcp_ul->handle_pkt(bits, mcs, sinr, distance);
}

float pdcp_handler::get_generated(int tx, bool elapsed)
{
    assert(tx==T_DL||tx==T_UL); 
    if(tx == T_DL) return pdcp_dl->get_generated(elapsed);
    else return pdcp_ul->get_generated(elapsed);
}

float pdcp_handler::get_error(int tx, bool elapsed)
{
    assert(tx==T_DL||tx==T_UL); 
    if(tx == T_DL) return pdcp_dl->get_error(elapsed);
    else return pdcp_ul->get_error(elapsed);
}

float pdcp_handler::get_tp(int tx, bool elapsed)
{
    assert(tx==T_DL||tx==T_UL); 
    if(tx == T_DL) return pdcp_dl->get_tp(elapsed);
    else return pdcp_ul->get_tp(elapsed);
}

float pdcp_handler::get_latency(int tx, bool elapsed)
{
    assert(tx==T_DL||tx==T_UL); 
    if(tx == T_DL) return pdcp_dl->get_latency(elapsed);
    else return pdcp_ul->get_latency(elapsed);
}

float pdcp_handler::get_ip_latency(int tx, bool elapsed)
{
    assert(tx==T_DL||tx==T_UL); 
    if(tx == T_DL) return pdcp_dl->get_ip_latency(elapsed);
    else return pdcp_ul->get_ip_latency(elapsed);
}


