#include "utils/monitoring/influx_sender.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <cstring>
#include <mutex>
#include <iostream>

struct influx_sender::impl
{
    monitoring_config cfg;

    struct dest
    {
        monitoring_output_config c;
        int fd = -1;
        sockaddr_storage addr{};
        socklen_t addr_len = 0;
        bool stream = false;
    };

    std::vector<dest> destinations;
    std::mutex mtx;

    void send_to_destination(const dest &d, const std::string &line) const
    {
        if(d.fd == -1) return;

        if(d.stream)
        {
            ssize_t s = send(d.fd, line.c_str(), line.size(), 0);
            if(s < 0)
            {
                std::cerr << "influx_sender: send failed for " << d.c.name
                          << ": " << std::strerror(errno) << "\n";
            }
        }
        else
        {
            ssize_t s = sendto(d.fd, line.c_str(), line.size(), 0, (sockaddr*)&d.addr, d.addr_len);
            if(s < 0)
            {
                std::cerr << "influx_sender: sendto failed for " << d.c.name
                          << ": " << std::strerror(errno) << "\n";
            }
        }
    }
};

influx_sender::~influx_sender()
{
    if(pimpl)
    {
        for(auto &d: pimpl->destinations) if(d.fd != -1) close(d.fd);
        delete pimpl;
        pimpl = nullptr;
    }
}

static int set_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if(flags == -1) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

bool influx_sender::init(const monitoring_config &cfg)
{
    pimpl = new impl();
    pimpl->cfg = cfg;

    for(const auto &out: cfg.outputs)
    {
        impl::dest d;
        d.c = out;
        if(out.type == "udp" || out.type == "tcp")
        {
            int fd = socket(AF_INET, out.type=="udp"?SOCK_DGRAM:SOCK_STREAM, 0);
            if(fd < 0)
            {
                std::cerr << "influx_sender: failed to create socket for " << out.name << "\n";
                continue;
            }
            set_nonblocking(fd);
            sockaddr_in sa{};
            sa.sin_family = AF_INET;
            sa.sin_port = htons(out.port);
            inet_pton(AF_INET, out.address.c_str(), &sa.sin_addr);
            d.addr_len = sizeof(sa);
            memcpy(&d.addr, &sa, sizeof(sa));
            d.fd = fd;
            d.stream = out.type=="tcp";
            if(d.stream)
            {
                connect(fd, (sockaddr*)&sa, d.addr_len);
            }
        }
        else if(out.type == "unix")
        {
            int fd = socket(AF_UNIX, SOCK_DGRAM, 0);
            if(fd < 0)
            {
                std::cerr << "influx_sender: failed to create unix socket for " << out.name << "\n";
                continue;
            }
            set_nonblocking(fd);
            sockaddr_un sa{};
            sa.sun_family = AF_UNIX;
            strncpy(sa.sun_path, out.address.c_str(), sizeof(sa.sun_path)-1);
            d.addr_len = sizeof(sa);
            memcpy(&d.addr, &sa, sizeof(sa));
            d.fd = fd;
            d.stream = false;
        }
        pimpl->destinations.push_back(std::move(d));
    }

    return true;
}

void influx_sender::send_line(const std::string &line)
{
    if(!pimpl) return;
    std::lock_guard<std::mutex> lk(pimpl->mtx);
    const std::string payload = line + "\n";
    for(const auto &d: pimpl->destinations) pimpl->send_to_destination(d, payload);
}
