#include <cassert>
#include <cmath>

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

    return 0;
}
