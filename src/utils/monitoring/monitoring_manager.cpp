#include "utils/monitoring/monitoring_manager.h"
#include <chrono>
#include <mutex>

monitoring_manager::monitoring_manager()
{
}

monitoring_manager &monitoring_manager::instance()
{
    static monitoring_manager m;
    return m;
}

bool monitoring_manager::init(const monitoring_config &c)
{
    cfg = c;
    if(!cfg.enabled) return false;

    sender.init(cfg);
    ag.init(cfg);
    ag.set_emit_callback([this](const std::string &line){ sender.send_line(line); });
    return true;
}

bool monitoring_manager::is_enabled() const
{
    return cfg.enabled;
}

const monitoring_config &monitoring_manager::get_config() const
{
    return cfg;
}

void monitoring_manager::publish(const metric_point &point)
{
    if(!cfg.enabled) return;
    metric_point effective_point = point;
    const std::int64_t slot_ts = slot_timestamp_ns.load();
    if(slot_ts > 0) effective_point.ts_ns = slot_ts;
    if(effective_point.ts_ns <= 0)
    {
        effective_point.ts_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }
    ag.add_point(effective_point);
}

void monitoring_manager::send_text_line(const std::string &line)
{
    if(!cfg.enabled || !cfg.emit_text_logs_compat) return;
    sender.send_line(line);
}

void monitoring_manager::set_slot_timestamp_ns(std::int64_t ts_ns)
{
    slot_timestamp_ns.store(ts_ns);
}

void monitoring_manager::clear_slot_timestamp_ns()
{
    slot_timestamp_ns.store(0);
}

aggregator &monitoring_manager::get_aggregator()
{
    return ag;
}

influx_sender &monitoring_manager::get_sender()
{
    return sender;
}
