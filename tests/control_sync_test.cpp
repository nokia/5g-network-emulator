// Barrier mode: credits gate simulated time, a lost peer fails open, and real time never
// blocks. Driven through simulator::run_steps with a client on its own thread.
#include <cassert>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <string>
#include <thread>

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <nlohmann/json.hpp>
#include <simulator/simulator.h>
#include <utils/control/proto_version.h>

using json = nlohmann::json;

namespace
{
const char *SOCKET_PATH = "build/tests/control_sync.sock";
const char *CONFIG = "build/tests/control_sync.ini";

double ms_since(const std::chrono::steady_clock::time_point &t0)
{
    return std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(
        std::chrono::steady_clock::now() - t0).count();
}

void write_config(const std::string &sync_mode, int period_ms, int timeout_ms,
                  const std::string &on_timeout)
{
    std::ifstream base("tests/control_smoke.ini");
    assert(base.is_open());

    std::string line, text;
    while (std::getline(base, line))
    {
        if (line.rfind("period:", 0) == 0) line = "period: " + std::to_string(period_ms);
        if (line.rfind("transport:", 0) == 0) line = "transport: unix";
        if (line.rfind("timeline_file:", 0) == 0)
            line = std::string("address: ") + SOCKET_PATH
                 + "\nsync_mode: " + sync_mode
                 + "\ncredit_timeout_ms: " + std::to_string(timeout_ms)
                 + "\non_timeout: " + on_timeout;
        text += line + "\n";
    }

    std::ofstream out(CONFIG);
    out << text;
}

// Minimal client: connect, handshake, then send lines. Lives in the test thread while the
// simulation runs on another one, or the other way around.
class client
{
public:
    bool connect_and_handshake()
    {
        fd_ = ::socket(AF_UNIX, SOCK_STREAM, 0);
        if (fd_ < 0) return false;

        struct sockaddr_un addr;
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

        for (int attempt = 0; attempt < 100; attempt++)
        {
            if (::connect(fd_, (struct sockaddr *)&addr, sizeof(addr)) == 0) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            if (attempt == 99) return false;
        }

        const std::string hello = read_line();
        if (hello.empty()) return false;
        json h = json::parse(hello);
        if (h["proto"].get<std::string>() != FIKORE_CONTROL_PROTO) return false;

        json reply;
        reply["proto"] = FIKORE_CONTROL_PROTO;
        write_line(reply.dump());
        return true;
    }

    void grant(std::int64_t until_tti)
    {
        json j;
        j["id"] = ++id_;
        j["op"] = "grant";
        j["until_tti"] = until_tti;
        write_line(j.dump());
    }

    std::string read_line()
    {
        std::string out;
        char ch = 0;
        while (::recv(fd_, &ch, 1, 0) == 1)
        {
            if (ch == '\n') return out;
            out.push_back(ch);
        }
        return out;
    }

    void write_line(const std::string &text)
    {
        const std::string line = text + "\n";
        ::send(fd_, line.data(), line.size(), MSG_NOSIGNAL);
    }

    void disconnect()
    {
        if (fd_ >= 0) ::close(fd_);
        fd_ = -1;
    }

private:
    int fd_ = -1;
    std::uint64_t id_ = 0;
};

// Simulated time advances exactly up to the granted TTI and stops there.
void test_credit_gates_simulated_time()
{
    write_config("barrier", -1, 400, "continue");
    simulator sim(CONFIG);
    assert(sim.control_plane().barrier_mode());

    client c;
    assert(c.connect_and_handshake());

    c.grant(5);
    const std::string ack = c.read_line();
    json a = json::parse(ack);
    assert(a["status"] == "ok");
    assert(a["credit_until_tti"] == 5);

    // TTIs 0..5 are paid for, so they run without waiting.
    std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();
    sim.run_steps(6);
    assert(ms_since(t0) < 400.0);

    // TTI 6 is not, so the barrier holds until the timeout expires.
    t0 = std::chrono::steady_clock::now();
    sim.run_steps(1);
    assert(ms_since(t0) >= 350.0);

    // Credit never moves backwards: a grant into the past is a no-op answered with ok.
    c.grant(2);
    a = json::parse(c.read_line());
    assert(a["status"] == "ok");
    assert(a["credit_until_tti"] == 5);

    c.disconnect();
}

// Losing the peer releases the emulator, for good.
void test_lost_peer_fails_open()
{
    write_config("barrier", -1, 5000, "continue");
    simulator sim(CONFIG);

    client c;
    assert(c.connect_and_handshake());
    c.grant(3);
    c.read_line();
    sim.run_steps(4);

    c.disconnect();
    // The socket thread needs a moment to notice; the barrier would otherwise wait the
    // full five seconds.
    std::this_thread::sleep_for(std::chrono::milliseconds(400));

    const std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();
    sim.run_steps(50);
    assert(ms_since(t0) < 3000.0);
    assert(!sim.control_plane().barrier_mode());   // degraded, and it stays degraded
}

// Barrier plus real time is refused at init: blocking the wall clock defeats the mode.
void test_barrier_is_refused_in_real_time()
{
    write_config("barrier", 1, 400, "continue");
    simulator sim(CONFIG);
    assert(!sim.control_plane().barrier_mode());

    const std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();
    sim.run_steps(20);
    assert(ms_since(t0) < 400.0);                  // never blocked
}

// on_timeout: abort asks for the run to end instead of carrying on uncontrolled.
void test_timeout_abort_requests_stop()
{
    write_config("barrier", -1, 200, "abort");
    simulator sim(CONFIG);

    client c;
    assert(c.connect_and_handshake());
    c.grant(1);
    c.read_line();

    sim.run_steps(2);
    assert(!sim.control_plane().stop_requested());

    sim.run_steps(1);                              // TTI 2, no credit
    assert(sim.control_plane().stop_requested());

    c.disconnect();
}
}

int main()
{
    test_credit_gates_simulated_time();
    test_lost_peer_fails_open();
    test_barrier_is_refused_in_real_time();
    test_timeout_abort_requests_stop();
    std::remove(CONFIG);
    return 0;
}
