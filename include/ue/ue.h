/**********************************************
* Copyright 2022 Nokia
* Licensed under the BSD 3-Clause Clear License
* SPDX-License-Identifier: BSD-3-Clause-Clear
**********************************************/

#ifndef UE_H
#define UE_H

#include <algorithm>
#include <string>
#include <phy_layer/phy_handler.h>
#include <mobility_models/mobility_model.h>
#include <pdcp_layer/pdcp_handler.h>
#include <traffic_models/traffic_model.h>
#include <mac_layer/harq_config.h>

// Logging
#include "utils/logging/mean_handler.h"
#include <ue/ue_config.h>
//--------------------------------------------------------------------------------------------------
// ue(): this class is in charge of modeling and updating the simulated UE's position, 
// generating/handling packets, estimating the Channel State Indicator and Releasing or Dropping the 
// already processed packets. The emulator's instances of mobility_model(), traffic_model(),
// netfilter_queues()/pkt_capture(), phy_handler() and pdcp_handler() are all part of the UE module
// Input: 
//      _queue_num_ul/_queue_num_dl: Netfilter Queues IDs (only if real traffic is being used)
//      _init_t: Initial timestamp (only for real IP traffic)
//      ue_c: configuration struct. Includes:
//              *ue_m: UE configuration struct which includes: 
//                      *n_antennas: number of physical antennas in the UE.
//                      *tx_power: transmission power for the UE.
//                      *cqi_period: period in ms for the PHY layer to re-estimate the CSI.
//                      *ri_period: period in ms for the PHY layer to estimate the Rank Indicator.
//                      *scaling_factor: used for carrier aggregation throughput calculation. Is signaled by the eNB in a real deployment. It can take the values: 1, 0.8, 0.75, 0.4
//                      *ue_h: height of the UE.
//              *traffic_c: traffic generator configuration struct which includes: 
//                      *type: selected traffic generator models for this type of UEs.
//                      *ul_target/_dl_target: target throughput for UL and DL respectively.
//                      *var_perc: size of the variance of the generated noise
//                      *pkt_size: virtual IP packet size in bits.
//              *mobility_c: mobility model configuration struct which includes: 
//                     _type: id of the desired mobility model for the current UE. 
//                     _x,_y: initial position only used if _random_init is set to false.
//                     _random_init: wether to randomly initialized or not
//                     _speed: target speed of the UEs.
//                     _speed_var: size of the white noise to be applied in each timestep to the target speed.
//                     _max_distance: max. distance of the UE to the gNB. 
//                     _time_target: target time used in some of the implemented models, such as random walk model.
//                     _time_target_var: size of the white noise to be applied in each timestep to the target time.
//              *delta_metric: metric-specific parameter.
//              *delay_t_metric: metric-specific parameter.
//              *beta_metric: metric-specific parameters. 
//              *priority: used by the user priotitization algorithm implemented in the MAC Layer. 
//      _scenario_c: scenario configuration struct which includes: 
//              *type: scenario type ID from the ones implemented.
//              *w: width of the beam.
//              *antenna_h: height of the antenna.
//              *eNB_h: height of the eNB/gNB.
//              *mcs_tables: enable/disable custom MCS-BLER-SINR curves for estimating the MCS indexes. 
//      _phy_enb_config: eNB configuration struct which includes: 
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
//      _pdcp_config_ul/_pdcp_config_dl: UL/DL PDCP configuration struct which includes:
//              *max_rtx_ul/dl: max number of retransmissions for UL/DL HARQ packets.
//              *air_delay_var: added variance to the delay comming from the air propagation.
//              *rtx_period_ul/dl: ack/nack delay in seconds for UL/DL HARQ packets.
//              *rtx_period_ul/dl_var: white noise spread around the ack/nack delay in seconds for UL/DL HARQ packets.
//              *rtx_proc_delay_ul/dl: ack/nack processing delay in seconds for UL/DL HARQ packets.
//              *rtx_proc_delay_ul/dl_var: white noise spread around the ack/nack processing delay in seconds for UL/DL HARQ packets.
//              *backhaul_d: backhauling delay, use for Release buffer.
//              *backhaul_d_var: white noise spread around the backhauling delay.
//              *order_pkts: enable/disable RLC reordering.
//      _harq_config: HARQ configuration struct which includes: 
//              *mod: 0 - 64 QAM | 1 - 256 QAM
//              *layers: Number of MIMO layers. 
//              *log_units: number of RBs per RBG group. 
//      log_c: Logger configuration struct which includes: 
//              *log_freq: data is logged according to this period in ms.
//              *log_ue: enable/disable UE logging
//              *log_id: UE's unique ID
//              *verbosity: enable/disable verbosity
//--------------------------------------------------------------------------------------------------
class ue
{
public: 
    ue(int _id, ue_config ue_c, scenario_config _scenario_c, phy_enb_config _phy_enb_config, pdcp_config _pdcp_config_ul, pdcp_config _pdcp_config_dl, harq_config _harq_config, bool _stochastics = true);
    ue(int _queue_num_ul, int _queue_num_dl, std::chrono::microseconds * _init_t, int _id, ue_config ue_c, scenario_config _scenario_c, phy_enb_config _phy_enb_config, pdcp_config _pdcp_config_ul, pdcp_config _pdcp_config_dl, harq_config _harq_config, bool _stochastics = true);

public: 
    void init(); 
    float get_metric(int tx_dir, int f_index, int n_ues);
    float get_tp(int tx_dir, int f_index);
    int get_cqi(int tx_dir, int f_index);
    float get_eff(int tx_dir, int f_index);
    int get_mcs(int tx_dir, int f_index);
    float get_sinr(int tx_dir, int f_index);
    float get_linear_sinr(int tx_dir, int f_index);
    float get_mean_sinr(int tx_dir);
    int get_ri(int tx_dir);
    float get_avg_tp(int tx_dir);
    bool get_traffic_verbosity();
    float get_avg_l(int tx_dir);
    float get_consumed_bits(int tx_dir);
    float get_oldest_timestamp(int tx_dir);
    float add_pkts(int tx, std::deque<ip_pkt> &pkts);
    int get_id(); 
    bool has_packets(int tx_dir);
    float get_delay_t(); 
    float get_delta(); 
    float handle_pkt(float bits, int tx_dir, int f_index);
    void step();
    void add_current_t(double _current_t){current_t = _current_t; }

private: 
    void init_pkt_capture(); 
    void init_logger(); 

protected: 

    void update_pos();
    void update_pdcp(); 
    float generate_data(int tx);
    void generate_traffic();
    void add_ts(); 

public:
    void print_traffic();

protected: 
    int id; 
    bool is_ip; 

protected: 
    pdcp_handler pdcp_h;
    phy_handler phy_h; 
    mobility_model mobility_m; 
    std::shared_ptr<traffic_model> traffic_m; 
    float delta_metric; 
    float delay_t_metric; 
    float beta_metric; 

protected: 
    bool log_ue; 
    bool log_traffic; 
    bool log_mobility; 
    bool log_quality; 
    int verbosity; 
    bool store_data; 

protected: 
    log_handler ue_log; 
    
protected: 
    int n_antennas; 
 
protected: 
    double current_t;

};

#endif