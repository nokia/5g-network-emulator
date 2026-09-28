# Copyright 2026 Nokia
# Licensed under the BSD 3-Clause Clear License
# SPDX-License-Identifier: BSD-3-Clause-Clear
"""A request and delivery interface over the transport model.

A caller submits opaque byte requests and reads timestamped delivery events. The
difference from handing bytes straight to the network is that they travel over a
modelled TCP connection: the window bounds what is outstanding, losses are
repaired by retransmission, and the round-trip time is measured rather than left
empty.

Two clocks meet in `advance()`. The harness asks for one step of its own window,
usually 10 ms, and the model runs that window one slot at a time underneath. The
player sees the granularity it wants and the transport sees the granularity it
needs.
"""
from __future__ import annotations

from dataclasses import dataclass
from typing import Iterable, TypeAlias

from .cc import CongestionControl, Cubic
from .ideal import IdealReceiver, IdealSender, SharedWindow
from .link import Direction, Ecn, Link
from .runner import Flow, Runner
from .tcp import TcpReceiver, TcpSender


@dataclass(frozen=True)
class DownloadProgress:
    ue_id: int
    request_id: str
    bytes_delivered: int
    time_s: float


@dataclass(frozen=True)
class DownloadCompleted:
    ue_id: int
    request_id: str
    time_s: float


@dataclass(frozen=True)
class DownloadCancelled:
    ue_id: int
    request_id: str
    bytes_delivered: int
    time_s: float


@dataclass(frozen=True)
class NetworkTelemetry:
    throughput_mbps: float | None = None
    rtt_ms: float | None = None
    ip_latency_ms: float | None = None
    pdcp_latency_ms: float | None = None
    ce_rate: float | None = None
    drop_rate: float | None = None
    retransmitted_bytes: int | None = None
    sinr_db: float | None = None
    queue_bytes: int | None = None


@dataclass(frozen=True)
class NetworkTelemetryReceived:
    ue_id: int
    time_s: float
    fields: NetworkTelemetry


NetworkEvent: TypeAlias = (DownloadProgress | DownloadCompleted | DownloadCancelled
                           | NetworkTelemetryReceived)


@dataclass(frozen=True)
class NetworkStep:
    time_s: float
    events: list[NetworkEvent]
    is_final: bool


@dataclass(frozen=True)
class UeControl:
    priority: float | None = None
    rmax_mbps: float | None = None


@dataclass
class _Request:
    ue_id: int
    request_id: str
    bytes_total: int
    flow: Flow
    reported_bytes: int = 0
    cancelled: bool = False
    closed: bool = False


@dataclass
class BackendConfig:
    window_ttis: int = 10             # one harness step
    horizon_ttis: int | None = None   # when the run ends
    mss: int = 1500
    direction: Direction = "dl"
    ecn: Ecn = "not-ect"
    rwnd: int = 256 * 1024
    ack_over_link: bool = False
    telemetry_every_windows: int = 1
    # One connection per object is what a player using a fresh request per segment
    # gets. Sharing one connection across an object queue is the other arrangement
    # worth studying, and it is a parameter rather than a design decision.
    cc_factory: type[CongestionControl] = Cubic
    # "tcp" is the transport model; "ideal" is bare injection, a fixed
    # window per UE and instant recovery of whatever the network reports as lost.
    # Everything else being equal, the two differ only in this.
    transport: str = "tcp"
    ideal_window_bytes: int = 128 * 1024
    ideal_recover: bool = True


class TransportBackend:
    """`NetworkBackend` over a `Link`. Works with the loopback link or with FikoRE."""

    def __init__(self, link: Link, cfg: BackendConfig | None = None) -> None:
        self.cfg = cfg or BackendConfig()
        self.link = link
        self.cfg.mss = getattr(link, "mss", self.cfg.mss)
        self.runner = Runner(link)
        self.requests: dict[tuple[int, str], _Request] = {}
        self._by_flow: dict[int, _Request] = {}
        self._next_flow = 1
        self._windows = 0
        self._closed = False
        self._telemetry_baseline: dict[int, dict] = {}
        self._windows_by_ue: dict[int, SharedWindow] = {}

    # -- NetworkBackend -----------------------------------------------------------

    def submit_request(self, ue_id: int, request_id: str, bytes_total: int) -> None:
        key = (ue_id, request_id)
        if key in self.requests:
            raise ValueError(f"request {request_id} is already live on ue {ue_id}")
        flow_id = self._next_flow
        self._next_flow += 1
        register = getattr(self.link, "register_flow", None)
        if register is not None:
            register(flow_id, ue_id)
        if self.cfg.transport == "ideal":
            window = self._windows_by_ue.setdefault(
                ue_id, SharedWindow(self.cfg.ideal_window_bytes))
            sender = IdealSender(flow_id, self.runner.clock, self.runner.sched,
                                 self.cfg.mss, window, direction=self.cfg.direction,
                                 ecn=self.cfg.ecn, recover=self.cfg.ideal_recover)
            receiver = IdealReceiver(flow_id, self.cfg.mss, sender)
        else:
            cc = self.cfg.cc_factory(mss=self.cfg.mss, cwnd=10 * self.cfg.mss)
            sender = TcpSender(flow_id, cc, self.runner.clock, self.runner.sched,
                               self.cfg.mss, direction=self.cfg.direction,
                               ecn=self.cfg.ecn, rwnd=self.cfg.rwnd)
            receiver = TcpReceiver(flow_id, self.cfg.mss)
        flow = Flow(sender, receiver, ack_over_link=self.cfg.ack_over_link)
        self.runner.add_flow(flow)
        sender.app_write(bytes_total)
        request = _Request(ue_id, request_id, bytes_total, flow)
        self.requests[key] = request
        self._by_flow[flow_id] = request

    def cancel_request(self, ue_id: int, request_id: str) -> None:
        request = self.requests.get((ue_id, request_id))
        if request is None or request.closed or request.cancelled:
            return
        request.cancelled = True
        request.flow.sender.app_cancel()

    def advance(self) -> NetworkStep:
        if self._closed:
            raise RuntimeError("the backend is closed")
        for _ in range(self.cfg.window_ttis):
            self.runner.tick()
        self._windows += 1

        now_s = self.runner.clock.s
        events: list[NetworkEvent] = list(self._request_events(now_s))
        if self._windows % self.cfg.telemetry_every_windows == 0:
            events.extend(self._telemetry_events(now_s))

        horizon = self.cfg.horizon_ttis
        is_final = horizon is not None and self.runner.clock.tti >= horizon
        return NetworkStep(time_s=now_s, events=events, is_final=is_final)

    def close(self) -> None:
        if self._closed:
            return
        self._closed = True
        self.link.close()

    def set_ue_control(self, ue_id: int, control: UeControl) -> None:
        setter = getattr(self.link, "set_params", None)
        if setter is None:
            return
        params: dict[str, float] = {}
        if control.priority is not None:
            params["priority"] = control.priority
        if control.rmax_mbps is not None:
            params[f"{self.cfg.direction}.rmax_mbps"] = control.rmax_mbps
        if params:
            setter(ue_id, params)

    # -- events -------------------------------------------------------------------

    def _request_events(self, now_s: float) -> Iterable[NetworkEvent]:
        for request in list(self.requests.values()):
            if request.closed:
                continue
            delivered = request.flow.receiver.rcv_nxt
            if delivered != request.reported_bytes:
                request.reported_bytes = delivered
                yield DownloadProgress(request.ue_id, request.request_id, delivered,
                                       now_s)
            if request.cancelled:
                # The request stays live until the tail already in the network has
                # drained, so the reported waste is what actually travelled.
                if request.flow.in_network == 0:
                    request.closed = True
                    yield DownloadCancelled(request.ue_id, request.request_id,
                                            delivered, now_s)
            elif delivered >= request.bytes_total:
                request.closed = True
                yield DownloadCompleted(request.ue_id, request.request_id, now_s)

    def _telemetry_events(self, now_s: float) -> Iterable[NetworkEvent]:
        state = getattr(self.link, "last_state", None)
        for ue_id in sorted({r.ue_id for r in self.requests.values()}):
            yield NetworkTelemetryReceived(ue_id, now_s,
                                           self._telemetry_for(ue_id, state, now_s))

    def _telemetry_for(self, ue_id: int, state: dict | None,
                       now_s: float) -> NetworkTelemetry:
        live = [r for r in self.requests.values() if r.ue_id == ue_id and not r.closed]
        srtts = [r.flow.sender.srtt_us for r in live if r.flow.sender.srtt_us]
        rtt_ms = sum(srtts) / len(srtts) / 1000.0 if srtts else None
        retransmitted = sum(r.flow.sender.stats.retransmits * self.cfg.mss
                            for r in self.requests.values() if r.ue_id == ue_id)
        if state is None:
            return NetworkTelemetry(rtt_ms=rtt_ms, retransmitted_bytes=retransmitted)

        now = state.get((ue_id, self.cfg.direction), {})
        base = self._telemetry_baseline.get(ue_id, {})
        self._telemetry_baseline[ue_id] = dict(now)
        window_s = self.cfg.window_ttis * self.cfg.telemetry_every_windows / 1000.0

        def delta(key: str) -> float:
            return max(float(now.get(key, 0.0)) - float(base.get(key, 0.0)), 0.0)

        delivered = delta("delivered_bytes_total")
        lost = delta("dropped_bytes_total") + delta("expired_bytes_total")
        received_pkts = delivered / self.cfg.mss if self.cfg.mss else 0.0
        return NetworkTelemetry(
            throughput_mbps=delivered * 8 / window_s / 1e6,
            rtt_ms=rtt_ms,
            pdcp_latency_ms=float(now.get("latency_s", 0.0)) * 1000.0 or None,
            ce_rate=(delta("ce_packets_total") / received_pkts
                     if received_pkts else None),
            drop_rate=(lost / (delivered + lost) if delivered + lost else 0.0),
            retransmitted_bytes=retransmitted,
            sinr_db=now.get("sinr_db"),
            queue_bytes=int(now.get("pending_bytes", 0)),
        )
