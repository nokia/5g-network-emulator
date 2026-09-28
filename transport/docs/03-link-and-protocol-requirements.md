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
| read what changed | cell-wide `events` with the last consumed cursor |
| release a finished segment | `forget` on its tag |
| set a rate cap or priority | `set` with `dl.rmax_mbps`, `priority` |
| read network telemetry | `events` with `include_state` at the end of a harness window |

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
per command written, in the order they were written. Only the final read command
carries a `result`. A client that reads one reply per `id` leaves N-1 in the socket and from
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

## Incremental feedback

`events` is additive to `get`: dashboards and diagnostics keep their complete
snapshot, while a transport asks only for counter movements newer than the cursor
it has consumed.

```json
{"id":7,"cmds":[{"op":"events","after":123,"include_state":false}]}
{"id":7,"status":"ok","tti":940,"result":{
  "cursor":125,
  "events":[
    {"seq":124,"at_tti":939,"target":"ue/0","dir":"dl","tag":8817,
     "delivered_bytes":1500,"ce_bytes":1500},
    {"seq":125,"at_tti":939,"target":"ue/1","dir":"dl","tag":730,
     "expired_bytes":1500}
  ]
}}
```

An event is a delta, not the emulator guessing that a tag is complete: the client
may inject one tag more than once and only it knows the total. `after` means
“consumed through this cursor”. Newer events are retained, so retrying the same
cursor replays them. A compact cumulative state without the object map is optional
for telemetry.

The replay log is bounded by `max_object_events`. Overflow aborts a barrier run
before feedback is lost. Async mode reports a gap and requires an atomic full
snapshot with `{"op":"events","after":0,"resync":true}`.

## What it costs

The barrier alone is about 40 µs per slot. The old full map costs 897 µs with
100 retained tags on one UE and 2381 µs with 60 tags on each of four UEs.
`events` costs 80 and 51 µs respectively: close to the barrier floor and
independent of the retained map.

The real end-to-end path gives the same answer. One UE moving 3 MB falls from
2.241 to 0.170 s and from 25.08 to 0.464 MB of replies. Four UEs moving 1 MB
each finish in the same 630 slots but fall from 3.700 to 0.266 s and from
42.90 to 0.805 MB. Every run conserves bytes exactly.

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

The same backlog used to compound the cost by growing the per-slot object map.
`events` removes that serialization term, but it cannot make the emulator's expiry
scan cheaper; bounding the receive window remains part of a healthy experiment.

**`max_cmds_per_tick` is for async and real-time control only.** It bounds how much
work one wall-clock TTI accepts before deferring the rest. Barrier mode is necessarily
fast mode — requesting it with `period > 0` degrades to async — and the client owns
the clock there, so the emulator applies every command addressed to the granted TTI.
`FikoreLink` therefore neither overrides the cap nor imposes a duplicate local one.
