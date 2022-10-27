/**********************************************
* Copyright 2022 Nokia
* Licensed under the BSD 3-Clause Clear License
* SPDX-License-Identifier: BSD-3-Clause-Clear
**********************************************/

#include <mac_layer/mac_layer.h>

mac_layer::mac_layer(bool _threading, std::vector<ue> *ue_list, int _mimo_layers, int _numerology, int _n_re_freq, int _n_ofdm_syms, int _bandwidth, int _scheduling_mode,
        int _scheduling_type, int _scheduling_config, int _metric_type, 
        int _duplexing_type, tdd_config _tdd_c, log_config log_c, int _verbosity)
        : grid_dl(T_DL, _mimo_layers, _numerology, _n_re_freq, _n_ofdm_syms, _metric_type,
                    _bandwidth, _scheduling_mode, _scheduling_type,
                    _scheduling_config, _duplexing_type, _tdd_c, _verbosity),
        grid_ul(T_UL, _mimo_layers, _numerology, _n_re_freq, _n_ofdm_syms, _metric_type,
                _bandwidth, _scheduling_mode, _scheduling_type,
                _scheduling_config, _duplexing_type, _tdd_c, _verbosity)
        {
            threading = _threading;
            if(threading) 
            {
                tp.init(2);
                tp.do_job(std::bind (&grid::step, &grid_dl));   
                tp.do_job(std::bind (&grid::step, &grid_ul));   
            }

            verbosity = _verbosity; 
            log = log_c.log_mac;
            if(log)
            {
                logger_dl.init("logs/" + log_c.log_id + "/mac/", "grid_log_dl", log_c.log_freq);
                grid_dl.set_logger(&logger_dl);
                logger_ul.init("logs/" + log_c.log_id + "/mac/", "grid_log_ul", log_c.log_freq);
                grid_ul.set_logger(&logger_ul);
            }
            bandwidth = _bandwidth; 
            grid_dl.init(ue_list); 
            grid_ul.init(ue_list); 
        }

mac_layer::mac_layer(bool _threading, std::vector<ue> *ue_list, mac_config mac_c, tdd_config _tdd_c, log_config log_c, int _verbosity)
        : grid_dl(T_DL, mac_c.mimo_layers, mac_c.numerology, mac_c.n_re_freq, mac_c.n_ofdm_syms, mac_c.metric_type,
                    mac_c.bandwidth, mac_c.scheduling_mode, mac_c.scheduling_type,
                    mac_c.scheduling_config, mac_c.duplexing_type, _tdd_c),
            grid_ul(T_UL, mac_c.mimo_layers, mac_c.numerology, mac_c.n_re_freq, mac_c.n_ofdm_syms, mac_c.metric_type, 
                    mac_c.bandwidth, mac_c.scheduling_mode, mac_c.scheduling_type,
                    mac_c.scheduling_config, mac_c.duplexing_type, _tdd_c)
            {
                threading = _threading;
                if(threading) 
                {
                    tp.init(8);
                    tp.do_job(std::bind (&grid::step, &grid_dl));
                    tp.do_job(std::bind (&grid::step, &grid_ul));
                } 

                verbosity = _verbosity; 
                log = log_c.log_mac;
                if(log)
                {
                    logger_dl.init("logs/" + log_c.log_id + "/mac/", "grid_log_dl", log_c.log_freq);
                    grid_dl.set_logger(&logger_dl);
                    logger_ul.init("logs/" + log_c.log_id + "/mac/", "grid_log_ul", log_c.log_freq);
                    grid_ul.set_logger(&logger_ul);
                }
                bandwidth = mac_c.bandwidth; 
                grid_dl.init(ue_list); 
                grid_ul.init(ue_list); 
            }

void mac_layer::plot_info()
{
    grid_dl.plot_info();
    grid_ul.plot_info();
}

void mac_layer::plot_info(int tx)
{
    if(tx == T_DL) grid_dl.plot_info();
    if(tx == T_UL) grid_ul.plot_info();
}

int mac_layer::get_bandwidth(){ return bandwidth; }
int mac_layer::get_n_freq_rb() { return grid_dl.get_n_freq_rb(); }
int mac_layer::get_n_freq_rbg() { return grid_dl.get_n_freq_rbg(); }
int mac_layer::get_n_sc_rbg() { return grid_dl.get_n_sc_rbg(); }
int mac_layer::get_rbg_size() { return grid_dl.get_rbg_size(); }
int mac_layer::get_logical_units() { return grid_dl.get_logical_units(); }
int mac_layer::get_n_time_rb() { return grid_dl.get_n_time_rb(); }

void mac_layer::init(std::vector<ue> *ue_list)
{
    grid_dl.init(ue_list);
    grid_ul.init(ue_list);
}

void mac_layer::flush_logs()
{
    if(log)
    {
        logger_dl.flush(); 
        logger_ul.flush(); 
    }
}

void mac_layer::update_ts(float t)
{
    grid_dl.add_current_ts(t);
    grid_ul.add_current_ts(t);
}

void mac_layer::step(float current_t)
{  

    update_ts(current_t);    
    if(threading)
    {
        tp.do_jobs();
        tp.wait_threads();
    }
    else
    {
        grid_dl.step();
        grid_ul.step();
    }
    flush_logs(); 
}