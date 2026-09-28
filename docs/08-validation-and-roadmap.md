# Validation and Roadmap

## Three layers of validation

**Without the emulator.** `LoopbackLink` has a known bandwidth-delay product, a
known queue limit and losses that happen exactly when the queue overflows, so the
transport layer can be checked against arithmetic: every byte arrives exactly once,
the window sawtooths, goodput approaches the bottleneck, the smoothed RTT matches
the configured delay plus the queue, retransmissions equal drops, and the same run
twice gives the identical trace. This is where the state machine is debugged, and it
is where the SACK and pacing findings in [docs/04](04-transport-model.md) came from.

**Against the emulator, with the radio held still.** One UE, fixed position, no
mobility, no variances, proportional fair. Checks that are only available here:

- Conservation: the emulator's per-tag delivered, dropped and expired counters must
  sum to what was injected, and the sender's acknowledged bytes must equal the
  receiver's contiguous stream.
- Loss attribution: the sender's retransmission count against the emulator's loss
  count. Equality means the sender is inferring loss; an excess means it is
  retransmitting on reordering. Reno measured 232 against 232.
- A rate cap: set `dl.rmax_mbps` and check that the achieved goodput converges to it
  and that the window settles at cap times RTT.
- Determinism: the same seed must give the same byte trace, which is what the
  pilot's paired comparisons depend on.

**Against real traffic.** The emulator has a real-traffic path, so the same
conditions can be driven by real `iperf3` over a real kernel stack and by the
simulated one offline, and the throughput, RTT, retransmission and CE numbers
compared. This is the only test that can find a modelling error rather than an
implementation error, and it is the reason the generators are shaped like `iperf3`
in the first place. For Prague the comparison target is the L4S reference sender
over the same DualPI2 queue.

## Roadmap

**Phase 1 — the transport layer.** Done in the core: clock, lockstep, sender,
receiver, Reno, CUBIC, the loopback link, the FikoRE link with one tag per segment.
Exit criteria: the loopback tests pass, and a transfer over the emulator conserves
bytes with retransmissions equal to reported losses.

**Phase 2 — the object generator and the pilot backend.** Built:
`TransportBackend` with concurrent objects per UE, cancellation that reports the
drained tail, telemetry with a measured round-trip time, and the harness window
decoupled from the transport slot. Checked against the contract on the loopback
link and against a real run with two UEs fetching a queue of objects.

What remains of the phase is the comparison it exists for: the same condition run
twice with only the backend changed — byte injection with adapter-side recovery,
against transport with retransmission — so that the difference between them is
measured rather than argued. That needs the pilot's harness, not this repository.

**Phase 3 — ECN and Prague.** Needs `ecn` on injection and CE per tag. The shim
over the reference implementation, Prague behind the same interface, and the first
marked flow through DualPI2. Exit criteria: Prague against the loopback marking
model matches the reference's behaviour on an identical input trace, and a Prague
flow through the emulator's L4S queue holds a low queue delay where CUBIC fills the
buffer.

**Phase 4 — coexistence.** A Prague flow and a classic flow sharing a bottleneck,
the dual-queue occupancies and marking probabilities recorded alongside the
transport view. This is the experiment the whole design is for.

**Phase 5 — performance.** Per-tag delta feedback is built and measured:
`events` stays close to the grant-only floor instead of scaling with the number
of retained tags. The remaining optional step is a grant that stops on an event,
so an idle sender need not pay one round trip per millisecond.

## Ordering note

Phase 2 does not depend on Phase 3. The performance prerequisite for long grids is
already in place: feedback no longer grows with the congestion window.
