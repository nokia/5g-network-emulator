/**********************************************
 * Copyright 2022 Nokia
 * Licensed under the BSD 3-Clause Clear License
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 **********************************************/

#include <phy_layer/phy_handler.h>

phy_handler::phy_handler(int id, scenario_config _scenario_c, phy_ue_config _phy_ue_config, phy_enb_config _phy_enb_config, bool _stochastics, int _verbosity)
    : phy_dl(TX_DL, id, _scenario_c, _phy_ue_config, _phy_enb_config, _stochastics, _verbosity),
      phy_ul(TX_UL, id, _scenario_c, _phy_ue_config, _phy_enb_config, _stochastics, _verbosity)
{
    verbosity = _verbosity;
    init_amc(_phy_enb_config.modulation_m, _phy_enb_config.cqi_mode, _phy_ue_config.cqi_period, _phy_ue_config.ri_period);
    freq = _phy_enb_config.frequency;
    speed = _phy_ue_config.max_speed;
    doppler_f = freq * speed / LIGHTSPEED;
}

void phy_handler::init(int n_rbs, int bandwidth)
{
    phy_dl.init(n_rbs, bandwidth);
    phy_ul.init(n_rbs, bandwidth);
}

void phy_handler::init_amc(int _modulation_m, int _cqi_m, int _cqi_p, int _ri_period)
{
    modulation_m = _modulation_m;
    cqi_mode = _cqi_m;
    cqi_period = _cqi_p;
    ri_period = _ri_period;
}

int phy_handler::get_cqi(int tx_dir, int f_index)
{
    if (tx_dir == TX_DL)
        return phy_dl.get_cqi(f_index);
    if (tx_dir == TX_UL)
        return phy_ul.get_cqi(f_index);
    return -1;
}

float phy_handler::get_eff(int tx_dir, int f_index)
{
    if (tx_dir == TX_DL)
        return phy_dl.get_eff(f_index);
    if (tx_dir == TX_UL)
        return phy_ul.get_eff(f_index);
    return -1;
}

int phy_handler::get_mcs(int tx_dir, int f_index)
{
    if (tx_dir == TX_DL)
        return phy_dl.get_mcs(f_index);
    if (tx_dir == TX_UL)
        return phy_ul.get_mcs(f_index);
    return -1;
}

int phy_handler::get_ri(int tx_dir)
{
    if (tx_dir == TX_DL)
        return phy_dl.get_ri();
    if (tx_dir == TX_UL)
        return phy_ul.get_ri();
    return -1;
}

float phy_handler::get_sinr(int tx_dir, int f_index)
{
    if (tx_dir == TX_DL)
        return phy_dl.get_sinr(f_index);
    if (tx_dir == TX_UL)
        return phy_ul.get_sinr(f_index);
    return -100;
}

float phy_handler::get_linear_sinr(int tx_dir, int f_index)
{
    if (tx_dir == TX_DL)
        return phy_dl.get_linear_sinr(f_index);
    if (tx_dir == TX_UL)
        return phy_ul.get_linear_sinr(f_index);
    return -1;
}

float phy_handler::get_mean_sinr(int tx_dir)
{
    if (tx_dir == TX_DL)
        return phy_dl.get_mean_sinr();
    if (tx_dir == TX_UL)
        return phy_ul.get_mean_sinr();
    return -100;
}

float phy_handler::get_metric(int tx_dir, int f, int n_ues)
{
    if (tx_dir == TX_DL)
        return phy_dl.get_metric(f, n_ues);
    if (tx_dir == TX_UL)
        return phy_ul.get_metric(f, n_ues);
    return -1.0;
}

float phy_handler::get_tp(int tx_dir, int f)
{
    if (tx_dir == TX_DL)
        return phy_dl.get_tp(f);
    if (tx_dir == TX_UL)
        return phy_ul.get_tp(f);
    return -1.0;
}

void phy_handler::set_logger(log_handler *_logger)
{
    phy_dl.set_logger(_logger);
    phy_ul.set_logger(_logger);
}

void phy_handler::init_update_rates(float _doppler_f, int _cqi_p, int _ri_p)
{
    if (_doppler_f > 0)
    {
        tc = 0.423 / _doppler_f;
        float min_sinr_update = tc / 0.001;
        int min_update = std::min(_cqi_p, _ri_p);
        if (min_sinr_update > min_update)
            sinr_period = min_update;
        else
            sinr_period = std::max(1, (int)floor(min_sinr_update));
    }
    else
    {
        tc = -1;
        sinr_period = std::min(_cqi_p, _ri_p);
    }
}

void phy_handler::estimate_channel_estate(int tx_dir, phy_shared &phy_s, float distance, const pos2d &pos, float oldest_t, float avg_tp, float _current_t)
{
    init_update_rates(doppler_f, cqi_period, ri_period);

    if (!init_o2i) // Initialize O2I condition if not done yet
    {
        o2i = phy_s.get_o2i();
        init_o2i = true;
        corr_distance = phy_s.get_correlation_distance();
    }

    bool update_sinr = period_counter % sinr_period == 0;
    if (update_sinr)
    {
        // Update correlation state if needed
        c_dist = get_distance(pos, prev_pos);
        if (c_dist > corr_distance || update_cs)
        {
            corr_distance = phy_s.get_correlation_distance();
            update_cs = false;
            prev_pos = pos;
        }

        if (tx_dir == TX_DL)
        {
            macro_fading = phy_s.getMacroFading(pos);
        }
    }

    if (tx_dir == TX_DL)
        phy_dl.estimate_channel_state(distance, phy_s, macro_fading, o2i, tx_dir, pos, oldest_t, avg_tp, _current_t);
    else if (tx_dir == TX_UL)
        phy_ul.estimate_channel_state(distance, phy_s, macro_fading, o2i, tx_dir, pos, oldest_t, avg_tp, _current_t);

    period_counter++;
}
