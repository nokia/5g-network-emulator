/**********************************************
* Copyright 2022 Nokia
* Licensed under the BSD 3-Clause Clear License
* SPDX-License-Identifier: BSD-3-Clause-Clear
**********************************************/

#ifndef PHY_LAYER_H
#define PHY_LAYER_H

#include <algorithm>
#include <iostream>
#include <random>
#include <vector>
#include <chrono>
#include <phy_layer/phy_l_definitions.h>
#include <phy_layer/phy_config.h>
#include <mac_layer/metric_handler.h>
#include <mobility_models/pos2d.h>
#include <utils/conversions.h>
#include <utils/terminal_logging.h>

// Logging

#include "utils/logging/log_handler.h"


double linearToDBm(double linear);
double linearToDb(double linear);
double dBmToLinear(double db);
double dBToLinear(double db);

//--------------------------------------------------------------------------------------------------
// phy_layer(): The PHY layer models the Channel Quality Measurements: SINR, RSRP, MCS, CQI, 
// HARQ ACK/NACK. These metrics, besides of being logged for later analysis, are used by the MAC layer
// for resource allocation decisions, and to determine what happens with each data block. This info
// is used to also determine the probability of a packet to be retransmitted, according to the HARQ model. 
// The implemented models are taken from 3GPP 38.901 specifications. 
// Input: 
//      _tx: transmission direction of the current instance (DL/UL)
//      _id: User internal ID
//      _scenario_config: scenario configuration struct 
//              *type: scenario type ID from the ones implemented.
//              *w: width of the beam.
//              *antenna_h: height of the antenna.
//              *eNB_h: height of the eNB/gNB.
//              *mcs_tables: enable/disable custom MCS-BLER-SINR curves for estimating the MCS indexes. 
//      _phy_ue_config: UE PHY layer configuration struct.
//              *_tx_power: transmission power of the eNB/gNB 
//              *_cqi_period: period in ms between CQI estimations.
//              *_ri_period: period in ms between Rank Indicator estimations.
//              *_max_ri: maximum Rank Indicator value for current UE. 
//              *_ue_h: height of UE above ground.
//              *_priority: UE's priority if user prioritization is used. 
//              *_delay_t: used in specific metric for resource allocation scheduling. 
//              *_delta: used in specific metric for resource allocation scheduling. 
//              *_max_speed: maximum UE's speed. 
//              *_scaling_factor: defined scaling factor used for throughput calculation. 
//      _phy_enb_config: PHY Layer eNB/gNB configuration struct.
//              *tx_power: transmission power of the eNB/gNB 
//              *modulation_m: 0 - 64 QAM | 1 - 256 QAM
//              *target_ber: used for MCS indexes estimation when the MCS tables are not used.  
//              *cqi_mode: 0 - Wideband | 1 - Subband
//              *frequency: base frequency in which the eNB/gNB is operating.
//              *bandwidth: total available bandwidth.
//              *mimo_l: number of available eNB/gNB MIMO layers. 
//              *metric_type: ID of the metric type.
//              *n_int_ues: dumber of interfering UEs, not attached to the simulated eNB/gNB
//              *d_interference: interference distance of nearby UEs
//              *thermal_noise: modeled thermanl noise of the emulated eNB/gNB.
//              *figure_noise: modeled noise figure of the emulated eNB/gNB.
//              *tx_gain: modeled transmission gain of the emulated eNB/gNB.
//              *rx_gain: modeled reception gain of the emulated eNB/gNB. 
//      verbosity: Enable/disable verbosity
//--------------------------------------------------------------------------------------------------
class phy_layer
{
    
public: 
    phy_layer(int _tx, int _id, scenario_config _scenario_config, phy_ue_config _phy_ue_config, phy_enb_config _phy_enb_config, bool _stochastics = true, int _verbosity = 0);

public: 
    void init(int _n_rbs, int _bandwidth);
    int get_cqi(int f_index);
    float get_eff(int f_index);
    int get_ri();
    int get_mcs(int f_index);
    float get_sinr(int f_index);
    float get_linear_sinr(int f_index);
    float get_mean_sinr();
    void set_logger(log_handler * _logger);
    float get_metric(int f, int n_ues);
    float get_tp(int f);

public:
    void estimate_channel_state(float distance, const pos2d &pos, float oldest_t, float avg_tp, float _current_t);

private: 
    void init_metric(int _index);
    void init_scenario(int _type, float _eNB_h, float _w, float _antenna_h, float _ue_h, float _ue_speed);
    void init_phy(float _freq, float _ue_speed, float _eNB_h, float _ue_h, int _tx, float _scaling_factor, int _mimo_l, int _n_sc_rbg, bool _mcs_tables);
    void estimate_noise_interference(float _tx_power, int _n_ues, float _d_interference, float _figure, float _thermal, float _gain_tx, float _gain_rx);
    void init_amc(int _modulation_m, int _cqi_m, int _cqi_p);
    void init_rank(int _period, int _max);
    void init_update_rates(float _doppler_f, int _cqi_p, int _ri_p);

private: 
    int estimate_cqi_from_distance(float distance);
    float estimate_eff_from_sinr(float sinr);
    float estimate_eff_from_mcs(int mcs);
    float estimate_cqi_from_se(float se);
    float estimate_mcs_from_se(float se, float &eff);
    void estimate_channel_q(int f);
    void channel_q_tables(int f);
    int get_mcs_index(float sinr);
    void channel_q_efficiency(int f);
    void estimate_ri();
    float sinr_power_model(float distance);
    float sinr_power_model(float pathloss, float shadowing, float distance);
    void estimate_sinr(float distance, int f);
    void estimate_attenuation(float distance, const pos2d &pos);
    void average_cqi(); 
    void average_sinr(); 
    void reset_cqi(); 
    bool compute_LOS(float distance);
    float compute_pathloss(float distance, bool LOS);
    void check_boundaries();
    void compute_coherence_bw();
    float compute_shadowing(const pos2d &pos); 
    float compute_rayleigh(); 
    void estimate_metric(int f); 
    void estimate_tp(int f); 
    void prepare_metrics(float oldest_t, float avg_tp);
    int get_modulation_index(); 
    int get_n_layers(); 

private: 
    std::mt19937 gen;
    std::uniform_real_distribution<float> distance_cqi_dist{-1,1};
    std::uniform_real_distribution<float> uniform_stochastics{0.0, 1.0};
    std::normal_distribution<float> sinr_stochastics{0.0, 1.0};

private:
    int modulation_m;

private: 
    float target_ber = 0.00005;

private: 
    int tx; 
    std::string tx_s; 

private: 
    int cqi_mode; 
    int cqi_period; 
    int sinr_period; 

private:
    int ri_period; 
    int max_ri; 

private: 
    float noise_interf; 
    float antenna_gain_tx = 18; 
    float antenna_gain_rx = 0; 
    float thermal_noise = -107; 
    float noise_figure = 7; 
    float interference = 0;     
    float noisedb = 0; 
    
private:  
    int period_counter = 0; 

private: 
    int verbosity = 0; 

private: 
    bool stochastics = true; 

private: 
    float tx_power; 

private: 
    bool los = true; 
    bool prev_los; 
    bool update_cs = true; 

private: 
    float pathloss; 
    float shadowing; 
    float c_dist; 

private: 
    float doppler_f; 
    float tc; 
    float ds; 
    float bc; 

private: 
    int n_rbs; 
    int n_sc_rb; 
    int n_rb_rbg; 
    int rbg_lookup_index; 
    int layers; 
    float overhead; 
    float scaling_factor; 
    int mimo_l; 
    int sinr_rbs; 
    int sinr_step; 
    int bandwidth; 

private: 
    bool mcs_tables;
    float mcs_lookup[MCS_INDEX_MAX];

private:
    bool is_init = false; 

private: 
    bool phy_log = false; 
    log_handler * logger;

private: 
    metric_handler metric_h; 
    metric_info metric_i; 

private: 
    std::vector<int> cqi_v; 
    std::vector<int> mcs_v; 
    std::vector<float> eff_v;
    std::vector<float> metric_v;
    std::vector<float> tp_v;
    std::vector<float> rsrp_v;
    std::vector<float> db_sinr_v;
    std::vector<float> l_sinr_v;

private: 
    float max_speed; 

private: 
    int cqi_s; 
    int mcs_s; 
    float eff_s;
    float rsrp_s;
    float db_sinr_s;
    float l_sinr_s;

private: 
    int scenario; 
    float eNB_h; 
    float ue_h; 
    float antenna_h;
    float antenna_h_172;  
    float w; 
    float dbp;

private: 
    float shadow_f; 
    float prev_shadow_f = 0; 
    float prev_shadow_v; 
    float corr_distance; 
    pos2d prev_pos; 

private: 
    float freq; 
    float freq_ghz; 

private:
    int current_ri;

private: 
    float current_t; 

private: 
    int id; 
};
#endif

