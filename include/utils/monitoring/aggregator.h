#pragma once

#include <string>
#include <unordered_map>
#include <map>
#include <mutex>
#include <chrono>
#include <functional>
#include <thread>
#include "monitoring_config.h"

enum class field_aggregation
{
    sum,
    mean,
    last
};

struct metric_field
{
    double value = 0.0;
    field_aggregation aggregation = field_aggregation::mean;
};

struct metric_point
{
    std::string measurement;
    std::map<std::string,std::string> tags;
    std::map<std::string,metric_field> fields;
    std::int64_t ts_ns = 0;
};

class aggregator
{
public:
    using emit_cb_t = std::function<void(const std::string &line)>;

    aggregator() = default;
    ~aggregator();

    void init(const monitoring_config &cfg);
    void add_point(const metric_point &p);
    void set_emit_callback(emit_cb_t cb) { emit_cb = cb; }

private:
    struct window_accum
    {
        field_aggregation aggregation = field_aggregation::mean;
        double sum = 0.0;
        int count = 0;
        double last_value = 0.0;
        std::int64_t last_ts = 0;
        bool initialized = false;
    };

    struct series_window
    {
        std::map<std::string, window_accum> fields;
        std::int64_t latest_ts_ns = 0;
    };

    monitoring_config cfg;
    std::mutex mtx;
    std::unordered_map<std::string, series_window> accum;
    bool running = false;
    std::thread worker;
    emit_cb_t emit_cb;
};
