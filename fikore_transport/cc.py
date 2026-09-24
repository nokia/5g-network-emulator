"""Congestion control, as a replaceable component of the sender.

The interface is the contract the sender relies on. Reno and CUBIC follow ns.py's
algorithms; Prague is a binding to the L4S reference implementation and lives in
`cc_prague.py`, because it needs the ECN counters the other two ignore.
"""
from __future__ import annotations

from dataclasses import dataclass
from typing import Protocol


@dataclass
class AckInfo:
    """What one acknowledgement tells congestion control."""

    acked_bytes: int
    rtt_us: int | None      # None when the sample is unusable (Karn: retransmitted)
    now_us: int
    pkts_received: int = 0  # cumulative, as echoed by the receiver
    pkts_ce: int = 0
    pkts_lost: int = 0
    pkts_sent: int = 0


class CongestionControl(Protocol):
    mss: int
    cwnd: float             # bytes

    def on_ack(self, ack: AckInfo) -> None: ...

    def on_dupacks(self, count: int) -> None: ...

    def on_recovered(self) -> None: ...

    def on_rto(self) -> None: ...

    def pacing_rate_bps(self) -> float | None: ...


@dataclass
class Classic:
    """Shared state and loss response of the RFC 5681 family."""

    mss: int = 1500
    cwnd: float = 10 * 1500     # IW10, not ns.py's one segment
    ssthresh: float = float("inf")
    in_recovery: bool = False

    def on_ack(self, ack: AckInfo) -> None:
        raise NotImplementedError

    def on_dupacks(self, count: int) -> None:
        if count == 3 and not self.in_recovery:
            # Fast retransmit and fast recovery: halve, then inflate by the segments
            # that have left the network.
            self.ssthresh = max(2 * self.mss, self.cwnd / 2)
            self.cwnd = self.ssthresh + 3 * self.mss
            self.in_recovery = True
        elif count > 3:
            self.cwnd += self.mss

    def on_recovered(self) -> None:
        if self.in_recovery:
            self.cwnd = self.ssthresh
            self.in_recovery = False

    def on_rto(self) -> None:
        self.ssthresh = max(2 * self.mss, self.cwnd / 2)
        self.cwnd = self.mss
        self.in_recovery = False

    def pacing_rate_bps(self) -> float | None:
        return None


@dataclass
class Reno(Classic):
    def on_ack(self, ack: AckInfo) -> None:
        if self.cwnd <= self.ssthresh:
            self.cwnd += self.mss
        else:
            self.cwnd += self.mss * self.mss / self.cwnd


@dataclass
class Cubic(Classic):
    """CUBIC as in ns.py, with the Linux constants and TCP friendliness kept."""

    C: float = 0.4
    beta: float = 0.2
    fast_convergence: bool = True
    tcp_friendliness: bool = True
    w_last_max: float = 0.0
    epoch_start: float = 0.0
    origin_point: float = 0.0
    d_min_s: float = 0.0
    w_tcp: float = 0.0
    K: float = 0.0
    ack_cnt: float = 0.0
    cwnd_cnt: float = 0.0
    cnt: float = 0.0

    def _reset(self) -> None:
        self.w_last_max = 0.0
        self.epoch_start = 0.0
        self.origin_point = 0.0
        self.d_min_s = 0.0
        self.w_tcp = 0.0
        self.K = 0.0
        self.ack_cnt = 0.0

    def on_ack(self, ack: AckInfo) -> None:
        now_s = ack.now_us / 1e6
        if ack.rtt_us is not None:
            rtt_s = ack.rtt_us / 1e6
            self.d_min_s = min(self.d_min_s, rtt_s) if self.d_min_s > 0 else rtt_s
        if self.cwnd <= self.ssthresh:
            self.cwnd += self.mss
            return
        self._update(now_s)
        if self.cwnd_cnt > self.cnt:
            self.cwnd += self.mss
            self.cwnd_cnt = 0
        else:
            self.cwnd_cnt += 1

    def _update(self, now_s: float) -> None:
        self.ack_cnt += 1
        if self.epoch_start <= 0:
            self.epoch_start = now_s
            if self.cwnd < self.w_last_max:
                self.K = ((self.w_last_max - self.cwnd) / self.C) ** (1.0 / 3)
            else:
                self.K = 0.0
                self.origin_point = self.cwnd
            self.ack_cnt = 1
            self.w_tcp = self.cwnd
        t = now_s + self.d_min_s - self.epoch_start
        target = self.origin_point + self.C * (t - self.K) ** 3
        self.cnt = self.cwnd / (target - self.cwnd) if target > self.cwnd else 100 * self.cwnd
        if self.tcp_friendliness:
            self.w_tcp += 3 * self.beta / (2 - self.beta) * (self.ack_cnt / self.cwnd)
            self.ack_cnt = 0
            if self.w_tcp > self.cwnd:
                self.cnt = min(self.cnt, self.cwnd / (self.w_tcp - self.cwnd))

    def on_dupacks(self, count: int) -> None:
        if count == 3 and not self.in_recovery:
            self.epoch_start = 0.0
            self.w_last_max = (self.cwnd * (2 - self.beta) / 2
                               if self.fast_convergence and self.cwnd < self.w_last_max
                               else self.cwnd)
            self.ssthresh = max(2 * self.mss, self.cwnd * (1 - self.beta))
            self.cwnd = self.ssthresh + 3 * self.mss
            self.in_recovery = True
        elif count > 3:
            self.cwnd += self.mss

    def on_rto(self) -> None:
        super().on_rto()
        self._reset()
