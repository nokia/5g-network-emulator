#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <deque>
#include <iostream>
#include <functional>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <netfilter/netfilter_interface.h>
#include <pkts/pkts.h>
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

enum class packet_capture_action
{
    ACCEPT,
    ACCEPT_CE,
    DROP
};

struct captured_packet_info
{
    uint64_t bytes = 0;
    uint32_t pkt_id = 0;
    uint8_t ecn = ECN_NOT_ECT;
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
        queue_num_v = _queue_num;
    }

    virtual ~pkt_capture()
    {
        close();
    }

public:
    virtual void close()
    {
        if (is_running && !is_closed && nfiface != nullptr)
            netfilter_interface_close(nfiface);
        nfiface = nullptr;
        is_running = false;
        is_closed = true;
    }

public:
    virtual void start()
    {
        add_pkt_callback_t cb = [this](void*, netfilter_interface_t*, const nfq_packet_metadata& meta)
        {
            handle_capture(meta);
        };
        nfiface = netfilter_interface_open(queue_num_v, cb, this);
        is_running = true;
        is_closed = false;
    }

    void check_pkt_order(int id)
    {
        if(prev_id > id) LOG_ERROR_I("pkt_capture:check_pkt_order") << "Wrong pkt order!!" << END(); 
        prev_id = id; 
    }

    virtual void verdict(uint32_t pkt_id, packet_capture_action action)
    {
        std::lock_guard<std::mutex> lock(mtx);
        check_pkt_order((int)pkt_id);

        if(action == packet_capture_action::DROP)
        {
            netfilter_interface_release_pkt(nfiface, pkt_id, 0);
            payloads.erase(pkt_id);
            return;
        }

        if(action == packet_capture_action::ACCEPT)
        {
            netfilter_interface_release_pkt(nfiface, pkt_id, 1);
            payloads.erase(pkt_id);
            return;
        }

        std::unordered_map<uint32_t, stored_payload>::iterator it = payloads.find(pkt_id);
        if(it == payloads.end())
        {
            LOG_ERROR_I("pkt_capture::verdict") << "Missing payload for pkt_id " << pkt_id
                                                << " with ACCEPT_CE; falling back to DROP" << END();
            netfilter_interface_release_pkt(nfiface, pkt_id, 0);
            return;
        }

        apply_ipv4_ecn(it->second.payload, ECN_CE);
        netfilter_interface_release_pkt_payload(nfiface, pkt_id, 1, it->second.payload.data(),
                                                (uint32_t)it->second.payload.size());
        payloads.erase(it);
    }

public:
    virtual bool pop_captured_packet(captured_packet_info& out)
    {
        std::lock_guard<std::mutex> lock(mtx);
        if(captured_packets.empty()) return false;
        out = captured_packets.front();
        captured_packets.pop_front();
        return true;
    }

    int captured_queue_size() const
    {
        std::lock_guard<std::mutex> lock(mtx);
        return (int)captured_packets.size();
    }

    bool peek_oldest_captured(captured_packet_info& out) const
    {
        std::lock_guard<std::mutex> lock(mtx);
        if(captured_packets.empty()) return false;
        out = captured_packets.front();
        return true;
    }

    virtual int queue_num() const
    {
        return queue_num_v;
    }

    virtual packet_capture_stats stats() const
    {
        std::lock_guard<std::mutex> lock(mtx);
        packet_capture_stats out;
        out.queue_num = queue_num_v;
        if(nfiface == nullptr) return out;
        out.total_recv = nfiface->total_recv;
        out.total_rlsd = nfiface->total_rlsd;
        out.bytes_recv = nfiface->bytes_recv;
        out.recv_fails = nfiface->recv_fails;
        out.rlsd_fails = nfiface->rlsd_fails;
        return out;
    }

private:
    struct stored_payload
    {
        stored_payload() {}
        stored_payload(std::vector<uint8_t> _payload, uint8_t _original_ecn)
            : payload(std::move(_payload)), original_ecn(_original_ecn) {}
        std::vector<uint8_t> payload;
        uint8_t original_ecn = ECN_NOT_ECT;
    };

    static uint16_t ipv4_checksum(const uint8_t* data, size_t len)
    {
        uint32_t sum = 0;
        for(size_t i = 0; i + 1 < len; i += 2)
        {
            sum += ((uint16_t)data[i] << 8) | data[i + 1];
        }
        if(len & 1) sum += ((uint16_t)data[len - 1] << 8);
        while(sum >> 16) sum = (sum & 0xffff) + (sum >> 16);
        return (uint16_t)(~sum);
    }

    static void apply_ipv4_ecn(std::vector<uint8_t>& payload, uint8_t ecn)
    {
        if(payload.size() < 20) return;
        uint8_t ihl = (payload[0] & 0x0f) * 4;
        if(ihl < 20 || payload.size() < ihl) return;
        payload[1] = (payload[1] & 0xfc) | (ecn & 0x03);
        payload[10] = 0;
        payload[11] = 0;
        uint16_t csum = ipv4_checksum(payload.data(), ihl);
        payload[10] = (uint8_t)(csum >> 8);
        payload[11] = (uint8_t)(csum & 0xff);
    }

protected:
    void enqueue_captured_packet(captured_packet_info info, std::vector<uint8_t> payload, uint8_t original_ecn)
    {
        std::lock_guard<std::mutex> lock(mtx);
        payloads[info.pkt_id] = stored_payload(std::move(payload), original_ecn);
        captured_packets.push_back(info);
    }

private:
    void handle_capture(const nfq_packet_metadata& meta)
    {
        captured_packet_info info;
        info.bytes = meta.bytes;
        info.pkt_id = meta.pkt_id;
        info.ecn = meta.ecn;
        enqueue_captured_packet(info, meta.payload, meta.ecn);
    }
    mutable std::mutex mtx;

private: 
    int prev_id = -1; 
private:
    int queue_num_v = -1;
    netfilter_interface_t *nfiface = nullptr;

private:
    std::unordered_map<uint32_t, stored_payload> payloads;
    std::deque<captured_packet_info> captured_packets;

private:
    bool is_running = false;
    bool is_closed = true;
};
