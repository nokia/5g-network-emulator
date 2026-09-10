// When a control command takes effect, and what "absolute priority" means.
// Driven through simulator::run_steps, so the timer thread is out of the picture and the
// TTI counter is exactly the step index.
#include <cassert>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <string>

#include <simulator/simulator.h>
#include <ue/ue.h>

namespace
{
bool near(double a, double b, double tol = 1e-4)
{
    return std::fabs(a - b) < tol;
}

const char *TIMELINE = "build/tests/control_apply_timeline.ndjson";
const char *CONFIG = "build/tests/control_apply.ini";

void write_file(const char *path, const std::string &text)
{
    std::ofstream out(path);
    assert(out.is_open());
    out << text;
}

void write_config(const std::string &timeline)
{
    std::ifstream base("tests/control_smoke.ini");
    assert(base.is_open());
    std::string line, text;
    while (std::getline(base, line))
    {
        // Same scenario as the smoke, with the timeline swapped and the duration cut:
        // run_steps drives the loop, so the .ini duration is irrelevant here.
        if (line.rfind("timeline_file:", 0) == 0) line = "timeline_file: " + timeline;
        text += line + "\n";
    }
    write_file(CONFIG, text);
}

// A command scheduled at a TTI must land on that TTI and not one step earlier.
void test_command_lands_on_its_tti()
{
    write_file(TIMELINE,
               "{\"id\":1,\"at_tti\":20,\"cmds\":[{\"target\":\"ue/0\",\"set\":{\"priority\":9.0}}]}\n");
    write_config(TIMELINE);

    simulator sim(CONFIG);
    ue &u = (*sim.ue_list())[0];

    assert(near(u.overrides().priority, 1.0));

    sim.run_steps(20);                                  // TTIs 0..19
    assert(near(u.overrides().priority, 1.0));          // not one step early

    sim.run_steps(1);                                   // TTI 20
    assert(near(u.overrides().priority, 9.0));
}

// Absolute, not cumulative: applying the .ini value changes nothing, and applying the
// same value twice is not the same as squaring it.
void test_priority_is_absolute()
{
    write_file(TIMELINE,
               "{\"id\":1,\"at_tti\":1,\"cmds\":[{\"target\":\"ue/*\",\"set\":{\"priority\":1.0}}]}\n"
               "{\"id\":2,\"at_tti\":2,\"cmds\":[{\"target\":\"ue/0\",\"set\":{\"priority\":4.0}}]}\n"
               "{\"id\":3,\"at_tti\":3,\"cmds\":[{\"target\":\"ue/0\",\"set\":{\"priority\":4.0}}]}\n");
    write_config(TIMELINE);

    simulator sim(CONFIG);
    ue &u0 = (*sim.ue_list())[0];
    ue &u1 = (*sim.ue_list())[1];

    sim.run_steps(2);
    assert(near(u0.overrides().priority, 1.0));         // same as the .ini: no change
    assert(near(u1.overrides().priority, 1.0));

    sim.run_steps(2);
    assert(near(u0.overrides().priority, 4.0));         // applied twice, still 4
    assert(near(u1.overrides().priority, 1.0));         // ue/* did not stick around
}

// A detached UE drops its buffers, stops being a candidate and leaves the rotation.
void test_disable_removes_the_ue()
{
    write_file(TIMELINE,
               "{\"id\":1,\"at_tti\":30,\"cmds\":[{\"target\":\"ue/1\",\"set\":{\"enabled\":false}}]}\n");
    write_config(TIMELINE);

    simulator sim(CONFIG);
    ue &u0 = (*sim.ue_list())[0];
    ue &u1 = (*sim.ue_list())[1];

    sim.run_steps(30);
    assert(u1.is_enabled());
    assert(u1.has_packets(TX_DL));                      // it had traffic queued
    assert(u0.overrides().rr_n == 2);
    assert(u1.overrides().rr_rank == 1);

    sim.run_steps(1);
    assert(!u1.is_enabled());
    assert(!u1.has_packets(TX_DL));                     // buffers gone, not frozen
    assert(!u1.has_packets(TX_UL));
    assert(u0.overrides().rr_n == 1);                   // rotation is over one UE now
    assert(u1.overrides().rr_rank == -1);
    assert(u0.overrides().rr_rank == 0);

    // And it stays gone: no traffic accumulates while detached.
    sim.run_steps(10);
    assert(!u1.has_packets(TX_DL));
}

// The rate cap keeps the UE out of the candidate list while the bucket is empty, and the
// refill brings it back on a schedule set by rmax.
void test_rate_cap_bucket()
{
    write_file(TIMELINE,
               "{\"id\":1,\"at_tti\":5,\"cmds\":[{\"target\":\"ue/0\",\"set\":{\"dl.rmax_mbps\":10.0}}]}\n");
    write_config(TIMELINE);

    simulator sim(CONFIG);
    ue &u = (*sim.ue_list())[0];

    sim.run_steps(6);
    assert(near(u.overrides().rmax_bps[TX_DL], 10e6, 1.0));

    // Depth is one TTI worth: 10 Mbps * 1 ms = 10000 bits, never more.
    for (int i = 0; i < 20; i++)
    {
        sim.run_steps(1);
        assert(u.overrides().rmax_tokens[TX_DL] <= 10000.0f + 1.0f);
    }

    // In debt the UE is not a candidate at all, whatever it has queued.
    u.overrides().rmax_tokens[TX_DL] = -30000.0f;
    assert(u.has_packets(TX_DL));
    schedule_candidate c = u.get_schedule_candidate(TX_DL, 0, 2, 0);
    assert(!c.has_data);
}

// End to end: a capped UE converges to the cap, not to what the link would give it.
void test_rate_cap_holds_the_rate()
{
    write_file(TIMELINE,
               "{\"id\":1,\"at_tti\":0,\"cmds\":[{\"target\":\"ue/0\",\"set\":{\"dl.rmax_mbps\":10.0}}]}\n");
    write_config(TIMELINE);
    simulator capped(CONFIG);

    write_file(TIMELINE, "\n");
    write_config(TIMELINE);
    simulator free_run(CONFIG);

    capped.run_steps(2000);
    free_run.run_steps(2000);

    const float capped_tp = (*capped.ue_list())[0].get_avg_tp(TX_DL);
    const float free_tp = (*free_run.ue_list())[0].get_avg_tp(TX_DL);

    // Both UEs demand 200 Mbps, so without a cap the link is the only limit.
    assert(free_tp > 20.0f);
    // The cap is on the bits granted over the air, so the delivered rate lands at or just
    // below 10 Mbps, never above.
    assert(capped_tp <= 10.5f);
    assert(capped_tp > 7.0f);
    assert(capped_tp < free_tp);
}
}

int main()
{
    test_command_lands_on_its_tti();
    test_priority_is_absolute();
    test_disable_removes_the_ue();
    test_rate_cap_bucket();
    test_rate_cap_holds_the_rate();
    std::remove(TIMELINE);
    std::remove(CONFIG);
    return 0;
}
