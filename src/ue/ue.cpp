/**********************************************
* Copyright 2022 Nokia
* Licensed under the BSD 3-Clause Clear License
* SPDX-License-Identifier: BSD-3-Clause-Clear
**********************************************/

#include <ue/ue.h>
#include <chrono>

ue::ue(int _id, ue_config ue_c, scenario_config _scenario_c,  phy_enb_config _phy_enb_config, pdcp_config _pdcp_config_ul, pdcp_config _pdcp_config_dl, harq_config _harq_config, bool _stochastics)
    :phy_h(_id, _scenario_c, ue_c.get_phy_config(), _phy_enb_config, _stochastics, ue_c.log_quality),
    mobility_m(_id, ue_c.mobility_c, _scenario_c.type), 
    pdcp_h(_pdcp_config_ul, _pdcp_config_dl, ue_c.log_traffic)
{
    is_ip = false; 
    if(!is_ip) traffic_m = std::shared_ptr<traffic_model>(new traffic_model(_id, ue_c.traffic_c));
    id = _id;
    delta_metric = ue_c.delta_metric; 
    delay_t_metric = ue_c.delay_t_metric; 
    beta_metric = ue_c.beta_metric; 

    verbosity = ue_c.log_ue; 
    log_ue = ue_c.log_ue && ue_c.log_id != "";
    if(log_ue)
    {
        ue_log.init("logs/" + ue_c.log_id + "/ue/", "ue_log_" + std::to_string(id), ue_c.log_freq);
    }
    log_mobility = ue_c.log_mobility && log_ue;
    log_traffic = ue_c.log_traffic && log_ue;
    log_quality = ue_c.log_quality && log_ue;
    n_antennas = std::max(std::min(MAX_N_ANTENNAS, ue_c.ue_m.n_antennas), MIN_N_ANTENNAS);

    phy_h.init(_phy_enb_config.n_rbgs, _phy_enb_config.bandwidth); 
    pdcp_h.init(_harq_config.mod, _harq_config.layers, _harq_config.log_units); 
}

ue::ue(int _queue_num_ul, int _queue_num_dl, std::chrono::microseconds * _init_t, int _id, ue_config ue_c, scenario_config _scenario_c,  phy_enb_config _phy_enb_config, pdcp_config _pdcp_config_ul, pdcp_config _pdcp_config_dl, harq_config _harq_config, bool _stochastics)
    :phy_h(_id, _scenario_c, ue_c.get_phy_config(), _phy_enb_config, _stochastics, ue_c.log_quality),
     mobility_m(_id, ue_c.mobility_c, _scenario_c.type),
     pdcp_h(_queue_num_ul, _queue_num_dl, _init_t, _pdcp_config_ul, _pdcp_config_dl, ue_c.log_quality)
{
    is_ip = true; 
    if(!is_ip) traffic_m = std::shared_ptr<traffic_model>(new traffic_model(_id, ue_c.traffic_c));
    id = _id;
    delta_metric = ue_c.delta_metric; 
    delay_t_metric = ue_c.delay_t_metric; 
    beta_metric = ue_c.beta_metric; 

    verbosity = ue_c.log_ue; 
    log_ue = ue_c.log_ue && ue_c.log_id != "";
    if(log_ue)
    {
        ue_log.init("logs/" + ue_c.log_id + "/ue/", "ue_log_" + std::to_string(id), ue_c.log_freq);
    }
    log_mobility = ue_c.log_mobility && log_ue;
    log_traffic = ue_c.log_traffic && log_ue;
    log_quality = ue_c.log_quality && log_ue;
    n_antennas = std::max(std::min(MAX_N_ANTENNAS, ue_c.ue_m.n_antennas), MIN_N_ANTENNAS);

    phy_h.init(_phy_enb_config.n_rbgs, _phy_enb_config.bandwidth); 
    pdcp_h.init(_harq_config.mod, _harq_config.layers, _harq_config.log_units); 
}

void ue::init()
{
    init_logger(); 
    init_pkt_capture(); 
}

void ue::init_logger()
{
    if(log_quality)
    {
        phy_h.set_logger(&ue_log);
    }
}

float ue::get_metric(int tx_dir, int f_index, int n_ues)
{
    return phy_h.get_metric(tx_dir, f_index, n_ues);
}

float ue::get_tp(int tx_dir, int f_index)
{
    return phy_h.get_tp(tx_dir, f_index);

}

int ue::get_cqi(int tx_dir, int f_index)
{
    return phy_h.get_cqi(tx_dir, f_index);
}

float ue::get_eff(int tx_dir, int f_index)
{
    return phy_h.get_eff(tx_dir, f_index);
}


int ue::get_mcs(int tx_dir, int f_index)
{
    return phy_h.get_mcs(tx_dir, f_index);
}

int ue::get_ri(int tx_dir)
{
    return phy_h.get_ri(tx_dir);
}

float ue::get_sinr(int tx_dir, int f_index)
{
    return phy_h.get_sinr(tx_dir, f_index);
}

float ue::get_linear_sinr(int tx_dir, int f_index)
{
    return phy_h.get_linear_sinr(tx_dir, f_index);
}

float ue::get_mean_sinr(int tx_dir)
{
    return phy_h.get_mean_sinr(tx_dir);
}

float ue::get_avg_tp(int tx_dir)
{
    return pdcp_h.get_tp(tx_dir, false);
}

bool ue::get_traffic_verbosity()
{
    return verbosity & log_traffic; 
}

float ue::get_avg_l(int tx_dir)
{
    return pdcp_h.get_latency(tx_dir, false);
}

float ue::get_oldest_timestamp(int tx_dir)
{
    return pdcp_h.get_oldest_timestamp(tx_dir);
}

bool ue::has_packets(int tx_dir)
{
    return pdcp_h.has_pkts(tx_dir);
}

void ue::update_pos()
{
    mobility_m.update_pos(current_t); 
    if(log_mobility) ue_log.log_partial("x:{} y:{} ", mobility_m.x(), mobility_m.y());
}

float ue::generate_data(int tx)
{
    float bits = traffic_m->generate(tx, current_t);
    pdcp_h.generate_pkts(tx, bits, traffic_m->get_pkt_size(tx), current_t);
    return bits; 
}

float ue::handle_pkt(float bits, int tx_dir, int f_index)
{
    float eff_tp = pdcp_h.handle_pkt(tx_dir, bits, phy_h.get_mcs(tx_dir, f_index), phy_h.get_sinr(tx_dir, f_index), mobility_m.get_distance()); 
    return eff_tp; 
}

float ue::get_delay_t()
{
    return delay_t_metric; 
}

float ue::get_delta()
{
    return delta_metric; 
}

int ue::get_id()
{
    return id; 
}

void ue::update_pdcp()
{
    pdcp_h.step(current_t);
    pdcp_h.release();
    if(log_traffic && ue_log.ready())
    {
        ue_log.log_partial("rul:{} rdl:{} ", pdcp_h.get_tp(T_UL, true), pdcp_h.get_tp(T_DL, true));     
        ue_log.log_partial("lul:{} ldl:{} ", pdcp_h.get_latency(T_UL, true), pdcp_h.get_latency(T_DL, true));     
        ue_log.log_partial("ilul:{} ildl:{} ", pdcp_h.get_ip_latency(T_UL, true), pdcp_h.get_ip_latency(T_DL, true));     
        ue_log.log_partial("eul:{} edl:{} ", pdcp_h.get_error(T_UL, true), pdcp_h.get_error(T_DL, true));     
    } 
}

void ue::init_pkt_capture()
{
    pdcp_h.init_pkt_capture();
}

void ue::generate_traffic()
{
    if(!is_ip)
    {
        float dl_bits = generate_data(T_DL); 
        float ul_bits = generate_data(T_UL); 
        if(log_traffic && ue_log.ready())
        {
            ue_log.log_partial("gul:{} gdl:{} ", pdcp_h.get_generated(T_UL, true), pdcp_h.get_generated(T_DL, true));     
        
        } 
    }
    else
    {
        float dl_bits = pdcp_h.get_ip_pkts(T_DL); 
        float ul_bits = pdcp_h.get_ip_pkts(T_UL); 
        if(log_traffic && ue_log.ready())
        {
            ue_log.log_partial("gul:{} gdl:{} ", pdcp_h.get_generated(T_UL, true), pdcp_h.get_generated(T_DL, true));     
        }
    }
}

void ue::add_ts()
{
    if(log_ue)
    {
        ue_log.log_partial("ts:{}", current_t);
        ue_log.flush(); 
    }
}

void ue::step()
{    
    update_pdcp(); 
    update_pos(); 
	generate_traffic(); 
    if(log_ue) add_ts(); 
    float distance = mobility_m.get_distance();
    phy_h.estimate_channel_estate(TX_DL, distance, *mobility_m.get_pos(), get_oldest_timestamp(TX_DL), get_avg_tp(TX_DL), current_t); 
    phy_h.estimate_channel_estate(TX_UL, distance, *mobility_m.get_pos(), get_oldest_timestamp(TX_UL), get_avg_tp(TX_UL), current_t); 
    
    return; 
}

void ue::print_traffic()
{
    if(is_ip)
    {
        if(verbosity >= 1)
        {
            LOG_INFO_I("ue::print_traffic") << " UE: " << id << " distance: " << mobility_m.get_distance() << END(); 
            LOG_INFO_I("ue::print_traffic") << " UE: " << id << " Mbps - UL gen. rate " << pdcp_h.get_generated(TX_UL, false) << " Mbps - DL gen. rate " << pdcp_h.get_generated(TX_DL, false) << END();
            LOG_INFO_I("ue::print_traffic") << " UE: " << id << " Mbps - UL error rate " << pdcp_h.get_error(TX_UL, false) << " Mbps - DL error rate " << pdcp_h.get_error(TX_DL, false) << END();
            LOG_INFO_I("ue::print_traffic") << " " << id << " Mbps - UL cons. rate " << pdcp_h.get_tp(TX_UL, false) << " Mbps - DL cons. rate " << pdcp_h.get_tp(TX_DL, false) << END();
            LOG_INFO_I("ue::print_traffic") << " " << id << " Mbps - UL latency " << pdcp_h.get_latency(TX_UL, false) << " s - DL latency " << pdcp_h.get_latency(TX_DL, false) << END();
            LOG_INFO_I("ue::print_traffic") << " " << id << " Mbps - UL IP latency " << pdcp_h.get_ip_latency(TX_UL, false) << " s - DL IP latency " << pdcp_h.get_ip_latency(TX_DL, false) << END();
        }
    }
}
