/**********************************************
* Copyright 2022 Nokia
* Licensed under the BSD 3-Clause Clear License
* SPDX-License-Identifier: BSD-3-Clause-Clear
**********************************************/

#ifndef UE_HANDLER_H
#define UE_HANDLER_H

#include <iostream>
#include <vector>

#include <ue/ue.h>
#include <threading/thread_pool.h>

//--------------------------------------------------------------------------------------------------
// ue_handler(): this helper class gets the UE-related configuration parameters and create all the
// emulated UEs. 
// Input: 
//      _threads: number of concurrent threads to be created. 
//--------------------------------------------------------------------------------------------------
class ue_handler
{
public: 
    ue_handler(int _threads)
    {
        threads = _threads; 
        multithreading = (threads > 0);
        if(multithreading) tp.init(threads);
        n_ues = 0; 
    }

public: 
    void add_ues(std::chrono::microseconds * _init_t, ue_full_config ue_c, phy_enb_config _phy_enb_config, scenario_config _scenario_c,  pdcp_config _pdcp_config_ul, pdcp_config _pdcp_config_dl, harq_config _harq_config, bool _stochastics = true)
    {
        n_ues += ue_c.n_ues; 
        int init_ids = id; 
        if(ue_c.ue_type == SIM_UE)
        {
            for(; id < init_ids + ue_c.n_ues; id++)
            {
                ue_list.emplace_back(id, ue_c.ue_c, _scenario_c, _phy_enb_config, _pdcp_config_ul, _pdcp_config_dl, _harq_config, _stochastics);  
            }
        }
        else
        {
            for(; id < init_ids + ue_c.n_ues; id++)
            {
                ue_list.emplace_back(ue_c.ue_c.ul_queue_n, ue_c.ue_c.dl_queue_n, _init_t, id, ue_c.ue_c, _scenario_c, _phy_enb_config, _pdcp_config_ul, _pdcp_config_dl, _harq_config, _stochastics);  
            }
        }
        
        log_ue_creation(id, ue_c.ue_type, ue_c.ue_c.ue_m.n_antennas, ue_c.ue_c.ue_m.cqi_period, ue_c.ue_c.ue_m.ri_period, ue_c.ue_c.ue_m.scaling_factor);
    }

    void init()
    {
        for(std::vector<ue>::iterator it = ue_list.begin(); it != ue_list.end(); it++)
        {
            if(multithreading) tp.do_job(std::bind (&ue::step, it));
            it->init(); 
        }
    }

    void add_ue(std::chrono::microseconds * _init_t, ue_full_config ue_c, phy_enb_config _phy_enb_config, scenario_config _scenario_c, pdcp_config _pdcp_config_ul, pdcp_config _pdcp_config_dl, harq_config _harq_config, bool _stochastics = true)
    {
        n_ues++; 
        id++; 
        if(ue_c.ue_type == SIM_UE) ue_list.emplace_back(id, ue_c.ue_c, _scenario_c, _phy_enb_config, _pdcp_config_ul, _pdcp_config_dl, _harq_config, _stochastics);
        if(ue_c.ue_type == REAL_UE) ue_list.emplace_back(ue_c.ue_c.ul_queue_n, ue_c.ue_c.dl_queue_n,_init_t, id, ue_c.ue_c, _scenario_c, _phy_enb_config, _pdcp_config_ul, _pdcp_config_dl, _harq_config, _stochastics);

        log_ue_creation(id, ue_c.ue_type, ue_c.ue_c.ue_m.n_antennas,
                        ue_c.ue_c.ue_m.cqi_period, ue_c.ue_c.ue_m.ri_period, ue_c.ue_c.ue_m.scaling_factor);
        tp.do_job(std::bind (&ue::step, &ue_list.back()));
        ue_list.back().init(); 
}

public: 


    void step(double _current_t)
    {
        //std::chrono::steady_clock::time_point t1 = std::chrono::steady_clock::now();
        for(int i = 0; i < ue_list.size(); i++)
        {
            ue_list[i].add_current_t(_current_t);
        }
        
        if(multithreading)
        {
            tp.do_jobs();
            tp.wait_threads();
        }

        else
        {
            for(int i = 0; i < ue_list.size(); i++)
            {
                ue_list[i].step();
            }
        }
    }

    void print_traffic()
    {
        float total_tp_ul = 0;
        float total_tp_dl = 0;
        float total_l_ul = 0;
        float total_l_dl = 0;
        float total_verb_ue = 0;
        for(int i = 0; i < ue_list.size(); i++)
        {
            ue_list[i].print_traffic(); 
            if(ue_list[i].get_traffic_verbosity())
            {
                total_tp_dl += ue_list[i].get_avg_tp(TX_DL);
                total_tp_ul += ue_list[i].get_avg_tp(TX_UL);
                total_l_dl += ue_list[i].get_avg_l(TX_DL);
                total_l_ul += ue_list[i].get_avg_l(TX_UL);
                total_verb_ue += 1;
            }
        }
        if(total_verb_ue>0)
        {
            LOG_INFO_I("ue_handler::print_traffic") << " Total DL throughput transmitted: " << total_tp_dl << " Mbps - with a mean value per user of: " << total_tp_dl/total_verb_ue << " Mbps." << END(); 
            LOG_INFO_I("ue_handler::print_traffic") << " Total UL throughput transmitted: " << total_tp_ul << " Mbps - with a mean value per user of: " << total_tp_ul/total_verb_ue << " Mbps." << END(); 
            LOG_INFO_I("ue_handler::print_traffic") << " Mean DL latency per user of: " << total_l_dl/total_verb_ue << " s." << END(); 
            LOG_INFO_I("ue_handler::print_traffic") << " Mean UL latency per user of: " << total_l_ul/total_verb_ue << " s." << END(); 
        }
    }

public: 
    std::vector<ue>* get_ue_list()
    {
        return &ue_list; 
    }

private: 
    
    void log_ue_creation(int _n_ues, int ue_type, int _n_antennas, int _cqi_period, int _ri_period, float scaling_factor)
    {
        LOG_INFO_I("ue_handler::log_ue_creation") << " Created " << _n_ues << " with the following configuration: " << END(); 
        LOG_INFO_I("ue_handler::log_ue_creation") << " Type;  " << ((ue_type == SIM_UE)? "SIM_UE" : "REAL_UE") << " with the following configuration: " << END(); 
        LOG_INFO_I("ue_handler::log_ue_creation") << " Number of antennas: " << _n_antennas << END(); 
        LOG_INFO_I("ue_handler::log_ue_creation") << " Scaling factor: " << scaling_factor << END(); 
        LOG_INFO_I("ue_handler::log_ue_creation") << " CQI update period: " << _cqi_period << END(); 
        LOG_INFO_I("ue_handler::log_ue_creation") << " RI period: " << _ri_period << END(); 
    }
private: 
    std::vector<ue> ue_list; 
    thread_pool tp; 
    int threads; 
    int n_ues; 
    int id = 0; 
    bool multithreading; 
    int n_freq_rbg = 0; 
    int bandwidth; 
};
#endif