#include <cassert>
#include <cmath>
#include <fstream>
#include <string>
#include <vector>

#include <common/direction.h>
#include <simulator/simulator.h>

namespace
{
void write_config(const std::string &path, int cqi_period)
{
    std::ofstream out(path);
    out << "[Global]\n"
        << "duration: 1.0\n"
        << "period: -1\n"
        << "multithreading: false\n"
        << "threads: 0\n"
        << "verbose: false\n"
        << "[UE]\n"
        << "ue_id: pfEqual\n"
        << "ue_type: 1\n"
        << "n_ues: 16\n"
        << "n_antennas: 1\n"
        << "set_ul_pow: true\n"
        << "tx_power_ul: 23\n"
        << "cqi_period: " << cqi_period << "\n"
        << "ri_period: 5\n"
        << "random_v: false\n"
        << "traffic_type: 0\n"
        << "ul_target: 1000\n"
        << "dl_target: 1000\n"
        << "var_perc: 0\n"
        << "pkt_size: 12000\n"
        << "mobility_type: 0\n"
        << "pos_x: 200\n"
        << "pos_y: 0\n"
        << "random_init: false\n"
        << "speed: 0\n"
        << "max_distance: 1000\n"
        << "priority: 1\n"
        << "pkt_delay_budget: 10\n"
        << "ue_height: 1.5\n"
        << "ue_location_type: outdoor\n"
        << "[Scenario]\n"
        << "scenario_type: 1\n"
        << "[eNBConfig]\n"
        << "modulation_m: 1\n"
        << "target_ber: 0.00005\n"
        << "cqi_mode: 1\n"
        << "tx_power: 46\n"
        << "eNB_gain: 8.7\n"
        << "UT_gain: 0\n"
        << "frequency: 3500000000\n"
        << "bandwidth: 20000000\n"
        << "[MACLayer]\n"
        << "metric_type: 6\n"
        << "pf_alpha: 1\n"
        << "pf_time_window_ms: 100\n"
        << "mimo_layers: 1\n"
        << "n_ofdm_syms: 14\n"
        << "n_re_freq: 12\n"
        << "numerology: 1\n"
        << "mcs_tables: true\n"
        << "scheduling_mode: 1\n"
        << "scheduling_type: 0\n"
        << "scheduling_config: 1\n"
        << "duplexing_type: 0\n"
        << "n_dl_slots: 7\n"
        << "n_ul_slots: 3\n"
        << "transition_c: 54\n"
        << "[PHYLayer]\n"
        << "interference_ues: 0\n"
        << "interference_eNBs: 0\n"
        << "interfered_bandwidth_ratio: 0\n"
        << "thermal_noise: -174\n"
        << "enb_noise_figure: 2\n"
        << "ut_noise_figure: 9\n";
}

struct scheduler_result
{
    std::vector<float> throughputs;
    std::vector<float> pf_averages;
};

scheduler_result run_scheduler(
    const std::string &path,
    int cqi_period)
{
    write_config(path, cqi_period);
    simulator sim(path);
    sim.run_steps(1000);

    scheduler_result result;
    for (ue &terminal : *sim.ue_list())
    {
        result.throughputs.push_back(terminal.get_avg_tp(TX_DL));
        result.pf_averages.push_back(
            terminal.get_pf_average_throughput_bits_per_tti(TX_DL));
        assert(terminal.get_pkt_delay_budget() == 10.0f);
    }
    return result;
}

float jain_index(const std::vector<float> &values)
{
    float sum = 0.0f;
    float squared_sum = 0.0f;
    for (float value : values)
    {
        sum += value;
        squared_sum += value * value;
    }
    return sum * sum
           / (static_cast<float>(values.size()) * squared_sum);
}
} // namespace

int main()
{
    const scheduler_result fast_cqi = run_scheduler(
        "build/tests/pf_scheduler_cqi_1.ini", 1);
    const scheduler_result slow_cqi = run_scheduler(
        "build/tests/pf_scheduler_cqi_20.ini", 20);

    assert(fast_cqi.throughputs.size() == 16);
    assert(slow_cqi.throughputs.size() == fast_cqi.throughputs.size());
    for (size_t index = 0; index < fast_cqi.throughputs.size(); index++)
    {
        assert(fast_cqi.throughputs[index] > 0.0f);
        assert(slow_cqi.throughputs[index] > 0.0f);
        assert(
            std::fabs(
                fast_cqi.pf_averages[index]
                - slow_cqi.pf_averages[index])
            < 1e-3f);
    }
    assert(jain_index(fast_cqi.throughputs) > 0.98f);
    assert(jain_index(slow_cqi.throughputs) > 0.98f);
    return 0;
}
