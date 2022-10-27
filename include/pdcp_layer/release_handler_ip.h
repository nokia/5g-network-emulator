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
            if(!add_data(jt->uid, jt->size) && jt->size > 10)
            {
                out_pkts.push_back(std::move(*jt));    
            }  
        }
        sort_by_id(); 
    }

    bool add_data(int id, float bits)
    {
        for(std::deque<ip_pkt>::iterator jt=out_pkts.begin(); jt!=out_pkts.end();jt++)
        {
            if(jt->uid == id)
            {
                jt->size += bits;
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
    
    float release() override
    {
        int count = 0; 
        float bits = 0; 
        float latency = 0;
        float ip_latency = 0; 
        for(std::deque<ip_pkt>::iterator it=out_pkts.begin(); it!=out_pkts.end();)
        {
            if(it->is_ready())
            {
                if(it->erase)
                {
                    update_order(it->uid);
                    pkt_cptr->drop(it->uid);
                    it = out_pkts.erase(it);
                }
                else{
                    if(check_order(it->prev_uid))
                    {
                        if(it->t_out <= current_t)
                        {
                            bits += it->original_size;                            
                            update_order(it->uid);
                            pkt_cptr->release(it->uid);
                            it = out_pkts.erase(it);
                            latency += current_t - it->current_t;
                            ip_latency += current_t - it->ip_t;
                            count++; 
                        }
                        else break;
                    }
                    else break;
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
};
