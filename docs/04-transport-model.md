# The Transport Model

## What was taken from ns.py, and what was not

ns.py's `TCPPacketGenerator`, `TCPSink` and congestion-control classes are a good
starting point and a bad finishing point. The algorithms are worth lifting; the
structure is coupled to SimPy and the state machine has gaps that matter at 5G
rates.

| ns.py | Here | Why |
| :-- | :-- | :-- |
| `TCPPacketGenerator.run()`, a SimPy generator with `yield env.timeout` | `TcpSender.send_window()`, called once per slot | the clock belongs to the emulator; a generator cannot be driven a slot at a time |
| `simpy.Store` as the "window available" signal | the runner asks every flow every slot | the store accumulates stale tokens; asking is simpler and has no state |
| one `Timer` per in-flight segment | one retransmission timer per flow | RFC 6298 has one timer, restarted on a new ack and on expiry |
| `rto = max(1.0, srtt + 4·rttvar)` | floor of 200 ms, ceiling of 60 s | a 1 s floor hides every recovery at a 20 ms RTT |
| RTT sample only when `dupack == 0`, retransmitted copies not handled | timestamp echo on every segment | the echo is what removes the retransmission ambiguity; Karn's algorithm is then unnecessary |
| cumulative acknowledgement only | selective acknowledgement, three blocks | without it a burst of losses costs one round trip per segment; measured below |
| dupack counting with an ad-hoc `dupack > 3` branch | RFC 6582 partial acks, RFC 6675 `IsLost` | several losses in one window otherwise cost one timeout each |
| no receive window | `rwnd`, default 4 MB | otherwise the window grows until something drops |
| no pacing | Linux pacing, 2× then 1.25× | a slot is the smallest instant there is, so an unpaced sender emits the whole window at once |
| `TCPSink` merges received ranges | same, and the ranges are the SACK blocks | the receiver already had to track them |

The congestion-control algorithms themselves are ns.py's, kept deliberately close
to the source so they can be diffed against it: Reno is four lines, CUBIC keeps the
Linux constants, the epoch handling and TCP friendliness.

## The sender

State: `snd_una` first unacknowledged byte, `snd_nxt` next byte to send, `snd_high`
highest byte ever sent. The three differ after a timeout, which rewinds `snd_nxt` to
`snd_una` while `snd_high` stays put; the walk from `snd_nxt` to `snd_high` is the
resend, and new data starts at `snd_high`.

The pipe estimate is RFC 6675's: `snd_nxt - snd_una - sacked_bytes`. Discounting
selectively acknowledged bytes is what opens room during recovery so that
retransmissions and new data can flow while holes are repaired.

Per slot, `send_window()`:

1. takes the budget, `min(cwnd, rwnd) - pipe`, capped by the pacing rate
2. raises it to one segment if a retransmission is pending, because a retransmission
   replaces bytes already counted as in flight rather than adding new ones — holding
   it back until the window opens is what deadlocks a recovery
3. emits retransmissions first, then the rewound range, then new data
4. arms the retransmission timer if anything went out

## Why pacing is not optional

A slot is the smallest instant that exists in this model, so an unpaced sender hands
its entire window over in one slot. At a 300-segment window that is 450 KB arriving
together, which no host attached to a 20 Mbps link can produce, and the queue it
builds is an artefact of the quantisation rather than of the protocol. The pacing
rate is Linux's: twice the window per round trip while `cwnd <= ssthresh`, 1.25
afterwards, with a floor of two segments per slot so a flow with no RTT sample yet
can start.

## Why SACK is not optional either

Measured on the loopback link, 2 MB over 20 Mbps with a 20 ms round trip and a
256 KB queue, where slow-start overshoot drops 193 segments:

| Sender | Goodput | Retransmits | Timeouts |
| :-- | --: | --: | --: |
| Cumulative acks only, RFC 6582 recovery | 6.3 Mbps | 193 | 0 |
| With SACK, RFC 6675 `IsLost` | 15.5 Mbps | 193 | 0 |

Without SACK the sender repairs exactly one hole per round trip, so 193 losses cost
2.3 seconds of recovery. Every stack in service has had SACK on by default for two
decades, and a video experiment run without it would be measuring a protocol nobody
uses, in precisely the stall-versus-quality regime the pilot is built to measure.

The `IsLost` rule matters as much as the blocks. An earlier version retransmitted
everything below the highest selectively acknowledged byte and issued 1151
retransmissions for 608 real drops; requiring three acknowledged segments above a
hole before calling it lost brought retransmissions to exactly the number of drops.

## The receiver

Cumulative acknowledgement from merged ranges, the ranges above the contiguous
prefix reported as up to three SACK blocks, and the counters scalable congestion
control needs: segments received and segments CE marked, both cumulative.

It acknowledges every arrival. Delayed acknowledgements and ACK thinning are not
modelled yet and they matter once acknowledgements cross the emulated uplink, which
is [docs/08](08-validation-and-roadmap.md)'s problem.

## The acknowledgement path

Two modes. Modelled, where the acknowledgement reaches the sender after a
configurable number of slots and does not consume radio resources; and over the
link, where it is injected as 40 bytes on the return direction with its own tag, so
it queues and is scheduled like anything else.

The second is the faithful one and it changes the result. The same 1 MB transfer
measured 17 ms of smoothed RTT with modelled acknowledgements and 65 ms with
acknowledgements over the uplink of the demo configuration, because that uplink is
narrow and the acknowledgements queue behind each other. That is a real effect in a
TDD cell, and it is also a warning: an uplink that cannot carry the acknowledgement
stream will dominate any result, so the configuration has to be checked before the
numbers mean anything.

## Known gaps

- Spurious retransmissions remain in heavily overloaded runs, where a retransmission
  is itself dropped and a later recovery episode resends segments the first one
  already handled. The per-episode guard is an approximation of Linux's per-segment
  retransmission bookkeeping.
- No delayed acknowledgements, no Nagle, no window scaling arithmetic, no
  three-way handshake, so the first slot of a flow is one round trip cheaper than a
  real connection. For bulk transfers this is invisible; for 200 kB video segments
  it is not, and a configurable handshake cost belongs in the object generator
  rather than in the sender.
- A 5 ms delay budget, which is below the achievable one-way delay, makes every
  segment expire and the run degenerates rather than failing cleanly.
