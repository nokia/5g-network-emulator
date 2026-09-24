#include <cassert>
#include <fstream>
#include <stdexcept>
#include <string>

#include <simulator/configuration_loader.h>

namespace
{
void write_config(const std::string &path, bool legacy_beta)
{
    std::ofstream out(path);
    out << "[Global]\n"
        << "duration: 0.001\n"
        << "period: -1\n"
        << "[UE]\n"
        << "ue_id: migration\n"
        << "ue_type: 1\n"
        << "n_ues: 1\n";
    if (legacy_beta)
        out << "beta_metric: 0.5\n";
    out << "[Scenario]\n"
        << "scenario_type: 1\n"
        << "[eNBConfig]\n"
        << "frequency: 3500000000\n"
        << "bandwidth: 20000000\n"
        << "[MACLayer]\n"
        << "metric_type: 6\n"
        << "pf_alpha: 1.0\n"
        << "pf_time_window_ms: 100\n";
}
} // namespace

int main()
{
    const std::string legacy_path =
        "build/tests/pf_config_legacy.ini";
    const std::string current_path =
        "build/tests/pf_config_current.ini";
    write_config(legacy_path, true);
    write_config(current_path, false);

    bool rejected = false;
    try
    {
        configuration_loader legacy(legacy_path);
    }
    catch (const std::invalid_argument &)
    {
        rejected = true;
    }
    assert(rejected);

    configuration_loader current(current_path);
    const mac_config mac = current.get_mac_config();
    assert(mac.metric_type == METRIC_PF);
    assert(mac.pf_alpha == 1.0f);
    assert(mac.pf_time_window_ms == 100.0f);
    return 0;
}
