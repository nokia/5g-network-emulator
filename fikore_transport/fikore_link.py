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
"""
from __future__ import annotations

import json
import socket
from dataclasses import dataclass

from .emulator import PROTO, Emulator
from .link import Arrival, Direction, Transmit

EPS_BYTES = 1.0


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

        emulator.wait_for_socket()
        self.sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        self.sock.connect(emulator.cfg.socket_path)
        self.io = self.sock.makefile("rwb")
        hello = json.loads(self.io.readline())
        if hello.get("proto") != PROTO:
            raise RuntimeError(f"unexpected protocol: {hello}")
        self._send({"proto": PROTO})

        self._next_id = 1
        self._next_tag = 1
        self._outstanding: dict[int, _Outstanding] = {}
        self._pending: dict[int, list[Transmit]] = {}
        self._to_forget: list[tuple[int, int]] = []   # (ue, tag)
        self._queued_sets: list[dict] = []
        self.round_trips = 0
        self.ce_by_ue: dict[int, int] = {}
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
        if items:
            self._pending.setdefault(at_tti, []).extend(items)

    def step(self, until_tti: int) -> list[Arrival]:
        """Apply this slot's injections, grant it, and read what became terminal.

        The state comes back resolved at the same quiescent point at which the
        injections are applied, so what it reports finished during the previous
        slot. The arrival is stamped with the slot in which it was learnt, which is
        the only instant the model can legitimately act on.
        """
        cmds: list[dict] = list(self._queued_sets)
        self._queued_sets.clear()
        for t in self._pending.pop(until_tti, []):
            ue = self.flow_to_ue[t.flow]
            tag = self._next_tag
            self._next_tag += 1
            self._outstanding[tag] = _Outstanding(t.flow, ue, t.seq, t.size, t.direction,
                                                  t.ts_us, t.kind)
            inject = {"op": "inject", "target": f"ue/{ue}", "tag": tag,
                      f"{t.direction}.bytes": t.size}
            if t.ecn != "not-ect":
                inject["ecn"] = t.ecn
            cmds.append(inject)
        for ue, tag in self._to_forget:
            cmds.append({"op": "forget", "target": f"ue/{ue}", "tag": tag})
        self._to_forget.clear()
        cmds.append({"op": "get", "target": "ue/*"})

        batch_id, grant_id = self._next_id, self._next_id + 1
        self._next_id += 2
        # Both messages go out before either reply is read: the grant is answered by
        # the transport and the rest at the quiescent point, so waiting for the first
        # reply in order would block the run until the credit timeout.
        self._send({"id": batch_id, "at_tti": until_tti, "cmds": cmds})
        self._send({"id": grant_id, "op": "grant", "until_tti": until_tti})
        acks = self._read_acks({batch_id, grant_id})
        self.round_trips += 1

        batch = acks[batch_id]
        if batch.get("status") != "ok":
            raise RuntimeError(f"emulator rejected a command: {batch}")
        return self._arrivals_from(batch.get("result") or [], until_tti)

    def close(self) -> None:
        try:
            self.sock.close()
        finally:
            self.emulator.close()

    # -- wire ---------------------------------------------------------------------

    def _send(self, msg: dict) -> None:
        self.io.write((json.dumps(msg) + "\n").encode())
        self.io.flush()

    def _read_acks(self, ids: set[int]) -> dict[int, dict]:
        got: dict[int, dict] = {}
        while not ids <= got.keys():
            line = self.io.readline()
            if not line:
                raise RuntimeError("the emulator closed the control channel")
            ack = json.loads(line)
            got[ack["id"]] = ack
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
                    tag = int(raw_tag)
                    pending = self._outstanding.get(tag)
                    if pending is None:
                        continue
                    delivered = counters.get("delivered_bytes", 0.0)
                    dropped = counters.get("dropped_bytes", 0.0)
                    expired = counters.get("expired_bytes", 0.0)
                    if delivered + dropped + expired + EPS_BYTES < pending.size:
                        continue
                    fate = ("delivered" if delivered + EPS_BYTES >= pending.size
                            else "expired" if expired >= dropped else "dropped")
                    out.append(Arrival(tti=tti, flow=pending.flow, seq=pending.seq,
                                       size=pending.size, fate=fate,
                                       ce=counters.get("ce_bytes", 0.0) > 0.0,
                                       delivered_bytes=int(delivered),
                                       ts_us=pending.ts_us, kind=pending.kind))
                    del self._outstanding[tag]
                    self._to_forget.append((ue, tag))
        # The object map comes back in the emulator's hash order, so without this
        # the receiver sees the segments of one slot shuffled, reports the gaps as
        # selective acknowledgements and the sender retransmits segments that were
        # never lost. Inside a slot the order is unobservable, so sequence order is
        # the only defensible one to present.
        out.sort(key=lambda a: (a.flow, a.kind, a.seq))
        return out
