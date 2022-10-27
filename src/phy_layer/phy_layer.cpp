/**********************************************
* Copyright 2022 Nokia
* Licensed under the BSD 3-Clause Clear License
* SPDX-License-Identifier: BSD-3-Clause-Clear
**********************************************/

#include <phy_layer/phy_layer.h>

double linearToDBm(double linear)
{
    return 10 * log10(1000 * linear);
}

double linearToDb(double linear)
{
    return 10 * log10(linear);
}

double dBmToLinear(double db)
{
    return pow(10, (db - 30) / 10);
}

double dBToLinear(double db)
{
    return pow(10, (db) / 10);
}

void phy_layer::init_metric(int _index)
{
    // Metric variables
    metric_i.index = _index; 
}

void phy_layer::init_scenario(int _type, float _eNB_h, float _w, float _antenna_h, float _ue_h, float _ue_speed)
{
    scenario = _type; 
    eNB_h = _eNB_h; 
    w = _w; 
    antenna_h = _antenna_h; 
    antenna_h_172 = pow(antenna_h, 1.72);
    ue_h = _ue_h;
    max_speed = _ue_speed*1000/3600; 
    check_boundaries();
}

void phy_layer::init_phy(float _freq, float _ue_speed, float _eNB_h, float _ue_h, int _tx, float _scaling_factor, int _mimo_l, int _n_sc_rbg, bool _mcs_tables)
{
    freq = _freq; 
    doppler_f = freq*_ue_speed/LIGHTSPEED;
    freq_ghz = freq/GHZ2HZ; 
    dbp = 4 * _eNB_h * _ue_h * freq / LIGHTSPEED; 
    overhead = get_overhead(freq, _tx); 
    scaling_factor = _scaling_factor; 
    mimo_l = _mimo_l; 
    n_sc_rb = _n_sc_rbg; 
    n_rb_rbg = n_sc_rb / SCH_N_RE_DEFAULT; 
    rbg_lookup_index = get_rbg_sndex(n_rb_rbg);
    mcs_tables = _mcs_tables; 
}

void phy_layer::estimate_noise_interference(float _tx_power, int _n_ues, float _d_interference, float _figure, float _thermal, float _gain_tx, float _gain_rx)
{
    tx_power = _tx_power; 
    float n_interference = _n_ues;
    float d_interference = _d_interference; 
    float pathloss_interference = compute_pathloss(d_interference, true);
    antenna_gain_tx = _gain_tx; //18  
    antenna_gain_rx = _gain_rx; // 0
    thermal_noise = _thermal; // -107
    noise_figure = _figure; // 7
    float noise = noise_figure + thermal_noise; 
    float linear_noise = dBmToLinear(noise);
    float recvPwrDBm = tx_power - pathloss_interference + antenna_gain_tx + antenna_gain_rx - 15;
    interference = n_interference*dBmToLinear(recvPwrDBm);
    noise_interf = linearToDBm(linear_noise + interference);
    noisedb = linear_noise; 
}

void phy_layer::init_amc(int _modulation_m, int _cqi_m, int _cqi_p)
{
    modulation_m = _modulation_m;
    cqi_mode = _cqi_m;
    cqi_period = _cqi_p;
}

void phy_layer::init_rank(int _period, int _max)
{
    ri_period = _period;
    max_ri = _max;
    current_ri = max_ri;
    layers = max_ri; 
}

void phy_layer::init_update_rates(float _doppler_f, int _cqi_p, int _ri_p)
{
    if(_doppler_f > 0)
    {
        tc = 0.423/_doppler_f; 
        float min_sinr_update = tc/0.001; 
        int min_update = std::min(_cqi_p, _ri_p);
        if(min_sinr_update > min_update) sinr_period = min_update; 
        else sinr_period = std::max(1, (int)floor(min_sinr_update));
    }      
    else
    {
        tc = -1;
        sinr_period = std::min(_cqi_p, _ri_p); 
    } 
}

phy_layer::phy_layer(int _tx, int _id, scenario_config _scenario_config, phy_ue_config _phy_ue_config, phy_enb_config _phy_enb_config, bool _stochastics, int _verbosity)
    : distance_cqi_dist(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count()),
      gen(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count()),
      metric_h(_phy_enb_config.metric_type, _phy_ue_config.priority, _phy_ue_config.beta, _phy_ue_config.delay_t, _phy_ue_config.delta)
{
    // UE unique id
    id = _id; 

    // Transmission direction
    tx = _tx;

    // Enable stochastics
    stochastics = _stochastics;
    // TX to string
    tx_s = tx == TX_DL ? "dl" : "ul";

    // Verbosity configuration
    verbosity = _verbosity;    

    // Metric variables
    init_metric(id); 

    // Model variables 
    init_scenario(  _scenario_config.type, _scenario_config.eNB_h, _scenario_config.w, 
                    _scenario_config.antenna_h,_phy_ue_config.ue_h, _phy_ue_config.max_speed);

    // Phy configuration
    init_phy(_phy_enb_config.frequency, max_speed, eNB_h, ue_h,
             tx, _phy_ue_config.scaling_factor, _phy_enb_config.mimo_l, 
             _phy_enb_config.n_sc_rbg, _scenario_config.mcs_tables);
    
    // Noise and interference
    estimate_noise_interference(tx == TX_DL ? _phy_enb_config.tx_power : _phy_ue_config.tx_power,
                                _phy_enb_config.n_int_ues, _phy_enb_config.d_interference,
                                _phy_enb_config.figure_noise, _phy_enb_config.thermal_noise, 
                                _phy_enb_config.tx_gain, _phy_enb_config.rx_gain); 
    
    // AMC
    init_amc(_phy_enb_config.modulation_m, _phy_enb_config.cqi_mode, _phy_ue_config.cqi_period);

    // RI estimation
    init_rank(_phy_ue_config.ri_period, _phy_ue_config.max_ri);

    // Update Rates
    init_update_rates(doppler_f, cqi_period, ri_period);

    if(mcs_tables) std::copy(MCS_SINR[modulation_m][rbg_lookup_index][layers], MCS_SINR[modulation_m][rbg_lookup_index][layers] + MCS_INDEX_MAX, mcs_lookup);
}

void phy_layer::compute_coherence_bw()
{
    switch (scenario)
    {
        case RURAL_MACROCELL:
            ds = pow(10, -7.46);
            break;
        case URBAN_MICROCELL:
            ds = pow(10, (-7.14-6.83)/2.0);
            break;
        case URBAN_MACROCELL:
            ds = pow(10, (-6.955-6.28)/2.0);            
            break;
        case INDOOR_HOTSPOT: 
            ds = pow(10, (-7.692-7.173)/2.0);
            break;
        case INDOOR_FACTORY:
            ds = pow(10, (-9.35-9.44)/2.0);
            break;    
    }
    bc = 1/(BC_CORRELATION_DEFAULT*ds);
    float n = (float)bandwidth/bc; 
    if(n_rbs >= 2*n)
    {
        sinr_rbs = ceil(n);
        sinr_step = floor(n_rbs/n);
    }
    else
    {
        sinr_rbs = n_rbs; 
        sinr_step = 1; 
    }
}

void phy_layer::check_boundaries()
{
    antenna_h = std::max(5.0f, antenna_h); 
    antenna_h = std::min(antenna_h, 50.0f);

    w = std::max(5.0f, w); 
    w = std::min(w, 50.0f); 

    switch (scenario)
    {
        case RURAL_MACROCELL:
            eNB_h = std::max(10.0f, eNB_h); 
            eNB_h = std::min(eNB_h, 150.0f);

            ue_h = std::max(1.0f, ue_h); 
            ue_h = std::min(ue_h, 10.0f);

            break;
        case URBAN_MICROCELL:
            eNB_h = 10.0f; 

            ue_h = std::max(1.5f, ue_h); 
            ue_h = std::min(ue_h, 22.5f);

            break;
        case URBAN_MACROCELL:
            eNB_h = 25.0f; 

            ue_h = std::max(1.5f, ue_h); 
            ue_h = std::min(ue_h, 22.5f);
            break;
   }
}
void phy_layer::init(int _n_rbs, int _bandwidth)
{
    n_rbs = _n_rbs;
    bandwidth = _bandwidth; 
    cqi_s = 0;
    mcs_s = 0;
    eff_s = 0;
    db_sinr_s = 0;
    l_sinr_s = 0;
    cqi_v = std::vector<int>(n_rbs, 0);
    mcs_v = std::vector<int>(n_rbs, 0);
    eff_v = std::vector<float>(n_rbs, 0);
    rsrp_v = std::vector<float>(n_rbs, 0);
    db_sinr_v = std::vector<float>(n_rbs, 0);
    metric_v = std::vector<float>(n_rbs, 0);
    tp_v = std::vector<float>(n_rbs, 0);
    l_sinr_v = std::vector<float>(n_rbs, 0);
    is_init = true;
    compute_coherence_bw(); 
}

bool phy_layer::compute_LOS(float d)
{
    if(stochastics)
    {
        float p = 0; 
        switch (scenario)
        {
            case RURAL_MACROCELL:
                if (d <= 10)
                    p = 1;
                else
                    p = exp(-1 * (d - 10) / 1000);
                break;
            case URBAN_MICROCELL:
                if (d <= 18)
                    p = 1;
                else
                    p = 18/d + exp(-d/36)*(1 - 18/d);
                break;
            case URBAN_MACROCELL:
                if (d <= 18)
                    p = 1;
                else
                {
                    float C = (eNB_h <= 13)? 0:pow((eNB_h -13)/10, 1.5);
                    p = (18/d + exp(-d/63)*(1 - 18/d))*(1 + C*(5/4)*pow(d/100, 3)*exp(-d/150));
                }
                break;
            case INDOOR_HOTSPOT: 
                if(d <= 5)
                    p = 1; 
                else if(d <= 49)
                    p = exp(-(d-5)/70.8);
                else
                    p = exp(- (d - 49)/211.7)*0.54; 
                break;
            case INDOOR_FACTORY: 
                p = exp(-d/(-6/log(1-0.4)));
                break;
            default: 
                LOG_ERROR_I("phy_layer::compute_LOS") << " Wrong path-loss scenario value..." << END();  
        }
        float random = uniform_stochastics(gen);
        if (random <= p)
            return true;
        else
            return false;
    }
    else return true; 
}

float phy_layer::compute_rayleigh()
{
    return 10 * log10(sqrt(pow(sinr_stochastics(gen), 2) + pow(sinr_stochastics(gen), 2)));
}

float phy_layer::compute_shadowing(const pos2d &pos)
{
    bool new_condition = (shadow_f != prev_shadow_f); 
    if(new_condition)
    {
        prev_shadow_v = sinr_stochastics(gen)*shadow_f;
    }
    else if(update_cs)
    {
        float prev_dist = get_distance(pos, prev_pos);
        float R = exp(-1 * prev_dist/ corr_distance);
        prev_shadow_v = R * prev_shadow_v + sqrt(1 - R*R) * sinr_stochastics(gen)*shadow_f; 
    }
    prev_shadow_f = shadow_f; 
    return prev_shadow_v; 
}
float phy_layer::compute_pathloss(float d, bool LOS)
{
    float pl = 0; 
    switch (scenario)
    {
        case RURAL_MACROCELL:
            pl = 20*log10(40*3.1415*d*freq_ghz/3) + std::min(0.03*antenna_h_172, 10.0)*log10(d) - std::min(0.044*antenna_h_172, 14.77)+ 0.002*log10(antenna_h)*d;
            if (LOS)
            {   
                if(d > 10 && d < dbp) shadow_f = 4; 
                else if(d < 10000)
                {
                    pl += 40*log10(d/dbp);
                    shadow_f = 6; 
                }
                corr_distance = 37; 
            }
            else
            {
                shadow_f = 8; 
                pl = std::max((double)pl, 161.04 - 7.1*log10(w) + 7.5*log10(antenna_h) - (24.73 - 3.7*pow(antenna_h/eNB_h, 2))*log10(eNB_h) + (43.42 - 3.1*log10(eNB_h))*(log10(d) - 3) + 20*log10(freq_ghz) - 3.2*pow(log10(11.75*ue_h), 2) - 4.97);  
                corr_distance = 120; 
            }
            break;
        case URBAN_MACROCELL:
            if(d > 10 && d < dbp) pl = 28.0 + 22.0*log10(d) + 20*log10(freq_ghz);
            else if(d < 10000) pl = 28.0 + 40.0*log10(d) + 20*log10(freq_ghz) - 9*log10(pow(dbp, 2) + pow(eNB_h - ue_h, 2)); 
            if (LOS)
            {
                shadow_f = 4; 
                corr_distance = 37; 
            }
            else
            {
                shadow_f = 6; 
                pl = std::max((double)pl, 13.54 + 39.08*log10(d) + 20*log10(freq_ghz) - 0.6*(ue_h - 1.5));
                corr_distance = 50; 
            }
            break;
        case URBAN_MICROCELL:
            if(d > 10 && d < dbp) pl = 32.4 + 21.0*log10(d) + 20*log10(freq_ghz);
            else if(d < 10000) pl = 32.4 + 40.0*log10(d) + 20*log10(freq_ghz) - 9.5*log10(pow(dbp, 2) + pow(eNB_h - ue_h, 2)); 
            if (LOS)
            {
                shadow_f = 4; 
                corr_distance = 10; 
            }
            else
            {
                shadow_f = 7.82; 
                pl = std::max((double)pl, 22.4 + 35.3*log10(d) + 21.3*log10(freq_ghz) - 0.3*(ue_h - 1.5));
                corr_distance = 13; 
            }
            break;
        case INDOOR_HOTSPOT: 
            pl = 32.4 + 17.3*log10(d) + 20*log10(freq_ghz);
            if(LOS)
            {
                shadow_f = 3;
                corr_distance = 10; 
            }
            else
            {
                shadow_f = 8.0;
                pl = std::max((double)pl, 17.3 + 38.3*log10(d) + 24.9*log10(freq_ghz));
                corr_distance = 6;
            }
        case INDOOR_FACTORY: 
            pl = 31.84 + 21.50*log10(d) + 19.0*log10(freq_ghz);
            if(LOS)
            {
                shadow_f = 4; 
                corr_distance = 10; 
            }
            else
            {
                shadow_f = 7.2;
                pl = std::max((double)pl, 18.6 + 35.7*log10(d) + 20*log10(freq_ghz));
                corr_distance = 10; 
            }      
   }
   return pl; 
}

int phy_layer::get_ri()
{
    return current_ri;
}
   
int phy_layer::get_cqi(int f_index)
{
    if (cqi_mode == CQI_WIDEBAND)
        return cqi_s;
    if (cqi_mode == CQI_RBGS)
        return cqi_v[f_index];
    return -1;
}
float phy_layer::get_eff(int f_index)
{
    if (cqi_mode == CQI_WIDEBAND)
        return eff_s;
    if (cqi_mode == CQI_RBGS)
        return eff_v[f_index];
    return -1;
}

int phy_layer::get_mcs(int f_index)
{
    if (cqi_mode == CQI_WIDEBAND)
        return mcs_s;
    if (cqi_mode == CQI_RBGS)
        return mcs_v[f_index];
    return -1;
}

float phy_layer::get_sinr(int f_index)
{
    if (cqi_mode == CQI_WIDEBAND)
        return db_sinr_s;
    if (cqi_mode == CQI_RBGS)
        return db_sinr_v[f_index];
    return -1;
}

float phy_layer::get_linear_sinr(int f_index)
{
    if (cqi_mode == CQI_WIDEBAND)
        return l_sinr_s;
    if (cqi_mode == CQI_RBGS)
        return l_sinr_v[f_index];
    return -1;
}

float phy_layer::get_mean_sinr()
{
    return db_sinr_s;
}

int phy_layer::get_modulation_index()
{
    return modulation_m; 
}

int phy_layer::get_n_layers()
{
    return layers; 
}

int phy_layer::estimate_cqi_from_distance(float distance)
{
    return (int)std::min(15.0, std::max(1.0, pow(int(15 * ((ENB_MAX_RANGE - distance) / ENB_MAX_RANGE)), 0.5) + distance_cqi_dist(gen)));
}

float phy_layer::estimate_eff_from_sinr(float sinr)
{
    return log2(1 + sinr/ (-log(5 * target_ber / 1.5)));
}

float phy_layer::estimate_cqi_from_se(float se)
{
    int i = 0;
    for (i = 0; i < NUM_CQI_VALUES && EFF_2_CQI[modulation_m][i + 1] <= se; i++);
    return i;
}

float phy_layer::estimate_mcs_from_se(float se, float &eff)
{
    int i = 0;
    for (i = 0; i < NUM_MCS_VALUES - 1 && EFF_2_MCS[modulation_m][i + 1] <= se; i++);
    eff = EFF_2_MCS[modulation_m][i];
    return i;
}

float phy_layer::estimate_eff_from_mcs(int index)
{
    return EFF_2_MCS[modulation_m][index];
}

int phy_layer::get_mcs_index(float sinr)
{
    int i = 0; 
    if(mcs_lookup[0] > sinr) return -1; 
    for (i = 0; i < MCS_INDEX_MAX - 1 && mcs_lookup[i + 1] < sinr; i++);
    return i; 
}

void phy_layer::channel_q_tables(int f)
{
    
    mcs_v[f] = get_mcs_index(db_sinr_v[f]);
    if(mcs_v[f] >= 0)
    {
        eff_v[f] = estimate_eff_from_mcs(mcs_v[f]);
        cqi_v[f] = estimate_cqi_from_se(eff_v[f]);
    }
    else
    {
        eff_v[f] = 0;
        cqi_v[f] = 0;
    }
}

void phy_layer::channel_q_efficiency(int f)
{
    float efficiency = estimate_eff_from_sinr(l_sinr_v[f]);
    cqi_v[f] = estimate_cqi_from_se(efficiency);
    if(cqi_v[f] > 0) mcs_v[f] = estimate_mcs_from_se(efficiency, eff_v[f]);
    else 
    {
        mcs_v[f] = 0; 
        eff_v[f] = 0; 
    }
}

void phy_layer::estimate_channel_q(int f)
{
    if(mcs_tables) channel_q_tables(f);
    else channel_q_efficiency(f);
    mcs_s += mcs_v[f]; 
    cqi_s += cqi_v[f]; 
    eff_s += eff_v[f];
}

void phy_layer::estimate_ri()
{
    if(tx==TX_DL)
    {
        int i;
        for (i = 0; i < max_ri; i++)
        {
            if (db_sinr_s <= RI_THRES[modulation_m][i])
            {
                current_ri = i + 1;
                current_ri = std::min(current_ri, mimo_l);
                return;
            }
        }
    }
    else
    {
        current_ri = 1; 
    }
}

float phy_layer::sinr_power_model(float distance)
{
    bool los = compute_LOS(distance); 
    float pathloss = compute_pathloss(distance, los); 
    float shadowing = stochastics ? compute_shadowing(distance) : 0;
    float rayleigh = stochastics ? compute_rayleigh() : 0; 
    float noise = -7 + 107; 
    float linear_noise = dBmToLinear(noise);
    float recvPwrDBm = tx_power - pathloss + shadowing + 18 + 0;
    float recvPwer = dBmToLinear(recvPwrDBm);
    float sinr = tx_power + rayleigh - pathloss + shadowing + 18 + 0 - linearToDBm(recvPwer + linear_noise);
    return sinr; 
}   

float phy_layer::sinr_power_model(float pathloss, float shadowing, float distance)
{
    float rayleigh = stochastics ? compute_rayleigh() : 0; 
    float sinr = tx_power + rayleigh - pathloss + shadowing + antenna_gain_tx + antenna_gain_rx - noise_interf;
    return sinr; 
} 

void phy_layer::estimate_attenuation(float distance, const pos2d &pos)
{
    db_sinr_s = 0;
 
    // ATTENUATION
    c_dist = get_distance(pos, prev_pos);
    if(c_dist > corr_distance || update_cs)
    {
        los = compute_LOS(distance);
        if(!update_cs && los != prev_los) update_cs = true; 
        prev_pos = pos;
        prev_los = los; 
    }
    pathloss = compute_pathloss(distance, los);
    shadowing = stochastics ? compute_shadowing(pos) : 0;
    update_cs = false; 
}


void phy_layer::estimate_sinr(float distance, int f)
{
    // SINR
    db_sinr_v[f] = sinr_power_model(pathloss, shadowing, distance); 
    db_sinr_s += db_sinr_v[f];
    rsrp_v[f] = db_sinr_v[f] + noise_interf; 
    rsrp_s += rsrp_v[f];
    l_sinr_v[f] = pow(10, db_sinr_v[f] / 10.0);
}

void phy_layer::average_sinr()
{
    db_sinr_s /= n_rbs;
    l_sinr_s = pow(10, db_sinr_s / 10.0);
}

void phy_layer::average_cqi()
{
    cqi_s /= n_rbs;
    mcs_s /= n_rbs;
    eff_s /= n_rbs;
}

void phy_layer::reset_cqi()
{
    cqi_s = 0; 
    mcs_s = 0;
    eff_s = 0;
}

void phy_layer::set_logger(log_handler * _logger)
{
    phy_log = true;
    logger = _logger;
}

void phy_layer::prepare_metrics(float oldest_t, float avg_tp)
{
    // Get UE info for metrics
    metric_i.req_time = oldest_t; 
    metric_i.avrg_tp = avg_tp*MBIT2BIT/S2MS;
    metric_i.current_delay = current_t - metric_i.req_time;
}

void phy_layer::estimate_metric(int f)
{
    metric_i.current_tp = tp_v[f];
             
    if(!metric_h.is_rr()) metric_v[f] = metric_h.get_metric(metric_i, current_t, f);
}

void phy_layer::estimate_tp(int f)
{
    float current_tp = current_ri*n_sc_rb*eff_v[f]*(1-overhead)*scaling_factor; 
    tp_v[f] = current_tp;  
}

float phy_layer::get_metric(int f, int n_ues)
{
    if(metric_h.is_rr()) return metric_h.get_rr_metric(f, id, n_ues);
    return metric_v[f];
}

float phy_layer::get_tp(int f)
{
    return tp_v[f];
}

void phy_layer::estimate_channel_state(float distance, const pos2d &pos, float oldest_t, float avg_tp, float _current_t)
{   
    current_t = _current_t; 
    bool dumb = period_counter % 1000 == 0;
    bool update_cqi = period_counter % cqi_period == 0;
    bool update_ri = period_counter % ri_period == 0;
    bool update_sinr = period_counter % sinr_period == 0;
    if(update_cqi) reset_cqi(); 
    if(update_sinr) estimate_attenuation(distance, pos);
    prepare_metrics(oldest_t, avg_tp);
    for (int i = 0; i < n_rbs; i++)
    {
        if(update_sinr) estimate_sinr(distance, i);
        if(update_cqi)
        {
            estimate_channel_q(i);
            estimate_tp(i);        
            estimate_metric(i);  
            /*if(tx==TX_DL && dumb)
            {
                std::cout << "MCS: ";
                for(const auto& value: mcs_v) std::cout << value << " : ";  
                std::cout << std::endl; 
                std::cout << "TP: ";
                for(const auto& value: tp_v) std::cout << value << " : ";  
                std::cout << std::endl;  
                std::cout << "M: ";
                for(const auto& value: metric_v) std::cout << value << " : ";  
                std::cout << std::endl;  
            }*/
        }
    }
    if(update_cqi) average_cqi();
    if(update_sinr) average_sinr();
    if(update_ri) estimate_ri(); 
    if (phy_log)
    {
        logger->log_partial("sinr:{} rsrp:{} cqi:{} mcs:{} eff:{} ri:{} tx:{} ts:{}", db_sinr_s, rsrp_s, cqi_s, mcs_s, eff_s, current_ri, tx, current_t);
        logger->flush();
    } 
    period_counter++;
}