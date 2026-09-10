/**********************************************
* Copyright 2022 Nokia
* Licensed under the BSD 3-Clause Clear License
* SPDX-License-Identifier: BSD-3-Clause-Clear
**********************************************/

#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <utils/control/control_config.h>
#include <utils/control/control_transport.h>

//--------------------------------------------------------------------------------------------------
// transport_socket(): unix or tcp stream carrying NDJSON, with its own thread.
//
// The thread accepts, does the protocol handshake, reads lines, parses them and leaves
// the commands in an inbox behind a mutex. It never touches simulation state: the
// simulation thread drains the inbox from control_manager::tick(), at the quiescent
// point, and applies everything there.
//
// One client at a time. A second connection is answered with an error and closed, which
// removes the whole question of arbitrating between concurrent controllers.
//--------------------------------------------------------------------------------------------------
class transport_socket : public control_transport
{
public:
    explicit transport_socket(const control_config &cfg);
    ~transport_socket() override;

    bool poll(std::vector<command> &out) override;
    void reply(const ack &a) override;
    void stop() override;

    bool ok() const { return listen_fd_ >= 0; }

    // True once a client completed the handshake and is still connected.
    bool peer_alive() const { return client_fd_.load() >= 0; }

private:
    void serve();
    bool handshake(int fd);
    void handle_lines(int fd, std::string &buffer);
    void send_line(int fd, const std::string &line);

private:
    control_config cfg_;
    int listen_fd_ = -1;
    std::string unix_path_;

    std::atomic<int> client_fd_{-1};
    std::mutex write_mtx_;

    std::mutex inbox_mtx_;
    std::vector<command> inbox_;
    std::uint64_t line_no_ = 0;

    std::atomic<bool> stopping_{false};
    std::thread thread_;
};
