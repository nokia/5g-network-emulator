/**********************************************
* Copyright 2022 Nokia
* Licensed under the BSD 3-Clause Clear License
* SPDX-License-Identifier: BSD-3-Clause-Clear
**********************************************/

#include <pdcp_layer/pdcp_layer.h>

pdcp_layer::pdcp_layer( int _max_rtx, float _air_delay_var, 
                        float _rtx_period, float _rtx_period_var, float _rtx_proc_delay, 
                        float _rtx_proc_delay_var, float _bh_d, float _bh_d_var, int _verbosity)
           :_harq_buffer(_max_rtx, _air_delay_var, _rtx_period, _rtx_period_var, _rtx_proc_delay, _rtx_proc_delay_var, _verbosity), 
            _ip_buffer(_verbosity)
           {
                _release_h = std::shared_ptr<release_handler>(new release_handler());
                bh_d = _bh_d; 
                bh_d_var = _bh_d_var;
                verbosity = _verbosity; 
           }

pdcp_layer::pdcp_layer( pdcp_config pdcp_c, int _verbosity)
           :_harq_buffer(pdcp_c.max_rtx, pdcp_c.air_delay_var, 
                            pdcp_c.rtx_period, pdcp_c.rtx_period_var, pdcp_c.rtx_proc_delay, pdcp_c.rtx_proc_delay_var, _verbosity), 
            _ip_buffer(_verbosity)
            {
                _release_h = std::shared_ptr<release_handler>(new release_handler());
                bh_d = pdcp_c.bh_d; 
                bh_d_var = pdcp_c.bh_d_var; 
                verbosity = _verbosity;
            }

pdcp_layer::pdcp_layer( int is_ip, int _max_rtx, float _air_delay_var, 
                        float _rtx_period, float _rtx_period_var, float _rtx_proc_delay, 
                        float _rtx_proc_delay_var, float _bh_d, float _bh_d_var, bool _order_pkts, int _verbosity)
           :_harq_buffer(_max_rtx, _air_delay_var, _rtx_period, _rtx_period_var, _rtx_proc_delay, _rtx_proc_delay_var, _verbosity), 
            _ip_buffer(_verbosity)
           {
                _release_h = std::shared_ptr<release_handler>(new release_handler_ip(_order_pkts));
                bh_d = _bh_d; 
                bh_d_var = _bh_d_var;
                verbosity = _verbosity; 
           }

pdcp_layer::pdcp_layer( int is_ip, pdcp_config pdcp_c, int _verbosity)
           :_harq_buffer(pdcp_c.max_rtx, pdcp_c.air_delay_var, 
                            pdcp_c.rtx_period, pdcp_c.rtx_period_var, pdcp_c.rtx_proc_delay, pdcp_c.rtx_proc_delay_var, _verbosity), 
            _ip_buffer(_verbosity)
            {
                _release_h = std::shared_ptr<release_handler>(new release_handler_ip(pdcp_c.order_packets));
                bh_d = pdcp_c.bh_d; 
                bh_d_var = pdcp_c.bh_d_var; 
                verbosity = _verbosity;
            }

void pdcp_layer::exit()
{
    _release_h->quit(); 
}

void pdcp_layer::init(int _mod_i, int _layers, int _logic_units)
{
    _harq_buffer.init(_mod_i, _layers, _logic_units);
    _release_h->init(); 
}

float pdcp_layer::release()
{
	return _release_h->release_one(); 
}

void pdcp_layer::step(float t)
{
    current_t = t;
    _ip_buffer.step(current_t);
    _harq_buffer.step(current_t);
    _release_h->step(current_t);
}

float pdcp_layer::request_pkts(float bits)
{
    return _ip_buffer.get_pkts(bits);
}

void pdcp_layer::drop_pkt(int bits)
{
    _ip_buffer.drop_pkt(bits);
}

void pdcp_layer::generate_pkts(float bits, float pkt_size, float t)
{
    _ip_buffer.generate(bits, pkt_size, t, bh_d, bh_d_var);
}

/*
float pdcp_layer::release_pkts(ip_pkt pkt)
{
    float bits = 0; 
    if(pkt.is_fragment)
    {
        bits = pkt.original_size;
        _release_h.push(pkt);
    }
    _ip_buffer.consume_bits(bits);
    return bits; 
}

float pdcp_layer::release_pkts(std::deque<ip_pkt> *pkts)
{
    float bits = 0; 
    for(int i = 0; i < pkts->size(); i++)
    {
        ip_pkt *pkt = &(*pkts)[i];
        if(!pkt->is_fragment)
        {
            bits += pkt->original_size;
            _release_h.push(*pkt);
        }
    }
    _ip_buffer.consume_bits(bits);
    return bits; 
}
*/

void pdcp_layer::release_pkts(harq_pkt pkt)
{
    _release_h->push(std::move(pkt));
}

bool pdcp_layer::has_pkts()
{
    return _ip_buffer.has_pkts() || _harq_buffer.is_pkt_ready();
}

float pdcp_layer::get_oldest_timestamp()
{
    if(_harq_buffer.is_pkt_ready())
    {
        return _harq_buffer.get_oldest_t();
    }
    else if(_ip_buffer.has_pkts())
    {
        return _ip_buffer.get_oldest_timestamp(); 
    }
    return current_t; 
}

float pdcp_layer::handle_pkt(float bits, int mcs, float sinr, float distance)
{
    if(!_harq_buffer.is_pkt_ready())
    {
        if(has_pkts())
        {
             if(bits > 0)
             {
                harq_pkt pkt(current_id, _ip_buffer.get_oldest_timestamp(), current_t, mcs, distance, 0, bh_d, bh_d_var);// = std::unique_ptr<std::deque<ip_pkt>>(new std::deque<ip_pkt>()); 
                current_id++; 
                pkt.bits = request_pkts(bits);
                if(_harq_buffer.get_rtx(mcs, sinr, 0)) 
                {
                    _harq_buffer.add_pkts(std::move(pkt));
                   return 0; 
                }
                else
                {   
                    bits = pkt.bits;
                    release_pkts(std::move(pkt));
                    return bits; 
                }       
            }
			return 0.0; 
        }
		return 0.0; 
    }
    else
    {
        harq_pkt pkt = _harq_buffer.get_pkt();
        if(_harq_buffer.get_rtx(mcs, sinr, pkt.n_tx))
        {       
            if(pkt.n_tx < 4) _harq_buffer.queue(std::move(pkt), distance);
            else 
            {
                drop_pkt(pkt.bits);
            }
            return 0; 
        }
        else
        {
            bits = pkt.bits; 
            release_pkts(std::move(pkt));
            return bits; 
        } 
    }
}

float pdcp_layer::get_generated(bool partial)
{
    return _ip_buffer.get_generated(partial); 
}


float pdcp_layer::get_error(bool partial)
{
    return _ip_buffer.get_error(partial); 
}

float pdcp_layer::get_latency(bool partial)
{
    return _release_h->get_latency(partial); 
}


float pdcp_layer::get_ip_latency(bool partial)
{
    return _release_h->get_ip_latency(partial); 
}


float pdcp_layer::get_tp(bool partial)
{
    return _release_h->get_tp(partial); 
}
