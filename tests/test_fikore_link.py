"""The integration, exercised against a real FikoRE process.

The loopback tests say whether the transport state machine is right. These say
whether the link that drives the emulator is right, which is a different question
and mostly a question about the control protocol: framing, lockstep, tag lifetime,
and whether a byte handed over is still accounted for at the end.

They need `bin/fikore`. Where there is no emulator they skip rather than fail, so
the loopback suite stays the one that has to pass everywhere.
"""
import os
import shutil
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from fikore_transport.backend import (BackendConfig, DownloadCompleted,
                                      TransportBackend)
from fikore_transport.cc import Cubic
from fikore_transport.emulator import Emulator, EmulatorConfig
from fikore_transport.fikore_link import FikoreLink
from fikore_transport.link import Transmit

EMU = os.environ.get("FIKORE_DIR", "/home/pablop/devel/fikore/5g-network-emulator")
BINARY = os.path.join(EMU, "bin/fikore")
BASE_INI = os.path.join(EMU, "config/control_demo.ini")


class Skipped(Exception):
    """Not a failure. There is no emulator on this machine."""


def build_link(n_ues=1, duration_s=6.0, delay_budget_s=0.3, **kw):
    if not os.path.exists(BINARY) or not os.path.exists(BASE_INI):
        raise Skipped(f"no emulator at {BINARY}; set FIKORE_DIR to run these")
    work = tempfile.mkdtemp(prefix="fikore-test-")
    cfg = EmulatorConfig(binary=BINARY, base_ini=BASE_INI,
                         socket_path=os.path.join(work, "control.sock"),
                         duration_s=duration_s, work_dir=EMU, n_ues=n_ues,
                         delay_budget_s=delay_budget_s,
                         log_path=os.path.join(work, "emulator.log"), **kw)
    link = FikoreLink(Emulator(cfg), flow_to_ue={})
    link._work_dir = work
    return link


def teardown(link, backend=None):
    (backend or link).close()
    shutil.rmtree(getattr(link, "_work_dir", ""), ignore_errors=True)


def run_backend(link, requests, max_windows, **kw):
    """Submit one request per (ue, id, size) and advance until all complete."""
    kw.setdefault("rwnd", 256 * 1024)
    backend = TransportBackend(link, BackendConfig(window_ttis=10, cc_factory=Cubic,
                                                   **kw))
    for ue, name, size in requests:
        backend.submit_request(ue, name, size)
    done = {}
    for _ in range(max_windows):
        for event in backend.advance().events:
            if isinstance(event, DownloadCompleted):
                done[event.request_id] = event.time_s
        if len(done) == len(requests):
            break
    return backend, done


def accounted(link):
    return sum(link.terminal_bytes.values()) + link.in_flight_bytes


def test_a_real_transfer_completes_and_conserves_bytes():
    size = 400 * 1024
    link = build_link()
    try:
        backend, done = run_backend(link, [(0, "seg", size)], 600)
        receiver = backend.requests[(0, "seg")].flow.receiver
        assert done, "the transfer never completed"
        assert receiver.rcv_nxt == size, f"{receiver.rcv_nxt} of {size} arrived"
        # Every byte handed to the link is delivered, lost, or still in the
        # emulator. Nothing may simply stop being accounted for.
        assert link.submitted_bytes >= size
        assert accounted(link) == link.submitted_bytes, (
            f"{accounted(link)} accounted of {link.submitted_bytes} submitted")
        assert link.terminal_bytes["delivered"] >= size
        assert link.round_trips > 0
    finally:
        teardown(link, backend)


def test_the_emulator_and_the_link_agree_on_the_bytes():
    """The link's account is its own bookkeeping; this is the emulator's."""
    link = build_link()
    try:
        backend, _ = run_backend(link, [(0, "seg", 300 * 1024)], 600)
        state = link.last_state[(0, "dl")]
        injected = state["injected_bytes_total"]
        assert abs(injected - link.submitted_bytes) < 1.0, (
            f"emulator took {injected}, the link sent {link.submitted_bytes}")
        settled = (state["delivered_bytes_total"] + state["dropped_bytes_total"]
                   + state["expired_bytes_total"] + state["pending_bytes"])
        assert settled <= injected + 1.0, f"{settled} settled of {injected}"
        # The drops are broken down, and the breakdown adds up to the sum.
        assert abs(state["dropped_bytes_total"]
                   - state["queue_dropped_bytes_total"]
                   - state["radio_dropped_bytes_total"]) < 1.0
    finally:
        teardown(link, backend)


def test_arrivals_come_back_on_the_slots_that_injected():
    """The regression test for the control framing.

    The emulator answers one acknowledgement per command, all carrying the
    message's id. A client that reads one per id closes the batch on the first
    inject's reply, which has no result, so a slot that injected anything reported
    no arrivals at all and the run learned everything one idle slot late.
    """
    link = build_link()
    link.register_flow(1, 0)
    try:
        slots = 150
        slots_with_arrivals = 0
        for tti in range(slots):
            link.submit([Transmit(flow=1, seq=tti, size=1500)], at_tti=tti + 1)
            if link.step(tti):
                slots_with_arrivals += 1
        # One segment goes in on every slot but the first and comes back a few
        # slots later, so almost every slot must report something.
        assert slots_with_arrivals > 100, (
            f"only {slots_with_arrivals} of {slots} slots reported an arrival "
            f"while injecting on every one of them")
        assert link.terminal_bytes["delivered"] > 100 * 1500
    finally:
        teardown(link)


def test_two_ues_share_the_cell_and_both_make_progress():
    link = build_link(n_ues=2, duration_s=8.0)
    try:
        backend = TransportBackend(link, BackendConfig(window_ttis=10,
                                                       rwnd=256 * 1024,
                                                       cc_factory=Cubic))
        backend.submit_request(0, "a", 300 * 1024)
        backend.submit_request(1, "b", 300 * 1024)
        flows = [backend.requests[(0, "a")].flow, backend.requests[(1, "b")].flow]

        done, last, together = {}, [0, 0], 0
        for _ in range(700):
            for event in backend.advance().events:
                if isinstance(event, DownloadCompleted):
                    done[event.request_id] = event.time_s
            now = [f.receiver.rcv_nxt for f in flows]
            if now[0] > last[0] and now[1] > last[1]:
                together += 1
            last = now
            if len(done) == 2:
                break

        assert set(done) == {"a", "b"}, f"only {sorted(done)} finished"
        assert together >= 2, (
            f"the two never advanced in the same window ({together}), so one was "
            f"waiting for the other rather than sharing the cell")
        for ue in (0, 1):
            assert link.last_state[(ue, "dl")]["delivered_bytes_total"] > 0
        assert accounted(link) == link.submitted_bytes
    finally:
        teardown(link, backend)


def test_loss_is_reported_with_a_cause_and_the_transfer_still_completes():
    """A delay budget far below the standing queue makes the emulator discard,
    which is the only path on which the fate and the cause are decided."""
    link = build_link(duration_s=12.0, delay_budget_s=0.02)
    try:
        backend, done = run_backend(link, [(0, "bulk", 3 * 1024 * 1024)], 1100,
                                    rwnd=512 * 1024)
        lost = [a for a in backend.runner.arrivals if a.fate != "delivered"]
        assert lost, "the delay budget did not manage to lose anything"
        assert all(a.cause for a in lost), "a loss with no cause"
        assert all(a.cause in ("queue", "radio", "budget") for a in lost)
        assert sum(link.lost_bytes_by_cause.values()) == sum(a.size for a in lost)
        assert backend.requests[(0, "bulk")].flow.sender.stats.retransmits > 0
        assert done, "the transfer did not survive the losses"
        assert accounted(link) == link.submitted_bytes
    finally:
        teardown(link, backend)


if __name__ == "__main__":
    failed = 0
    for name, fn in sorted(list(globals().items())):
        if name.startswith("test_") and callable(fn):
            try:
                fn()
                print(f"ok   {name}")
            except Skipped as exc:
                print(f"skip {name}: {exc}")
            except AssertionError as exc:
                failed += 1
                print(f"FAIL {name}: {exc}")
    sys.exit(1 if failed else 0)
