/**********************************************
* Copyright 2022 Nokia
* Licensed under the BSD 3-Clause Clear License
* SPDX-License-Identifier: BSD-3-Clause-Clear
**********************************************/


#ifndef PHY_HANDLER_H
#define PHY_HANDLER_H

#include <phy_layer/phy_layer.h>

//--------------------------------------------------------------------------------------------------
// phy_handler(): class which handles the UL and DL PHY Layer instances interfacing them with other
// modules.
// Input: 
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
class phy_handler
{
public: 
    phy_handler(int id, scenario_config _scenario_c, phy_ue_config _phy_ue_config, phy_enb_config _phy_enb_config, bool _stochastics = true,  int _verbosity = 0);
    void init(int n_rbs, int bandwidth);
    int get_cqi(int tx_dir, int f_index);
    float get_eff(int tx_dir, int f_index);
    int get_mcs(int tx_dir, int f_index);
    float get_sinr(int tx_dir, int f_index);
    int get_ri(int tx_dir);
    float get_linear_sinr(int tx_dir, int f_index);
    float get_mean_sinr(int tx_dir);
    void estimate_channel_estate(int tx_dir, float distance, const pos2d &pos, float oldest_t, float avg_tp, float _current_t);
    void set_logger(log_handler *_logger); 
    float get_metric(int tx, int f, int n_ues);
    float get_tp(int tx, int f);

private: 
    phy_layer phy_dl; 
    phy_layer phy_ul; 
private: 
    int verbosity = 0; 
};

#endif