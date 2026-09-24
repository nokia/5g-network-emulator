#pragma once

#include <algorithm>
#include <cmath>

struct ul_power_allocation
{
    float total_dbm = 0.0f;
    float per_prb_dbm = 0.0f;
};

inline int normalized_prb_count(int allocated_prbs)
{
    return std::max(1, allocated_prbs);
}

inline float power_per_prb_dbm(float total_power_dbm, int allocated_prbs)
{
    return total_power_dbm
           - 10.0f * std::log10(static_cast<float>(normalized_prb_count(allocated_prbs)));
}

inline float downlink_power_per_prb_dbm(float total_power_dbm, int carrier_prbs)
{
    return power_per_prb_dbm(total_power_dbm, carrier_prbs);
}

inline float thermal_noise_per_prb_dbm(
    float noise_density_dbm_hz,
    float noise_figure_db,
    int numerology)
{
    const float prb_bandwidth_hz = 12.0f * 15000.0f * std::pow(2.0f, numerology);
    return noise_density_dbm_hz + noise_figure_db
           + 10.0f * std::log10(prb_bandwidth_hz);
}

inline ul_power_allocation calculate_ul_power_allocation(
    bool use_fixed_total_power,
    float fixed_total_power_dbm,
    float nominal_power_per_prb_dbm,
    int allocated_prbs,
    float minimum_total_power_dbm = 10.0f,
    float maximum_total_power_dbm = 23.0f)
{
    const int prbs = normalized_prb_count(allocated_prbs);
    const float requested_total_dbm =
        use_fixed_total_power
            ? fixed_total_power_dbm
            : nominal_power_per_prb_dbm + 10.0f * std::log10(static_cast<float>(prbs));
    const float total_dbm =
        std::max(minimum_total_power_dbm, std::min(requested_total_dbm, maximum_total_power_dbm));
    return {total_dbm, power_per_prb_dbm(total_dbm, prbs)};
}
