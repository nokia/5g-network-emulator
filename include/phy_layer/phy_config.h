/**********************************************
* Copyright 2022 Nokia
* Licensed under the BSD 3-Clause Clear License
* SPDX-License-Identifier: BSD-3-Clause-Clear
**********************************************/

#ifndef PHY_CONFIG_H
#define PHY_CONFIG_H

//--------------------------------------------------------------------------------------------------
// scenario_config(): modeled scenario configuration struct.
// Input: 
//              *type: scenario type ID from the ones implemented.
//              *w: width of the beam.
//              *antenna_h: height of the antenna.
//              *eNB_h: height of the eNB/gNB.
//              *mcs_tables: enable/disable custom MCS-BLER-SINR curves for estimating the MCS indexes. 
//--------------------------------------------------------------------------------------------------
struct scenario_config
{
    scenario_config(){}
    scenario_config(int _type, float _w, float _antenna_h, float _eNB_h, bool _mcs_tables = true)
              {
                    // Modulation
                    type = _type; 
                    w = _w; 
                    antenna_h = _antenna_h; 
                    eNB_h = _eNB_h;
                    mcs_tables = _mcs_tables;                
              }
    int type;
    float w; 
    float antenna_h; 
    float eNB_h; 
    bool mcs_tables; 
};

//--------------------------------------------------------------------------------------------------
// phy_enb_config(): PHY Layer eNB/gNB configuration struct.
// Input: 
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
//--------------------------------------------------------------------------------------------------
struct phy_enb_config
{
    phy_enb_config(){}
    phy_enb_config( float _tx_power, int _modulation_m, float _target_ber, 
                    int _cqi_mode, float _frequency, int _bandwidth, int _mimo_l, 
                    int _metric_type, int _n_int_ues, float _d_int, 
                    float _thermal_n, float _figure_n, float _tx_gain, float _rx_gain)
                    {
                        tx_power = _tx_power; 
                        modulation_m = _modulation_m; 
                        target_ber = _target_ber; 
                        cqi_mode = _cqi_mode; 
                        frequency = _frequency;      
                        bandwidth = _bandwidth; 
                        mimo_l = _mimo_l;    
                        metric_type = _metric_type;       
                        n_int_ues = _n_int_ues;
                        d_interference = _d_int; 
                        thermal_noise = _thermal_n; 
                        figure_noise = _figure_n; 
                        tx_gain = _tx_gain; 
                        rx_gain = _rx_gain; 
                    }
    float tx_power; 
    int modulation_m;
    float target_ber; 
    int cqi_mode; 
    float frequency; 
    int bandwidth; 
    int n_rbgs; 
    int n_sc_rbg;
    int mimo_l; 
    int metric_type; 
    int n_int_ues;
    float d_interference; 
    float thermal_noise; 
    float figure_noise; 
    float tx_gain; 
    float rx_gain; 
};

//--------------------------------------------------------------------------------------------------
// phy_ue_config(): UE PHY layer configuration struct.
// Input: 
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
//--------------------------------------------------------------------------------------------------
struct phy_ue_config
{
    phy_ue_config(){}
    phy_ue_config(float _tx_power, int _cqi_period, 
              int _ri_period, int _max_ri, float _ue_h, float _priority, 
              float _delay_t, float _delta, float _beta, float _max_speed, float _scaling_factor)
              {
                    tx_power = _tx_power; 
                    cqi_period = _cqi_period; 

                    // RI estimation
                    ri_period = _ri_period; 
                    max_ri = _max_ri; 
                
                    ue_h = _ue_h; 
                    delay_t = _delay_t; 
                    delta = _delta;
                    beta = _beta;
                    max_speed = _max_speed;

                    scaling_factor = _scaling_factor; 

                    priority = _priority;
              }
    float tx_power; 
    int cqi_period; 
    int ri_period; 
    int max_ri; 
    float ue_h; 
    float delay_t; 
    float delta;
    float beta;
    float max_speed; 
    float scaling_factor; 
    float priority = 1; 
}; 

#endif
