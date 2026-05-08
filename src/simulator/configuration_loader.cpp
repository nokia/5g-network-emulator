#include <simulator/configuration_loader.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <libgen.h>
#include <sstream>
#include <unistd.h>

#include <utils/terminal_logging.h>

std::string getBaseMapPath()
{
    char exe_path[1024];
    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (len != -1)
    {
        exe_path[len] = '\0';
        std::string path_str(dirname(exe_path));
        return path_str + "/../include/maps_scenarios/macroscopic_fading_map_";
    }

    return "include/maps_scenarios/macroscopic_fading_map_";
}

ue_full_config::ue_full_config()
{
    // UE CONFIG
    ue_c.ue_m.n_antennas = N_ANTENNAS_DEFAULT;
    ue_c.ue_m.alpha_ul = ALPHA_UL_DEFAULT;
    ue_c.ue_m.nominal_pusch_p0 = NOMINAL_PUSCH_P0_DEFAULT;
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
    ue_c.pkt_delay_budget = PKT_DELAY_BUDGET_DEFAULT;

    // PRIORITY
    ue_c.priority = DEFAULT_UE_PRIORITY;
    ue_c.ue_m.ue_h = UE_HEIGHT_DEFAULT;
    ue_c.ue_m.o2i = O2I_DEFAULT;
}

enb_config::enb_config(phy_enb_config _phy_config, pdcp_config _pdcp_ul, pdcp_config _pdcp_dl)
{
    phy_config = _phy_config;
    pdcp_ul = _pdcp_ul;
    pdcp_dl = _pdcp_dl;
}

configuration_loader::configuration_loader()
{
}

configuration_loader::configuration_loader(std::string cfg_file)
{
    load(cfg_file);
}

void configuration_loader::load(std::string cfg_file)
{
    std::ifstream infile(cfg_file);
    if (infile.good())
    {
        std::string line;
        std::string mode = "None";
        while (std::getline(infile, line))
        {
            if (std::isalpha(line.front()))
            {
                std::istringstream iss(line);
                std::string key, value;
                if (iss >> key >> value)
                {
                    if (key.back() == ':')
                    {
                        key.pop_back();
                        if (mode == "UE")
                        {
                            if (key == "random_v")
                            {
                                if (value == "true" || value == "1")
                                    random_v = true;
                                if (value == "false" || value == "0")
                                    random_v = false;
                            }
                            if (key == "ue_id")
                            {
                                LOG_INFO_I("configuration_loader::load") << "Adding UEs with config [" << value << "]: " << END();
                                ue_full_config ue_c;
                                ue_c.id = value;
                                ue_c.ue_c.log_id = get_unique_id();
                                ue_c_list.push_back(ue_c);
                                ue_c_list.back().ue_c.mobility_c.random_v = random_v;
                                ue_c_list.back().ue_c.traffic_c.random_v = random_v;
                            }
                            if (key == "n_ues")
                                ue_c_list.back().n_ues = std::stoi(value);
                            if (key == "ue_type")
                                ue_c_list.back().ue_type = std::stoi(value);
                            // QUEUE_NUM
                            if (key == "ul_queue_n")
                                ue_c_list.back().ue_c.ul_queue_n = std::stoi(value);
                            if (key == "dl_queue_n")
                                ue_c_list.back().ue_c.dl_queue_n = std::stoi(value);
                            // UE CONFIG
                            if (key == "n_antennas")
                                ue_c_list.back().ue_c.ue_m.n_antennas = std::stoi(value);
                            if (key == "alpha_ul")
                                ue_c_list.back().ue_c.ue_m.alpha_ul = std::stof(value);
                            if (key == "nominal_pusch_p0")
                                ue_c_list.back().ue_c.ue_m.nominal_pusch_p0 = std::stof(value);
                            if (key == "tx_power_ul")
                                ue_c_list.back().ue_c.ue_m.tx_power_ul = std::stof(value);
                            if (key == "set_ul_pow")
                            {
                                if (value == "true" || value == "1")
                                    ue_c_list.back().ue_c.ue_m.set_ul_pow = true;
                                if (value == "false" || value == "0")
                                    ue_c_list.back().ue_c.ue_m.set_ul_pow = false;
                            }
                            if (key == "cqi_period")
                                ue_c_list.back().ue_c.ue_m.cqi_period = std::stoi(value);
                            if (key == "ri_period")
                                ue_c_list.back().ue_c.ue_m.ri_period = std::stoi(value);
                            if (key == "scaling_factor")
                                ue_c_list.back().ue_c.ue_m.scaling_factor = std::stof(value);
                            // PRIORITY
                            if (key == "priority")
                                ue_c_list.back().ue_c.priority = std::stof(value);
                            // TRAFFIC MODEL CONFIG
                            if (key == "ul_target")
                                ue_c_list.back().ue_c.traffic_c.ul_target = std::stof(value);
                            if (key == "dl_target")
                                ue_c_list.back().ue_c.traffic_c.dl_target = std::stof(value);
                            if (key == "ul_traffic_file")
                                ue_c_list.back().ue_c.traffic_c.ul_traffic_file = value;
                            if (key == "dl_traffic_file")
                                ue_c_list.back().ue_c.traffic_c.dl_traffic_file = value;
                            if (key == "delay")
                                ue_c_list.back().ue_c.traffic_c.delay = std::stof(value);
                            if (key == "var_perc")
                                ue_c_list.back().ue_c.traffic_c.var_perc = random_v ? std::stof(value) : 0;
                            if (key == "pkt_size")
                                ue_c_list.back().ue_c.traffic_c.pkt_size = std::stoi(value);
                            if (key == "traffic_type")
                                ue_c_list.back().ue_c.traffic_c.type = std::stoi(value);
                            // MOBILITY MODEL CONFIG
                            if (key == "mobility_type")
                                ue_c_list.back().ue_c.mobility_c.type = std::stof(value);
                            if (key == "pos_x")
                                ue_c_list.back().ue_c.mobility_c.init_pos._x(std::stof(value));
                            if (key == "pos_y")
                                ue_c_list.back().ue_c.mobility_c.init_pos._y(std::stof(value));
                            if (key == "random_init")
                            {
                                if (value == "true" || value == "1")
                                    ue_c_list.back().ue_c.mobility_c.random_init = true;
                                if (value == "false" || value == "0")
                                    ue_c_list.back().ue_c.mobility_c.random_init = false;
                            }
                            if (key == "speed")
                                ue_c_list.back().ue_c.mobility_c.speed = TOMS * std::stof(value);
                            if (key == "speed_var")
                                ue_c_list.back().ue_c.mobility_c.speed_var = TOMS * std::stof(value);
                            if (key == "max_distance")
                                ue_c_list.back().ue_c.mobility_c.max_distance = std::stof(value);
                            if (key == "time_target_var")
                                ue_c_list.back().ue_c.mobility_c.time_target_var = std::stof(value);
                            if (key == "time_target")
                                ue_c_list.back().ue_c.mobility_c.time_target = std::stof(value);
                            if (key == "delta_metric")
                                ue_c_list.back().ue_c.delta_metric = std::stof(value);
                            if (key == "delay_t_metric")
                                ue_c_list.back().ue_c.delay_t_metric = std::stof(value);
                            if (key == "beta_metric")
                                ue_c_list.back().ue_c.beta_metric = std::stof(value);
                            if (key == "pkt_delay_budget")
                                ue_c_list.back().ue_c.pkt_delay_budget = std::stof(value);
                            // SCENARIO
                            if (key == "ue_height")
                                ue_c_list.back().ue_c.ue_m.ue_h = std::stof(value);
                            if (key == "o2i")
                                ue_c_list.back().ue_c.ue_m.o2i = std::stoi(value);
                            // LOGGING
                            if (key == "log_freq")
                                ue_c_list.back().ue_c.log_freq = std::stoi(value);
                            if (key == "log_ue")
                            {
                                if (value == "true" || value == "1")
                                    ue_c_list.back().ue_c.log_ue = true;
                                if (value == "false" || value == "0")
                                    ue_c_list.back().ue_c.log_ue = false;
                            }
                            if (key == "log_traffic")
                            {
                                if (value == "true" || value == "1")
                                    ue_c_list.back().ue_c.log_traffic = true;
                                if (value == "false" || value == "0")
                                    ue_c_list.back().ue_c.log_traffic = false;
                            }
                            if (key == "log_mobility")
                            {
                                if (value == "true" || value == "1")
                                    ue_c_list.back().ue_c.log_mobility = true;
                                if (value == "false" || value == "0")
                                    ue_c_list.back().ue_c.log_mobility = false;
                            }
                            if (key == "log_quality")
                            {
                                if (value == "true" || value == "1")
                                    ue_c_list.back().ue_c.log_quality = true;
                                if (value == "false" || value == "0")
                                    ue_c_list.back().ue_c.log_quality = false;
                            }
                            LOG_INFO_I("configuration_loader::load") << "               Configuration - " << key << " = " << value << " added." << END();
                        }
                        else
                        {
                            if (mode == "Global")
                            {
                                if (key == "duration")
                                    duration = std::stof(value);
                                if (key == "period")
                                    period = std::stof(value);
                                if (key == "multithreading")
                                {
                                    if (value == "true" || value == "1")
                                        multithreading = true;
                                    if (value == "false" || value == "0")
                                        multithreading = false;
                                }

                                if (key == "threads")
                                    threads = std::stoi(value);
                                if (key == "verbose")
                                {
                                    if (value == "true" || value == "1")
                                        verbose = true;
                                    if (value == "false" || value == "0")
                                        verbose = false;
                                }
                            }
                            if (mode == "Scenario")
                            {
                                // SCENARIO
                                if (key == "scenario_type")
                                {
                                    scenario_type = std::stoi(value);
                                }
                            }
                            if (mode == "eNBConfig")
                            {
                                // ENB CONFIG
                                if (key == "modulation_m")
                                    modulation_m = std::stoi(value);
                                if (key == "target_ber")
                                    target_ber = std::stof(value);
                                if (key == "cqi_mode")
                                    cqi_mode = std::stoi(value);
                                if (key == "tx_power")
                                    tx_power = std::stof(value);
                                if (key == "eNB_gain")
                                    eNB_gain = std::stof(value);
                                if (key == "UT_gain")
                                    UT_gain = std::stoi(value);
                                if (key == "power_boost")
                                    power_boost = std::stoi(value);
                                if (key == "bandwidth")
                                    bandwidth = std::stoi(value);
                                if (key == "frequency")
                                {
                                    frequency = std::stod(value);
                                    std::cout << "frequency " << frequency << std::endl;
                                    std::cout << "scenario_type " << scenario_type << std::endl;
                                }

                                map_file = getClosestMapFile(scenario_type, frequency);
                                if (map_file.empty())
                                {
                                    std::cerr << "Failed to find a valid map file for scenario_type: " << scenario_type << " and frequency: " << frequency << std::endl;
                                }
                            }
                            if (mode == "PDCP_RLC")
                            {
                                // PDCP CONFIG
                                if (key == "backhaul_d")
                                    backhaul_d = std::stof(value);
                                if (key == "backhaul_d_var")
                                    backhaul_d_var = random_v ? std::stof(value) : 0;
                                if (key == "order_pkts")
                                {
                                    if (value == "true" || value == "1")
                                        order_pkts = true;
                                    if (value == "false" || value == "0")
                                        order_pkts = false;
                                }
                            }
                            if (mode == "MACLayer")
                            {
                                // MAC LAYER CONFIG

                                // METRIC CONFIG
                                if (key == "metric_type")
                                    metric_type = std::stoi(value);

                                // LOG DATA CONFIG
                                if (key == "log_freq")
                                    log_freq = std::stoi(value);
                                if (key == "log_mac")
                                {
                                    if (value == "true" || value == "1")
                                        log_mac = true;
                                    if (value == "false" || value == "0")
                                        log_mac = false;
                                }
                                if (key == "mimo_layers")
                                    mimo_layers = std::stoi(value);
                                if (key == "numerology")
                                    numerology = std::stoi(value);
                                if (key == "n_ofdm_syms")
                                    n_ofdm_syms = std::stoi(value);
                                if (key == "n_re_freq")
                                    n_re_freq = std::stoi(value);
                                if (key == "max_rtx_ul")
                                    max_rtx_ul = std::stoi(value);
                                if (key == "max_rtx_dl")
                                    max_rtx_dl = std::stoi(value);
                                if (key == "mcs_tables")
                                {
                                    if (value == "true" || value == "1")
                                        mcs_tables = true;
                                    if (value == "false" || value == "0")
                                        mcs_tables = false;
                                }
                                if (key == "scheduling_mode")
                                    scheduling_mode = std::stoi(value);
                                if (key == "scheduling_type")
                                    scheduling_type = std::stoi(value);
                                if (key == "scheduling_config")
                                    scheduling_config = std::stoi(value);
                                if (key == "duplexing_type")
                                    duplexing_type = std::stoi(value);
                                if (key == "n_dl_slots")
                                    n_dl_slots = std::stoi(value);
                                if (key == "n_ul_slots")
                                    n_ul_slots = std::stoi(value);
                                if (key == "transition_c")
                                    transition_c = std::stoi(value);
                                if (key == "ratio_DL_UL")
                                    ratio_DL_UL = std::stof(value);
                            }
                            if (mode == "PHYLayer")
                            {
                                // NOISE INTERFERENCE
                                if (key == "interference_ues")
                                    interference_ues = std::stoi(value);
                                if (key == "interference_eNBs")
                                    interference_eNBs = std::stoi(value);
                                if (key == "distance_interference")
                                    distance_interference = std::stof(value);
                                if (key == "interfered_bandwidth_ratio")
                                    interfered_bandwidth_ratio = std::stof(value);
                                if (key == "thermal_noise")
                                    thermal_noise = std::stof(value);
                                if (key == "enb_noise_figure")
                                    enb_noise_figure = std::stoi(value);
                                if (key == "ut_noise_figure")
                                    ut_noise_figure = std::stoi(value);
                                if (key == "air_delay_var_ul")
                                    air_delay_var_ul = random_v ? std::stof(value) : 0;
                                if (key == "rtx_period_ul")
                                    rtx_period_ul = std::stof(value);
                                if (key == "rtx_period_var_ul")
                                    rtx_period_var_ul = random_v ? std::stof(value) : 0;
                                if (key == "rtx_proc_delay_ul")
                                    rtx_proc_delay_ul = std::stof(value);
                                if (key == "rtx_proc_delay_var_ul")
                                    rtx_proc_delay_var_ul = random_v ? std::stof(value) : 0;
                                if (key == "air_delay_var_dl")
                                    air_delay_var_dl = random_v ? std::stof(value) : 0;
                                if (key == "rtx_period_dl")
                                    rtx_period_dl = std::stof(value);
                                if (key == "rtx_period_var_dl")
                                    rtx_period_var_dl = random_v ? std::stof(value) : 0;
                                if (key == "rtx_proc_delay_dl")
                                    rtx_proc_delay_dl = std::stof(value);
                                if (key == "rtx_proc_delay_var_dl")
                                    rtx_proc_delay_var_dl = random_v ? std::stof(value) : 0;
                            }
                            LOG_INFO_I("configuration_loader::load") << " Configuration - " << key << " = " << value << " added." << END();
                        }
                    }
                    else
                    {
                        LOG_WARNING_I("configuration_loader::load") << " Wrong file format for key [" << key << "] with \':\' missing" << END();
                    }
                }
            }
            else if (line.front() == '[')
            {
                mode = line;
                mode.erase(0, 1).pop_back();
            }
        }
    }
    else
    {
        LOG_WARNING_I("configuration_loader::load") << "File doesn't exit...falling back to defaults" << END();
    }
}

float configuration_loader::get_period()
{
    return period;
}

float configuration_loader::get_duration()
{
    return duration;
}

std::list<ue_full_config> configuration_loader::get_ue_c_list()
{
    return ue_c_list;
}

phy_enb_config configuration_loader::get_phy_enb_config()
{
    return phy_enb_config(tx_power, modulation_m, target_ber,
                          cqi_mode, frequency, bandwidth, mimo_layers,
                          metric_type, interference_ues, interference_eNBs, distance_interference, interfered_bandwidth_ratio,
                          thermal_noise, enb_noise_figure, ut_noise_figure, eNB_gain, UT_gain, power_boost, numerology);
}

pdcp_config configuration_loader::get_pdcp_config_ul()
{
    return pdcp_config(max_rtx_ul, air_delay_var_ul, rtx_period_ul, rtx_period_var_ul, rtx_proc_delay_ul, rtx_proc_delay_ul, backhaul_d, backhaul_d_var, order_pkts);
}

pdcp_config configuration_loader::get_pdcp_config_dl()
{
    return pdcp_config(max_rtx_dl, air_delay_var_dl, rtx_period_dl, rtx_period_var_dl, rtx_proc_delay_dl, rtx_proc_delay_dl, backhaul_d, backhaul_d_var, order_pkts);
}

enb_config configuration_loader::get_enb_config()
{
    return enb_config(get_phy_enb_config(), get_pdcp_config_ul(), get_pdcp_config_dl());
}

scenario_config configuration_loader::get_scenario_config()
{
    return scenario_config(scenario_type, street_width, antenna_height, enb_height, map_file, mcs_tables);
}

int configuration_loader::get_threads()
{
    return threads;
}

bool configuration_loader::get_ue_threading()
{
    return threads > 0;
}

bool configuration_loader::get_threading()
{
    return multithreading;
}

mac_config configuration_loader::get_mac_config()
{
    return mac_config(mimo_layers, numerology, n_re_freq, n_ofdm_syms, bandwidth, scheduling_mode,
                      scheduling_type, scheduling_config, metric_type,
                      duplexing_type, ratio_DL_UL);
}

tdd_config configuration_loader::get_tdd_config()
{
    return tdd_config(n_dl_slots, n_ul_slots, transition_c);
}

std::string configuration_loader::get_unique_id()
{
    if (unique_id == "")
    {
        time_t now = std::time(0);
        tm *ltm = std::localtime(&now);
        return std::to_string(1 + ltm->tm_mon) + "_" + std::to_string(ltm->tm_mday) + "_" + std::to_string(ltm->tm_hour) + "_" + std::to_string(ltm->tm_min) + "_" + std::to_string(ltm->tm_sec);
    }

    return unique_id;
}

log_config configuration_loader::get_log_config()
{
    int verbosity = 0;
    if (verbose && log_mac)
        verbosity = 2;
    if (!verbose && log_mac)
        verbosity = 1;
    if (!verbose && !log_mac)
        verbosity = 0;
    return log_config(log_mac, log_freq, get_unique_id(), verbosity);
}

bool configuration_loader::get_stochastics()
{
    return random_v;
}

int configuration_loader::get_log_freq()
{
    return log_freq;
}

bool configuration_loader::get_log_mac()
{
    return log_mac;
}

float configuration_loader::get_freq()
{
    return frequency;
}

std::string configuration_loader::getClosestMapFile(int scenario_type, double frequency)
{
    double freqGHz = frequency / 1e9;

    auto scenarioIt = SCENARIO_TYPE_MAP.find(scenario_type);
    if (scenarioIt == SCENARIO_TYPE_MAP.end())
    {
        std::cerr << "Unknown scenario_type: " << scenario_type << std::endl;
        return "";
    }

    const std::string &scenarioName = scenarioIt->second;

    auto freqIt = AVAILABLE_FREQUENCIES.find(scenarioName);
    if (freqIt == AVAILABLE_FREQUENCIES.end())
    {
        std::cerr << "No frequencies available for scenario: " << scenarioName << std::endl;
        return "";
    }

    const std::vector<double> &scenarioFrequencies = freqIt->second;

    auto closestFreq = *std::min_element(
        scenarioFrequencies.begin(), scenarioFrequencies.end(),
        [freqGHz](double a, double b)
        {
            return std::abs(a - freqGHz) < std::abs(b - freqGHz);
        });

    std::ostringstream freqStream;
    freqStream << std::fixed << std::setprecision(2) << closestFreq;
    std::string freqStr = freqStream.str();

    if (freqStr.find('.') != std::string::npos)
    {
        freqStr.erase(freqStr.find_last_not_of('0') + 1, std::string::npos);
        if (freqStr.back() == '.')
        {
            freqStr.pop_back();
        }
    }

    std::string mapFilePath = getBaseMapPath() + scenarioName + "_" + freqStr + ".json";

    std::cout << "Generated map file path: " << mapFilePath << std::endl;

    return mapFilePath;
}
