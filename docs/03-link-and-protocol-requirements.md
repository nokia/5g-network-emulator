# The Link, and What the Control Protocol Needs

## The interface

```python
class Link(Protocol):
    mss: int
    def submit(self, items: list[Transmit], at_tti: int) -> None: ...
    def step(self, until_tti: int) -> list[Arrival]: ...
    def close(self) -> None: ...
```

`Arrival` is the terminal outcome of one segment: the slot it happened in, the
flow, the sequence number, the size, whether it was delivered, dropped or expired,
whether it was CE marked, and the echoed timestamp.

Two implementations. `LoopbackLink` is a FIFO per direction served at a fixed rate,
with a queue limit, an optional delay budget and optional marking above a queue
delay. It is not a radio model and does not pretend to be one; its job is to make
the transport layer testable with a known bandwidth-delay product and losses that
happen exactly when the queue overflows. `FikoreLink` is the real one.

## How FikoreLink maps onto the control protocol

| Model operation | Control protocol |
| :-- | :-- |
| hand a segment over | `inject` on `ue/<id>`, with a tag, `dl.bytes` or `ul.bytes` |
| advance one slot | `grant` with `until_tti` |
| read what became terminal | `get` on `ue/*`, then diff the per-tag counters |
| release a finished segment | `forget` on its tag |
| set a rate cap or priority | `set` with `dl.rmax_mbps`, `priority` |
| check the MSS | `state.pkt_size_bits` from the same `get` |

**One tag per segment.** This is what turns a byte counter into per-segment
feedback: a tag whose `delivered + dropped + expired` reaches its size has
finished, and the counter it landed in says how. Tags are released as soon as they
are terminal, so the number of live objects stays around the congestion window
instead of growing with the transfer.

**Both messages go out before either reply is read.** The grant is answered by the
transport thread and everything else at the quiescent point, so replies do not come
back in the order they were sent, and waiting for the first one blocks the run until
the credit timeout.

**A message of N commands is answered N times, and every reply carries the
message's `id`.** The `id` identifies the message, not the command, so it cannot be
used to tell one reply from the next; what aligns the stream is counting, one reply
per command written, in the order they were written. Only the `get` carries a
`result`. A client that reads one reply per `id` leaves N-1 in the socket and from
then on reads a previous slot's answers — which are well formed, say `ok`, and
describe the wrong instant. That was the state of this client until it was first
run against a real emulator, and the symptom was not an error but a plausible
number: slots that injected anything reported no arrivals at all, arrivals appeared
only on the idle slots where the backlog happened to drain, and the round-trip time
the sender measured was that backlog rather than the network.

**The reply lags by one slot.** Commands are applied and state is resolved at the
same quiescent point, so the state read at slot `n` reports what finished during
`n-1`. Arrivals are stamped with the slot in which they were learnt, because that is
the only instant the model may legitimately act on.

**Arrivals are sorted by sequence number before they are returned.** The object map
comes back in the emulator's hash order. Without the sort, the receiver sees the
segments of one slot shuffled, reports the gaps as selective acknowledgements, and
the sender retransmits segments that were never lost — a measured 4 spurious
retransmissions in a 1 MB transfer that lost nothing at all. Inside a slot the
ordering is unobservable, so sequence order is the only defensible one to present.

## What works today, and what it costs

Everything above exists in the protocol as it stands, so Phase 1 needs no emulator
change at all. The measured cost is the object map. The barrier on its own is 40 µs
per slot, 12 s over a 300 s run, and it is not the problem. Adding the state read
costs 103 µs with one live tag on one UE, 829 µs with a hundred; across 4 UEs it is
309 µs with one tag each, 749 µs with 20, and 1890 µs with 60. A congestion window
of 60 segments is not large — it is 90 KB, about right for 15 Mbps at 50 ms — so a
300 s run is about nine minutes of wall clock, and worse as capacity grows.

The real path confirms it. The same end-to-end script costs 245 to 314 µs per slot
while a 1 MB object is in flight, and 600 to 2100 µs once a bulk flow holds a window
of tens to hundreds of segments: about 9.7 KB of JSON per slot, 660 µs of it spent
waiting on the emulator and 250 µs parsing in Python. The extra time is the whole
object map being serialised, parsed and diffed every slot to learn about the handful
of segments that changed.

These figures are roughly twice what this document reported before the client was
fixed, and the earlier ones were understated for a reason worth recording: reading
one reply per message left the state reply buffered for the next slot to collect, so
the client never waited for the emulator to produce the answer it was using. Lagging
a slot behind looks like speed until you notice which slot the answer describes.

The map can be isolated from everything else by sending the same `get` twice in one
message: the second copy costs what producing it costs, and nothing else. With 2486
live tags and a 207 KB reply, one copy is 8.0 ms of a 9.9 ms slot — the simulation
and the barrier are the remaining 1.9 ms. About a third of the 8.0 ms is the
client's `json.loads` and the rest is the emulator building and serialising. Which
settles where the change has to be made: a client that parsed for free would still
pay three quarters of the cost, because the payload can only be cut where it is
produced.

## The one addition that matters

**Per-tag deltas instead of the whole map.** Everything the model needs is *what
changed*, and what changed in one slot is a handful of segments rather than the
whole window. Either shape works:

```json
{"id":7,"cmds":[{"op":"get","target":"ue/*","objects":"changed"}]}
```

or a dedicated drain:

```json
{"id":7,"cmds":[{"op":"events","target":"ue/*"}]}
{"id":7,"status":"ok","tti":940,"result":[
  {"target":"ue/0","dir":"dl","events":[
    {"tag":8817,"fate":"delivered","bytes":1500,"ce":false},
    {"tag":8818,"fate":"expired","bytes":1500}]}]}
```

The expected cost then follows the grant-only floor plus a small payload, around
130 to 150 µs per slot with four UEs: 45 s of overhead for a 300 s run, and
independent of the window size. This is the single change that decides whether the
approach scales, and it is worth making before anything else.

## What ECN needs

The abstract packet already carries what is required: `ip_pkt` has `ecn`,
`original_ecn` and `ce_marked`, DualPI2 classifies on `ECT(1)`, the L4S queue
reports `l4s_ce_packets`, `l4s_ce_bits`, `l4s_aqm_drops` and the dual-queue
probabilities, and CE marking already applies to injected traffic as well as to
captured traffic. Two things were missing at the interface, and both are now
present:

1. **`ecn` on `inject`.** Injected packets used to be created as `Not-ECT`, so they
   always took the classic queue and were dropped rather than marked. The addition
   is one field, `{"op":"inject", ..., "ecn":"ect1"}`, with `not-ect` as the default
   so nothing changes for a client that does not ask. It is named rather than
   numbered, and an unknown name is a rejected command rather than a silent
   `not-ect`.

2. **CE per tag.** Marks used to be counted per UE and direction, which is enough
   only while a direction carries one flow. `ce_bytes` now sits alongside the three
   terminal counters in the `objects` block, so a segment's marks are its own. A
   mark is not a fate: `delivered + dropped + expired` still adds up to what was
   injected, and `ce_bytes` is a subset of `delivered_bytes` because a marked
   fragment that is then dropped is a loss, and counting it as both would tell the
   sender to back off twice for one event.

Prague needs no more than counters — its interface takes `packets_received`,
`packets_CE`, `packets_lost` — so per-tag CE counts are sufficient. Nothing needs
per-packet marking detail.

## Optional, for speed rather than fidelity

**A grant that stops on an event.** `{"op":"grant","until_tti":N,"break_on_event":true}`
would let the emulator run ahead and block as soon as an object becomes terminal,
returning the events. Idle slots would then cost nothing, and the harness would stop
paying a round trip per millisecond for a flow that has nothing in flight. It
changes no semantics: the model still only ever sees terminal events in the slot
they happened.

## An emulator-side constraint worth knowing

Run cost depends on the queue the client creates. A client that overfeeds a UE
leaves a large PDCP backlog, and the per-slot expiry scan grows with it: a run with
a 20 ms delay budget and an unbounded window measured 43 ms per slot inside the
emulator, a thousand times the cost of a healthy one. A bounded receive window is
therefore not only realism (see [docs/04](04-transport-model.md)), it is what keeps
a run fast.

The same backlog is what makes the per-slot object map expensive, and the two
compound: a 300 ms budget with no AQM and a 4 MB window builds a queue of thousands
of live tags, the whole map is serialized every slot in both directions, and a run
of that shape eventually spends more than the emulator's 30 s credit timeout in a
single slot and is aborted. It is the scaling problem above, reached from the other
side, and the same per-tag delta feedback fixes it.

**`max_cmds_per_tick` and the barrier do not compose.** The emulator applies at most
that many commands per TTI and defers the rest to the next one, which is the right
thing to do when nobody is waiting. Under the barrier nobody can be waiting for
anything else: the client is blocked on acknowledgements the deferred commands have
not produced, and the TTI that would produce them needs credit the client cannot
grant until it is unblocked. The run stops until `credit_timeout_ms` expires and
then, with `on_timeout: abort`, ends. A lockstep slot carries one command per
segment, so the shipped 256 is below an ordinary congestion window and the config
has to raise it; `EmulatorConfig.max_cmds_per_tick` does, and the client refuses a
slot it knows will not fit rather than hanging. Deferring the limit in the emulator
would be reasonable too — a barrier could take everything addressed to the TTI it is
about to run — but nothing here needs it.
