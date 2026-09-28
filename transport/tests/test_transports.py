# Copyright 2026 Nokia
# Licensed under the BSD 3-Clause Clear License
# SPDX-License-Identifier: BSD-3-Clause-Clear
"""The other two transports: the injection rule, and open-loop datagrams."""
import os
import sys

from fikore_transport.backend import (BackendConfig, DownloadCompleted,
                                      TransportBackend)
from fikore_transport.ideal import IdealReceiver, IdealSender, SharedWindow
from fikore_transport.link import LoopbackConfig, LoopbackLink
from fikore_transport.runner import Flow, Runner
from fikore_transport.udp import udp_flow

MSS = 1500


def link(rate_bps=20e6, queue=64 * 1024, owd=10, **kw):
    return LoopbackLink(LoopbackConfig(rate_bps=rate_bps, owd_ttis=owd, mss=MSS,
                                       queue_bytes=queue, **kw))


# -- the injection rule ------------------------------------------------------------

def test_ideal_transfer_completes_and_respects_the_window():
    lk = link()
    runner = Runner(lk)
    window = SharedWindow(128 * 1024)
    sender = IdealSender(1, runner.clock, runner.sched, MSS, window)
    runner.add_flow(Flow(sender, IdealReceiver(1, MSS, sender)))
    sender.app_write(400 * 1024)

    peak = 0
    for _ in range(3000):
        runner.tick()
        peak = max(peak, window.in_flight)
        if sender.complete():
            break
    assert sender.delivered == 400 * 1024
    assert peak <= 128 * 1024, f"the window was exceeded: {peak}"


def test_the_window_is_shared_between_objects_of_one_ue():
    lk = link()
    runner = Runner(lk)
    window = SharedWindow(128 * 1024)
    senders = []
    for i in range(1, 4):
        sender = IdealSender(i, runner.clock, runner.sched, MSS, window)
        runner.add_flow(Flow(sender, IdealReceiver(i, MSS, sender)))
        sender.app_write(100 * 1024)
        senders.append(sender)

    peak = 0
    for _ in range(3000):
        runner.tick()
        peak = max(peak, window.in_flight)
        if all(s.complete() for s in senders):
            break
    assert all(s.delivered == 100 * 1024 for s in senders)
    assert peak <= 128 * 1024, f"three objects together exceeded the window: {peak}"


def test_without_recovery_a_lossy_transfer_never_completes():
    lk = link(queue=16 * 1024)
    runner = Runner(lk)
    sender = IdealSender(1, runner.clock, runner.sched, MSS,
                         SharedWindow(128 * 1024), recover=False)
    runner.add_flow(Flow(sender, IdealReceiver(1, MSS, sender)))
    sender.app_write(300 * 1024)
    for _ in range(2000):
        runner.tick()
    assert lk.dropped > 0
    assert sender.delivered < 300 * 1024, "without recovery the bytes are gone"


def test_ideal_beats_tcp_by_wasting_the_radio():
    """The comparison the ideal transport exists for."""
    results = {}
    for transport in ("tcp", "ideal"):
        lk = link()
        backend = TransportBackend(lk, BackendConfig(mss=MSS, transport=transport))
        backend.submit_request(0, "seg", 300 * 1024)
        finished = None
        for _ in range(400):
            step = backend.advance()
            done = [e for e in step.events if isinstance(e, DownloadCompleted)]
            if done:
                finished = done[0].time_s
                break
        sender = backend.requests[(0, "seg")].flow.sender
        results[transport] = (finished, lk.dropped, sender.stats.retransmits)

    assert all(r[0] is not None for r in results.values()), results
    assert results["ideal"][0] <= results["tcp"][0], "the injection rule is optimistic"
    assert results["ideal"][1] > results["tcp"][1] * 5, (
        "and it pays for it in wasted transmissions", results)


# -- open loop ---------------------------------------------------------------------

def test_udp_rate_is_what_was_asked_for():
    lk = link(rate_bps=50e6)
    runner = Runner(lk)
    flow, source, sink = udp_flow(1, runner.clock, runner.sched, rate_mbps=10.0,
                                  duration_ttis=1000)
    runner.add_flow(flow)
    runner.run_until(1100)
    sent_mbps = source.stats.bytes_sent * 8 / 1.0 / 1e6
    assert 9.5 <= sent_mbps <= 10.5, sent_mbps
    assert sink.lost == 0, "an uncongested link should lose nothing"
    assert sink.received == source.stats.datagrams_sent


def test_udp_measures_delay_and_jitter():
    lk = link(rate_bps=50e6, owd=20)
    runner = Runner(lk)
    flow, _, sink = udp_flow(1, runner.clock, runner.sched, rate_mbps=5.0,
                             duration_ttis=500)
    runner.add_flow(flow)
    runner.run_until(600)
    report = sink.report()
    assert 20.0 <= report["owd_ms_median"] <= 25.0, report
    assert report["jitter_ms"] < 2.0, report


def test_udp_over_the_capacity_loses_and_reports_it():
    lk = link(rate_bps=5e6, queue=16 * 1024)
    runner = Runner(lk)
    flow, source, sink = udp_flow(1, runner.clock, runner.sched, rate_mbps=20.0,
                                  duration_ttis=500)
    runner.add_flow(flow)
    runner.run_until(700)
    assert sink.lost > 0
    assert 0.6 < sink.loss_ratio < 0.85, sink.report()
    assert sink.received + sink.lost == sink.highest_seq + 1


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
