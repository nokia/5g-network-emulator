/**********************************************
* Copyright 2022 Nokia
* Licensed under the BSD 3-Clause Clear License
* SPDX-License-Identifier: BSD-3-Clause-Clear
**********************************************/

#pragma once

#include <netfilter/pkt_capture.h>
#include <utils/terminal_logging.h>
#include <pdcp_layer/release_handler.h>

#include <algorithm>


class release_handler_ip: public release_handler
{
public: 
    release_handler_ip(bool _do_check)
    : release_handler()
    {
        do_check = _do_check; 

    }

public: 
    void set_pkt_cptr(const std::shared_ptr<pkt_capture> &_pkt_cptr) override
    {
        pkt_cptr = _pkt_cptr; 
        debug_queue_num = pkt_cptr->queue_num;
    }

public: 
    
    void sort_by_id()
    {
        // and a few lines down
        sort(out_pkts.begin(),out_pkts.end(), sorter);
    }

    void push(harq_pkt pkt) override
    {
        pkt.t_out += pkt.backhaul_d + gauss_dist(gauss_dist_gen)*pkt.backhaul_d_var;
        // Store ip pkts
        for(std::deque<ip_pkt>::iterator jt=pkt.pkts.begin(); jt!=pkt.pkts.end();jt++)
        {
            jt->t_out = pkt.t_out; 
            jt->ip_t = pkt.ip_t; 
            if(!add_data(&(*jt), pkt.id) /*&& jt->size > TEN_BIT_ROUND_MARGIN*/)
            {
                jt->frags_recovered++;
                out_pkts.push_back(std::move(*jt));
                // LOG_INFO_I("PKT_PUSH") << "(" << debug_queue_num << ")" << "UID [" << jt->uid << "." << pkt.id << 
                //     "]  frags [" << jt->frags_recovered << "/"  << jt->frags_created <<
                //     "]  bits [" << jt->size << "/"  << jt->size << "/" << jt->original_size << "] NEW" << END();
            }
        }
        sort_by_id(); 
    }

    bool add_data(ip_pkt *recv_pkt, int harq_id)
    {
        for(std::deque<ip_pkt>::iterator jt=out_pkts.begin(); jt!=out_pkts.end();jt++)
        {
            if(jt->uid == recv_pkt->uid)
            {
                jt->size += recv_pkt->size;
                jt->frags_recovered++;
                jt->frags_created = recv_pkt->frags_created;
                // LOG_INFO_I("PKT_PUSH") << "(" << debug_queue_num << ")" << "UID [" << jt->uid << "." << harq_id << 
                //     "]  frags [" << jt->frags_recovered << "/"  << jt->frags_created <<
                //     "]  bits [" << recv_pkt->size << "/"  << jt->size << "/" << jt->original_size << "]" << END();
                return true; 
            }
        }
        return false; 
    }

    bool remove_data(int id, float bits)
    {
        for(std::deque<ip_pkt>::iterator jt=out_pkts.begin(); jt!=out_pkts.end();jt++)
        {
            if(jt->uid == id)
            {
                jt->erase = true; 
                jt->size += bits; 
                return true; 
            }
        }
        return false; 
    }

    void drop( harq_pkt pkt)
    {
        for(std::deque<ip_pkt>::iterator jt=pkt.pkts.begin(); jt!=pkt.pkts.end();jt++)
        {
            jt->t_out = pkt.t_out; 
            jt->ip_t = pkt.ip_t; 
            if(!remove_data(jt->uid, jt->size))
            {
                jt->erase = true; 
                out_pkts.push_back(std::move(*jt));    
            }  
        } 
    }

    void fill_queue_status(pdcp_queue_status& status, float current_t) const override
    {
        status.release_size = (int)out_pkts.size();
        if(!out_pkts.empty())
        {
            const ip_pkt &pkt = out_pkts.front();
            status.release_oldest_uid = pkt.uid;
            status.release_oldest_age = current_t - pkt.ip_t;
        }
    }
    
    float release() override
    {
        int count = 0; 
        float bits = 0; 
        float latency = 0;
        float ip_latency = 0; 
        for(std::deque<ip_pkt>::iterator it=out_pkts.begin(); it!=out_pkts.end();)
        {
            // LOG_INFO_I("PKT_RELEASE") << "(" << debug_queue_num << ")" << "UID [" << it->uid << "] with bits [" << it->size << "/" << it->original_size << "] IS_READY [" << it->is_ready() << "]" << END();

            if(it->is_ready())
            {
                if(it->erase)
                {
                    // LOG_INFO_I("PKT_RELEASE") << "(" << debug_queue_num << ")" <<"UID [" << it->uid << "] DROP" << END();
                    update_order(it->uid);
                    pkt_cptr->drop(it->uid);
                    it = out_pkts.erase(it);
                }
                else{
                    if(check_order(it->prev_uid))
                    {
                        if(it->t_out <= current_t)
                        {
                            // LOG_INFO_I("PKT_RELEASE") << "(" << debug_queue_num << ")" << "UID [" << it->uid << "] RELEASE" << END(); 
                            bits += it->original_size;                            
                            update_order(it->uid);
                            pkt_cptr->release(it->uid);
                            it = out_pkts.erase(it);
                            latency += current_t - it->current_t;
                            ip_latency += current_t - it->ip_t;
                            count++;
                        }
                        else
                        {
                            // LOG_INFO_I("PKT_RELEASE") << "(" << debug_queue_num << ")" << "UID [" << it->uid << "] WAIT ms " << current_t - it->t_out  << END(); 
                            break;
                        }
                    }
                    else { 
                        // LOG_INFO_I("PKT_RELEASE") << "(" << debug_queue_num << ")" << "UID [" << it->uid << "] WAIT uid " << it->prev_uid << END(); 
                        break;
                    }
                }
            }
            else break; 
        }
        tp_mean.add(bits);
        if(count > 0)
        {
            l_mean.add(latency/count);
            ipl_mean.add(ip_latency/count);
            l_mean.step();
            ipl_mean.step();
        }          
        return bits; 
    }

private: 

    bool check_order(int id)
    {
        if(prev_id > -1 && do_check) return prev_id == id; 
        else return true; 
    }

    void update_order(int id)
    {
        prev_id = id; 
    }

private: 
    bool do_check = true; 
    std::deque<ip_pkt> out_pkts; 
    std::shared_ptr<pkt_capture> pkt_cptr; 
    int debug_queue_num =  -1;
};
