# Application Patterns — As Built

**Status:** implemented application-facing operations
**Code:** `fikore_transport/tcp.py`, `fikore_transport/udp.py`,
`fikore_transport/backend.py`
**Tests:** `test_loopback_transfer.py`, `test_transports.py`,
`test_backend.py`, `test_fikore_link.py`

This document names only application patterns that exist in the current code.
There is no standalone `iperf3`-like command-line tool.

## Stream writes

`TcpSender.app_write(nbytes)` adds a finite number of application bytes:

```python
sender = TcpSender(
    flow=1,
    cc=Cubic(mss=1500),
    clock=runner.clock,
    sched=runner.sched,
    mss=1500,
)
runner.add_flow(Flow(sender, TcpReceiver(flow=1, mss=1500)))
sender.app_write(8 * 1024 * 1024)
runner.run_until(10_000)
```

The sender applies congestion control, receiver-window limits, pacing,
retransmission and acknowledgement processing. This does not open a real socket
or read a host file.

For a saturated bulk stream, `TcpSender.set_unlimited()` keeps application data
available until the caller stops the run.

## UDP offered load

`udp_flow()` creates a `Flow`, `UdpSource` and `UdpSink` for Runner-level
open-loop traffic:

```python
flow, source, sink = udp_flow(
    flow=1,
    clock=runner.clock,
    sched=runner.sched,
    rate_mbps=10.0,
    duration_ttis=1_000,
)
runner.add_flow(flow)
runner.run_until(1_100)
report = sink.report()
```

This UDP primitive is not exposed through `TransportBackend`. It has no
congestion control or delivery recovery.

## Ideal transport

`BackendConfig(transport="ideal")` selects the diagnostic fixed-window
transport. Its `IdealSender` sends against a `SharedWindow` and can recover from
reported terminal Link outcomes immediately.

It is used to separate network capacity from TCP controller dynamics; it is not
a TCP variant or deployable application protocol.

## NetworkBackend objects

The harness-facing interface is `TransportBackend`:

```python
backend = TransportBackend(link, BackendConfig(
    window_ttis=10,
    horizon_ttis=5_000,
))
backend.submit_request(ue_id=0, request_id="video-3-segment-7",
                       bytes_total=512_000)

step = backend.advance()  # initial step at t=0
step = backend.advance()  # advances one configured window
backend.cancel_request(ue_id=0, request_id="video-3-segment-7")
```

Direction, ECN, receive window and controller are backend configuration fields.
An accepted object receives its own transport flow. The backend maps flow
progress to:

- `DownloadProgress`;
- `DownloadCompleted`;
- `DownloadCancelled`;
- `NetworkTelemetryReceived`.

The request ID belongs to the application; segment tags belong to the Link.
Neither is inferred from the other.

## Object cancellation

Cancellation stops new application data. Segments already submitted to the Link
remain attributed to the request and form its cancellation tail. The backend
emits `DownloadCancelled` after `flow.in_network` reaches zero, then retires the
flow after any Link-carried ACK tail also drains.

This distinction is required for SFV wastage accounting: cancellation does not
erase bytes that already consumed network resources.

## Multi-UE operation

Multiple request flows can share one Link and emulator cell. Every
`TransportBackend.advance()` ticks all active flows on one Runner clock.
`FikoreLink` can map logical backend UE identifiers to sparse physical emulator
indices or textual configured targets.

## Available entry points

| Need | Current entry point |
| :-- | :-- |
| Saturated TCP flow | `TcpSender.set_unlimited()` |
| Finite TCP stream write | `TcpSender.app_write()` |
| Open-loop UDP | `udp_flow()` |
| Ideal object baseline | `BackendConfig(transport="ideal")` |
| Finite downloadable object | `TransportBackend.submit_request()` |
| Cancel downloadable object | `TransportBackend.cancel_request()` |
| SFV integration | `benchmarks/validate_sfv.py` |

Options such as `iperf3 -t`, `-P`, `-l`, `-R` and `-i` are not implemented
interfaces in this repository. Planned application tooling belongs in
[`FUTURE-ROADMAP.md`](FUTURE-ROADMAP.md); current behavioural constraints belong
in [`LIMITATIONS.md`](LIMITATIONS.md).
