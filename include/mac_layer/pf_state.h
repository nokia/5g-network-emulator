#pragma once

#include <algorithm>
#include <cmath>

class pf_throughput_state
{
public:
    explicit pf_throughput_state(float time_window_ms = 100.0f)
        : time_window_ms_(time_window_ms)
    {
    }

    void set_time_window_ms(float value)
    {
        time_window_ms_ = value;
    }

    void prepare(float standalone_rate)
    {
        if (initialized_)
            return;
        average_throughput_ = std::max(standalone_rate, 1e-6f);
        initialized_ = true;
    }

    void update(float effective_bits, bool active)
    {
        if (!active)
            return;
        if (!initialized_)
            prepare(effective_bits);
        const float coefficient =
            1.0f - std::exp(-1.0f / time_window_ms_);
        average_throughput_ =
            (1.0f - coefficient) * average_throughput_
            + coefficient * effective_bits;
    }

    void reset()
    {
        average_throughput_ = 0.0f;
        initialized_ = false;
    }

    bool initialized() const { return initialized_; }
    float average_throughput() const { return average_throughput_; }
    float time_window_ms() const { return time_window_ms_; }

private:
    float time_window_ms_ = 100.0f;
    float average_throughput_ = 0.0f;
    bool initialized_ = false;
};
