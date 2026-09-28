# fikore-transport

A discrete-event transport and traffic-generation layer in Python, driving FikoRE
through its runtime control protocol. It gives a caller a TCP abstraction:
applications hand over bytes, the model segments them, manages a congestion window,
acknowledges, estimates RTT, times out and retransmits, and the emulator provides
what it is good at — queueing, scheduling, radio, delay, loss and ECN marking.

No real TCP packets and no kernel stack are involved. Segments are simulation
objects, and the only thing that crosses the boundary is a byte count with an
object tag on it.

## Why

An offline co-simulation with no transport layer loses a byte for good, so whoever
drives it has to invent a recovery rule, a sender window and a concurrency policy.
All three are TCP mechanisms. Modelling TCP instead of approximating its effects
turns those invented policies into consequences, and it gives the experiments a
measured RTT, a real congestion response and a place to put L4S.

The request and delivery interface that `TransportBackend` exposes follows the
`NetworkBackend` contract specified by VQEG's CAP-CSP collaboration test pilot, so
a harness written against that contract can use this without adaptation. Nothing
else here depends on that project.

## Status

Working. What runs today:

- The slot-quantised clock, scheduler and lockstep runner.
- The transport state machine: segmentation, cumulative and selective
  acknowledgement, RFC 6298 timers, RFC 6582 partial-ack recovery, RFC 6675 loss
  inference, Linux-style pacing.
- Reno, CUBIC, and **Prague**, bound to the L4S reference implementation rather
  than reimplemented.
- Two other transports for comparison: **ideal**, bare injection with a fixed
  window per UE and instant recovery of whatever the network reports as lost, and
  **UDP**, open loop with no reaction.
- `LoopbackLink`, a deterministic Python bottleneck, with the test suites that
  exercise all of it without an emulator.
- `FikoreLink`, which drives a real FikoRE run one slot at a time: one object tag
  per segment, replayable counter deltas read back every slot, tags released as
  soon as they are terminal.
- `TransportBackend`, the request and delivery interface: object requests,
  concurrent objects per UE, cancellation with a drained tail, and telemetry with
  a measured round-trip time.

Prague and ECT(1) need the emulator to accept `ecn` on `inject` and to report
`ce_bytes` per object. That is now in the emulator, and the two counters are the
whole of it; see [docs/03](docs/03-link-and-protocol-requirements.md).

### Measured

End to end against FikoRE (one UE, 20 MHz, proportional fair, 20 ms PDCP delay
budget, 256 KB receive window, 3 s of simulated time), saturating the cell:

| Congestion control | Goodput | SRTT | Window | Retransmits | Losses reported by the emulator | RTOs | Wall clock |
| :-- | --: | --: | --: | --: | --: | --: | --: |
| Reno | 59.7 Mbps | 18.5 ms | 100 seg | 265 | 265 expired | 0 | 1.1 s |
| CUBIC | 25.4 Mbps | 6.2 ms | 15 seg | 155 | 121 expired | 1 | 0.6 s |

Reno's retransmission count matches the emulator's loss count exactly, which is the
check that the sender is inferring loss rather than being told about it. CUBIC
takes a timeout on this bottleneck and does not recover the pipe within the run;
that is the transport model's behaviour, not the link's, and it is unexplained.

Every run is checked for byte conservation: what the link submitted comes back
delivered, lost with a cause, or still inside the emulator, and the emulator's own
per-UE totals are compared against the link's. A 1 MB object over an unloaded cell
finishes in 142 ms at 56.3 Mbps with all 1 000 000 bytes delivered and none lost.

### What a slot costs

This decides whether a 300 s experiment is affordable, and the answer is that the
barrier is nearly free while reading the state back is not. From
`benchmarks/bench_lockstep.py`:

| Round trip | Per slot | Extrapolated to a 300 s run |
| :-- | --: | --: |
| grant only, no state read | 40 us | 12 s |
| grant + `get`, 1 UE, 1 live object | 112 us | 34 s |
| grant + `get`, 1 UE, 100 live objects | 897 us | 269 s |
| grant + `events`, 1 UE, 100 live objects | 80 us | 24 s |
| grant + `get`, 4 UEs, 20 live objects each | 935 us | 281 s |
| grant + `get`, 4 UEs, 60 live objects each | 2381 us | 714 s |
| grant + `events`, 4 UEs, 60 live objects each | 51 us | 15 s |
| grant + `get` every 10 slots, 4 UEs, 20 each | 809 us | 24 s |

The old cost is linear in the number of live object tags because `get`
re-serialises the whole UE state every slot. `events` carries only counter
movements and remains close to the grant-only floor regardless of how many
completed tags are retained. A compact cumulative state, without knobs or the
object map, is requested once per harness window for telemetry.

### Full 300 s validation

Measured by `benchmarks/validate_scale.py`; the raw summaries are under
`benchmarks/results/`.

| Scenario | Wall | Goodput / objects | Feedback | Conservation |
| :-- | --: | :-- | :-- | :-- |
| 4 UEs, sequential 375 kB objects, CUBIC | 156.9 s | 5554 objects, 180 ms median; 17.01 / 17.01 / 8.68 / 12.86 Mbps, Jain 0.942 | max 15 events/reply | exact |
| 1 UE bulk, CUBIC, 20 ms delay budget | 142.9 s | 58.43 Mbps; 4262 retransmits, 7 RTOs, 4.99 MB expired | max 112 events/reply | exact |
| 1 UE Prague/ECT(1), DualPI2 target 5 ms | 113.5 s | 38.86 Mbps, 6.03 ms SRTT, 23440 CE segments, zero loss/retransmit | max 10 events/reply | exact |

The four-UE condition is deliberately not channel-homogeneous; its throughput
fairness reflects the different per-UE radio realizations rather than transport
starvation. The live registries remain bounded even though request history keeps
all 5554 completed objects for reporting.

### SFV player integration

`benchmarks/validate_sfv.py` substitutes this backend underneath Michi's generic
SFV-VQEG v0.7.2 Python–Node bridge; the JavaScript player sees only
`NetworkStep` and returns opaque requests/cancellations.

- The repository's exact 0.2 s mock example produces the same request IDs, sizes,
  states and delivered bytes over FikoRE for both UEs. B1 wastage is identical;
  B2 differs by 261 bytes at the cutoff because delivery timing is no longer the
  deterministic mock budget.
- Over 5 s, B1 requests no future-video media before the swipe; B2 prefetches
  segments 0 and 1 of videos 2 and 3. All 1,095,975 submitted bytes terminate
  and are conserved.
- With both UEs capped at 1 Mbps, swipes produce four and six cancelled requests.
  Every cancellation tail is non-negative, and delivered plus in-flight bytes
  exactly equals the 319,426 bytes submitted at cutoff.

The measured compatibility summary is
`benchmarks/results/sfv-pilot.json`. The third-party repositories remain
unmodified; the bridge is loaded only by this optional validation tool.

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
simulated time: 110 objects, median 110 ms each, about 28 Mbps per UE, with
throughput, round-trip time, drop rate, queue occupancy and SINR reported per
window. The event path runs the 6 s experiment in 3.3 s of wall clock.

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
  backend.py       request/delivery NetworkBackend over the transport model
prague/            the C++ shim over the L4S reference, and its Makefile
tests/             every transport against the loopback link, and the link
                   itself against a real emulator
benchmarks/        lockstep cost, one transfer, an object queue, and Prague
docs/              the design
```

## Running

```bash
python3 -m venv transport/.venv                 # from the emulator root
transport/.venv/bin/pip install -e transport
make test-transport                             # deterministic links, no emulator
make test-transport-integration                 # builds and drives bin/fikore
make -C transport/prague                        # optional Prague binding

PYTHONPATH=transport python3 transport/benchmarks/bench_lockstep.py
PYTHONPATH=transport python3 transport/benchmarks/e2e_fikore.py
PYTHONPATH=transport python3 transport/benchmarks/e2e_backend.py
PYTHONPATH=transport python3 transport/benchmarks/e2e_prague.py
```

The scripts resolve the emulator from the repository that contains this directory;
`FIKORE_DIR` is only an override. `PRAGUE_DIR` points at the L4S `udp_prague` tree
and defaults to the sibling workspace path `../L4STeam/udp_prague`; nothing in it
is modified and no compiled `.so` is stored in git.

## Reading order

1. [Scope and the boundary](docs/01-scope-and-boundary.md)
2. [The clock, and why one slot at a time](docs/02-clock-and-lockstep.md)
3. [The link, and what the control protocol needs](docs/03-link-and-protocol-requirements.md)
4. [The transport model](docs/04-transport-model.md)
5. [Congestion control, including Prague](docs/05-congestion-control.md)
6. [Traffic generators](docs/06-traffic-generators.md)
7. [Integration with an external harness](docs/07-harness-integration.md)
8. [Validation and roadmap](docs/08-validation-and-roadmap.md)
