#include <cassert>
#include <cmath>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include <common/direction.h>
#include <mac_layer/harq_handler.h>
#include <netfilter/pkt_capture.h>
#include <pdcp_layer/captured_packet_handler.h>
#include <pdcp_layer/ip_buffer.h>
#include <pdcp_layer/packet_handler.h>
#include <pdcp_layer/pdcp_layer.h>
#include <simulator/configuration_loader.h>
#include <traffic_models/traffic_config.h>

namespace
{
class fake_packet_capture : public pkt_capture
{
public:
    explicit fake_packet_capture(int queue_num) : pkt_capture(queue_num) {}

    void start() override {}
    void close() override {}
    void verdict(uint32_t pkt_id, packet_capture_action action) override
    {
        verdicts.emplace_back(pkt_id, action);
        if(action == packet_capture_action::DROP)
        {
            dropped.push_back((int)pkt_id);
            payloads.erase(pkt_id);
            original_ecns.erase(pkt_id);
            return;
        }

        released.push_back((int)pkt_id);
        std::unordered_map<uint32_t, std::vector<uint8_t>>::iterator it = payloads.find(pkt_id);
        if(it != payloads.end())
        {
            std::vector<uint8_t> payload = it->second;
            if(action == packet_capture_action::ACCEPT_CE && !payload.empty())
            {
                payload[1] = (payload[1] & 0xfc) | ECN_CE;
            }
            released_payloads.push_back(std::move(payload));
            payloads.erase(it);
        }
        original_ecns.erase(pkt_id);
    }
    packet_capture_stats stats() const override
    {
        packet_capture_stats out;
        out.queue_num = pkt_capture::queue_num();
        out.total_rlsd = (uint32_t)(released.size() + dropped.size());
        return out;
    }

    void capture(uint64_t size_bytes, uint32_t id, uint8_t ecn = ECN_NOT_ECT, std::vector<uint8_t> payload = std::vector<uint8_t>())
    {
        captured_packet_info info;
        info.bytes = size_bytes;
        info.pkt_id = id;
        info.ecn = ecn;
        payloads[id] = payload;
        original_ecns[id] = ecn;
        enqueue_captured_packet(info, std::move(payload), ecn);
    }

    std::vector<int> released;
    std::vector<int> dropped;
    std::vector<std::vector<uint8_t> > released_payloads;
    std::vector<std::pair<uint32_t, packet_capture_action>> verdicts;

private:
    std::unordered_map<uint32_t, std::vector<uint8_t>> payloads;
    std::unordered_map<uint32_t, uint8_t> original_ecns;
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

std::vector<uint8_t> make_ipv4_payload(uint8_t ecn)
{
    std::vector<uint8_t> payload(20, 0);
    payload[0] = 0x45;
    payload[1] = ecn & 0x03;
    payload[2] = 0;
    payload[3] = 20;
    payload[8] = 64;
    payload[9] = 17;
    return payload;
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
    std::unique_ptr<pkt_capture> capture(std::move(fake));
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
    std::unique_ptr<pkt_capture> capture(std::move(fake));
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

void test_dualpi2_classification_and_ce_marking()
{
    dualpi2_config l4s;
    l4s.enabled = true;
    l4s.classic_guard_s = 1.0f;
    ip_buffer buffer(1);
    buffer.configure_l4s(l4s);
    buffer.step(0.0f);

    ip_pkt classic(0.0f, 1000.0f, 1000.0f, 1, 0.0f, 0.0f);
    classic.ecn = ECN_NOT_ECT;
    classic.original_ecn = ECN_NOT_ECT;
    ip_pkt l_pkt(0.0f, 1000.0f, 1000.0f, 2, 0.0f, 0.0f);
    l_pkt.ecn = ECN_ECT1;
    l_pkt.original_ecn = ECN_ECT1;

    assert(buffer.add_pkt(classic));
    assert(buffer.add_pkt(l_pkt));

    harq_pkt out(10, buffer.get_oldest_timestamp(), 0.010f, 0, 0, 0.0f, 0.0f, 0.0f);
    buffer.step(0.010f);
    assert(near(buffer.get_pkts(1000.0f, out), 1000.0f));
    assert(out.pkts.size() == 1);
    assert(out.pkts.front().uid == 2);
    assert(out.pkts.front().ecn == ECN_CE);
    assert(out.pkts.front().ce_marked);

    dualpi2_stats stats = buffer.get_l4s_stats();
    assert(stats.ce_packets == 1);
    assert(stats.classic_queue_size == 1);
}

void test_dualpi2_classic_drop_notification()
{
    dualpi2_config l4s;
    l4s.enabled = true;
    l4s.target_s = 0.001f;
    l4s.rtt_max_s = 0.003f;
    ip_buffer buffer(1);
    buffer.configure_l4s(l4s);
    buffer.step(0.0f);

    ip_pkt classic(0.0f, 1000.0f, 1000.0f, 3, 0.0f, 0.0f);
    classic.ecn = ECN_NOT_ECT;
    classic.original_ecn = ECN_NOT_ECT;
    assert(buffer.add_pkt(classic));

    for(int i = 1; i < 40; ++i) buffer.step(i * 0.002f);

    harq_pkt out(11, buffer.get_oldest_timestamp(), 0.080f, 0, 0, 0.0f, 0.0f, 0.0f);
    buffer.get_pkts(1000.0f, out);
    harq_pkt dropped;
    assert(buffer.pop_aqm_dropped_pkt(dropped));
    assert(dropped.pkts.size() == 1);
    assert(dropped.pkts.front().uid == 3);
    assert(dropped.pkts.front().aqm_dropped);
}

void test_captured_packet_handler_rewrites_ecn_payload()
{
    pdcp_config config(4, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, true);
    std::unique_ptr<fake_packet_capture> fake(new fake_packet_capture(79));
    fake_packet_capture *fake_ptr = fake.get();
    std::unique_ptr<pkt_capture> capture(std::move(fake));
    captured_packet_handler handler(std::move(capture), nullptr, config, 1);

    handler.init();
    handler.step(0.0f);
    fake_ptr->capture(20, 99, ECN_ECT1, make_ipv4_payload(ECN_ECT1));

    ip_pkt pkt(0.0f, 160.0f, 160.0f, 99, 0.0f, 0.0f);
    pkt.ecn = ECN_CE;
    pkt.original_ecn = ECN_ECT1;
    pkt.ce_marked = true;

    harq_pkt harq(12, 0.0f, 0.0f, 0, 0, pkt.size, 0.0f, 0.0f);
    harq.pkts.push_back(pkt);
    handler.push(std::move(harq));
    assert(near(handler.release(), 160.0f));
    assert(fake_ptr->released.size() == 1);
    assert(fake_ptr->released_payloads.size() == 1);
    assert(fake_ptr->verdicts.front().second == packet_capture_action::ACCEPT_CE);
    assert((fake_ptr->released_payloads.front()[1] & 0x03) == ECN_CE);
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
    test_dualpi2_classification_and_ce_marking();
    test_dualpi2_classic_drop_notification();
    test_captured_packet_handler_rewrites_ecn_payload();
    return 0;
}
