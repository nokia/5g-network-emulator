#include <cassert>
#include <cmath>

#include <mac_layer/harq_handler.h>
#include <pdcp_layer/ip_buffer.h>
#include <pdcp_layer/pdcp_layer.h>
#include <pdcp_layer/release_handler.h>

namespace
{
bool near(float lhs, float rhs)
{
    return std::fabs(lhs - rhs) < 0.001f;
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
    float exact_bits = buffer.get_pkts(1000.0f);
    assert(near(exact_bits, 1000.0f));
    assert(!buffer.has_pkts());
}

void test_pdcp_release_flow()
{
    pdcp_config config(4, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, true);
    pdcp_layer layer(config, 1);
    layer.init(0, 1, 1);
    layer.step(0.0f);
    layer.generate_pkts(2000.0f, 1000.0f, 0.0f);

    assert(layer.has_pkts());

    layer.step(0.001f);
    float first_sent = layer.handle_pkt(1500.0f, 0, 30.0f, 10.0f);
    assert(near(first_sent, 1500.0f));
    assert(near(layer.release(), 1500.0f));

    float second_sent = layer.handle_pkt(1000.0f, 0, 30.0f, 10.0f);
    assert(near(second_sent, 500.0f));
    assert(near(layer.release(), 500.0f));
    assert(!layer.has_pkts());
}

void test_pdcp_captured_ingress_flow()
{
    pdcp_config config(4, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, true);
    pdcp_layer layer(config, 1);
    layer.init(0, 1, 1);
    layer.step(0.0f);
    layer.enqueue_ip_pkt(ip_pkt(0.0f, 1200.0f, 1200.0f, -1, 42, 0.0f, 0.0f));

    assert(layer.has_pkts());
    assert(near(layer.get_oldest_timestamp(), 0.0f));

    layer.step(0.001f);
    float sent = layer.handle_pkt(1200.0f, 0, 30.0f, 10.0f);
    assert(near(sent, 1200.0f));
    assert(near(layer.release(), 1200.0f));
    assert(!layer.has_pkts());
}

void test_harq_and_release_handlers()
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

    release_handler release;
    release.step(0.0f);
    release.push(std::move(ready));
    assert(near(release.release(), 1200.0f));
}
}

int main()
{
    test_ip_buffer_fragmentation();
    test_pdcp_release_flow();
    test_pdcp_captured_ingress_flow();
    test_harq_and_release_handlers();
    return 0;
}
