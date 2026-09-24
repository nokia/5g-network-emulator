# fikore-transport

A discrete-event transport and traffic-generation layer in Python, driving FikoRE
through its runtime control protocol. It gives the harness a TCP abstraction:
applications hand over bytes, the model segments them, manages a congestion window,
acknowledges, estimates RTT, times out and retransmits, and the emulator provides
what it is good at — queueing, scheduling, radio, delay, loss and ECN marking.

No real TCP packets and no kernel stack are involved. Segments are simulation
objects, and the only thing that crosses the boundary is a byte count with an
object tag on it.

## Why

Offline co-simulation currently has no transport layer, so a byte the network loses
is lost for good and the harness has to invent a recovery rule, a sender window and
a concurrency policy. All three are TCP mechanisms. Modelling TCP instead of
approximating its effects turns those invented policies into consequences, and it
gives the experiments a measured RTT, a real congestion response and a place to put
L4S.

## Status

Working. What runs today:

- The slot-quantised clock, scheduler and lockstep runner.
- The transport state machine: segmentation, cumulative and selective
  acknowledgement, RFC 6298 timers, RFC 6582 partial-ack recovery, RFC 6675 loss
  inference, Linux-style pacing.
- Reno, CUBIC, and **Prague**, bound to the L4S reference implementation rather
  than reimplemented.
- Two other transports for comparison: **ideal**, the pilot's injection rule with a
  fixed window per UE and instant recovery, and **UDP**, open loop with no
  reaction.
- `LoopbackLink`, a deterministic Python bottleneck, with the test suites that
  exercise all of it without an emulator.
- `FikoreLink`, which drives a real FikoRE run one slot at a time: one object tag
  per segment, per-tag counters read back every slot, tags released as soon as they
  are terminal.
- `TransportBackend`, the pilot's `NetworkBackend`: object requests, concurrent
  objects per UE, cancellation with a drained tail, and telemetry with a measured
  round-trip time.

Prague and ECT(1) need the emulator to accept `ecn` on `inject` and to report
`ce_bytes` per object. That is now in the emulator, and the two counters are the
whole of it; see [docs/03](docs/03-link-and-protocol-requirements.md).

### Measured

End to end against FikoRE (one UE, 20 MHz, proportional fair, 20 ms PDCP delay
budget, 256 KB receive window, 3 s of simulated time):

| Congestion control | Goodput | SRTT | Window | Retransmits | Losses reported by the emulator | RTOs | Wall clock |
| :-- | --: | --: | --: | --: | --: | --: | --: |
| CUBIC | 60.5 Mbps | 22.1 ms | 138 seg | 521 | 491 expired | 0 | 2.5 s |
| Reno | 56.4 Mbps | 12.6 ms | 85 seg | 232 | 232 expired | 0 | 1.7 s |

Reno's retransmission count matches the emulator's loss count exactly, which is the
check that the sender is inferring loss rather than being told about it.

Prague against the same bottleneck, over the deterministic link so that the two
runs differ in nothing but the controller (50 Mbps, 20 ms, 512 KB queue, CE above
2 ms of queueing delay):

| | Goodput | SRTT | CE marks | Segments lost | Retransmits |
| :-- | --: | --: | --: | --: | --: |
| CUBIC, not-ECT | 49.9 Mbps | 76.2 ms | 0 | 3738 | 7978 |
| Prague, ECT(1) | 49.9 Mbps | 14.2 ms | 1703 | 0 | 0 |

Same throughput, a fifth of the delay, and nothing lost. That result is the reason
the library exists, and it now also runs against the emulator's own DualPI2:
`benchmarks/e2e_prague.py`.

Through the backend, two UEs fetching a queue of 375 kB objects over 6 s of
simulated time: 106 objects, median 110 ms each, about 27 Mbps per UE, with
throughput, round-trip time, drop rate, queue occupancy and SINR reported per
window. 4.7 s of wall clock, of which most is the per-slot object map.

The ideal transport against the transport model, same 300 kB object over the same
bottleneck: it finishes in 0.14 s against 0.23 s, and drops 2503 segments doing it
against 46. It is faster because it is told about every loss immediately and repairs
it with no penalty, and the radio pays for the difference. Quantifying that gap is
what it is for.

## Layout

```
fikore_transport/
  clock.py         slots, scheduling phases, deterministic ordering
  link.py          the network boundary, and the loopback bottleneck
  tcp.py           sender and receiver state machines
  cc.py            congestion control interface, Reno, CUBIC
  cc_prague.py     Prague, over the binding to the L4S reference
  ideal.py         the injection rule: fixed window per UE, instant recovery
  udp.py           open-loop datagrams, and a sink that measures them
  runner.py        the lockstep loop
  emulator.py      process wrapper and .ini generation
  fikore_link.py   the control-protocol client as a Link
  backend.py       the pilot's NetworkBackend over the transport model
prague/            the C++ shim over the L4S reference, and its Makefile
tests/             every transport, against the loopback link
benchmarks/        lockstep cost, one transfer, an object queue, and Prague
docs/              the design
```

## Running

```bash
make -C prague                                 # the Prague binding, once
python3 tests/test_loopback_transfer.py        # no emulator needed
python3 tests/test_backend.py                  # no emulator needed
python3 tests/test_transports.py               # ideal and UDP
python3 tests/test_prague.py                   # needs the binding
python3 benchmarks/bench_lockstep.py           # cost of one slot per round trip
python3 benchmarks/e2e_fikore.py               # one transfer over FikoRE
python3 benchmarks/e2e_backend.py              # a player-like object queue, 2 UEs
python3 benchmarks/e2e_prague.py               # Prague over the emulator's DualPI2
```

`FIKORE_DIR` points at the emulator checkout and defaults to
`../5g-network-emulator`. `PRAGUE_DIR` points at the L4S `udp_prague` tree and
defaults to `../../L4STeam/udp_prague`; nothing in it is modified.

## Reading order

1. [Scope and the boundary](docs/01-scope-and-boundary.md)
2. [The clock, and why one slot at a time](docs/02-clock-and-lockstep.md)
3. [The link, and what the control protocol needs](docs/03-link-and-protocol-requirements.md)
4. [The transport model](docs/04-transport-model.md)
5. [Congestion control, including Prague](docs/05-congestion-control.md)
6. [Traffic generators](docs/06-traffic-generators.md)
7. [Integration with the pilot harness](docs/07-pilot-integration.md)
8. [Validation and roadmap](docs/08-validation-and-roadmap.md)
