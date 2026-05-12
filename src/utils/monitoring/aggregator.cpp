#include "utils/monitoring/aggregator.h"
#include <thread>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <algorithm>

namespace
{
std::string format_numeric_field(double value)
{
    std::ostringstream out;
    out << std::fixed << std::setprecision(6) << value;
    return out.str();
}
}

aggregator::~aggregator()
{
    running = false;
    if(worker.joinable()) worker.join();
}

void aggregator::init(const monitoring_config &c)
{
    cfg = c;
    running = true;
    worker = std::thread([this]() {
        while(running)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(cfg.aggregation_window_ms));
            // swap accumulators
            std::unordered_map<std::string, series_window> snapshot;
            {
                std::lock_guard<std::mutex> lk(mtx);
                snapshot.swap(accum);
            }
            // emit aggregated metrics
            for(auto &kv: snapshot)
            {
                const std::string &key = kv.first;
                const series_window &series = kv.second;
                // key format: measurement|tag1=val,tag2=val
                std::string measurement;
                std::string tags;
                auto pos = key.find('|');
                if(pos != std::string::npos)
                {
                    measurement = key.substr(0,pos);
                    tags = key.substr(pos+1);
                }
                else measurement = key;

                std::ostringstream out;
                out << measurement;
                if(!tags.empty()) out << "," << tags;
                bool first_field = true;
                for(const auto &field_kv : series.fields)
                {
                    const std::string &field_name = field_kv.first;
                    const window_accum &a = field_kv.second;
                    double value = 0.0;
                    switch(a.aggregation)
                    {
                        case field_aggregation::sum:
                            value = a.sum;
                            break;
                        case field_aggregation::mean:
                            value = a.count ? (a.sum / a.count) : 0.0;
                            break;
                        case field_aggregation::last:
                            value = a.last_value;
                            break;
                    }
                    out << (first_field ? " " : ",");
                    out << field_name << "=" << format_numeric_field(value);
                    first_field = false;
                }
                if(first_field) continue;
                const std::int64_t ts_ns = series.latest_ts_ns > 0 ? series.latest_ts_ns
                    : std::chrono::duration_cast<std::chrono::nanoseconds>(
                          std::chrono::system_clock::now().time_since_epoch()).count();
                out << " " << ts_ns;

                if(emit_cb) emit_cb(out.str());
            }
        }
    });
}

void aggregator::add_point(const metric_point &p)
{
    // build key
    std::string key = p.measurement;
    if(!p.tags.empty())
    {
        key += "|";
        bool first = true;
        for(auto &t: p.tags)
        {
            if(!first) key += ","; first = false;
            key += t.first + "=" + t.second;
        }
    }

    if(p.fields.empty()) return;

    std::lock_guard<std::mutex> lk(mtx);
    auto &series = accum[key];
    if(p.ts_ns > series.latest_ts_ns) series.latest_ts_ns = p.ts_ns;
    for(const auto &field_kv : p.fields)
    {
        const std::string &field_name = field_kv.first;
        const metric_field &field = field_kv.second;
        window_accum &a = series.fields[field_name];
        a.aggregation = field.aggregation;
        a.initialized = true;
        switch(field.aggregation)
        {
            case field_aggregation::sum:
                a.sum += field.value;
                break;
            case field_aggregation::mean:
                a.sum += field.value;
                a.count += 1;
                break;
            case field_aggregation::last:
                if(p.ts_ns >= a.last_ts)
                {
                    a.last_value = field.value;
                    a.last_ts = p.ts_ns;
                }
                break;
        }
        if(field.aggregation != field_aggregation::last) a.last_ts = p.ts_ns;
    }
}
