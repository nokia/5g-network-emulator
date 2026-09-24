#include <cassert>
#include <cmath>

#include <mac_layer/metric_handler.h>

namespace
{
bool near(float actual, float expected, float tolerance = 1e-5f)
{
    return std::fabs(actual - expected) <= tolerance;
}
} // namespace

int main()
{
    metric_info info;
    info.current_tp = 20.0f;
    info.avrg_tp = 100.0f;

    metric_handler proportional_fair(
        METRIC_PF, 0.5f, 0.1f, 0.001f, 1.0f);
    assert(proportional_fair.is_pf());
    assert(near(proportional_fair.get_metric(info, 0.0f, 0), 0.2f));

    metric_handler throughput_fair(
        METRIC_PF, 0.5f, 0.1f, 0.001f, 0.0f);
    assert(near(throughput_fair.get_metric(info, 0.0f, 0), 0.01f));

    metric_handler bet(
        METRIC_BET, 0.5f, 0.1f, 0.001f, 1.0f);
    assert(!bet.is_pf());
    assert(near(
        bet.get_metric(info, 0.0f, 0),
        1.0f / (0.5f * 100.0f + 0.5f * 20.0f)));

    return 0;
}
