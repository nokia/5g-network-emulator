# Copyright 2026 Nokia
# Licensed under the BSD 3-Clause Clear License
# SPDX-License-Identifier: BSD-3-Clause-Clear
"""The request and delivery interface, checked against the loopback link."""
import os
import sys

from fikore_transport.backend import (BackendConfig, DownloadCancelled,
                                      DownloadCompleted, DownloadProgress,
                                      NetworkTelemetryReceived, TransportBackend,
                                      UeControl)
from fikore_transport.link import LoopbackConfig, LoopbackLink

MSS = 1500


def build(**kw):
    link = LoopbackLink(LoopbackConfig(rate_bps=20e6, owd_ttis=10, mss=MSS,
                                       queue_bytes=128 * 1024))
    return TransportBackend(link, BackendConfig(mss=MSS, **kw))


def drain(backend, steps):
    events = []
    for _ in range(steps):
        events.extend(backend.advance().events)
    return events


def test_request_completes_with_monotonic_progress():
    backend = build()
    backend.submit_request(0, "seg-1", 300 * 1024)
    events = drain(backend, 300)

    progress = [e for e in events if isinstance(e, DownloadProgress)]
    completed = [e for e in events if isinstance(e, DownloadCompleted)]
    assert len(completed) == 1, "exactly one completion"
    assert [p.bytes_delivered for p in progress] == sorted(
        p.bytes_delivered for p in progress), "progress must be cumulative"
    assert progress[-1].bytes_delivered == 300 * 1024
    assert completed[0].time_s >= progress[-1].time_s
    assert all(e.time_s <= backend.runner.clock.s for e in events)


def test_time_starts_at_zero_and_advances_monotonically():
    backend = build(window_ttis=10)
    backend.submit_request(0, "a", 50 * 1024)
    times = [backend.advance().time_s for _ in range(5)]
    assert times == sorted(times)
    assert abs(times[0] - 0.010) < 1e-9, times[0]


def test_concurrent_requests_on_one_ue_share_the_link():
    backend = build()
    for i in range(3):
        backend.submit_request(0, f"seg-{i}", 120 * 1024)
    events = drain(backend, 400)
    completed = {e.request_id for e in events if isinstance(e, DownloadCompleted)}
    assert completed == {"seg-0", "seg-1", "seg-2"}


def test_cancellation_reports_the_drained_tail():
    backend = build()
    backend.submit_request(0, "big", 4 * 1024 * 1024)
    drain(backend, 20)
    backend.cancel_request(0, "big")
    events = drain(backend, 200)

    cancelled = [e for e in events if isinstance(e, DownloadCancelled)]
    assert len(cancelled) == 1
    request = backend.requests[(0, "big")]
    assert request.flow.in_network == 0, "the tail must have drained"
    assert 0 < cancelled[0].bytes_delivered < 4 * 1024 * 1024
    # Nothing more is emitted for a closed request.
    assert not [e for e in drain(backend, 20)
                if getattr(e, "request_id", None) == "big"]


def test_cancelling_an_unknown_or_finished_request_is_a_no_op():
    backend = build()
    backend.cancel_request(0, "nope")
    backend.submit_request(0, "x", 20 * 1024)
    drain(backend, 200)
    backend.cancel_request(0, "x")
    backend.cancel_request(0, "x")


def test_telemetry_carries_a_measured_round_trip():
    backend = build(telemetry_every_windows=5)
    backend.submit_request(0, "x", 400 * 1024)
    events = drain(backend, 60)
    telemetry = [e for e in events if isinstance(e, NetworkTelemetryReceived)]
    assert telemetry, "telemetry must be emitted"
    rtts = [t.fields.rtt_ms for t in telemetry if t.fields.rtt_ms]
    assert rtts and all(10.0 <= r <= 80.0 for r in rtts), rtts


def test_close_is_idempotent_and_control_is_accepted():
    backend = build()
    backend.set_ue_control(0, UeControl(priority=4.0, rmax_mbps=10.0))
    backend.close()
    backend.close()


def test_completed_objects_do_not_remain_in_the_per_tti_flow_registry():
    backend = build()
    for index in range(100):
        backend.submit_request(0, f"seg-{index}", 30 * 1024)
        for _ in range(100):
            events = backend.advance().events
            if any(isinstance(event, DownloadCompleted) for event in events):
                break
        else:
            raise AssertionError(f"seg-{index} did not complete")
        assert not backend.runner.flows
        assert not backend._active_requests
    # History remains available to callers without being visited on every slot.
    assert len(backend.requests) == 100


if __name__ == "__main__":
    failed = 0
    for name, fn in sorted(list(globals().items())):
        if name.startswith("test_") and callable(fn):
            try:
                fn()
                print(f"ok   {name}")
            except AssertionError as exc:
                failed += 1
                print(f"FAIL {name}: {exc}")
    sys.exit(1 if failed else 0)
