/**********************************************
* Copyright 2022 Nokia
* Licensed under the BSD 3-Clause Clear License
* SPDX-License-Identifier: BSD-3-Clause-Clear
**********************************************/

#pragma once

#include <mutex>

#include <pdcp_layer/pdcp_layer.h>
#include <netfilter/pkt_capture.h>

class pdcp_layer_ip: public pdcp_layer
{
public: 
    pdcp_layer_ip( int _queue_num, std::chrono::microseconds * _init_t, int _max_rtx, float _air_delay_var, 
                float _rtx_period, float _rtx_period_var, float _rtx_proc_delay, 
                float _rtx_proc_delay_var, float _bh_d, float _bh_d_var, int _verbosity = 0)
                : pkt_cptr(std::shared_ptr<pkt_capture>(new pkt_capture(_queue_num))),
                  pdcp_layer(true, _max_rtx, _air_delay_var, _rtx_period, _rtx_period_var, _rtx_proc_delay, _rtx_proc_delay_var, _bh_d, _bh_d_var, _verbosity)
                {
                    init_t = _init_t; 
                    queue_num = _queue_num; 
                    _release_h->set_pkt_cptr(pkt_cptr);
                    pkt_cptr->start(std::bind(&pdcp_layer_ip::cb, this, std::placeholders::_1, std::placeholders::_2, 
													std::placeholders::_3, std::placeholders::_4, 
													std::placeholders::_5, std::placeholders::_6));
                }
    pdcp_layer_ip( int _queue_num, std::chrono::microseconds * _init_t, pdcp_config pdcp_c, int _verbosity = 0)
                : pdcp_layer(true, pdcp_c, _verbosity)
                {
                    init_t = _init_t; 
                    queue_num = _queue_num; 
                    pkt_cptr = std::shared_ptr<pkt_capture>(new pkt_capture(_queue_num));
                    _release_h->set_pkt_cptr(pkt_cptr);
                    pkt_cptr->start(std::bind(&pdcp_layer_ip::cb, this, std::placeholders::_1, std::placeholders::_2, 
													std::placeholders::_3, std::placeholders::_4, 
													std::placeholders::_5, std::placeholders::_6));
                }
private: 
    int pprev_id=0;
public: 
    float get_ip_pkts() override
    {
        float bits = 0; 
        pkt_cptr->lock();
        for(std::deque<ip_pkt>::iterator it=pkts.begin(); it!=pkts.end();)
        {
            bits += it->size;
            _ip_buffer.add_pkt(std::move(*it));
            it = pkts.erase(it);
        }
        pkt_cptr->unlock();
        return bits; 
    }

    void init_pkt_capture() override
    {      
    }

    void cb(void* handler, netfilter_interface_t *nfiface, uint64_t timestamp_sec, uint64_t timestamp_usec, 
               uint64_t bytes, uint32_t pkt_id)
               {    
                    int size = (int)bytes*8;
                    pkt_cptr->lock();
                    pkts.push_back(ip_pkt(get_current_ts(), size, size, prev_id, pkt_id, bh_d, bh_d_var));
                    prev_id = pkt_id; 
                    pkt_cptr->unlock();
               }


private: 

    float get_current_ts()
    {
        return (float)(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch() - *init_t).count())*0.000001f; 
    }

private: 
    std::chrono::microseconds * init_t; 

private: 
    uint32_t prev_id = -1; 

private: 
    std::shared_ptr<pkt_capture> pkt_cptr; 
    int queue_num; 

private: 
    std::deque<ip_pkt> pkts; 

public: 
    float request_pkts(float bits, harq_pkt & pkt)
    {
        return _ip_buffer.get_pkts(bits, pkt);
    }
    
    void drop_pkt(harq_pkt pkt)
    {
        _ip_buffer.drop_pkt(pkt.bits);
        _release_h->drop(std::move(pkt));
    }

    float handle_pkt(float bits, int mcs, float sinr, float distance) override
    {
        if(!_harq_buffer.is_pkt_ready())
        {
            if(has_pkts())
            {
                if(bits > 0)
                {
                    harq_pkt pkt(current_id, _ip_buffer.get_oldest_timestamp(), current_t, mcs, distance, 0, bh_d, bh_d_var);// = std::unique_ptr<std::deque<ip_pkt>>(new std::deque<ip_pkt>()); 
                    current_id++; 
                    request_pkts(bits, pkt);
                    if(_harq_buffer.get_rtx(mcs, sinr, 0)) 
                    {
                        _harq_buffer.add_pkts(pkt);
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
                LOG_WARNING_I("HARQ_HANDLER") << "RTX number [" << pkt.n_tx << "] for pkt [" << pkt.id << "]" << END(); 
                if(pkt.n_tx < 4) _harq_buffer.queue(pkt, distance);
                else 
                {
                    drop_pkt(std::move(pkt));
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
};
