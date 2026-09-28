# The Clock, and Why One Slot at a Time

## The clock is not ours

Simulated time belongs to the emulator, which advances in 1 ms radio slots and, in
barrier mode, executes slot `n` only while `n <= credit_until_tti`. The harness owns
the credit, so it owns the clock, but it can only move it forwards a whole slot at a
time.

Everything in the model reads that clock and schedules against it. `env.now` becomes
`clock.tti`, and `yield env.timeout(d)` becomes `sched.after(d, callback)`. No
component keeps a wall clock, sleeps, or runs a second simulation engine. This is
the one structural change that porting ns.py's TCP requires, and it is why the
algorithms can be lifted while the plumbing cannot.

## Why the window has to be one slot

The pilot's harness synchronises every 10 ms because a video player reacts in
hundreds of milliseconds. A transport sender reacts inside the slot in which the
acknowledgement lands.

Consider a 10 ms window. The harness grants slots 0 to 9, and the emulator runs all
ten before answering. An acknowledgement that arrives at slot 3 would let the sender
transmit at slot 4, but slot 4 has already been executed by the time the harness
learns anything. The harness cannot schedule a command into the past, so the segment
would leave at slot 10: the model would run with an RTT inflated by up to a full
window, and the inflation would depend on where in the window the acknowledgement
happened to fall. That is not an approximation, it is a bias that moves with the
window size.

So the credit window is one slot. Within a slot the order is fixed:

1. read what the network reports as terminal up to `n`
2. receivers turn arrivals into acknowledgements
3. acknowledgements due at `n` reach their sender, and timers fire
4. senders are asked for their segments
5. the segments are handed over for slot `n+1`, and `n+1` is granted

Step 4 can only depend on what step 1 reported, so a segment sent in reaction to an
arrival always leaves at least one slot after it. That one slot is the model's host
turnaround, and it is the floor of the modelled RTT. Calling it zero would be
cheaper and wrong.

## What it costs

Measured on this machine, against `bin/fikore` in fast mode, 2000 slots per run
(`benchmarks/bench_lockstep.py`):

| Configuration | µs per slot | Projected overhead of a 300 s run |
| :-- | --: | --: |
| 1 UE, grant only | 40 | 12 s |
| 1 UE, grant + full `get`, 100 live objects | 897 | 269 s |
| 1 UE, grant + `events`, 100 live objects | 80 | 24 s |
| 4 UEs, grant only | 42 | 13 s |
| 4 UEs, grant + full `get`, 20 live objects per UE | 935 | 281 s |
| 4 UEs, grant + full `get`, 60 live objects per UE | 2381 | 714 s |
| 4 UEs, grant + `events`, 60 live objects per UE | 51 | 15 s |

Two conclusions. A slot per round trip is affordable: the lockstep itself costs
tens of microseconds. And `events` removes the wrong scaling term: feedback stays
close to the grant-only floor regardless of how many completed tags are retained,
because only counter movements cross the socket. [docs/03](03-link-and-protocol-requirements.md)
defines that protocol.

The real path confirms it. One UE moving 3 MB falls from 5465 to 416 µs per
slot; four UEs moving 1 MB each fall from 5873 to 422 µs, including the model's
own work and one compact telemetry snapshot per 10-TTI harness window.

## Skipping idle slots

The runner does not have to visit every slot. When no flow has anything outstanding
and the next scheduled event is at slot `m`, nothing can happen in between and the
credit can jump there: `Scheduler.next_tti()` exists for this. It matters for the
pilot, where a player often waits hundreds of milliseconds with an empty pipe.

While data is in flight the jump is not available, because the emulator knows when
the next delivery is and the harness does not. The clean fix belongs on the
emulator's side of the interface and is described as an optional extension in
[docs/03](03-link-and-protocol-requirements.md): let a grant stop early when an
object reaches a terminal state, so idle slots cost nothing without the harness
having to predict them.
