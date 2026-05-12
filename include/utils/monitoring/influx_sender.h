#pragma once

#include <string>
#include <vector>
#include <memory>
#include "../monitoring/monitoring_config.h"

class influx_sender
{
public:
    influx_sender() = default;
    ~influx_sender();

    bool init(const monitoring_config &cfg);
    void send_line(const std::string &line);

private:
    struct impl;
    impl *pimpl = nullptr;
};
