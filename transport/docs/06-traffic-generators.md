# Traffic Generators

Applications sit above the transport model and decide only *what* to send and
*when*. They never decide how fast.

## The application interface

```python
sender.app_write(nbytes)     # this many bytes are now available
sender.set_unlimited()       # data is always available
```

That is the whole contract. The achieved bitrate emerges from the window, the RTT,
the acknowledgement stream, loss, timeouts, marks and the algorithm — which is the
point, because it is what makes the generated traffic react to the conditions the
emulator creates.

## iperf3, in TCP mode

An `iperf3 -t` sender is an application with data continuously available for a
configured period, so it is `set_unlimited()` plus a stop time. The parameters worth
reproducing map cleanly:

| iperf3 | Here |
| :-- | :-- |
| `-t` duration | stop time in slots |
| `-P` parallel streams | one flow, sender and receiver, per stream, all on one UE |
| `-l` block size | how much the application makes available at a time; unlimited by default |
| `-R` reverse | the flow's data direction becomes `ul` and its acknowledgements `dl` |
| `-i` interval | the reporting period of the recorder |
| `-C` congestion control | which `CongestionControl` the flow is built with |

No control protocol, no handshake, no real datagrams. The value of the resemblance
is that the same parameters can be given to real `iperf3` through the emulator's
real-traffic path and the two outputs compared; see
[docs/08](08-validation-and-roadmap.md).

## iperf3, in UDP mode

Much simpler, and the control case: no window, no acknowledgements, no reaction.
`udp.py` is `UdpSource`, which turns a rate and a datagram size into a schedule,
and `UdpSink`, which measures what arrives. `udp_flow()` builds both, and the
parameters are `iperf3 -u`'s: `-b` the rate, `-l` the datagram size, `-t` the
duration.

The sink reports only what a receiver can observe: delivery, loss inferred from
gaps in the sequence, one-way delay, RFC 3550 interarrival jitter and CE marks. It
does not read the network's own counters, so its numbers can be compared against
them rather than being derived from them.

It is also the cheapest way to create background load with an exact rate, which the
pilot's congested conditions need.

## Without a transport at all

`ideal.py` is injection with no transport at all, kept as something to compare
against: a fixed window per UE shared by the objects on it, and whatever the
network reports as lost is handed over again. No congestion window, no
acknowledgements, no round trip, no timers.

It is strictly more optimistic than any real sender, in two ways that matter. It is
told about a loss the instant the network reports it, where a sender needs a round
trip to suspect one; and it re-sends everything at once, where a sender is limited
by a window it has just reduced. Both are visible in one measurement: over the same
bottleneck, the same 300 kB object finishes in 0.14 s against the transport model's
0.23 s, and costs 2503 dropped segments against 46. The injection rule wins the
stopwatch by spending radio that a real sender does not have.

`BackendConfig.transport = "ideal"` selects it, so an A/B is one parameter and
everything else about the run is held fixed.

## Objects

The generator the harness actually needs is neither of those: a request for an
object of known size, which starts, progresses and finishes, with several of them
concurrently per UE. It is built, and it is the pilot's own interface rather than a
new one — `TransportBackend.submit_request(ue_id, request_id, bytes_total)`, which
creates a connection on that UE and hands the application `bytes_total`. See
[docs/07](07-harness-integration.md).

One connection per object is the default, which is what a player issuing a fresh
request per segment gets. Sharing one connection across a queue of objects is the
other arrangement worth studying; it is an experiment parameter rather than a
design decision, because the mapping from flow to UE is explicit.

Cancellation stops the application from offering more bytes, stops retransmissions,
and lets the tail already in the network drain. Nothing else is needed: the model
knows exactly which segments are outstanding, so the waste a cancellation costs is
counted rather than estimated, and the request is only reported cancelled once that
tail has actually reached a terminal state.

## Reporting

Per interval and per flow, in simulation time: bytes sent and acknowledged,
goodput, retransmissions, timeouts, fast retransmits, partial acknowledgements, the
smoothed RTT and its variance, the current window and pacing rate, CE marks, and
the emulator's own per-flow delivered, dropped and expired counters alongside them.

The last column is the important one. The model's view and the network's view are
collected separately and compared, which is what makes the difference between them
measurable instead of assumed.
