/**********************************************
* Copyright 2022 Nokia
* Licensed under the BSD 3-Clause Clear License
* SPDX-License-Identifier: BSD-3-Clause-Clear
**********************************************/

#ifndef CONFIGURATION_LOADER_H
#define CONFIGURATION_LOADER

#include <iostream>
#include <sstream>
#include <string>
#include <fstream>
#include <ctime>
#include <list>

#include <utils/terminal_logging.h>

#include <mac_layer/mac_definitions.h>
#include <phy_layer/phy_l_definitions.h>
#include <mobility_models/mobility_config.h>
#include <traffic_models/traffic_config.h>
#include <pdcp_layer/pdcp_config.h>
#include <ue/ue_config.h>
#include <mac_layer/mac_config.h>
#include <phy_layer/phy_config.h>
#include <mac_layer/tdd_handler.h>
#include <utils/logging/log_handler.h>

#define DURATION_DEFAULT 10.0f
#define PERIOD_DEFAULT 1.0f
//
// MAC LAYER
//

// HARQ DEFINITIONS
#define HARQ_PROCESSING_DELAY_DFLT 1  // ms
#define HARQ_PROCESSING_DELAY_VAR 1 // ms
#define HARQ_RTX_PERIOD_DFLT 4  // ms
#define HARQ_RTX_PERIOD_VAR 0  // ms
#define HARQ_RTX_VAR_DFLT 1 // ms
#define HARQ_MAX_DFLT_RTX 4
#define HARQ_AIR_DELAY_DFLT 0.01 // Percentage

// SCHEDULER DEFINITIONS
#define DEFAULT_METRIC SCH_METRIC_DEFAULT // Round Robin
#define DELTA_METRIC_DEFAULT 1.0
#define DELAY_METRIC_DEFAULT 0.1
#define BETA_METRIC_DEFAULT 0.5

// UE PRIORITY
#define DEFAULT_UE_PRIORITY 1

// DEFAULT TDD
#define DFLT_CONFIG 45 // Balanced between UL/DL

// 
// MOBILITY MODELS
//

#define MOBILITY_MODEL_DEFAULT STATIC_MODEL // STATIC MODEL
#define MAX_DISTANCE_DEFAULT 1000 // METERS
#define T_TARGET_DEFAULT 10 // S
#define T_TARGET_VAR_DEFAULT 0 //

// 
// PDCP LAYER
//

#define BACKHAUL_D_DEFAULT 0.005
#define BACKHAUL_D_VAR_DEFAULT 0.001
#define DEFAULT_ORDER_PKTS true
#define MAX_RTX_DEFAULT 4
#define AIR_DELAY_VAR_DEFAULT 0.01 // %
#define RTX_PERIOD_DEFAULT 4 // ms
#define RTX_PERIOD_VAR_DEFAULT 0 // ms
#define RTX_PROC_DELAY_DEFAULT 1 // ms
#define RTX_PROC_DELAY_VAR_DEFAULT 1 // ms

//
// SCENARIO
//

#define SCENARIO_TYPE_DEFAULT 0
#define STREET_WIDTH_DEFAULT 10
#define ANTENNA_HEIGHT_DEFAULT 10
#define ENB_HEIGHT_DEFAULT 15
#define MCS_TABLES_DEFAULT true 

//
// MAC LAYER
//

#define NUMEROLOGY_DEFAULT SCH_NUMEROLOGY_DEFAULT
#define N_RE_DEFAULT SCH_N_RE_DEFAULT
#define N_OFDM_DEFAULT SCH_N_OFDM_DEFAULT
#define BANDWIDTH_DEFAULT 75000000
#define FREQUENCY_DEFAULT 3500000000
#define OFDM_SYMBOLS_DEFAULT 14
#define SCHEDULING_MODE_DEFAULT SCH_LOCALIZED_MODE
#define SCHEDULING_TYPE_DEFAULT 0
#define SCHEDULING_CONFIG_DEFAULT 1 
#define DUPLEXING_TYPE_DEFAULT FDD
#define N_DL_SLOTS_DEFAULT 1
#define N_UL_SLOTS_DEFAULT 1
#define N_TRANSITION_C_DEFAULT DFLT_CONFIG 

//
// PHY LAYER
//

#define MIMO_LAYERS_DEFAULT 4
#define ENB_TX_POWER_DEFAULT 40.0
#define MODULATION_DEFAULT MODULATION_256
#define TARGET_BER_DEFAULT  0.00005 
#define CQI_MODE_DEFAULT CQI_WIDEBAND
#define CQI_PERIOD_DEFAULT 5
#define ENB_ANTENNAS_DEFAULT 4

// 
// INTERFERENCE & NOISE
//
#define INTERF_UES_DEFAULT 10 
#define INTERF_DISTANCE_DEFAULT 1500
#define THERMAL_N_DEFAULT -107
#define NOISE_FIGURE_DEFAULT 7
#define TX_GAIN_DEFAULT 18
#define RX_GAIN_DEFAULT 0

// SINR CONFIG
#define RI_PERIOD_DEFAULT 5

//
// TRAFFIC MODEL
//

#define TRAFFIC_MODEL_DEFAULT CONSTANT_TRAFFIC_MODEL

#define UL_TARGET_DEFAULT 10.0
#define DL_TARGET_DEFAULT 50.0

#define VAR_PERCENT_DEFAULT 50.0
#define PKT_SIZE_DEFAULT 500

//
// UE
//
#define UE_TX_POWER_DEFAULT 40.0
#define N_UES_DEFAULT 10
#define SIM_UE 1
#define REAL_UE 0

#define N_ANTENNAS_DEFAULT 2

#define UE_HEIGHT_DEFAULT 1.5

#define TOMS 1000.0f/3600.0f

//
// UE CONFIG
//
struct ue_full_config
{
    ue_full_config()
    {
        // UE CONFIG
        ue_c.ue_m.n_antennas = N_ANTENNAS_DEFAULT;
        ue_c.ue_m.tx_power = UE_TX_POWER_DEFAULT;
        ue_c.ue_m.cqi_period = CQI_PERIOD_DEFAULT;
        ue_c.ue_m.ri_period = RI_PERIOD_DEFAULT;
        // TRAFFIC MODEL CONFIG
        ue_c.traffic_c.type = TRAFFIC_MODEL_DEFAULT;
        ue_c.traffic_c.ul_target = UL_TARGET_DEFAULT;
        ue_c.traffic_c.dl_target = DL_TARGET_DEFAULT;
        ue_c.traffic_c.ul_traffic_file = "";
        ue_c.traffic_c.dl_traffic_file = "";
        ue_c.traffic_c.var_perc = VAR_PERCENT_DEFAULT;
        ue_c.traffic_c.pkt_size = PKT_SIZE_DEFAULT;
        // MOBILITY MODEL CONFIG
        ue_c.mobility_c.type = MOBILITY_MODEL_DEFAULT;
        ue_c.mobility_c.init_pos = pos2d(0.0, 0.0); 
        ue_c.mobility_c.random_init = true;
        ue_c.mobility_c.speed = 0.0;
        ue_c.mobility_c.speed_var = 0.0;
        ue_c.mobility_c.max_distance = MAX_DISTANCE_DEFAULT;
        ue_c.mobility_c.time_target = T_TARGET_DEFAULT;
        ue_c.mobility_c.time_target_var = T_TARGET_VAR_DEFAULT;

        // METRICS
        ue_c.delay_t_metric = DELAY_METRIC_DEFAULT; 
        ue_c.beta_metric = BETA_METRIC_DEFAULT; 
        ue_c.delta_metric = DELTA_METRIC_DEFAULT; 

        // PRIORITY
        ue_c.priority = DEFAULT_UE_PRIORITY;

        ue_c.ue_m.ue_h = UE_HEIGHT_DEFAULT; 
    }
    int n_ues = N_UES_DEFAULT; 
    int ue_type = SIM_UE; 
    std::string id;
    ue_config ue_c; 
    
};

//
// GENERAL ENB CONFIG
//
struct enb_config
{
    enb_config(phy_enb_config _phy_config, pdcp_config _pdcp_ul, pdcp_config _pdcp_dl)
    {
        phy_config = _phy_config; 
        pdcp_ul = _pdcp_ul; 
        pdcp_dl = _pdcp_dl; 
    }
    phy_enb_config phy_config;
    pdcp_config pdcp_ul;
    pdcp_config pdcp_dl;
};

class configuration_loader
{
public:
    configuration_loader()
    {
        
    }
    configuration_loader(std::string dir)
    {
        load(dir);
    }
    
public: 
    void load(std::string dir)
    {
        if(!dir.back() == '/') dir += '/';
        std::ifstream infile(dir + "config.ini");
        if(infile.good())
        {
            std::string line;
            std::string mode = "None";
            while (std::getline(infile, line))
            {
                if(std::isalpha(line.front()))
                {    
                    std::istringstream iss(line);
                    std::string key, value;
                    if(iss >> key >> value)
                    {
                        if(key.back() == ':'){
                            key.pop_back(); 
                            if(mode == "UE")
                            {
                                if(key == "ue_id") 
                                {
                                    LOG_INFO_I("configuration_loader::load") << "Adding UEs with config [" << value << "]: " << END();
                                    ue_full_config ue_c; 
                                    ue_c.id = value; 
                                    ue_c.ue_c.log_id = get_unique_id();
                                    ue_c_list.push_back(ue_c);
                                    ue_c_list.back().ue_c.mobility_c.random_v=random_v;
                                    ue_c_list.back().ue_c.traffic_c.random_v=random_v;
                                }  
                                if(key == "n_ues") ue_c_list.back().n_ues = std::stoi(value);
                                if(key == "ue_type") ue_c_list.back().ue_type = std::stoi(value);
                                // QUEUE_NUM
                                if(key == "ul_queue_n") ue_c_list.back().ue_c.ul_queue_n = std::stoi(value);
                                if(key == "dl_queue_n") ue_c_list.back().ue_c.dl_queue_n = std::stoi(value);
                                // UE CONFIG
                                if(key == "n_antennas") ue_c_list.back().ue_c.ue_m.n_antennas = std::stoi(value);
                                if(key == "tx_power") ue_c_list.back().ue_c.ue_m.tx_power = std::stof(value);
                                if(key == "cqi_period") ue_c_list.back().ue_c.ue_m.cqi_period = std::stoi(value);
                                if(key == "ri_period") ue_c_list.back().ue_c.ue_m.ri_period = std::stoi(value);  
                                if(key == "scaling_factor") ue_c_list.back().ue_c.ue_m.scaling_factor = std::stof(value);  
                                // PRIORITY
                                if(key == "priority") ue_c_list.back().ue_c.priority = std::stof(value);
                                // TRAFFIC MODEL CONFIG
                                if(key == "ul_target") ue_c_list.back().ue_c.traffic_c.ul_target = std::stof(value);
                                if(key == "dl_target") ue_c_list.back().ue_c.traffic_c.dl_target = std::stof(value);
                                if(key == "ul_traffic_file") ue_c_list.back().ue_c.traffic_c.ul_traffic_file = value;
                                if(key == "dl_traffic_file") ue_c_list.back().ue_c.traffic_c.dl_traffic_file = value;
                                if(key == "delay") ue_c_list.back().ue_c.traffic_c.delay = std::stof(value);
                                if(key == "var_perc") ue_c_list.back().ue_c.traffic_c.var_perc =  random_v ? std::stof(value) : 0;
                                if(key == "pkt_size") ue_c_list.back().ue_c.traffic_c.pkt_size = std::stoi(value);
                                if(key == "traffic_type") ue_c_list.back().ue_c.traffic_c.type = std::stoi(value);
                                // MOBILITY MODEL CONFIG
                                if(key == "mobility_type") ue_c_list.back().ue_c.mobility_c.type = std::stof(value);
                                if(key == "pos_x") ue_c_list.back().ue_c.mobility_c.init_pos._x(std::stof(value));
                                if(key == "pos_y") ue_c_list.back().ue_c.mobility_c.init_pos._y(std::stof(value));
                                if(key == "random_init")
                                {
                                    if(value == "true" || value == "1") ue_c_list.back().ue_c.mobility_c.random_init = true;
                                    if(value == "false" || value == "0") ue_c_list.back().ue_c.mobility_c.random_init = false;
                                }
                                if(key == "speed") ue_c_list.back().ue_c.mobility_c.speed = TOMS*std::stof(value);
                                if(key == "speed_var") ue_c_list.back().ue_c.mobility_c.speed_var = TOMS*std::stof(value);
                                if(key == "max_distance") ue_c_list.back().ue_c.mobility_c.max_distance = std::stof(value);
                                if(key == "time_target_var") ue_c_list.back().ue_c.mobility_c.time_target_var = std::stof(value);
                                if(key == "time_target") ue_c_list.back().ue_c.mobility_c.time_target = std::stof(value);
                                if(key == "delta_metric") ue_c_list.back().ue_c.delta_metric = std::stof(value);
                                if(key == "delay_t_metric") ue_c_list.back().ue_c.delay_t_metric = std::stof(value);
                                if(key == "beta_metric") ue_c_list.back().ue_c.beta_metric = std::stof(value);
                                // SCENARIO
                                if(key == "ue_height") ue_c_list.back().ue_c.ue_m.ue_h = std::stof(value);
                                
                                // LOGGING
                                if(key == "log_freq") ue_c_list.back().ue_c.log_freq = std::stoi(value);
                                if(key == "log_ue")
                                {
                                    if(value == "true" || value == "1") ue_c_list.back().ue_c.log_ue = true;
                                    if(value == "false" || value == "0") ue_c_list.back().ue_c.log_ue = false;
                                }
                                if(key == "log_traffic")
                                {
                                    if(value == "true" || value == "1") ue_c_list.back().ue_c.log_traffic = true;
                                    if(value == "false" || value == "0") ue_c_list.back().ue_c.log_traffic = false;
                                }
                                if(key == "log_mobility")
                                {
                                    if(value == "true" || value == "1") ue_c_list.back().ue_c.log_mobility = true;
                                    if(value == "false" || value == "0") ue_c_list.back().ue_c.log_mobility = false;
                                }
                                if(key == "log_quality")
                                {
                                    if(value == "true" || value == "1") ue_c_list.back().ue_c.log_quality = true;
                                    if(value == "false" || value == "0") ue_c_list.back().ue_c.log_quality = false;
                                }
                                LOG_INFO_I("configuration_loader::load") << "               Configuration - " << key << " = " << value << " added." << END();
                            }
                            else
                            {
                                if(mode == "Global")
                                {
                                    if(key == "duration") duration = std::stof(value);
                                    if(key == "period") period = std::stof(value);
                                    if(key == "multithreading")
                                    {
                                        if(value == "true" || value == "1") multithreading = true;
                                        if(value == "false" || value == "0") multithreading = false;
                                    }
                                    if(key == "random_v")
                                    {
                                        if(value == "true" || value == "1") random_v = true;
                                        if(value == "false" || value == "0") random_v = false;
                                    }
                                    if(key == "threads") threads = std::stoi(value);
                                    if(key == "verbose")
                                    {
                                        if(value == "true" || value == "1") verbose = true;
                                        if(value == "false" || value == "0") verbose = false;
                                    }
                                }
                                if(mode == "Scenario")
                                {
                                    // SCENARIO
                                    if(key == "scenario_type") scenario_type = std::stoi(value);
                                    if(key == "street_width") street_width = std::stof(value);
                                    if(key == "antenna_height") antenna_height = std::stof(value);
                                    if(key == "enb_height") enb_height = std::stof(value);
                                }
                                if(mode == "eNBConfig")
                                {
                                    // ENB CONFIG
                                    if(key == "modulation_m") modulation_m = std::stoi(value);
                                    if(key == "target_ber") target_ber = std::stof(value);
                                    if(key == "cqi_mode") cqi_mode = std::stoi(value);
                                    if(key == "tx_power") tx_power = std::stof(value);
                                    if(key == "tx_gain") tx_power = std::stof(value);
                                    if(key == "rx_gain") rx_gain = std::stoi(value);
                                    if(key == "bandwidth") bandwidth = std::stoi(value);
                                    if(key == "frequency") frequency = std::stof(value);
                                }
                                if(mode == "PDCP_RLC")
                                {
                                    // PDCP CONFIG
                                    if(key == "backhaul_d") backhaul_d = std::stof(value);
                                    if(key == "backhaul_d_var") backhaul_d_var =  random_v ? std::stof(value) : 0;
                                    if(key == "order_pkts")
                                    {
                                        if(value == "true" || value == "1") order_pkts = true;
                                        if(value == "false" || value == "0") order_pkts = false;
                                    }
                                }
                                if(mode == "MACLayer")
                                {
                                    // MAC LAYER CONFIG

                                    // METRIC CONFIG
                                    if(key == "metric_type") metric_type = std::stoi(value);
                                    
                                    // LOG DATA CONFIG
                                    if(key == "log_freq") log_freq = std::stoi(value);
                                    if(key == "log_mac")
                                    {
                                        if(value == "true" || value == "1") log_mac = true;
                                        if(value == "false" || value == "0") log_mac = false;
                                    }
                                    if(key == "mimo_layers") mimo_layers = std::stoi(value);
                                    if(key == "numerology") numerology = std::stoi(value);
                                    if(key == "n_ofdm_syms") n_ofdm_syms = std::stoi(value);
                                    if(key == "n_re_freq") n_re_freq = std::stoi(value);
                                    if(key == "max_rtx_ul") max_rtx_ul = std::stoi(value);
                                    if(key == "max_rtx_dl") max_rtx_dl = std::stoi(value);
                                    if(key == "mcs_tables")
                                    {
                                        if(value == "true" || value == "1") mcs_tables = true;
                                        if(value == "false" || value == "0") mcs_tables = false;
                                    }
                                    if(key == "scheduling_mode") scheduling_mode = std::stoi(value);
                                    if(key == "scheduling_type") scheduling_type = std::stoi(value);
                                    if(key == "scheduling_config") scheduling_config = std::stoi(value);
                                    if(key == "duplexing_type") duplexing_type = std::stoi(value);
                                    if(key == "n_dl_slots") n_dl_slots = std::stoi(value);
                                    if(key == "n_ul_slots") n_ul_slots = std::stoi(value);
                                    if(key == "transition_c") transition_c = std::stoi(value);
                                }
                                if(mode == "PHYLayer")
                                {
                                    // NOISE INTERFERENCE
                                    if(key == "interference_ues") interference_ues = std::stoi(value);
                                    if(key == "distance_interference") distance_interference = std::stof(value);
                                    if(key == "thermal_noise") thermal_noise = std::stof(value);
                                    if(key == "noise_figure") noise_figure = std::stoi(value);
                                    if(key == "air_delay_var_ul") air_delay_var_ul = random_v ? std::stof(value) : 0;
                                    if(key == "rtx_period_ul") rtx_period_ul = std::stof(value);
                                    if(key == "rtx_period_var_ul") rtx_period_var_ul =  random_v ? std::stof(value) : 0;
                                    if(key == "rtx_proc_delay_ul") rtx_proc_delay_ul = std::stof(value);
                                    if(key == "rtx_proc_delay_var_ul") rtx_proc_delay_var_ul =  random_v ? std::stof(value)  : 0;
                                    if(key == "air_delay_var_dl") air_delay_var_dl =  random_v ? std::stof(value) : 0;
                                    if(key == "rtx_period_dl") rtx_period_dl = std::stof(value);
                                    if(key == "rtx_period_var_dl") rtx_period_var_dl =  random_v ? std::stof(value) : 0;
                                    if(key == "rtx_proc_delay_dl") rtx_proc_delay_dl = std::stof(value);
                                    if(key == "rtx_proc_delay_var_dl") rtx_proc_delay_var_dl =  random_v ? std::stof(value) : 0;
                                }    
                                LOG_INFO_I("configuration_loader::load") << " Configuration - " << key << " = " << value << " added." << END();
                            }
                        }
                        else{
                            LOG_WARNING_I("configuration_loader::load") << " Wrong file format for key [" << key << "] with \':\' missing" << END();
                        }
                    }
                }
                else if(line.front() == '[')
                {
                    mode = line; 
                    mode.erase(0,1).pop_back(); 
                }                        
            }
        }
        else
        {
            LOG_WARNING_I("configuration_loader::load") << "File doesn't exit...falling back to defaults" << END();
        }
    }
public: 

    float get_period()
    {
        return period; 
    }
    
    float get_duration()
    {
        return duration; 
    }

    std::list<ue_full_config> get_ue_c_list()
    {
        return ue_c_list; 
    }
    
    phy_enb_config get_phy_enb_config()
    {
        return phy_enb_config(  tx_power, modulation_m, target_ber, 
                                cqi_mode, frequency, bandwidth, mimo_layers,
                                metric_type, interference_ues, distance_interference, 
                                thermal_noise, noise_figure, tx_gain, rx_gain);
    }

    pdcp_config get_pdcp_config_ul()
    {
        return pdcp_config(max_rtx_ul, air_delay_var_ul, rtx_period_ul, rtx_period_var_ul, rtx_proc_delay_ul, rtx_proc_delay_ul, backhaul_d, backhaul_d_var, order_pkts);
    }

    pdcp_config get_pdcp_config_dl()
    {
        return pdcp_config(max_rtx_dl, air_delay_var_dl, rtx_period_dl, rtx_period_var_dl, rtx_proc_delay_dl, rtx_proc_delay_dl, backhaul_d, backhaul_d_var, order_pkts);
    }

    enb_config get_enb_config()
    {
        return enb_config(get_phy_enb_config(),  get_pdcp_config_ul(), get_pdcp_config_dl());
    }
    
    scenario_config get_scenario_config()
    {
        return scenario_config(scenario_type, street_width, antenna_height, enb_height, mcs_tables);
    }

    int get_threads()
    {
        return threads; 
    }

    bool get_ue_threading()
    {
        return threads > 0; 
    }

    bool get_threading()
    {
        return multithreading;
    }

    mac_config get_mac_config()
    {
        return mac_config(mimo_layers, numerology, n_re_freq, n_ofdm_syms, bandwidth, scheduling_mode,
                            scheduling_type, scheduling_config, metric_type, 
                            duplexing_type);
    }

    tdd_config get_tdd_config()
    {
        return tdd_config(n_dl_slots, n_ul_slots, transition_c);
    }

    std::string get_unique_id()
    {
        if(unique_id == "")
        {
            time_t now = std::time(0);
            tm *ltm = std::localtime(&now);
            return std::to_string(1 + ltm->tm_mon) + "_" + std::to_string(ltm->tm_mday) + "_" + std::to_string(ltm->tm_hour) + "_"+ std::to_string(ltm->tm_min) + "_" + std::to_string(ltm->tm_sec);
        }
        else return unique_id; 
    }

    log_config get_log_config()
    { 
        int verbosity; 
        if(verbose && log_mac) verbosity = 2; 
        if(!verbose && log_mac) verbosity = 1; 
        if(!verbose && !log_mac) verbosity = 0; 
        return log_config(log_mac, log_freq, get_unique_id(), verbosity);
    }

    bool get_stochastics()
    {
        return random_v; 
    }

    int get_log_freq(){ return log_freq;}
    bool get_log_mac(){ return log_mac;}

    float get_freq(){return frequency; }

private: 
    float duration = DURATION_DEFAULT; 
    float period = PERIOD_DEFAULT; 
    // UE CONFIG
    std::list<ue_full_config> ue_c_list; 
    // ENB CONFIG
    float tx_power = ENB_TX_POWER_DEFAULT; 
    int modulation_m = MODULATION_DEFAULT;
    float target_ber = TARGET_BER_DEFAULT;
    int cqi_mode = CQI_MODE_DEFAULT;
    int interference_ues = INTERF_UES_DEFAULT; 
    float distance_interference = INTERF_DISTANCE_DEFAULT; 
    float thermal_noise = THERMAL_N_DEFAULT; 
    float noise_figure = NOISE_FIGURE_DEFAULT; 
    float tx_gain = TX_GAIN_DEFAULT; 
    float rx_gain = RX_GAIN_DEFAULT; 
    //PDCP_CONFIG
    float backhaul_d = BACKHAUL_D_DEFAULT; 
    float backhaul_d_var = BACKHAUL_D_VAR_DEFAULT;
    bool order_pkts = DEFAULT_ORDER_PKTS; 
    // PDCP UL CONFIG
    int max_rtx_ul = MAX_RTX_DEFAULT;
    float air_delay_var_ul = AIR_DELAY_VAR_DEFAULT;
    float rtx_period_ul = RTX_PERIOD_DEFAULT;
    float rtx_period_var_ul = RTX_PERIOD_VAR_DEFAULT;
    float rtx_proc_delay_ul = RTX_PROC_DELAY_DEFAULT;
    float rtx_proc_delay_var_ul = RTX_PROC_DELAY_VAR_DEFAULT;
    // PDCP DL CONFIG
    int max_rtx_dl = MAX_RTX_DEFAULT;
    float air_delay_var_dl = AIR_DELAY_VAR_DEFAULT;
    float rtx_period_dl = RTX_PERIOD_DEFAULT;
    float rtx_period_var_dl = RTX_PERIOD_VAR_DEFAULT;
    float rtx_proc_delay_dl = RTX_PROC_DELAY_DEFAULT;
    float rtx_proc_delay_var_dl = RTX_PROC_DELAY_VAR_DEFAULT;
    // METRIC COFIGURATION
    int metric_type = DEFAULT_METRIC;
    // LOG DATA
    int log_freq = -1;
    bool log_mac = false;
    bool verbose = false;
    bool random_v = false; 
    // SCENARIO
    int scenario_type = SCENARIO_TYPE_DEFAULT; 
    float street_width = STREET_WIDTH_DEFAULT; 
    float antenna_height = ANTENNA_HEIGHT_DEFAULT; 
    float enb_height = ENB_HEIGHT_DEFAULT; 
    bool mcs_tables = MCS_TABLES_DEFAULT; 
    // MAC LAYER
    int mimo_layers = MIMO_LAYERS_DEFAULT;
    int ofdm_symbols = OFDM_SYMBOLS_DEFAULT;  
    int numerology = NUMEROLOGY_DEFAULT; 
    int n_ofdm_syms = N_OFDM_DEFAULT; 
    int n_re_freq = N_RE_DEFAULT; 
    int bandwidth = BANDWIDTH_DEFAULT; 
    float frequency = FREQUENCY_DEFAULT; 
    int scheduling_mode = SCHEDULING_MODE_DEFAULT; 
    int scheduling_type = SCHEDULING_TYPE_DEFAULT; 
    int scheduling_config = SCHEDULING_CONFIG_DEFAULT; 
    int duplexing_type = DUPLEXING_TYPE_DEFAULT;
    int n_dl_slots = N_DL_SLOTS_DEFAULT; 
    int n_ul_slots = N_UL_SLOTS_DEFAULT;
    int transition_c = N_TRANSITION_C_DEFAULT; 
    std::string unique_id = "";
    // GENERAL 
    int threads = 0; 
    bool multithreading = true; 
}; 


#endif