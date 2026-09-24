# Congestion Control

## The interface

```python
class CongestionControl(Protocol):
    mss: int
    cwnd: float
    def on_ack(self, ack: AckInfo) -> None: ...
    def on_dupacks(self, count: int) -> None: ...
    def on_recovered(self) -> None: ...
    def on_rto(self) -> None: ...
    def pacing_rate_bps(self) -> float | None: ...
```

`AckInfo` carries the acknowledged bytes, the RTT sample, the current instant, and
the receiver's cumulative counters: `pkts_received`, `pkts_ce`, `pkts_lost`,
`pkts_sent`. Reno and CUBIC ignore the last four. Prague needs nothing else.

`pacing_rate_bps()` returning `None` means "no opinion", and the sender then uses
its own Linux-style rate. A rate-based controller answers with its own.

## Reno and CUBIC

Both are ns.py's algorithms, with the loss response shared in a `Classic` base:
`ssthresh = max(2·MSS, cwnd/2)` and `cwnd = MSS` on a timeout, fast recovery
inflation on three duplicate acknowledgements, `cwnd = ssthresh` when recovery ends.
CUBIC adds the cubic epoch, `W_last_max` with fast convergence, TCP friendliness and
`beta = 0.2`, and resets its epoch on a loss.

One deliberate difference from ns.py: the initial window is ten segments, not one.
IW10 has been the default everywhere since RFC 6928, and starting at one segment
would make every 200 kB video segment spend its life in slow start.

## Prague

The reference implementation is L4S's `prague_cc.cpp`, and it is unusually well
suited to being driven by a simulator. Three properties:

- **All of its time comes from one virtual method.** `PragueCC::Now()` returns
  microseconds and every timestamp inside the controller goes through it. Overriding
  it to read the simulated clock makes the whole controller simulation-driven with
  no other change.
- **Its feedback is counters, not packets.** `ACKReceived(packets_received,
  packets_CE, packets_lost, packets_sent, error_L4S, inflight)` is exactly what the
  receiver model already produces and what the emulator's per-tag counters can
  supply.
- **Its output is a rate and a window.** `GetCCInfo(pacing_rate, packet_window,
  packet_burst, packet_size)`, which maps directly onto the sender's per-slot
  budget: the window bounds the pipe and the rate bounds what leaves in one slot.

### Binding rather than porting

The reference implementation is bound through a small C++ shim with a C ABI rather
than ported. `prague_cc.cpp` is full of 64-bit fixed-point helpers
(`mul_64_64_shift`, `div_64_64_round`) and wrap-around-safe 32-bit signed time, and
a hand port would be a source of silent divergence from the algorithm everyone else
is comparing against. `prague/prague_shim.cpp` is the whole of the C++: a subclass
whose `Now()` returns simulated microseconds, and eight free functions around it.
`make -C prague` compiles it together with the reference source into
`libpraguesim.so`, and `cc_prague.py` calls it through `ctypes`.

The reference tree is not modified. Its own `libprague.a` is not used either,
because it is built without `-fPIC` and cannot go into a shared object, so the
source is compiled again here with the right flags.

Two details are worth knowing. The constructor calls `Now()` before the override
exists, and the reference's first call returns 1 whatever the wall clock says, so a
controller is born at t=1µs and the Python side offsets its clock by one to match.
And Prague's own initial rate is 100 kbps, which is right for a sender that knows
nothing about its path and wrong for a cell known to carry tens of Mbps: the
default here is ten segments over the reference RTT, the same argument that gives
the classic controllers IW10.

Duplicate acknowledgements are not forwarded to it. A classic sender infers
congestion from them because that is all it has; Prague is told by the counters,
and reacting to both would be reducing twice for one event. The sender still
retransmits — retransmission is its business, the window is the controller's.

A pure-Python port could follow, validated against the binding on identical input
traces. The order is the decision: the binding is the reference, and a port is only
worth trusting once it agrees with it.

### What Prague needs from the network

ECT(1) on the injected packets, and CE marks back per flow. Both now exist: the
`inject` command takes an `ecn` field and the per-object counters include
`ce_bytes`; see [docs/03](03-link-and-protocol-requirements.md).

It also needs the receiver to report loss, which a window-based controller never
had to do: Prague works out what is in flight as `sent - received - lost`, so a
wrong loss count is a sender that stalls or overshoots. `TcpReceiver` derives both
counts from what it holds at that instant rather than accumulating them as events,
which is what makes them right under retransmission and under reordering alike. An
earlier version counted each gap as it was noticed and never took it back, and
overstated loss by every reordering the emulator produced — 126 phantom losses in a
3 s run that lost nothing.

### The queue on the other side

Supporting Prague is not the same as modelling L4S. The emulator already implements
DualPI2 with real CE marking, per-UE enabled with `l4s_dual_queue`, and reports the
dual-queue occupancies and the `p_l`, `p_c` and `p_cl` probabilities, so the
coexistence experiment — a Prague flow and a CUBIC flow sharing a bottleneck, one
marked and one dropped — is available as soon as ECT(1) can be set on injection.
That is the experiment worth aiming at, and it is the one this library exists to
make possible.
