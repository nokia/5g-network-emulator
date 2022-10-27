/**********************************************
* Copyright 2022 Nokia
* Licensed under the BSD 3-Clause Clear License
* SPDX-License-Identifier: BSD-3-Clause-Clear
**********************************************/

#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <thread>
#include <iostream>
#include <functional>

#include <simulator/configuration_loader.h>
#include <ue/ue_handler.h>
#include <mac_layer/mac_layer.h>
#include <timer/timer.h>
#include <utils/terminal_logging.h>

class simulator
{
public:
    simulator(std::string config_dir)
    :config_loader(config_dir), 
     mac_l(config_loader.get_threading(), ue_h.get_ue_list(), config_loader.get_mac_config(), config_loader.get_tdd_config(), config_loader.get_log_config()), 
     ue_h(config_loader.get_threads()), 
     ticker(config_loader.get_period(), config_loader.get_duration())
     {   
        verbosity = config_loader.get_log_config().verbosity; 
        duration = config_loader.get_duration(); 
        std::list<ue_full_config> ue_c_list = config_loader.get_ue_c_list();
        phy_enb_config phy_c = config_loader.get_phy_enb_config(); 
        phy_c.n_rbgs = mac_l.get_n_freq_rbg();
        phy_c.n_sc_rbg = mac_l.get_n_sc_rbg();
        pdcp_config pdcp_c_ul = config_loader.get_pdcp_config_ul(); 
        pdcp_config pdcp_c_dl = config_loader.get_pdcp_config_dl(); 
        scenario_config scenario_c = config_loader.get_scenario_config(); 
        for(std::list<ue_full_config>::iterator it = ue_c_list.begin(); it != ue_c_list.end(); it++)
        {
            harq_config harq_c(phy_c.modulation_m, it->ue_c.get_phy_config().max_ri, mac_l.get_logical_units());
            ue_h.add_ues(ticker.get_init_t(), *it, phy_c, scenario_c , pdcp_c_ul, pdcp_c_ul, harq_c, config_loader.get_stochastics());
        }
        ue_h.init();
    }

public: 

    void override_duration(float _duration)
    {

    }

    void start()
    {
        ticker.start(std::bind(&simulator::step, this, std::placeholders::_1));
    }

    void join()
    {
        ticker.wait_finished();
    }
    
    void terminate()
    {
        ticker.stop(); 
    }

    void print_traffic()
    {
        ue_h.print_traffic();
    }
private: 

    void handle_time_log(unsigned int _ts)
    {
        step_c += 1;
        if(step_c == freq)
        {
            LOG_INFO_I("simulator::handle_time_log") << " MAC Layer processing time: " << _ts << ":" << (time_mac.count()/step_c) << END();      
            LOG_INFO_I("simulator::handle_time_log") << " UEs processing time: " << _ts << ":" << (time_ue.count()/step_c) << END();      
            LOG_INFO_I("simulator::handle_time_log") << " Step Processing time: " << _ts << ":" << ((time_ue.count()+time_mac.count())/step_c) << END();      
            print_traffic();
            step_c = 0;
            time_mac = std::chrono::duration<double>().zero(); 
            time_ue = std::chrono::duration<double>().zero(); 
        }
    }

    void step(unsigned int _ts)
    {
        double ts = (double)_ts*US2S; 
        std::chrono::steady_clock::time_point t = std::chrono::steady_clock::now();
        mac_l.step(ts); 
        time_mac += std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - t);

        t = std::chrono::steady_clock::now();
        ue_h.step(ts);
        time_ue += std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - t);
        handle_time_log(ts); 
    }

private: 
    configuration_loader config_loader; 
    ue_handler ue_h; 
    mac_layer mac_l;
private: 
    int freq = 1000; 
    int step_c = 0; 
    std::chrono::duration<double> time_mac = std::chrono::duration<double>().zero(); 
    std::chrono::duration<double> time_ue = std::chrono::duration<double>().zero(); 
    float total_time_mac; 
    float total_time_ue; 
private: 
    timer ticker; 
private: 
    float duration;
private: 
    int verbosity = 0; 

};
#endif

