# Scope and the Boundary

## What is being modelled

The mechanisms that determine when bytes move, and how many:

- segmentation of application data into abstract packets
- the congestion window, and its response to loss and to congestion marks
- acknowledgements, cumulative and selective
- RTT and RTO estimation
- retransmission
- ECN feedback, for scalable congestion control
- applications that either have data continuously available or hand over objects

## What is not

No headers, no checksums, no socket API, no three-way handshake, no FIN or RST, no
kernel buffers, no byte streams. A segment is a record:

```python
Transmit(flow, seq, size, direction, ecn, ts_us, kind)
```

`seq` is the first byte of the segment in the stream, which doubles as its
identifier; `ts_us` is the timestamp that comes back in the acknowledgement. That
is the whole wire format, and it exists only inside Python.

## The boundary

Everything below the `Link` interface is the network's business:

| The network decides | The transport model decides |
| :-- | :-- |
| queueing and queue limits | how many bytes are outstanding |
| scheduling between UEs | which bytes go next |
| radio conditions and capacity | when to give up on a segment |
| propagation and backhaul delay | what the RTT is |
| dropping, expiring, CE marking | how to respond to a drop or a mark |

The split is not negotiable in one direction: the model must never read a loss
counter to decide that a segment was lost.

## Loss is discovered, not announced

The interface reports a terminal outcome per segment, but only the **receiver**
consumes it. The sender sees acknowledgements and its own timers, nothing else. A
segment the network drops simply produces no acknowledgement, and the sender reacts
the way a real one does: three selective acknowledgements above the hole, or a
timeout.

This is worth the discipline it costs. A sender told directly that a segment was
lost reacts one round trip earlier than any real sender can, and the whole point of
modelling transport rather than approximating it is the timing of that reaction.

It also leaves a free instrument: the emulator's own per-object loss counters are
ground truth that the model never sees, so comparing the sender's retransmissions
against them is a real test. In the measured run above, Reno retransmitted 232
segments and the emulator reported 232 expired. Any excess would be the sender
retransmitting on reordering, and there is a version of this code that did exactly
that; see [docs/04](04-transport-model.md).

## Segment size

The MSS must be the emulator's IP packet size, which it reports as
`state.pkt_size_bits`. Then one segment is one IP packet, and loss and CE marking
apply to whole segments.

If the MSS were larger, the emulator would split a segment into several packets, and
a segment could come back half delivered. Partial delivery has no meaning in TCP —
the segment is lost and the delivered part is waste — so the model would have to
discard real work and the accounting would get harder for nothing. The link checks
the two agree at startup.
