"""Starting the emulator and generating the `.ini` a co-simulated run needs.

The emulator owns the socket, so it is started first and the client connects to it.
Nothing here knows about transport or congestion control.
"""
from __future__ import annotations

import os
import re
import subprocess
import time
from dataclasses import dataclass, field

PROTO = "fikore-control-1"


@dataclass
class EmulatorConfig:
    binary: str
    base_ini: str
    socket_path: str
    duration_s: float
    work_dir: str | None = None       # fading maps are resolved next to the binary
    n_ues: int = 1
    pkt_size_bits: int = 12000        # the MSS the transport model must match
    delay_budget_s: float = 30.0      # see docs/03: the budget is not the experiment
    log_path: str | None = None
    extra: dict[str, str] = field(default_factory=dict)

    def render(self, path: str) -> str:
        src = open(self.base_ini).read()
        subs = {
            "duration": f"{self.duration_s}",
            "period": "-1",                     # fast mode; the barrier needs it
            "n_ues": f"{self.n_ues}",
            "dl_target": "0.0",                 # every byte comes from the client
            "ul_target": "0.0",
            "pkt_size": f"{self.pkt_size_bits}",
            "pkt_delay_budget": f"{self.delay_budget_s}",
            "random_v": "false",
            "sync_mode": "barrier",
            "address": self.socket_path,
            "on_timeout": "abort",
            "progress_log_period_s": "0",
            **self.extra,
        }
        out = src
        for key, value in subs.items():
            pattern = rf"^{re.escape(key)}: .*$"
            if re.search(pattern, out, flags=re.M):
                out = re.sub(pattern, f"{key}: {value}", out, flags=re.M)
            else:
                # A key the template does not carry is added to the UE section, which
                # is where every knob worth setting from here lives. Silently dropping
                # it would turn a typo into an experiment that ran the wrong scenario.
                out = re.sub(r"^\[UE\]$", f"[UE]\n{key}: {value}", out, count=1,
                             flags=re.M)
        out = out.replace("[Monitoring]\nenabled: true", "[Monitoring]\nenabled: false")
        open(path, "w").write(out)
        return path


class Emulator:
    def __init__(self, cfg: EmulatorConfig) -> None:
        self.cfg = cfg
        self.ini = cfg.render(os.path.join(os.path.dirname(cfg.socket_path) or ".",
                                           "cosim-run.ini"))
        if os.path.exists(cfg.socket_path):
            os.unlink(cfg.socket_path)
        log = open(cfg.log_path or "/tmp/fikore-cosim.log", "w")
        self._log = log
        self.proc = subprocess.Popen([cfg.binary, self.ini],
                                     cwd=cfg.work_dir or os.path.dirname(cfg.binary) or ".",
                                     stdout=log, stderr=subprocess.STDOUT)

    def wait_for_socket(self, timeout_s: float = 10.0) -> None:
        deadline = time.time() + timeout_s
        while time.time() < deadline:
            if os.path.exists(self.cfg.socket_path):
                return
            if self.proc.poll() is not None:
                raise RuntimeError(f"emulator exited early, see {self._log.name}")
            time.sleep(0.01)
        raise TimeoutError(f"socket never appeared, see {self._log.name}")

    def close(self) -> None:
        if self.proc.poll() is None:
            self.proc.terminate()
            try:
                self.proc.wait(timeout=10)
            except subprocess.TimeoutExpired:
                self.proc.kill()
        self._log.close()
