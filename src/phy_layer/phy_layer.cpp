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
    max_speed = _ue_speed * 1000 / 3600;

    check_boundaries();
}

void phy_layer::init_phy(float _freq, float _ue_speed, float _target_ber, float _ue_h, int _tx, float _scaling_factor, int _mimo_l, int _numerology, int _n_sc_rbg, bool _mcs_tables, float _gain_tx, float _gain_rx, float _tx_power, float _alpha_ul, float _nominal_pusch_p0, bool _set_ul_pow, float _tx_power_ul, float _power_boost, float _NF_UT, float _NF_eNB, int _num_interf_ues, int _num_interf_eNBs, float _d_interference, float _interfered_ratio)
{
    freq = _freq;
    tx_power_eNb = _tx_power;
    alpha_ul = _alpha_ul;
    nominal_pusch_p0 = _nominal_pusch_p0;
    set_ul_pow = _set_ul_pow;
    tx_power_ul = _tx_power_ul;
    doppler_f = freq * _ue_speed / LIGHTSPEED;
    freq_ghz = freq / GHZ2HZ;
    overhead = get_overhead(freq, _tx);
    scaling_factor = _scaling_factor;
    mimo_l = _mimo_l;
    n_sc_rb = _n_sc_rbg;
    numerology = _numerology;
    n_rb_rbg = n_sc_rb / SCH_N_RE_DEFAULT;
    rbg_lookup_index = get_rbg_sndex(n_rb_rbg);
    mcs_tables = _mcs_tables;
    power_boost = _power_boost;
    antenna_gain_tx = _gain_tx;
    antenna_gain_rx = _gain_rx;
    NF_UT = _NF_UT;
    NF_eNB = _NF_eNB;
    num_interf_ues = _num_interf_ues;
    num_interf_eNBs = _num_interf_eNBs;
    d_interference = _d_interference;
    interfered_ratio = _interfered_ratio;
    target_ber=_target_ber;
}

void phy_layer::init_amc(int _modulation_m, int _cqi_m, int _cqi_p)
{
    modulation_m = _modulation_m;
    cqi_mode = _cqi_m;
    cqi_period = _cqi_p;
}

void phy_layer::init_rank(int _period, int _n_antennas,int _mimo_layers)
{
    ri_period = _period;
    max_ri = std::min(_n_antennas,_mimo_layers);
    current_ri = max_ri;
    layers = max_ri;
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
    // stochastics = false;

    // TX to string
    tx_s = tx == TX_DL ? "dl" : "ul";

    // Verbosity configuration
    verbosity = _verbosity;
    bandwidth = _phy_enb_config.bandwidth;

    // Metric variables
    init_metric(id);

    // Model variables
    init_scenario(_scenario_config.type, _scenario_config.eNB_h, _scenario_config.w,
                  _scenario_config.antenna_h, _phy_ue_config.ue_h, _phy_ue_config.max_speed);

    // Phy configuration
    init_phy(_phy_enb_config.frequency, max_speed, _phy_enb_config.target_ber, ue_h,
             tx, _phy_ue_config.scaling_factor, _phy_enb_config.mimo_l, _phy_enb_config.numerology,
             _phy_enb_config.n_sc_rbg, _scenario_config.mcs_tables, tx == TX_DL ? _phy_enb_config.eNB_gain : _phy_enb_config.UT_gain, tx == TX_DL ? _phy_enb_config.UT_gain : _phy_enb_config.eNB_gain, _phy_enb_config.tx_power, _phy_ue_config.alpha_ul, _phy_ue_config.nominal_pusch_p0, _phy_ue_config.set_ul_pow, _phy_ue_config.tx_power_ul, _phy_enb_config.power_boost, _phy_enb_config.figure_noise_ut, _phy_enb_config.figure_noise_enb, _phy_enb_config.n_int_ues, _phy_enb_config.n_int_eNBs, _phy_enb_config.d_interference, _phy_enb_config.interfered_ratio);

    // AMC
    init_amc(_phy_enb_config.modulation_m, _phy_enb_config.cqi_mode, _phy_ue_config.cqi_period);

    // RI estimation
    init_rank(_phy_ue_config.ri_period, _phy_ue_config.n_antennas,_phy_enb_config.mimo_l);

    // Update Rates
    init_update_rates(doppler_f, cqi_period, ri_period);

    if (mcs_tables)
        std::copy(MCS_SINR[modulation_m][rbg_lookup_index][layers], MCS_SINR[modulation_m][rbg_lookup_index][layers] + MCS_INDEX_MAX, mcs_lookup);
}

void phy_layer::compute_coherence_bw()
{
    switch (scenario)
    {
    case RURAL_MACROCELL:
        ds = pow(10, -7.46);
        break;
    case URBAN_MICROCELL:
        ds = pow(10, (-7.14 - 6.83) / 2.0);
        break;
    case URBAN_MACROCELL:
        ds = pow(10, (-6.955 - 6.28) / 2.0);
        break;
    case INDOOR_OPEN_OFFICE:
        ds = pow(10, (-7.692 - 7.173) / 2.0);
        break;
    case INDOOR_MIXED_OFFICE:
        ds = pow(10, (-7.692 - 7.173) / 2.0);
        break;
    case INDOOR_SHOPPING_MALL:
        ds = pow(10, (-9.35 - 9.44) / 2.0);
        break;
    }
    bc = 1 / (BC_CORRELATION_DEFAULT * ds);

    float n = (float)bandwidth / bc;
   // float n = 10000;

    if (n_rbs >= 2 * n)
    {
        sinr_rbs = ceil(n);
        sinr_step = floor(n_rbs / n);
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
    // bandwidth = _bandwidth;
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
    // Noise and interference
    estimate_noise_interference(tx_power_eNb,
                                tx == TX_DL ? num_interf_eNBs : num_interf_ues, d_interference, interfered_ratio,
                                tx == TX_DL ? NF_UT : NF_eNB,
                                antenna_gain_tx, antenna_gain_rx);
}

float phy_layer::compute_rayleigh()
{
    return 10 * log10(sqrt(pow(sinr_stochastics(gen), 2) + pow(sinr_stochastics(gen), 2)));
}

float phy_layer::compute_pathloss_ABG(float distance, bool los) 
{
    float alpha, beta, gamma;

    switch (scenario)
    {
    case RURAL_MACROCELL:

        if (los)
        {
            alpha = 2.16;
            beta = 32.4;
            gamma = 2;
        }
        else
        {
            alpha = 2.75;
            beta = 32.4;
            gamma = 2;
        }

        break;
    case URBAN_MACROCELL:

        if (los)
        {
            if (freq_ghz < 6)
            {
                alpha = 2.2;
                beta = 28;
                gamma = 2;
            }
            else
            {
                alpha = 1.9;
                beta = 35.8;
                gamma = 1.9;
            }
        }
        else
        {
            alpha = 3.5;
            beta = 13.6;
            gamma = 2.4;
        }
        break;

    case URBAN_MICROCELL:

        if (los)
        {
            if (freq_ghz < 6)
            {
                alpha = 2.27;
                beta = 27.02;
                gamma = 2;
            }
            else
            {
                alpha = 1.1;
                beta = 46.8;
                gamma = 2.1;
            }
        }
        else
        {
            alpha = 2.8;
            beta = 31.4;
            gamma = 2.7;
        }

        break;
    case INDOOR_OPEN_OFFICE:
    case INDOOR_MIXED_OFFICE:
        if (los)
        {
            if (freq_ghz < 6)
            {
                alpha = 1.87;
                beta = 32.82;
                gamma = 2;
            }
            else
            {
                alpha = 1.6;
                beta = 32.9;
                gamma = 1.8;
            }
        }
        else
        {
            if (freq_ghz < 6)
            {
                alpha = 4.33;
                beta = 11.5;
                gamma = 2;
            }
            else
            {
                alpha = 3.9;
                beta = 19;
                gamma = 2.1;
            }
        }
        break;
    case INDOOR_SHOPPING_MALL:

        if (los)
        {
            alpha = 1.9;
            beta = 31.2;
            gamma = 2.2;
        }
        else
        {
            alpha = 2;
            beta = 34.4;
            gamma = 2.3;
        }
        break;
    }

    float plb = 10 * alpha * log10(distance - d_in) + beta + 10 * gamma * log10(freq_ghz);

    return plb;
}
float phy_layer::compute_penetration_losses()
{

    if (o2i == OUTDOOR)
    {
        pltw = 0.0f;
    }
    else if (o2i == IN_BUILDING)
    {
        if (freq_ghz <= 6.0f)
        {

            pltw = 12.0f; // maybe 12 could be more accurate, 20dB is in 3GPP
        }
        else
        {
            // Compute penetration loss based on the scenario for frequencies > 6 GHz.
            switch (scenario)
            {
            case RURAL_MACROCELL:
                pltw = 5.0f - 10.0f * log10(
                                          0.3f * pow(10.0f, (-2.0f - 0.2f * freq_ghz) / 10.0f) +
                                          0.7f * pow(10.0f, (-5.0f - 4.0f * freq_ghz) / 10.0f));
                break;

            case URBAN_MICROCELL:
            case URBAN_MACROCELL:
                pltw = 5.0f - 10.0f * log10(
                                          0.7f * pow(10.0f, (-23.0f - 0.3f * freq_ghz) / 10.0f) +
                                          0.3f * pow(10.0f, (-5.0f - 4.0f * freq_ghz) / 10.0f));
                break;

            default:

                pltw = 10.0f;
                break;
            }
        }
    }
    else
    {
        // for car scenarios
        pltw = 6.0f; // maybe 3-6dB,in 3GPP is said 9dB
    }

    return pltw;
}

float phy_layer::calculateOxygenLoss()
{
    float a_oxygen = 0;

    switch (static_cast<int>(freq_ghz))
    {
    case 53:
        a_oxygen = 1;
        break;
    case 54:
        a_oxygen = 2.2;
        break;
    case 55:
        a_oxygen = 4;
        break;
    case 56:
        a_oxygen = 6.6;
        break;
    case 57:
        a_oxygen = 9.7;
        break;
    case 58:
        a_oxygen = 12.6;
        break;
    case 59:
        a_oxygen = 14.6;
        break;
    case 60:
        a_oxygen = 15;
        break;
    case 61:
        a_oxygen = 14.6;
        break;
    case 62:
        a_oxygen = 14.3;
        break;
    case 63:
        a_oxygen = 10.5;
        break;
    case 64:
        a_oxygen = 6.8;
        break;
    case 65:
        a_oxygen = 3.9;
        break;
    case 66:
        a_oxygen = 1.9;
        break;
    case 67:
        a_oxygen = 1;
        break;
    default:
        a_oxygen = 0;
        break;
    }

    return a_oxygen;
}
float phy_layer::compute_d_in(float _d)
{

    do
    {
        float urban_distance1 = uniform_stochastics(gen) * 25.0f;
        float urban_distance2 = uniform_stochastics(gen) * 25.0f;
        float rma_distance1 = uniform_stochastics(gen) * 10.0f;
        float rma_distance2 = uniform_stochastics(gen) * 10.0f;

        switch (scenario)
        {
        case RURAL_MACROCELL:
            d_in = std::min(rma_distance1, rma_distance2);
            break;

        case URBAN_MICROCELL:
        case URBAN_MACROCELL:
            d_in = std::min(urban_distance1, urban_distance2);
            break;

        default:
            return 0.0f;
        }
    } while (d_in >= _d);
    return d_in;
}

float phy_layer::compute_losses(float distance)
{

    a_oxygen = calculateOxygenLoss();

    float pltotal = a_oxygen * distance / 1000 + pltw + plin;
    return pltotal;
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
    return log2(1 + sinr / (-log(5 * target_ber / 1.5)));
}

float phy_layer::estimate_cqi_from_se(float se)
{
    int i = 0;
    for (i = 0; i < NUM_CQI_VALUES && EFF_2_CQI[modulation_m][i + 1] <= se; i++)
        ;
    return i;
}

float phy_layer::estimate_mcs_from_se(float se, float &eff)
{
    int i = 0;
    for (i = 0; i < NUM_MCS_VALUES - 1 && EFF_2_MCS[modulation_m][i + 1] <= se; i++)
        ;
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
    if (mcs_lookup[0] > sinr)
        return -1;
    for (i = 0; i < MCS_INDEX_MAX - 1 && mcs_lookup[i + 1] < sinr; i++)
        ;
    return i;
}

void phy_layer::channel_q_tables(int f)
{

    mcs_v[f] = get_mcs_index(db_sinr_v[f]);
    if (mcs_v[f] >= 0)
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
    if (cqi_v[f] > 0)
        mcs_v[f] = estimate_mcs_from_se(efficiency, eff_v[f]);
    else
    {
        mcs_v[f] = 0;
        eff_v[f] = 0;
    }
}

void phy_layer::estimate_channel_q(int f)
{
    if (mcs_tables)
        channel_q_tables(f);
    else
        channel_q_efficiency(f);
    mcs_s += mcs_v[f];
    cqi_s += cqi_v[f];
    eff_s += eff_v[f];
}

void phy_layer::estimate_ri()
{
    if (tx == TX_DL)
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
void phy_layer::estimate_noise_interference(float _tx_power, int _n_ues, float _d_interference, float _interfered_ratio, float _figure, float _gain_tx, float _gain_rx)
{
    float tx_power;
    float n_rbs_local;

    if (tx_dir == 1) // UL
    {
        tx_power = -23 + uniform_stochastics(gen) * 46;
        n_rbs_local = 1.0f;
    }
    else
    {
        tx_power = _tx_power;
        n_rbs_local = n_rbs;
    }

    noise_figure = _figure;
    antenna_gain_tx = _gain_tx;
    antenna_gain_rx = _gain_rx;

    float RB_bandwidth = 12 * 15000 * pow(2, numerology);
    float noise = -174 + noise_figure + 10 * log10(RB_bandwidth);
    linear_noise = dBmToLinear(noise);

    float n_interference = _n_ues;
    float random = uniform_stochastics(gen);
    d_interference = _d_interference + random * 100;

    float PwrDBm = tx_power - 10 * log10(n_rbs_local) + 10 * log10(_interfered_ratio);
    linear_interference_transmitted = n_interference * dBmToLinear(PwrDBm);

    float interference = linearToDBm(linear_interference_transmitted) - compute_losses(d_interference) - compute_pathloss_ABG(d_interference, true);
    float linear_interference = dBmToLinear(interference);

    noise_interf = linearToDBm(linear_noise + linear_interference);
}

float phy_layer::sinr_power_model(float _tx_power, float distance, float pathloss, float _macro_fading)
{
    float n_rbs_local;
    float tx_power;

    if (tx_dir == 1)
    {
        if (set_ul_pow)
        {
            tx_power = tx_power_ul;
        }

        else
        {
            tx_power = nominal_pusch_p0 + alpha_ul * compute_pathloss_ABG(distance, true);
        }
        tx_power = std::max(10.0f, std::min(tx_power, 23.0f));
       
        n_rbs_local = 1.0f;
    }
    else
    {
        tx_power = _tx_power;
        n_rbs_local = n_rbs;
    }
    float rayleigh = stochastics ? compute_rayleigh() : 0;

    float sinr = tx_power - 10 * log10(n_rbs_local) + rayleigh - pathloss + _macro_fading + antenna_gain_tx + antenna_gain_rx - noise_interf - 10 * log10(current_ri);
    return sinr;
}

void phy_layer::estimate_sinr(float distance, int f, float _macro_fading)
{
    // SINR
    pathloss = compute_losses(distance);

    if (f % sinr_step == 0)
    {

        db_sinr_v[f] = sinr_power_model(tx_power_eNb, distance, pathloss, _macro_fading);
    }
    else
    {
        db_sinr_v[f] = db_sinr_v[f - 1];
    }

    db_sinr_s += db_sinr_v[f];
    rsrp_v[f] = db_sinr_v[f] + noise_interf - 10 * log10(n_rbs) - 10 * log10(n_sc_rb) + power_boost;

    rsrp_s = rsrp_v[f];
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

void phy_layer::set_logger(log_handler *_logger)
{
    phy_log = true;
    logger = _logger;
}

void phy_layer::prepare_metrics(float oldest_t, float avg_tp)
{
    // Get UE info for metrics
    metric_i.req_time = oldest_t;
    metric_i.avrg_tp = avg_tp * MBIT2BIT / S2MS;
    metric_i.current_delay = current_t - metric_i.req_time;
}

void phy_layer::estimate_metric(int f)
{
    metric_i.current_tp = tp_v[f];

    if (!metric_h.is_rr())
        metric_v[f] = metric_h.get_metric(metric_i, current_t, f);
}

void phy_layer::estimate_tp(int f)
{
    float current_tp = current_ri * n_sc_rb * eff_v[f] * (1 - overhead) * scaling_factor;
    tp_v[f] = current_tp;
}

float phy_layer::get_metric(int f, int n_ues)
{
    if (metric_h.is_rr())
        return metric_h.get_rr_metric(f, id, n_ues);
    return metric_v[f];
}

float phy_layer::get_tp(int f)
{
    return tp_v[f];
}
void phy_layer::init_update_rates(float _doppler_f, int _cqi_p, int _ri_p)
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

void phy_layer::estimate_channel_state(float distance, phy_shared &phy_s, float _macro_fading, int _o2i, int _tx_dir, const pos2d &pos, float oldest_t, float avg_tp, float _current_t)
{
    // doppler_f = freq * _speed / LIGHTSPEED;

    current_t = _current_t;
    o2i = _o2i;
    tx_dir = _tx_dir;
    macro_fading = _macro_fading;

    if (!d_in_computed)
    {
        d_in = compute_d_in(distance);
        pltw = compute_penetration_losses();
        plin = 0.5 * d_in;

        d_in_computed = true;
    }

    bool dumb = period_counter % 1000 == 0;
    bool update_cqi = period_counter % cqi_period == 0;
    bool update_ri = period_counter % ri_period == 0;
    bool update_sinr = period_counter % sinr_period == 0;
    if (update_cqi)
        reset_cqi();
    if (update_sinr)
        // estimate_attenuation(distance,pos,phy_s);
        // pathloss=compute_pathloss(distance,_los);
        db_sinr_s = 0;
    prepare_metrics(oldest_t, avg_tp);

    for (int i = 0; i < n_rbs; i++)
    {
        if (update_sinr)

            estimate_sinr(distance, i, macro_fading);
        if (update_cqi)
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

    if (update_cqi)
        average_cqi();
    if (update_sinr)
        average_sinr();
    if (update_ri)
        estimate_ri();
    if (phy_log)
    {

        logger->log_partial("sinr:{} rsrp:{} cqi:{} mcs:{} eff:{} ri:{} tx:{} ts:{}", db_sinr_s, rsrp_s, cqi_s, mcs_s, eff_s, current_ri, tx, current_t);
        logger->flush();
    }
    period_counter++;
}