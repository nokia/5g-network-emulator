/**********************************************
* Copyright 2022 Nokia
* Licensed under the BSD 3-Clause Clear License
* SPDX-License-Identifier: BSD-3-Clause-Clear
**********************************************/

#include <phy_layer/phy_handler.h>


phy_handler::phy_handler(int id, scenario_config _scenario_c, phy_ue_config _phy_ue_config, phy_enb_config _phy_enb_config, bool _stochastics, int _verbosity)
            :phy_dl(TX_DL, id, _scenario_c, _phy_ue_config, _phy_enb_config, _stochastics, _verbosity),
             phy_ul(TX_UL, id, _scenario_c, _phy_ue_config, _phy_enb_config, _stochastics, _verbosity)
             {
                 verbosity = _verbosity;
             }

void phy_handler::init(int n_rbs, int bandwidth)
{
    phy_dl.init(n_rbs, bandwidth); 
    phy_ul.init(n_rbs, bandwidth); 
}

int phy_handler::get_cqi(int tx_dir, int f_index)
{
    if(tx_dir == TX_DL) return phy_dl.get_cqi(f_index);
    if(tx_dir == TX_UL) return phy_ul.get_cqi(f_index);
    return -1; 
}

float phy_handler::get_eff(int tx_dir, int f_index)
{
    if(tx_dir == TX_DL) return phy_dl.get_eff(f_index);
    if(tx_dir == TX_UL) return phy_ul.get_eff(f_index);
    return -1; 
}

int phy_handler::get_mcs(int tx_dir, int f_index)
{
    if(tx_dir == TX_DL) return phy_dl.get_mcs(f_index);
    if(tx_dir == TX_UL) return phy_ul.get_mcs(f_index);
    return -1; 
}

int phy_handler::get_ri(int tx_dir)
{
    if(tx_dir == TX_DL) return phy_dl.get_ri();
    if(tx_dir == TX_UL) return phy_ul.get_ri();
    return -1; 
}

float phy_handler::get_sinr(int tx_dir, int f_index)
{
    if(tx_dir == TX_DL) return phy_dl.get_sinr(f_index);
    if(tx_dir == TX_UL) return phy_ul.get_sinr(f_index);
    return -100; 
}

float phy_handler::get_linear_sinr(int tx_dir, int f_index)
{
    if(tx_dir == TX_DL) return phy_dl.get_linear_sinr(f_index);
    if(tx_dir == TX_UL) return phy_ul.get_linear_sinr(f_index);
    return -1; 
}

float phy_handler::get_mean_sinr(int tx_dir)
{
    if(tx_dir == TX_DL) return phy_dl.get_mean_sinr();
    if(tx_dir == TX_UL) return phy_ul.get_mean_sinr();
    return -100; 
}

float phy_handler::get_metric(int tx_dir, int f, int n_ues)
{
    if(tx_dir == TX_DL) return phy_dl.get_metric(f, n_ues);
    if(tx_dir == TX_UL) return phy_ul.get_metric(f, n_ues);
    return -1.0;
}

float phy_handler::get_tp(int tx_dir, int f)
{
    if(tx_dir == TX_DL) return phy_dl.get_tp(f);
    if(tx_dir == TX_UL) return phy_ul.get_tp(f);
    return -1.0;
}

void phy_handler::set_logger(log_handler * _logger)
{
    phy_dl.set_logger(_logger);
    phy_ul.set_logger(_logger);
}

void phy_handler::estimate_channel_estate(int tx_dir, float distance, const pos2d &pos, float oldest_t, float avg_tp, float _current_t)
{
    if(tx_dir == TX_DL) phy_dl.estimate_channel_state(distance, pos, oldest_t, avg_tp, _current_t);
    else if(tx_dir == TX_UL) phy_ul.estimate_channel_state(distance, pos, oldest_t, avg_tp, _current_t);
}

