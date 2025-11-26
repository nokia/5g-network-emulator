/**********************************************
* Copyright 2022 Nokia
* Licensed under the BSD 3-Clause Clear License
* SPDX-License-Identifier: BSD-3-Clause-Clear
**********************************************/

#pragma once

#include <mutex>

#include <pdcp_layer/pdcp_layer.h>
#include <netfilter/pkt_capture.h>
#include <utils/terminal_logging.h>

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
                    _ip_buffer.debug_queue_num = _queue_num;
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
                    _ip_buffer.debug_queue_num = _queue_num;
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
                    // LOG_INFO_I("PKT_CAPTURE") << "(" << queue_num << ")" << "UID [" << pkt_id << "] with bits [" << size << "] " << END(); 
                    
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

    pdcp_queue_status get_queue_status() const override
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

        pkt_cptr->lock();
        status.capture_size = (int)pkts.size();
        if(!pkts.empty())
        {
            const ip_pkt &cap_pkt = pkts.front();
            status.capture_oldest_uid = cap_pkt.uid;
            status.capture_oldest_age = current_t - cap_pkt.ip_t;
        }
        pkt_cptr->unlock();

        status.pkt_delay_budget_s = pkt_delay_budget_s;

        _release_h->fill_queue_status(status, current_t);

        return status;
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
                while(bits > 0 && has_pkts())
                {
                    harq_pkt pkt(current_id, _ip_buffer.get_oldest_timestamp(), current_t, mcs, distance, 0, bh_d, bh_d_var);// = std::unique_ptr<std::deque<ip_pkt>>(new std::deque<ip_pkt>()); 
                    current_id++; 
                    request_pkts(bits, pkt);
                    if(is_pkt_too_old(pkt))
                    {
                        //LOG_INFO_I("PKT_DROP") << "(" << queue_num << ")" << "UID [" << pkt.id << "] with bits [" << pkt.bits << "] " << END(); 
                        drop_pkt(std::move(pkt));
                        continue;
                    }
                    if(_harq_buffer.get_rtx(mcs, sinr, 0)) 
                    {
                        _harq_buffer.add_pkts(pkt); //FIXME std::move!!
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

private:
    void cleanup_old_pkts() override
    {
        while(_ip_buffer.has_pkts())
        {
            float oldest_ip_t = _ip_buffer.get_oldest_timestamp();
            if((current_t - oldest_ip_t) <= pkt_delay_budget_s) break;

            harq_pkt pkt(current_id, oldest_ip_t, current_t, 0, 0, 0, bh_d, bh_d_var);
            current_id++;

            if(!_ip_buffer.pop_oldest_pkt(pkt)) break;

            // LOG_INFO_I("PKT_DROP") << "(" << queue_num << ")" << "UID [" << pkt.id << "] with bits [" << pkt.bits << "] (cleanup)" << END(); 
            drop_pkt(std::move(pkt));
        }
    }

    bool is_pkt_too_old(const harq_pkt& pkt) const
    {
        return (current_t - pkt.ip_t) > pkt_delay_budget_s;
    }
};
