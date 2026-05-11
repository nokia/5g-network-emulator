#include <cassert>
#include <cmath>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include <common/direction.h>
#include <mac_layer/harq_handler.h>
#include <netfilter/packet_capture_interface.h>
#include <pdcp_layer/captured_packet_handler.h>
#include <pdcp_layer/ip_buffer.h>
#include <pdcp_layer/packet_handler.h>
#include <pdcp_layer/pdcp_layer.h>
#include <simulator/configuration_loader.h>
#include <traffic_models/traffic_config.h>

namespace
{
class fake_packet_capture : public packet_capture_interface
{
public:
    explicit fake_packet_capture(int queue_num) : queue(queue_num) {}

    void start(add_pkt_callback_t cb) override { callback = cb; }
    void close() override {}
    void release(int id) override { released.push_back(id); }
    void drop(int id) override { dropped.push_back(id); }
    void lock() override {}
    void unlock() override {}
    int queue_num() const override { return queue; }

    void capture(uint64_t size_bytes, uint32_t id)
    {
        callback(nullptr, nullptr, 0, 0, size_bytes, id);
    }

    std::vector<int> released;
    std::vector<int> dropped;

private:
    int queue;
    add_pkt_callback_t callback;
};

class fake_packet_handler : public packet_handler
{
public:
    fake_packet_handler(pdcp_config config, int verbosity) : packet_handler(config, verbosity) {}
    float ingest(int, float) override { return 0.0f; }
};

bool near(float lhs, float rhs)
{
    return std::fabs(lhs - rhs) < 0.001f;
}

packet_handler_config make_sim_config(pdcp_config pdcp_c, traffic_config traffic)
{
    packet_handler_config cfg;
    cfg.ue_type = SIM_UE;
    cfg.tx_dir = TX_DL;
    cfg.queue_num = -1;
    cfg.ue_id = 1;
    cfg.init_t = nullptr;
    cfg.traffic_c = traffic;
    cfg.pdcp_c = pdcp_c;
    cfg.log_traffic = true;
    cfg.log_quality = false;
    return cfg;
}

void test_ip_buffer_fragmentation()
{
    ip_buffer buffer(1);
    buffer.step(0.0f);
    buffer.generate(2500.0f, 1000.0f, 1.0f, 0.0f, 0.0f);

    assert(buffer.has_pkts());
    assert(near(buffer.get_oldest_timestamp(), 1.0f));

    harq_pkt first(0, buffer.get_oldest_timestamp(), 1.0f, 0, 0, 0.0f, 0.0f, 0.0f);
    float first_bits = buffer.get_pkts(1500.0f, first);

    assert(near(first_bits, 1500.0f));
    assert(near(first.bits, 1500.0f));
    assert(first.pkts.size() == 2);
    assert(!first.pkts.front().is_fragment);
    assert(first.pkts.back().is_fragment);
    assert(near(first.pkts.back().size, 500.0f));
    assert(buffer.has_pkts());

    harq_pkt second(1, buffer.get_oldest_timestamp(), 1.0f, 0, 0, 0.0f, 0.0f, 0.0f);
    float second_bits = buffer.get_pkts(2000.0f, second);

    assert(near(second_bits, 1000.0f));
    assert(near(second.bits, 1000.0f));
    assert(second.pkts.size() == 2);
    assert(!buffer.has_pkts());

    buffer.step(2.0f);
    buffer.generate(1000.0f, 1000.0f, 2.0f, 0.0f, 0.0f);
    harq_pkt exact(2, buffer.get_oldest_timestamp(), 2.0f, 0, 0, 0.0f, 0.0f, 0.0f);
    float exact_bits = buffer.get_pkts(1000.0f, exact);
    assert(near(exact_bits, 1000.0f));
    assert(near(exact.bits, 1000.0f));
    assert(!buffer.has_pkts());
}

void test_ip_buffer_admission()
{
    ip_buffer buffer(1);
    buffer.step(0.0f);

    assert(buffer.add_pkt(ip_pkt(0.0f, 12000.0f, 12000.0f, 1, 0.0f, 0.0f)));
    assert(buffer.size() == 1);
    assert(near(buffer.get_oldest_timestamp(), 0.0f));

    assert(!buffer.add_pkt(ip_pkt(0.0f, 20000000000.0f, 20000000000.0f, 2, 0.0f, 0.0f)));
    assert(buffer.size() == 1);
    assert(buffer.get_error(true) > 0.0f);
}

void test_pdcp_release_flow()
{
    pdcp_config config(4, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, true);
    traffic_config traffic(CONSTANT_TRAFFIC_MODEL, 1.0f, 1.0f, "", "", 0.0f, 1000, 0.0f, 0.0f);
    pdcp_layer layer(make_sim_config(config, traffic), 1);
    layer.init(0, 1, 1);
    layer.step(0.0f);
    layer.step(0.001f);
    layer.step(0.003f);

    assert(layer.has_pkts());

    float first_sent = layer.handle_pkt(1500.0f, 0, 30.0f, 10.0f);
    assert(near(first_sent, 1500.0f));
    assert(near(layer.release(), 1500.0f));

    float second_sent = layer.handle_pkt(1000.0f, 0, 30.0f, 10.0f);
    assert(near(second_sent, 500.0f));
    assert(near(layer.release(), 500.0f));
    assert(!layer.has_pkts());
}

void test_simulated_pdcp_timeout_drop()
{
    pdcp_config config(4, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, true);
    traffic_config traffic(CONSTANT_TRAFFIC_MODEL, 1.0f, 1.0f, "", "", 0.0f, 1000, 0.0f, 0.0f);
    pdcp_layer layer(make_sim_config(config, traffic), 1);

    layer.init(0, 1, 1);
    layer.set_pkt_delay_budget(0.001f);
    layer.step(0.0f);
    layer.step(0.001f);
    layer.step(0.003f);

    assert(layer.has_pkts());
    layer.step(0.006f);

    pdcp_queue_status status = layer.get_queue_status();
    assert(layer.get_error(true) > 0.0f);
    if(status.ip_buffer_size > 0)
    {
        assert(status.ip_oldest_age <= layer.get_pkt_delay_budget());
    }
}

void test_harq_and_packet_handlers()
{
    harq_handler harq(4, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0);
    harq.init(0, 1, 1);

    harq_pkt pkt(7, 0.0f, 0.0f, 0, 0, 1200.0f, 0.0f, 0.0f);
    harq.add_pkts(pkt);
    harq.step(0.0f);

    assert(harq.is_pkt_ready());
    harq_pkt ready = harq.get_pkt();
    assert(ready.id == 7);
    assert(near(ready.bits, 1200.0f));
    assert(!harq.is_pkt_ready());

    pdcp_config config(4, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, true);
    fake_packet_handler release(config, 1);
    release.step(0.0f);
    release.push(std::move(ready));
    assert(near(release.release(), 1200.0f));
}

void test_harq_timeout_drop()
{
    harq_handler harq(4, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0);
    harq.init(0, 1, 1);

    harq_pkt fresh(8, 0.99f, 1.0f, 0, 0, 1000.0f, 0.0f, 0.0f);
    harq_pkt old(9, 0.0f, 1.0f, 0, 0, 1200.0f, 0.0f, 0.0f);
    harq.add_pkts(fresh);
    harq.add_pkts(old);

    harq_pkt expired;
    assert(harq.pop_pkt_older_than(0.5f, expired));
    assert(expired.id == 9);
    assert(near(expired.bits, 1200.0f));
    assert(harq.size() == 1);
    assert(!harq.pop_pkt_older_than(0.5f, expired));
}

void test_captured_packet_handler_release()
{
    pdcp_config config(4, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, true);
    std::unique_ptr<fake_packet_capture> fake(new fake_packet_capture(77));
    fake_packet_capture *fake_ptr = fake.get();
    std::unique_ptr<packet_capture_interface> capture(std::move(fake));
    captured_packet_handler handler(std::move(capture), nullptr, config, 1);

    handler.init();
    fake_ptr->capture(250, 42);
    handler.step(0.0f);
    handler.ingest(TX_DL, 0.0f);

    ip_buffer buffer(1);
    buffer.step(0.0f);
    while(handler.has_ingress_pkts())
    {
        buffer.add_pkt(handler.pop_ingress_pkt());
    }

    assert(buffer.has_pkts());
    harq_pkt first(0, buffer.get_oldest_timestamp(), 0.0f, 0, 0, 0.0f, 0.0f, 0.0f);
    assert(near(buffer.get_pkts(1500.0f, first), 1500.0f));
    handler.push(std::move(first));
    assert(fake_ptr->released.empty());
    assert(near(handler.release(), 0.0f));
    assert(fake_ptr->released.empty());

    harq_pkt second(1, buffer.get_oldest_timestamp(), 0.0f, 0, 0, 0.0f, 0.0f, 0.0f);
    assert(near(buffer.get_pkts(1000.0f, second), 500.0f));
    handler.push(std::move(second));
    assert(near(handler.release(), 2000.0f));
    assert(fake_ptr->released.size() == 1);
    assert(fake_ptr->released.front() == 42);
}

void test_captured_packet_handler_timeout_drop()
{
    pdcp_config config(4, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, true);
    std::unique_ptr<fake_packet_capture> fake(new fake_packet_capture(78));
    fake_packet_capture *fake_ptr = fake.get();
    std::unique_ptr<packet_capture_interface> capture(std::move(fake));
    captured_packet_handler handler(std::move(capture), nullptr, config, 1);

    handler.init();
    fake_ptr->capture(125, 43);
    handler.step(0.0f);
    handler.ingest(TX_DL, 0.0f);

    harq_pkt pkt(2, 0.0f, 0.01f, 0, 0, 0.0f, 0.0f, 0.0f);
    assert(handler.has_ingress_pkts());
    pkt.pkts.push_back(handler.pop_ingress_pkt());
    pkt.bits = pkt.pkts.front().size;
    handler.drop(std::move(pkt));
    handler.step(0.01f);
    handler.release();

    assert(fake_ptr->dropped.size() == 1);
    assert(fake_ptr->dropped.front() == 43);
}
}

int main()
{
    test_ip_buffer_fragmentation();
    test_ip_buffer_admission();
    test_pdcp_release_flow();
    test_simulated_pdcp_timeout_drop();
    test_harq_and_packet_handlers();
    test_harq_timeout_drop();
    test_captured_packet_handler_release();
    test_captured_packet_handler_timeout_drop();
    return 0;
}
