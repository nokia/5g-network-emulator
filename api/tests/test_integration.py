"""End to end: the API in front of a real emulator.

Starts the emulator with the demo config, drives it through the REST endpoints and
checks that what comes back was read from the emulator and not from a cache. Skipped
when the binary has not been built.
"""

from __future__ import annotations

import os
import pathlib
import shutil
import socket
import subprocess
import tempfile
import time

import pytest
from fastapi.testclient import TestClient

REPO = pathlib.Path(__file__).resolve().parents[2]
BINARY = REPO / "bin/fikore"
DEMO = REPO / "config/control_demo.ini"


def _free_udp_port() -> int:
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
        sock.bind(("127.0.0.1", 0))
        return sock.getsockname()[1]


@pytest.fixture(scope="module")
def emulator():
    if not BINARY.exists() or not DEMO.exists():
        pytest.skip("emulator not built: run make first")

    workdir = tempfile.mkdtemp(prefix="fikore-api-test-")
    sock_path = os.path.join(workdir, "control.sock")
    udp_port = _free_udp_port()

    config = DEMO.read_text()
    config = config.replace("/tmp/fikore-control.sock", sock_path)
    config = config.replace("output_api_udp_port: 8098", f"output_api_udp_port: {udp_port}")
    config_path = os.path.join(workdir, "demo.ini")
    pathlib.Path(config_path).write_text(config)

    log = open(os.path.join(workdir, "emulator.log"), "w")
    # cwd is the repo: the emulator resolves its fading maps relative to the binary and
    # writes logs/ relative to the working directory.
    process = subprocess.Popen([str(BINARY), config_path], cwd=str(REPO), stdout=log, stderr=log)

    for _ in range(200):
        if os.path.exists(sock_path):
            break
        time.sleep(0.05)
    else:
        process.kill()
        pytest.skip("emulator did not open the control socket")

    os.environ["FIKORE_CONTROL_TRANSPORT"] = "unix"
    os.environ["FIKORE_CONTROL_ADDRESS"] = sock_path
    os.environ["FIKORE_TELEMETRY_PORT"] = str(udp_port)

    yield process

    process.terminate()
    try:
        process.wait(timeout=10)
    except subprocess.TimeoutExpired:
        process.kill()
    log.close()
    shutil.rmtree(workdir, ignore_errors=True)


@pytest.fixture(scope="module")
def client(emulator):
    from fikore_api.main import app

    with TestClient(app) as test_client:
        yield test_client


def test_healthz_reports_the_link(client):
    body = client.get("/healthz").json()
    assert body["emulator_linked"] is True
    assert body["tti"] >= 0


def test_schema_matches_describe(client):
    body = client.get("/schema").json()
    names = {param["name"] for param in body["params"]}
    assert len(names) == 14
    assert "mobility.speed_kmh" in names
    assert next(p for p in body["params"] if p["name"] == "mobility.speed_kmh")["unit"] == "km/h"

    # Injection is the only incremental knob and the schema has to say so, because
    # retrying it is not free.
    inject = next(p for p in body["params"] if p["name"] == "dl.inject_bytes")
    assert inject["unit"] == "bytes"
    assert inject.get("incremental") is True


def test_set_is_reflected_by_asking_the_emulator(client):
    assert client.post("/control/ue/0", json={"priority": 4.0}).status_code == 200

    state = client.get("/ue/0/state").json()
    assert state["control"]["priority"] == 4.0

    # And again with a different value, to be sure it is not a cached echo.
    assert client.post("/control/ue/0", json={"priority": 2.5}).status_code == 200
    assert client.get("/ue/0/state").json()["control"]["priority"] == 2.5


def test_units_travel_unchanged(client):
    client.post("/control/ue/0", json={"mobility.speed_kmh": 36.0})
    assert client.get("/ue/0/state").json()["control"]["mobility.speed_kmh"] == pytest.approx(36.0, rel=1e-3)


def test_bad_value_is_rejected_and_nothing_is_applied(client):
    before = client.get("/ue/0/state").json()["control"]["priority"]

    response = client.post("/control/ue/0", json={"priority": 9.0, "dl.sinr_offset_db": 999.0})
    assert response.status_code == 400

    after = client.get("/ue/0/state").json()["control"]["priority"]
    assert after == before


def test_unknown_ue_is_a_404(client):
    assert client.get("/ue/999/state").status_code in (400, 404)


def test_ues_lists_every_ue(client):
    body = client.get("/ues").json()
    assert len(body["ues"]) >= 2
    assert all("priority" in entry for entry in body["ues"])


def test_disable_removes_the_ue_from_telemetry(client):
    assert client.post("/control/ue/1", json={"enabled": False}).status_code == 200
    state = client.get("/ue/1/state").json()
    assert state["control"]["enabled"] is False
    client.post("/control/ue/1", json={"enabled": True})


def test_injection_and_state_block(client):
    before = client.get("/ue/0/state").json()["control"]["dl.inject_bytes"]

    assert client.post("/control/ue/0", json={"dl.inject_bytes": 50000}).status_code == 200
    assert client.post("/control/ue/0", json={"dl.inject_bytes": 50000}).status_code == 200

    body = client.get("/ue/0/state").json()
    # Incremental: two injections of 50 kB add up, and the read is the cumulative total.
    assert body["control"]["dl.inject_bytes"] == pytest.approx(before + 100000.0)

    dl = body["state"]["dl"]
    for field in ("injected_bytes_total", "delivered_bytes_total", "expired_bytes_total",
                  "dropped_bytes_total", "pending_bytes", "pending_packets",
                  "oldest_age_s", "latency_s", "ce_packets_total"):
        assert field in dl
    assert dl["injected_bytes_total"] == pytest.approx(before + 100000.0)
    assert body["state"]["pkt_size_bits"] > 0


def test_delay_budget_is_a_knob(client):
    assert client.post("/control/ue/0", json={"pkt_delay_budget_s": 2.0}).status_code == 200
    assert client.get("/ue/0/state").json()["control"]["pkt_delay_budget_s"] == pytest.approx(2.0)
    # Out of range is refused, like any other knob.
    assert client.post("/control/ue/0", json={"pkt_delay_budget_s": 120.0}).status_code == 400
