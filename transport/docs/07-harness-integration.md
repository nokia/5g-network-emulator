# Integration with an External Harness

The interface an external harness needs is small: submit a request for a number
of bytes, cancel it, advance, and receive progress, completion, cancellation and
telemetry events. `TransportBackend` implements exactly that, over either link.

The concrete contract used here is the `NetworkBackend` specified by VQEG's
CAP-CSP collaboration test pilot, which is where the names below come from. It
is a reasonable shape for any harness that hands over opaque byte objects, and
nothing in the transport model depends on that project.

## The mapping

| `NetworkBackend` | Here |
| :-- | :-- |
| `submit_request(ue_id, request_id, bytes_total)` | an `ObjectFlow` on that UE, `app_write(bytes_total)` |
| `cancel_request(ue_id, request_id)` | the application stops offering bytes; the pipe drains |
| `advance() -> NetworkStep` | run the internal slots of one harness window and return the events collected |
| `DownloadProgress.bytes_delivered` | the receiver's contiguous stream, `rcv_nxt` |
| `DownloadCompleted` | `rcv_nxt == bytes_total` |
| `NetworkTelemetry.rtt_ms` | the sender's smoothed RTT |
| `NetworkTelemetry.ce_rate` | CE marked segments over segments received |
| `NetworkTelemetry.retransmitted_bytes` | the sender's retransmissions |
| `set_ue_control(priority, rmax_mbps)` | `set` on the control protocol, unchanged |

**The two window sizes are independent.** The harness asks for 10 ms steps because
a player reacts in hundreds of milliseconds; the transport model runs ten internal
slots inside that call. The player sees the granularity it wants and TCP sees the
granularity it needs, and neither has to compromise. This is the piece that
reconciles [docs/02](02-clock-and-lockstep.md) with the pilot's synchronisation
window.

## Three open pilot questions that stop being questions

**The sender window.** The pilot proposes 128 KiB per UE, to be calibrated against
real HTTP traces, and notes that it collides with the PDCP delay budget under
congestion. With a transport model the in-flight bytes are `min(cwnd, rwnd)`, which
is what a real sender uses, and the calibration target moves from an invented
constant to two ordinary parameters: the receive window and the congestion control
algorithm. The 128 KiB figure becomes a receive window, which is a thing that
exists in the systems being modelled.

**Recovery of lost bytes.** The pilot assigns recovery to the adapter: read the
per-object dropped and expired counters and reinject that many bytes, so every
object completes at whatever delay the losses cost. That is a reasonable
approximation of what TCP would show the player, and it is also strictly more
optimistic than TCP, because it recovers immediately and in bulk while a real sender
needs a round trip to notice and then repairs at the pace its window allows. Here
recovery is retransmission, triggered by the sender's own inference, and the delay
it costs is measured rather than assumed.

**Concurrency across objects.** Round-robin injection bounded by an aggregate window
becomes several flows sharing a UE, each with its own window, competing in the same
queue. The fairness that results is TCP's, not a policy the harness chose.

## What changes in the experiment configuration

**The delay budget is no longer a workaround.** The pilot raises
`pkt_delay_budget_s` for driven UEs so the sender window is the only limiter, and
makes `expired_bytes_total == 0` an acceptance criterion, because the budget would
otherwise silently dominate the congested conditions. With a congestion window the
occupancy is self-limiting, so expiry becomes what it is in a real network: a
tail-drop signal the sender reacts to. The budget can then be left at a realistic
value and studied, and the measured runs show exactly that — a 20 ms budget produced
491 expiries that CUBIC detected and repaired without a single timeout.

Keeping the raised budget for the first comparison is still right, so that the
transport model and the injection backend can be compared with only one thing
different.

**`rtt_ms` gets a source.** The pilot leaves it `None` because the emulator measures
one-way latency and doubling it would fabricate a value. A transport model measures
the round trip the way a real client does, including the return path, the queueing
on it and the host turnaround, so the field can be filled honestly and the L2
signalling level gets a field it did not have.

**The uplink starts to matter.** Acknowledgements over the uplink cost radio
resources and queue there, and in a TDD configuration with a narrow uplink they
dominate: the same transfer measured 17 ms of RTT with modelled acknowledgements and
65 ms with real ones. Any condition used for comparisons has to have an uplink
provisioned for the acknowledgement stream, and that is now an experiment parameter
to record.

## What it does not change

The `NetworkBackend` interface, the player API, the signalling levels, the scoring,
and the pilot's event semantics. Those were designed around opaque byte requests and
timestamped delivery events, which is exactly what this produces.
