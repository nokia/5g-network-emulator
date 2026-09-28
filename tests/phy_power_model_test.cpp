#include <cassert>
#include <cmath>
#include <initializer_list>
#include <random>

#include <phy_layer/power_model.h>

namespace
{
bool near(float actual, float expected, float tolerance = 1e-3f)
{
    return std::fabs(actual - expected) <= tolerance;
}
} // namespace

int main()
{
    assert(near(downlink_power_per_prb_dbm(46.0f, 100), 26.0f));
    assert(near(
        thermal_noise_per_prb_dbm(-174.0f, 9.0f, 1),
        -109.436f,
        1e-2f));

    const ul_power_allocation one_prb = calculate_ul_power_allocation(
        false, 23.0f, 12.0f, 1);
    const ul_power_allocation four_prbs = calculate_ul_power_allocation(
        false, 23.0f, 12.0f, 4);
    const ul_power_allocation sixteen_prbs = calculate_ul_power_allocation(
        false, 23.0f, 12.0f, 16);

    assert(near(one_prb.total_dbm, 12.0f));
    assert(near(one_prb.per_prb_dbm, 12.0f));
    assert(near(four_prbs.per_prb_dbm, 12.0f));
    assert(near(sixteen_prbs.total_dbm, 23.0f));
    assert(near(sixteen_prbs.per_prb_dbm, 10.959f, 1e-2f));

    const ul_power_allocation fixed_four = calculate_ul_power_allocation(
        true, 23.0f, 0.0f, 4);
    assert(near(fixed_four.total_dbm, 23.0f));
    assert(near(fixed_four.per_prb_dbm, 16.979f, 1e-2f));

    const int carrier_prbs = 256;
    const float signal_per_prb =
        downlink_power_per_prb_dbm(46.0f, carrier_prbs);
    const float noise_per_prb =
        thermal_noise_per_prb_dbm(-174.0f, 9.0f, 1);
    const float reference_sinr = signal_per_prb - noise_per_prb;
    for (int allocation_prbs : {1, 2, 4, 8, 16})
    {
        const float allocation_gain =
            10.0f * std::log10(static_cast<float>(allocation_prbs));
        const float integrated_signal =
            signal_per_prb + allocation_gain;
        const float integrated_noise =
            noise_per_prb + allocation_gain;
        assert(near(
            integrated_signal - integrated_noise,
            reference_sinr));
    }

    for (int allocation_prbs = 1; allocation_prbs <= 275; ++allocation_prbs)
    {
        const ul_power_allocation allocation =
            calculate_ul_power_allocation(
                true, 23.0f, 0.0f, allocation_prbs);
        const float reconstructed_total =
            allocation.per_prb_dbm
            + 10.0f
                  * std::log10(static_cast<float>(allocation_prbs));
        assert(near(reconstructed_total, allocation.total_dbm, 1e-2f));
        assert(allocation.total_dbm <= 23.0f + 1e-3f);
    }

    assert(near(rayleigh_power_gain_db(1.0f, 1.0f), 0.0f));
    assert(near(
        rayleigh_power_gain_db(std::sqrt(20.0f), 0.0f),
        10.0f));
    std::mt19937 generator(42);
    std::normal_distribution<float> normal(0.0f, 1.0f);
    double mean_linear_power = 0.0;
    constexpr int samples = 100000;
    for (int sample = 0; sample < samples; sample++)
    {
        const float gain_db =
            rayleigh_power_gain_db(normal(generator), normal(generator));
        mean_linear_power += std::pow(10.0, gain_db / 10.0);
    }
    mean_linear_power /= samples;
    assert(std::fabs(mean_linear_power - 1.0) < 0.01);

    return 0;
}
