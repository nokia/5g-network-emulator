/**********************************************
* Copyright 2022 Nokia
* Licensed under the BSD 3-Clause Clear License
* SPDX-License-Identifier: BSD-3-Clause-Clear
**********************************************/

#include <functional>

#include <netfilter/pkt_capture.h>
#include <pdcp_layer/pdcp_layer.h>
#include <pdcp_layer/release_handler_ip.h>
#include <utils/terminal_logging.h>

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

pdcp_layer::pdcp_layer( int _is_ip, int _max_rtx, float _air_delay_var, 
                        float _rtx_period, float _rtx_period_var, float _rtx_proc_delay, 
                        float _rtx_proc_delay_var, float _bh_d, float _bh_d_var, bool _order_pkts, int _verbosity)
           :_harq_buffer(_max_rtx, _air_delay_var, _rtx_period, _rtx_period_var, _rtx_proc_delay, _rtx_proc_delay_var, _verbosity), 
            _ip_buffer(_verbosity)
           {
                is_ip = _is_ip != 0;
                _release_h = is_ip ? std::shared_ptr<release_handler>(new release_handler_ip(_order_pkts))
                                   : std::shared_ptr<release_handler>(new release_handler());
                bh_d = _bh_d; 
                bh_d_var = _bh_d_var;
                verbosity = _verbosity; 
           }

pdcp_layer::pdcp_layer( int _is_ip, pdcp_config pdcp_c, int _verbosity)
           :_harq_buffer(pdcp_c.max_rtx, pdcp_c.air_delay_var, 
                            pdcp_c.rtx_period, pdcp_c.rtx_period_var, pdcp_c.rtx_proc_delay, pdcp_c.rtx_proc_delay_var, _verbosity), 
            _ip_buffer(_verbosity)
            {
                is_ip = _is_ip != 0;
                _release_h = is_ip ? std::shared_ptr<release_handler>(new release_handler_ip(pdcp_c.order_packets))
                                   : std::shared_ptr<release_handler>(new release_handler());
                bh_d = pdcp_c.bh_d; 
                bh_d_var = pdcp_c.bh_d_var; 
                verbosity = _verbosity;
            }

pdcp_layer::pdcp_layer(int _queue_num, std::chrono::microseconds *_init_t, pdcp_config pdcp_c, int _verbosity)
           :pdcp_layer(1, pdcp_c, _verbosity)
{
    init_ip_capture(_queue_num, _init_t);
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
    cleanup_old_pkts();
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

void pdcp_layer::enqueue_ip_pkt(ip_pkt pkt)
{
    _ip_buffer.add_pkt(std::move(pkt));
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
            while(bits > 0 && has_pkts())
            {
                harq_pkt pkt(current_id, _ip_buffer.get_oldest_timestamp(), current_t, mcs, distance, 0, bh_d, bh_d_var);// = std::unique_ptr<std::deque<ip_pkt>>(new std::deque<ip_pkt>()); 
                current_id++; 
                _ip_buffer.get_pkts(bits, pkt);
                if(is_ip && is_pkt_too_old(pkt))
                {
                    drop_harq_pkt(std::move(pkt));
                    continue;
                }
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
        if(_harq_buffer.get_rtx(pkt.mcs_i, sinr, pkt.n_tx))
        {       
            if(is_ip)
            {
                LOG_WARNING_I("HARQ_HANDLER") << "RTX number [" << pkt.n_tx << "] for pkt [" << pkt.id << "]" << END();
            }
            if(pkt.n_tx < 4) _harq_buffer.queue(std::move(pkt), distance);
            else 
            {
                drop_harq_pkt(std::move(pkt));
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

float pdcp_layer::get_ip_pkts()
{
    if(!is_ip || !pkt_cptr) return 0.0f;

    float bits = 0.0f;
    pkt_cptr->lock();
    for(std::deque<ip_pkt>::iterator it = captured_pkts.begin(); it != captured_pkts.end();)
    {
        bits += it->size;
        enqueue_ip_pkt(std::move(*it));
        it = captured_pkts.erase(it);
    }
    pkt_cptr->unlock();
    return bits;
}

void pdcp_layer::init_pkt_capture()
{
}

void pdcp_layer::init_ip_capture(int _queue_num, std::chrono::microseconds *_init_t)
{
    init_t = _init_t;
    queue_num = _queue_num;
    pkt_cptr = std::shared_ptr<pkt_capture>(new pkt_capture(_queue_num));
    _release_h->set_pkt_cptr(pkt_cptr);
    pkt_cptr->start(std::bind(&pdcp_layer::cb, this, std::placeholders::_1, std::placeholders::_2,
                              std::placeholders::_3, std::placeholders::_4,
                              std::placeholders::_5, std::placeholders::_6));
    _ip_buffer.debug_queue_num = _queue_num;
}

void pdcp_layer::cb(void* handler, netfilter_interface_t *nfiface, uint64_t timestamp_sec,
                    uint64_t timestamp_usec, uint64_t bytes, uint32_t pkt_id)
{
    (void)handler;
    (void)nfiface;
    (void)timestamp_sec;
    (void)timestamp_usec;

    int size = (int)bytes * 8;
    pkt_cptr->lock();
    captured_pkts.push_back(ip_pkt(get_current_ts(), size, size, prev_uid, pkt_id, bh_d, bh_d_var));
    prev_uid = pkt_id;
    pkt_cptr->unlock();
}

float pdcp_layer::get_current_ts() const
{
    if(init_t == nullptr) return current_t;
    return (float)(std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch() - *init_t).count()) * 0.000001f;
}

void pdcp_layer::drop_harq_pkt(harq_pkt pkt)
{
    _ip_buffer.drop_pkt(pkt.bits);
    _release_h->drop(std::move(pkt));
}

bool pdcp_layer::is_pkt_too_old(const harq_pkt& pkt) const
{
    return (current_t - pkt.ip_t) > pkt_delay_budget_s;
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

void pdcp_layer::cleanup_old_pkts()
{
    if(!is_ip) return;

    while(_ip_buffer.has_pkts())
    {
        float oldest_ip_t = _ip_buffer.get_oldest_timestamp();
        if((current_t - oldest_ip_t) <= pkt_delay_budget_s) break;

        harq_pkt pkt(current_id, oldest_ip_t, current_t, 0, 0, 0, bh_d, bh_d_var);
        current_id++;

        if(!_ip_buffer.pop_oldest_pkt(pkt)) break;
        drop_harq_pkt(std::move(pkt));
    }
}

pdcp_queue_status pdcp_layer::get_queue_status() const
{
    pdcp_queue_status status;

    status.ip_buffer_size = _ip_buffer.size();
    const ip_pkt* oldest_ip = _ip_buffer.peek_oldest_pkt();
    if(oldest_ip != nullptr)
    {
        status.ip_oldest_uid = oldest_ip->uid;
        status.ip_oldest_age = current_t - oldest_ip->ip_t;
    }

    status.harq_size = _harq_buffer.size();
    const harq_pkt* oldest_harq = _harq_buffer.peek_oldest();
    if(oldest_harq != nullptr)
    {
        status.harq_oldest_id = oldest_harq->id;
        status.harq_oldest_age = current_t - oldest_harq->ip_t;
        status.harq_oldest_n_tx = oldest_harq->n_tx;
    }

    if(is_ip && pkt_cptr)
    {
        pkt_cptr->lock();
        status.capture_size = (int)captured_pkts.size();
        if(!captured_pkts.empty())
        {
            const ip_pkt &cap_pkt = captured_pkts.front();
            status.capture_oldest_uid = cap_pkt.uid;
            status.capture_oldest_age = current_t - cap_pkt.ip_t;
        }
        pkt_cptr->unlock();
    }

    status.pkt_delay_budget_s = pkt_delay_budget_s;
    _release_h->fill_queue_status(status, current_t);
    return status;
}
