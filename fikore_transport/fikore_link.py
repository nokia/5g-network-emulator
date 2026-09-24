# Copyright 2026 Nokia
# Licensed under the BSD 3-Clause Clear License
# SPDX-License-Identifier: BSD-3-Clause-Clear
"""The `Link` implementation that drives FikoRE over its control protocol.

One slot per round trip, because a transport model reacts within the slot in which
it learns something: the harness injects what the senders produced, grants exactly
one slot of credit, reads the per-object counters back, and turns their movement
into arrivals.

Each segment travels under its own object tag, which is what makes a byte counter
into per-segment feedback: a tag whose terminal counters add up to its size has
finished, the counter it landed in says how, and `ce_bytes` says whether it was
marked on the way. Tags are released as soon as they are terminal, so the number of
live objects stays around the congestion window rather than growing with the
transfer.

Two properties of the wire protocol shape everything below, and neither is obvious
from the message format:

* **The emulator answers per command, not per message.** A message of N commands
  comes back as N acknowledgements, all carrying the message's `id`, in the order
  the commands were written. Only the `get` carries a `result`. A client that reads
  one acknowledgement per `id` leaves N-1 in the socket and every later slot reads
  the previous slot's replies.
* **`grant` is answered from the socket thread and everything else at the quiescent
  point**, so the two messages of a slot must both be written before either reply is
  read, and the replies must be matched by `id` rather than by order.
"""
from __future__ import annotations

import json
from dataclasses import dataclass

from .emulator import PROTO, Emulator
from .link import Arrival, Cause, Direction, Transmit

# The counters are carried as bits divided by eight, so a whole segment can come
# back a hair under its own size. A byte of slack closes an object that is complete
# without closing one that still has a fragment in flight: fragments are packets,
# and the smallest packet is far larger than this.
EPS_BYTES = 1.0

# The emulator's name for each way of losing a byte, in the order the fate is
# decided. `dropped_bytes` is the sum of the two below it and is only read as a
# fallback, for the case of a build that does not break it down.
_CAUSES: tuple[tuple[str, str, Cause], ...] = (
    ("expired_bytes", "expired", "budget"),
    ("queue_dropped_bytes", "dropped", "queue"),
    ("radio_dropped_bytes", "dropped", "radio"),
)


@dataclass
class _Outstanding:
    flow: int
    ue: int
    seq: int
    size: int
    direction: Direction
    ts_us: int
    kind: str


class FikoreLink:
    """Transport-facing view of a running emulator."""

    def __init__(self, emulator: Emulator, flow_to_ue: dict[int, int]) -> None:
        self.emulator = emulator
        self.flow_to_ue = flow_to_ue
        self.mss = emulator.cfg.pkt_size_bits // 8
        self.max_cmds_per_tick = emulator.cfg.max_cmds_per_tick

        self.sock = emulator.connect()
        self.io = self.sock.makefile("rwb")
        hello = json.loads(self.io.readline() or b"{}")
        if hello.get("proto") != PROTO:
            raise RuntimeError(f"unexpected protocol: {hello}"
                               f"{emulator.log_tail()}")
        self._send({"proto": PROTO})

        self._next_id = 1
        self._next_tag = 1
        self._last_tti = -1
        self._outstanding: dict[int, _Outstanding] = {}
        self._pending: dict[int, list[Transmit]] = {}
        self._to_forget: list[tuple[int, int]] = []   # (ue, tag)
        self._queued_sets: list[dict] = []
        self.round_trips = 0
        self.ce_by_ue: dict[tuple[int, Direction], int] = {}
        # Byte accounting of what this link was asked to carry, for whoever wants to
        # check that the run conserved bytes without going back to the emulator.
        self.submitted_bytes = 0
        self.terminal_bytes: dict[str, int] = {"delivered": 0, "dropped": 0,
                                               "expired": 0}
        self.lost_bytes_by_cause: dict[Cause, int] = {"queue": 0, "radio": 0,
                                                      "budget": 0}
        # Per (ue, direction) snapshot of the last state block, for whoever needs
        # the network's own view alongside the model's.
        self.last_state: dict[tuple[int, str], dict] = {}

    # -- Link interface -----------------------------------------------------------

    def register_flow(self, flow: int, ue: int) -> None:
        self.flow_to_ue[flow] = ue

    def set_params(self, ue: int, params: dict[str, float]) -> None:
        """Queued rather than sent: every command of a slot goes in one message,
        applied at the same quiescent point."""
        self._queued_sets.append({"target": f"ue/{ue}", "set": dict(params)})

    def submit(self, items: list[Transmit], at_tti: int) -> None:
        if not items:
            return
        if at_tti <= self._last_tti:
            # There is no way to inject into a slot the emulator has already run, so
            # accepting these would lose them silently and break the byte account.
            raise ValueError(f"cannot submit into slot {at_tti}, "
                             f"already past {self._last_tti}")
        self._pending.setdefault(at_tti, []).extend(items)

    def step(self, until_tti: int) -> list[Arrival]:
        """Apply this slot's injections, grant it, and read what became terminal.

        The state comes back resolved at the same quiescent point at which the
        injections are applied, so what it reports finished during the previous
        slot. The arrival is stamped with the slot in which it was learnt, which is
        the only instant the model can legitimately act on.
        """
        if until_tti <= self._last_tti:
            raise ValueError(f"slots must advance: {until_tti} after {self._last_tti}")
        self._last_tti = until_tti

        cmds = self._commands_for(until_tti)
        batch_id, grant_id = self._next_id, self._next_id + 1
        self._next_id += 2
        # Both messages go out before either reply is read: the grant is answered by
        # the transport and the rest at the quiescent point, so waiting for the first
        # reply in order would block the run until the credit timeout.
        self._send({"id": batch_id, "at_tti": until_tti, "cmds": cmds})
        self._send({"id": grant_id, "op": "grant", "until_tti": until_tti})
        acks = self._read_acks({batch_id: len(cmds), grant_id: 1})
        self.round_trips += 1

        for ack, cmd in zip(acks[batch_id], cmds):
            if ack.get("status") != "ok":
                raise RuntimeError(f"the emulator rejected {cmd}: {ack}")
        if acks[grant_id][0].get("status") != "ok":
            raise RuntimeError(f"the grant was refused: {acks[grant_id][0]}")

        # The `get` is written last, so its reply is the last of the batch, and it is
        # the only one carrying a result.
        state = acks[batch_id][-1].get("result") or []
        return self._arrivals_from(state, until_tti)

    def close(self) -> None:
        try:
            self.io.close()
            self.sock.close()
        finally:
            self.emulator.close()

    # -- commands -----------------------------------------------------------------

    def _commands_for(self, tti: int) -> list[dict]:
        cmds: list[dict] = list(self._queued_sets)
        self._queued_sets.clear()
        for t in self._pending.pop(tti, []):
            ue = self.flow_to_ue[t.flow]
            tag = self._next_tag
            self._next_tag += 1
            self._outstanding[tag] = _Outstanding(t.flow, ue, t.seq, t.size,
                                                  t.direction, t.ts_us, t.kind)
            self.submitted_bytes += t.size
            inject = {"op": "inject", "target": f"ue/{ue}", "tag": tag,
                      f"{t.direction}.bytes": t.size}
            if t.ecn != "not-ect":
                inject["ecn"] = t.ecn
            cmds.append(inject)

        # One command per segment plus one per release, and the emulator defers
        # whatever a TTI cannot take. Deferring an injection would move a segment to
        # a slot the model did not choose, so the releases give way first: a tag held
        # one slot longer only costs a line of JSON in the next `get`.
        budget = self.max_cmds_per_tick - len(cmds) - 1
        if budget < 0:
            raise RuntimeError(
                f"slot {tti} needs {len(cmds) + 1} commands but the emulator applies "
                f"{self.max_cmds_per_tick} per TTI; raise EmulatorConfig."
                f"max_cmds_per_tick above the largest window this run can reach")
        release, self._to_forget = self._to_forget[:budget], self._to_forget[budget:]
        for ue, tag in release:
            cmds.append({"op": "forget", "target": f"ue/{ue}", "tag": tag})

        cmds.append({"op": "get", "target": "ue/*"})
        return cmds

    # -- wire ---------------------------------------------------------------------

    def _send(self, msg: dict) -> None:
        self.io.write((json.dumps(msg) + "\n").encode())
        self.io.flush()

    def _read_acks(self, expected: dict[int, int]) -> dict[int, list[dict]]:
        """Read exactly the acknowledgements a slot is owed, one per command.

        Counting them is what keeps the stream aligned. Reading "one per id" leaves
        the rest behind and every later slot then reads a previous slot's replies,
        which is silent: the acknowledgements are well formed and say `ok`, they just
        describe a different instant.
        """
        got: dict[int, list[dict]] = {i: [] for i in expected}
        remaining = sum(expected.values())
        while remaining:
            line = self.io.readline()
            if not line:
                raise RuntimeError("the emulator closed the control channel"
                                   f"{self.emulator.log_tail()}")
            ack = json.loads(line)
            bucket = got.get(ack.get("id"))
            if bucket is None or len(bucket) >= expected[ack["id"]]:
                raise RuntimeError(f"unexpected acknowledgement, the control stream "
                                   f"is out of step: {ack}")
            bucket.append(ack)
            remaining -= 1
        return got

    def _arrivals_from(self, result: list[dict], tti: int) -> list[Arrival]:
        out: list[Arrival] = []
        for entry in result:
            ue = int(str(entry["target"]).split("/")[1])
            for direction in ("dl", "ul"):
                state = entry.get("state", {}).get(direction, {})
                self.last_state[(ue, direction)] = state
                self.ce_by_ue[(ue, direction)] = int(state.get("ce_packets_total", 0))
                for raw_tag, counters in (state.get("objects") or {}).items():
                    arrival = self._terminal(int(raw_tag), counters, ue, tti)
                    if arrival is not None:
                        out.append(arrival)
        # The object map comes back in the emulator's hash order, so without this
        # the receiver sees the segments of one slot shuffled, reports the gaps as
        # selective acknowledgements and the sender retransmits segments that were
        # never lost. Inside a slot the order is unobservable, so sequence order is
        # the only defensible one to present.
        out.sort(key=lambda a: (a.flow, a.kind, a.seq))
        return out

    def _terminal(self, tag: int, counters: dict, ue: int, tti: int) -> Arrival | None:
        """An object whose counters account for every byte it was given, or nothing.

        What is in none of the counters is still in flight; the emulator never learns
        how big an object is, so this is the only place that knows it has finished.
        """
        pending = self._outstanding.get(tag)
        if pending is None:
            return None
        delivered = float(counters.get("delivered_bytes", 0.0))
        lost = {name: float(counters.get(name, 0.0)) for name, _, _ in _CAUSES}
        # A build that does not break the drops down still reports their sum, and a
        # loss of unknown cause is better than an object that never closes.
        total_dropped = float(counters.get("dropped_bytes", 0.0))
        unattributed = total_dropped - lost["queue_dropped_bytes"] - lost["radio_dropped_bytes"]
        if delivered + total_dropped + lost["expired_bytes"] + EPS_BYTES < pending.size:
            return None

        del self._outstanding[tag]
        self._to_forget.append((ue, tag))

        if delivered + EPS_BYTES >= pending.size:
            fate, cause = "delivered", ""
        else:
            # Partial delivery is loss as far as the sender is concerned: the segment
            # has a hole and the receiver cannot use it. The cause that took the most
            # bytes is the one worth reporting.
            name, fate, cause = max(_CAUSES, key=lambda c: lost[c[0]])
            if unattributed > max(lost.values()) + EPS_BYTES:
                fate, cause = "dropped", ""
            self.lost_bytes_by_cause[cause] = (self.lost_bytes_by_cause.get(cause, 0)
                                               + pending.size)
        self.terminal_bytes[fate] += pending.size
        return Arrival(tti=tti, flow=pending.flow, seq=pending.seq, size=pending.size,
                       fate=fate, ce=float(counters.get("ce_bytes", 0.0)) > 0.0,
                       delivered_bytes=int(delivered), ts_us=pending.ts_us,
                       kind=pending.kind, cause=cause)

    # -- accounting ---------------------------------------------------------------

    @property
    def in_flight_bytes(self) -> int:
        """Submitted, not yet terminal. What the byte account is short of."""
        return sum(o.size for o in self._outstanding.values())
