"""REST and WebSocket front end for the FikoRE control plane.

A stateless proxy plus a telemetry cache, and nothing else. No VQEG logic, no shadow
copy of the control state, no reconciliation after a restart: GET /ue/{id}/state is
answered by asking the emulator, never from memory.
"""

from __future__ import annotations

import asyncio
import os
from contextlib import asynccontextmanager
from typing import Any, AsyncIterator, Dict, List, Optional

from fastapi import Body, FastAPI, HTTPException, WebSocket, WebSocketDisconnect

from .control import ControlClient, ControlConfig, ControlError
from .proto import PROTO
from .telemetry import TelemetryCollector


def _env(name: str, default: str) -> str:
    return os.environ.get(name, default)


control = ControlClient(
    ControlConfig(
        transport=_env("FIKORE_CONTROL_TRANSPORT", "unix"),
        address=_env("FIKORE_CONTROL_ADDRESS", "/tmp/fikore-control.sock"),
        port=int(_env("FIKORE_CONTROL_PORT", "8097")),
    )
)
collector = TelemetryCollector(
    host=_env("FIKORE_TELEMETRY_HOST", "127.0.0.1"),
    port=int(_env("FIKORE_TELEMETRY_PORT", "8098")),
)

_subscribers: List[asyncio.Queue] = []
_loop: Optional[asyncio.AbstractEventLoop] = None


def _fan_out(entry: Dict[str, Any]) -> None:
    """Called from the collector thread, so it hops onto the event loop."""
    if _loop is None:
        return
    for queue in list(_subscribers):
        _loop.call_soon_threadsafe(queue.put_nowait, {"type": "telemetry", "point": entry})


@asynccontextmanager
async def lifespan(_: FastAPI) -> AsyncIterator[None]:
    global _loop
    _loop = asyncio.get_running_loop()
    collector.on_point = _fan_out
    collector.start()
    yield
    collector.stop()
    control.close()


app = FastAPI(title="FikoRE control API", version=PROTO, lifespan=lifespan)


def _forward(call, *args) -> Dict[str, Any]:
    try:
        reply = call(*args)
    except ControlError as exc:
        raise HTTPException(status_code=503, detail=f"control channel: {exc}") from exc

    if reply.get("status") == "error":
        raise HTTPException(status_code=400, detail=reply.get("errors", reply))
    return reply


@app.post("/control/ue/{ue_id}")
async def set_ue(ue_id: int, values: Dict[str, Any] = Body(...)) -> Dict[str, Any]:
    return _forward(control.set, f"ue/{ue_id}", values)


@app.post("/control/batch")
async def set_batch(payload: Dict[str, Any] = Body(...)) -> Dict[str, Any]:
    commands = payload.get("cmds")
    if not isinstance(commands, list) or not commands:
        raise HTTPException(status_code=400, detail="expected a non-empty 'cmds' list")
    return _forward(control.batch, commands)


@app.get("/ue/{ue_id}/state")
async def ue_state(ue_id: int) -> Dict[str, Any]:
    # Control state comes from the emulator every time; only the metrics come from cache.
    reply = _forward(control.get, f"ue/{ue_id}")
    result = reply.get("result") or []
    if not result:
        raise HTTPException(status_code=404, detail=f"unknown ue {ue_id}")

    control_state = dict(result[0])
    # The state block is read back from the emulator too: buffer occupancy, delivered and
    # lost bytes, and the rules in force. It is what a client pacing its own injection
    # needs, and unlike the telemetry cache it is exact and in simulated time.
    state = control_state.pop("state", {})

    return {
        "ue_id": ue_id,
        "control": control_state,
        "state": state,
        "telemetry": collector.cache.for_ue(ue_id),
        "tti": reply.get("tti"),
        "t": reply.get("t"),
    }


@app.get("/ues")
async def ues() -> Dict[str, Any]:
    reply = _forward(control.get, "ue/*")
    return {"ues": reply.get("result", []), "tti": reply.get("tti"), "t": reply.get("t")}


@app.get("/schema")
async def schema() -> Dict[str, Any]:
    reply = _forward(control.describe)
    return {"proto": PROTO, "params": reply.get("result", [])}


@app.post("/sync/grant")
async def grant(payload: Dict[str, Any] = Body(...)) -> Dict[str, Any]:
    if "until_tti" in payload:
        until_tti = int(payload["until_tti"])
    elif "until_t" in payload:
        until_tti = round(float(payload["until_t"]) * 1000.0)
    else:
        raise HTTPException(status_code=400, detail="expected until_tti or until_t")
    return _forward(control.grant, until_tti)


@app.get("/healthz")
async def healthz() -> Dict[str, Any]:
    try:
        reply = control.ping()
        linked, detail = True, None
        tti, t = reply.get("tti"), reply.get("t")
    except ControlError as exc:
        linked, detail, tti, t = False, str(exc), None, None

    return {
        "proto": PROTO,
        "emulator_linked": linked,
        "detail": detail,
        "tti": tti,
        "t": t,
        "telemetry_series": len(collector.cache.snapshot()),
    }


@app.websocket("/stream")
async def stream(ws: WebSocket) -> None:
    await ws.accept()
    queue: asyncio.Queue = asyncio.Queue(maxsize=1000)
    _subscribers.append(queue)
    try:
        while True:
            message = await queue.get()
            await ws.send_json(message)
    except WebSocketDisconnect:
        pass
    finally:
        if queue in _subscribers:
            _subscribers.remove(queue)
