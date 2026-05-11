/**********************************************
 * Copyright 2022 Nokia
 * Licensed under the BSD 3-Clause Clear License
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 **********************************************/

#include <ue/ue.h>
#include <cassert>
#include <chrono>

namespace
{
packet_handler_config make_pdcp_packet_config(int ue_type,
                                              int tx_dir,
                                              int queue_num,
                                              int ue_id,
                                              std::chrono::microseconds *init_t,
                                              traffic_config traffic_c,
                                              pdcp_config pdcp_c,
                                              dualpi2_config l4s_c,
                                              bool log_traffic,
                                              bool log_quality)
{
    packet_handler_config cfg;
    cfg.ue_type = ue_type;
    cfg.tx_dir = tx_dir;
    cfg.queue_num = queue_num;
    cfg.ue_id = ue_id;
    cfg.init_t = init_t;
    cfg.traffic_c = traffic_c;
    cfg.pdcp_c = pdcp_c;
    cfg.l4s_c = l4s_c;
    cfg.log_traffic = log_traffic;
    cfg.log_quality = log_quality;
    return cfg;
}
}

ue::ue(int _id,
       ue_config ue_c,
       scenario_config _scenario_c,
       phy_enb_config _phy_enb_config,
       pdcp_config _pdcp_config_ul,
       pdcp_config _pdcp_config_dl,
       harq_config _harq_config,
       int ue_type,
       std::chrono::microseconds *init_t,
       bool _stochastics)
     :  map(_scenario_c.map_file),
        pdcp_dl(make_pdcp_packet_config(ue_type, TX_DL, ue_c.dl_queue_n, _id, init_t, ue_c.traffic_c, _pdcp_config_dl, ue_c.l4s_c, ue_c.log_traffic, ue_c.log_quality), ue_c.log_traffic || ue_c.log_quality),
        pdcp_ul(make_pdcp_packet_config(ue_type, TX_UL, ue_c.ul_queue_n, _id, init_t, ue_c.traffic_c, _pdcp_config_ul, ue_c.l4s_c, ue_c.log_traffic, ue_c.log_quality), ue_c.log_traffic || ue_c.log_quality),
        phy_dl(TX_DL, _id, _scenario_c, ue_c.get_phy_config(), _phy_enb_config, _stochastics, ue_c.log_quality),
        phy_ul(TX_UL, _id, _scenario_c, ue_c.get_phy_config(), _phy_enb_config, _stochastics, ue_c.log_quality),
        mobility_m(_id, ue_c.mobility_c, _scenario_c.type, map.getMaxApothem()),
        phy_s(_id, ue_c, _scenario_c, _phy_enb_config)
{
    id = _id;
    delta_metric = ue_c.delta_metric;
    delay_t_metric = ue_c.delay_t_metric;
    beta_metric = ue_c.beta_metric;

    verbosity = ue_c.log_ue;
    log_ue = ue_c.log_ue && ue_c.log_id != "";
    if (log_ue)
    {
        ue_log.init("logs/" + ue_c.log_id + "/ue/", "ue_log_" + std::to_string(id), ue_c.log_freq);
    }
    log_mobility = ue_c.log_mobility && log_ue;
    log_traffic = ue_c.log_traffic && log_ue;
    log_quality = ue_c.log_quality && log_ue;
    n_antennas = std::max(std::min(MAX_N_ANTENNAS, ue_c.ue_m.n_antennas), MIN_N_ANTENNAS);

    pdcp_ul.set_pkt_delay_budget(ue_c.pkt_delay_budget);
    pdcp_dl.set_pkt_delay_budget(ue_c.pkt_delay_budget);
    phy_cqi_period = ue_c.ue_m.cqi_period;
    phy_ri_period = ue_c.ue_m.ri_period;
    phy_freq = _phy_enb_config.frequency;
    phy_speed = ue_c.get_phy_config().max_speed;
    phy_doppler_f = phy_freq * phy_speed / LIGHTSPEED;
    phy_dl.init(_phy_enb_config.n_rbgs, _phy_enb_config.bandwidth);
    phy_ul.init(_phy_enb_config.n_rbgs, _phy_enb_config.bandwidth);
    pdcp_ul.init(_harq_config.mod, _harq_config.layers, _harq_config.log_units);
    pdcp_dl.init(_harq_config.mod, _harq_config.layers, _harq_config.log_units);
}

void ue::init()
{
    init_logger();
}

void ue::init_logger()
{
    if (log_quality)
    {
        phy_dl.set_logger(&ue_log);
        phy_ul.set_logger(&ue_log);
    }
}

phy_layer& ue::phy(int tx_dir)
{
    assert(tx_dir == TX_DL || tx_dir == TX_UL);
    return tx_dir == TX_DL ? phy_dl : phy_ul;
}

const phy_layer& ue::phy(int tx_dir) const
{
    assert(tx_dir == TX_DL || tx_dir == TX_UL);
    return tx_dir == TX_DL ? phy_dl : phy_ul;
}

pdcp_layer& ue::pdcp(int tx_dir)
{
    assert(tx_dir == TX_DL || tx_dir == TX_UL);
    return tx_dir == TX_DL ? pdcp_dl : pdcp_ul;
}

const pdcp_layer& ue::pdcp(int tx_dir) const
{
    assert(tx_dir == TX_DL || tx_dir == TX_UL);
    return tx_dir == TX_DL ? pdcp_dl : pdcp_ul;
}

float ue::get_metric(int tx_dir, int f_index, int n_ues)
{
    return phy(tx_dir).get_metric(f_index, n_ues);
}

float ue::get_tp(int tx_dir, int f_index)
{
    return phy(tx_dir).get_tp(f_index);
}

int ue::get_cqi(int tx_dir, int f_index)
{
    return phy(tx_dir).get_cqi(f_index);
}

float ue::get_eff(int tx_dir, int f_index)
{
    return phy(tx_dir).get_eff(f_index);
}

int ue::get_mcs(int tx_dir, int f_index)
{
    return phy(tx_dir).get_mcs(f_index);
}

int ue::get_ri(int tx_dir)
{
    return phy(tx_dir).get_ri();
}

float ue::get_sinr(int tx_dir, int f_index)
{
    return phy(tx_dir).get_sinr(f_index);
}

float ue::get_linear_sinr(int tx_dir, int f_index)
{
    return phy(tx_dir).get_linear_sinr(f_index);
}

float ue::get_mean_sinr(int tx_dir)
{
    return phy(tx_dir).get_mean_sinr();
}

float ue::get_avg_tp(int tx_dir)
{
    return pdcp(tx_dir).get_tp(false);
}

bool ue::get_traffic_verbosity()
{
    return verbosity & log_traffic;
}

float ue::get_avg_l(int tx_dir)
{
    return pdcp(tx_dir).get_latency(false);
}

float ue::get_oldest_timestamp(int tx_dir)
{
    return pdcp(tx_dir).get_oldest_timestamp();
}

bool ue::has_packets(int tx_dir)
{
    return pdcp(tx_dir).has_pkts();
}

schedule_candidate ue::get_schedule_candidate(int tx_dir, int f_index, int n_ues, int ue_index)
{
    schedule_candidate candidate;
    candidate.ue_index = ue_index;
    candidate.ue_id = id;
    candidate.bits_per_symbol = phy(tx_dir).get_tp(f_index);
    candidate.metric = phy(tx_dir).get_metric(f_index, n_ues);
    candidate.has_data = pdcp(tx_dir).has_pkts() && candidate.bits_per_symbol > 0;
    return candidate;
}

void ue::update_pos()
{
    mobility_m.update_pos(current_t);
    if (log_mobility)
        ue_log.log_partial("x:{} y:{} ", mobility_m.x(), mobility_m.y());
}



float ue::handle_pkt(float bits, int tx_dir, int f_index)
{
    float eff_tp = pdcp(tx_dir).handle_pkt(bits, phy(tx_dir).get_mcs(f_index), phy(tx_dir).get_sinr(f_index), mobility_m.get_distance());
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
    pdcp_dl.step(current_t);
    pdcp_ul.step(current_t);
    pdcp_dl.release();
    pdcp_ul.release();
    if (log_traffic && ue_log.ready())
    {
        ue_log.log_partial("rul:{} rdl:{} ", pdcp_ul.get_tp(true), pdcp_dl.get_tp(true));
        ue_log.log_partial("lul:{} ldl:{} ", pdcp_ul.get_latency(true), pdcp_dl.get_latency(true));
        ue_log.log_partial("ilul:{} ildl:{} ", pdcp_ul.get_ip_latency(true), pdcp_dl.get_ip_latency(true));
        ue_log.log_partial("eul:{} edl:{} ", pdcp_ul.get_error(true), pdcp_dl.get_error(true));
        dualpi2_stats l4s_ul = pdcp_ul.get_l4s_interval_stats();
        dualpi2_stats l4s_dl = pdcp_dl.get_l4s_interval_stats();
        ue_log.log_partial("ceul:{} cedl:{} cebul:{} cebdl:{} ", l4s_ul.ce_packets, l4s_dl.ce_packets, l4s_ul.ce_bits, l4s_dl.ce_bits);
        ue_log.log_partial("aqmdul:{} aqmddl:{} aqmdbul:{} aqmdbdl:{} ", l4s_ul.aqm_drops, l4s_dl.aqm_drops, l4s_ul.aqm_drop_bits, l4s_dl.aqm_drop_bits);
        ue_log.log_partial("qlul:{} qldl:{} qcul:{} qcdl:{} ", l4s_ul.l4s_queue_size, l4s_dl.l4s_queue_size, l4s_ul.classic_queue_size, l4s_dl.classic_queue_size);
        ue_log.log_partial("plul:{} pldl:{} pcul:{} pcdl:{} pclul:{} pcldl:{} ", l4s_ul.p_l, l4s_dl.p_l, l4s_ul.p_c, l4s_dl.p_c, l4s_ul.p_cl, l4s_dl.p_cl);
    }
}

void ue::add_ts()
{
    if (log_ue)
    {
        ue_log.log_partial("ts:{}", current_t);
        ue_log.flush();
    }
}

void ue::init_phy_update_rates(float _doppler_f, int _cqi_p, int _ri_p)
{
    if (_doppler_f > 0)
    {
        phy_tc = 0.423 / _doppler_f;
        float min_sinr_update = phy_tc / 0.001;
        int min_update = std::min(_cqi_p, _ri_p);
        if (min_sinr_update > min_update)
            phy_sinr_period = min_update;
        else
            phy_sinr_period = std::max(1, (int)floor(min_sinr_update));
    }
    else
    {
        phy_tc = -1;
        phy_sinr_period = std::min(_cqi_p, _ri_p);
    }
}

void ue::estimate_channel_state()
{

    const pos2d &pos = *mobility_m.get_pos();
    float distance = mobility_m.get_distance();

    init_phy_update_rates(phy_doppler_f, phy_cqi_period, phy_ri_period);

    if (!phy_init_o2i)
    {
        phy_o2i = phy_s.get_o2i();
        phy_init_o2i = true;
        phy_corr_distance = phy_s.get_correlation_distance();
    }

    bool update_sinr = phy_period_counter % phy_sinr_period == 0;
    if (update_sinr)
    {
        phy_c_dist = get_distance(pos, phy_prev_pos);
        if (phy_c_dist > phy_corr_distance || phy_update_cs)
        {
            phy_corr_distance = phy_s.get_correlation_distance();
            phy_update_cs = false;
            phy_prev_pos = pos;
        }

        
        phy_macro_fading = map.getMacroFadingValue(pos.x, pos.y);
        
    }

    phy(TX_DL).estimate_channel_state(distance, phy_s, phy_macro_fading, phy_o2i, TX_DL, pos, get_oldest_timestamp(TX_DL), get_avg_tp(TX_DL), current_t);
    phy(TX_UL).estimate_channel_state(distance, phy_s, phy_macro_fading, phy_o2i, TX_UL, pos, get_oldest_timestamp(TX_UL), get_avg_tp(TX_UL), current_t);
    phy_period_counter++;
}

void ue::step()
{
    update_pdcp();
    update_pos();
    if (log_traffic && ue_log.ready())
    {
        ue_log.log_partial("gul:{} gdl:{} ", pdcp_ul.get_generated(true), pdcp_dl.get_generated(true));
    }
    if (log_ue)
        add_ts();

    estimate_channel_state();

    return;
}

void ue::print_traffic()
{
    if (verbosity >= 1)
    {
        LOG_INFO_I("ue::print_traffic") << " UE: " << id << " distance: " << mobility_m.get_distance() << END();
        LOG_INFO_I("ue::print_traffic") << " UE: " << id << " Mbps - UL gen. rate " << pdcp_ul.get_generated(false) << " Mbps - DL gen. rate " << pdcp_dl.get_generated(false) << END();
        LOG_INFO_I("ue::print_traffic") << " UE: " << id << " Mbps - UL error rate " << pdcp_ul.get_error(false) << " Mbps - DL error rate " << pdcp_dl.get_error(false) << END();
        LOG_INFO_I("ue::print_traffic") << " " << id << " Mbps - UL cons. rate " << pdcp_ul.get_tp(false) << " Mbps - DL cons. rate " << pdcp_dl.get_tp(false) << END();
        LOG_INFO_I("ue::print_traffic") << " " << id << " Mbps - UL latency " << pdcp_ul.get_latency(false) << " s - DL latency " << pdcp_dl.get_latency(false) << END();
        LOG_INFO_I("ue::print_traffic") << " " << id << " Mbps - UL IP latency " << pdcp_ul.get_ip_latency(false) << " s - DL IP latency " << pdcp_dl.get_ip_latency(false) << END();

        auto ul_status = pdcp_ul.get_queue_status();
        auto dl_status = pdcp_dl.get_queue_status();

        LOG_INFO_I("ue::print_traffic") << " UL IP buf: " << ul_status.ip_buffer_size << " pkts (oldest UID " << ul_status.ip_oldest_uid << ", delay " << ul_status.ip_oldest_age << " s)" << END();
        LOG_INFO_I("ue::print_traffic") << " DL IP buf: " << dl_status.ip_buffer_size << " pkts (oldest UID " << dl_status.ip_oldest_uid << ", delay " << dl_status.ip_oldest_age << " s)" << END();

        LOG_INFO_I("ue::print_traffic") << " UL HARQ buf: " << ul_status.harq_size << " pkts (oldest ID " << ul_status.harq_oldest_id << ", delay " << ul_status.harq_oldest_age << " s, n_tx " << ul_status.harq_oldest_n_tx << ")" << END();
        LOG_INFO_I("ue::print_traffic") << " DL HARQ buf: " << dl_status.harq_size << " pkts (oldest ID " << dl_status.harq_oldest_id << ", delay " << dl_status.harq_oldest_age << " s, n_tx " << dl_status.harq_oldest_n_tx << ")" << END();

        LOG_INFO_I("ue::print_traffic") << " UL capture buf: " << ul_status.capture_size << " pkts (oldest UID " << ul_status.capture_oldest_uid << ", delay " << ul_status.capture_oldest_age << " s)" << END();
        LOG_INFO_I("ue::print_traffic") << " DL capture buf: " << dl_status.capture_size << " pkts (oldest UID " << dl_status.capture_oldest_uid << ", delay " << dl_status.capture_oldest_age << " s)" << END();

        LOG_INFO_I("ue::print_traffic") << " UL release buf: " << ul_status.release_size << " pkts (oldest UID " << ul_status.release_oldest_uid << ", delay " << ul_status.release_oldest_age << " s)" << END();
        LOG_INFO_I("ue::print_traffic") << " DL release buf: " << dl_status.release_size << " pkts (oldest UID " << dl_status.release_oldest_uid << ", delay " << dl_status.release_oldest_age << " s)" << END();
    }
}
