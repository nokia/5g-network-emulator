#pragma once

#include <memory>

#include <netfilter/pkt_capture.h>

class packet_capture_interface
{
public:
    virtual ~packet_capture_interface() {}
    virtual void start(add_pkt_callback_t cb) = 0;
    virtual void close() = 0;
    virtual void release(int id, const std::vector<uint8_t>* payload = nullptr) = 0;
    virtual void drop(int id) = 0;
    virtual void lock() = 0;
    virtual void unlock() = 0;
    virtual int queue_num() const = 0;
};

class real_packet_capture : public packet_capture_interface
{
public:
    explicit real_packet_capture(int queue_num)
        : capture(new pkt_capture(queue_num))
    {
    }

    void start(add_pkt_callback_t cb) override { capture->start(cb); }
    void close() override { capture->close(); }
    void release(int id, const std::vector<uint8_t>* payload = nullptr) override { capture->release(id, payload); }
    void drop(int id) override { capture->drop(id); }
    void lock() override { capture->lock(); }
    void unlock() override { capture->unlock(); }
    int queue_num() const override { return capture->queue_num; }

private:
    std::unique_ptr<pkt_capture> capture;
};
