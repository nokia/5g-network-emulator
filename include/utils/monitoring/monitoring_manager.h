#pragma once

#include "monitoring_config.h"
#include "aggregator.h"
#include "influx_sender.h"
#include <atomic>
#include <memory>

class monitoring_manager
{
public:
    static monitoring_manager &instance();
    bool init(const monitoring_config &cfg);
    bool is_enabled() const;
    const monitoring_config &get_config() const;
    void publish(const metric_point &point);
    void send_text_line(const std::string &line);
    void set_slot_timestamp_ns(std::int64_t ts_ns);
    void clear_slot_timestamp_ns();
    aggregator &get_aggregator();
    influx_sender &get_sender();

private:
    monitoring_manager();
    monitoring_config cfg;
    aggregator ag;
    influx_sender sender;
    std::atomic<std::int64_t> slot_timestamp_ns{0};
};
