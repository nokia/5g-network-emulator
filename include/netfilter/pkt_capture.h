#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <functional>
#include <mutex>
#include <vector>
#include <netfilter/netfilter_interface.h>
#include <utils/terminal_logging.h>

struct packet_capture_stats
{
    int queue_num = -1;
    uint32_t total_recv = 0;
    uint32_t total_rlsd = 0;
    uint32_t bytes_recv = 0;
    uint32_t recv_fails = 0;
    uint32_t rlsd_fails = 0;
};

//--------------------------------------------------------------------------------------------------
// pkt_capture(): simple class which interfaces with the C API provided by Netfilter Queues to
// assign callbacks to the configured Netfilter Queues. This class is instanced for each transmission
// direction (DL/UL) for each UE attached to actual IP traffic. It configures a callback associated 
// to the specific Netfilter Queue. There are two methods (release(id)/drop(id)) defined by this class 
// to send the appropiate command to Netfilter Queues. 
// Input: 
//      _queue_num: the queue id for the Netfilter Queue that we want this instance to handle.
//--------------------------------------------------------------------------------------------------
class pkt_capture
{
public:
    pkt_capture(int _queue_num)
    {
        queue_num = _queue_num;
    }

    ~pkt_capture()
    {
        close();
    }

public:
    void close()
    {
        if (is_running && !is_closed)
            netfilter_interface_close(nfiface);
        is_running = false;
        is_closed = true;
    }

public:
    void start(add_pkt_callback_t _cb)
    {
        cb = _cb;
        nfiface = netfilter_interface_open(queue_num, cb, NULL);
        is_running = true;
        is_closed = false;
    }

    void check_pkt_order(int id)
    {
        if(prev_id > id) LOG_ERROR_I("pkt_capture:check_pkt_order") << "Wrong pkt order!!" << END(); 
        prev_id = id; 
    }

    void release(int id, const std::vector<uint8_t>* payload = nullptr)
    {
        check_pkt_order(id);
        if(payload != nullptr && !payload->empty())
            netfilter_interface_release_pkt_payload(nfiface, id, 1, payload->data(), (uint32_t)payload->size());
        else
            netfilter_interface_release_pkt(nfiface, id, 1);
    }

    void drop(int id)
    {
        std::lock_guard<std::mutex> lock(mtx);
        netfilter_interface_release_pkt(nfiface, id, 0);
    }

public:
    void lock()
    {
        mtx.lock();
    }
    void unlock()
    {
        mtx.unlock();
    }

    packet_capture_stats stats() const
    {
        packet_capture_stats out;
        out.queue_num = queue_num;
        if(nfiface == nullptr) return out;
        out.total_recv = nfiface->total_recv;
        out.total_rlsd = nfiface->total_rlsd;
        out.bytes_recv = nfiface->bytes_recv;
        out.recv_fails = nfiface->recv_fails;
        out.rlsd_fails = nfiface->rlsd_fails;
        return out;
    }

private:
    add_pkt_callback_t cb;

private: 
    int prev_id = -1; 
public: // FIXME debug only!
    int queue_num;
private:
    netfilter_interface_t *nfiface;

private:
    std::mutex mtx;

private:
    bool is_running = false;
    bool is_closed = true;
};
