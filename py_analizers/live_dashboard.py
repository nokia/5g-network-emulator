#!/usr/bin/env python3
import argparse
import collections
import math
import os
import socket
import threading
import time

import matplotlib
import numpy as np

plt = None
FuncAnimation = None


def configure_matplotlib_backend(backend_name):
    global plt
    global FuncAnimation

    selected = backend_name
    if selected == "auto":
        selected = pick_default_backend()

    matplotlib.use(selected, force=True)
    import matplotlib.pyplot as _plt
    from matplotlib.animation import FuncAnimation as _FuncAnimation

    plt = _plt
    FuncAnimation = _FuncAnimation
    return selected


def pick_default_backend():
    if os.environ.get("MPLBACKEND"):
        return os.environ["MPLBACKEND"]

    for module_name, backend_name in (
        ("tkinter", "TkAgg"),
        ("PyQt5", "Qt5Agg"),
        ("PySide6", "QtAgg"),
    ):
        try:
            __import__(module_name)
            return backend_name
        except Exception:
            continue

    return "WebAgg"


def parse_influx_line(line):
    line = line.strip()
    if not line:
        return None

    first_space = line.find(" ")
    second_space = line.rfind(" ")
    if first_space <= 0 or second_space <= first_space:
        return None

    measurement_and_tags = line[:first_space]
    fields_part = line[first_space + 1:second_space]

    try:
        ts_ns = int(line[second_space + 1:])
    except ValueError:
        return None

    head_parts = measurement_and_tags.split(",")
    measurement = head_parts[0]
    tags = {}
    for item in head_parts[1:]:
        if "=" not in item:
            continue
        key, value = item.split("=", 1)
        tags[key] = value.replace("\\,", ",").replace("\\ ", " ").replace("\\=", "=")

    fields = {}
    for item in fields_part.split(","):
        if "=" not in item:
            continue
        key, value = item.split("=", 1)
        if value.endswith("i"):
            value = value[:-1]
        try:
            fields[key] = float(value)
        except ValueError:
            continue

    return {
        "measurement": measurement,
        "tags": tags,
        "fields": fields,
        "ts_ns": ts_ns,
    }


class SlidingSeries:
    def __init__(self):
        self.points = collections.deque()

    def append(self, ts_s, values):
        self.points.append((ts_s, values))

    def prune(self, min_ts_s):
        while self.points and self.points[0][0] < min_ts_s:
            self.points.popleft()

    def extract(self, key):
        xs = []
        ys = []
        for ts_s, values in self.points:
            if key not in values:
                continue
            xs.append(ts_s)
            ys.append(values[key])
        return xs, ys


class DashboardState:
    def __init__(self, window_s):
        self.window_s = window_s
        self.lock = threading.Lock()
        self.latest_ts_s = 0.0
        self.ue_ids = set()
        self.mobility = collections.defaultdict(SlidingSeries)
        self.pdcp = {
            "ul": collections.defaultdict(SlidingSeries),
            "dl": collections.defaultdict(SlidingSeries),
        }
        self.l4s = {
            "ul": collections.defaultdict(SlidingSeries),
            "dl": collections.defaultdict(SlidingSeries),
        }
        self.phy = {
            "ul": collections.defaultdict(SlidingSeries),
            "dl": collections.defaultdict(SlidingSeries),
        }
        self.runtime = SlidingSeries()

    def ingest(self, metric):
        ts_s = metric["ts_ns"] / 1e9
        measurement = metric["measurement"]
        tags = metric["tags"]
        fields = metric["fields"]

        ue_id = tags.get("ue_id")
        if ue_id is not None:
            self.ue_ids.add(ue_id)

        if ts_s > self.latest_ts_s:
            self.latest_ts_s = ts_s
        min_ts_s = self.latest_ts_s - self.window_s

        if measurement == "ue_mobility" and ue_id is not None:
            self.mobility[ue_id].append(ts_s, fields)
            self.mobility[ue_id].prune(min_ts_s)
            return

        if measurement == "emulator_runtime":
            self.runtime.append(ts_s, fields)
            self.runtime.prune(min_ts_s)
            return

        tx_dir = tags.get("tx_dir")
        if tx_dir not in ("ul", "dl") or ue_id is None:
            return

        if measurement == "ue_pdcp":
            self.pdcp[tx_dir][ue_id].append(ts_s, fields)
            self.pdcp[tx_dir][ue_id].prune(min_ts_s)
            return

        if measurement == "ue_l4s":
            l4s_fields = dict(fields)
            total_packets = fields.get("generated_packets_sum")
            ce_packets = fields.get("ce_packets_sum")
            if total_packets is not None and total_packets > 0 and ce_packets is not None:
                l4s_fields["ce_rate"] = ce_packets / total_packets
            self.l4s[tx_dir][ue_id].append(ts_s, l4s_fields)
            self.l4s[tx_dir][ue_id].prune(min_ts_s)
            return

        if measurement == "ue_phy":
            self.phy[tx_dir][ue_id].append(ts_s, fields)
            self.phy[tx_dir][ue_id].prune(min_ts_s)
            return


def udp_listener(state, host, port, stop_event):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((host, port))
    sock.settimeout(0.5)
    while not stop_event.is_set():
        try:
            data, _ = sock.recvfrom(65535)
        except socket.timeout:
            continue
        except OSError:
            break

        for raw_line in data.decode("utf-8", errors="ignore").splitlines():
            metric = parse_influx_line(raw_line)
            if metric is None:
                continue
            with state.lock:
                state.ingest(metric)
    sock.close()


def relative_times(xs, latest_ts_s):
    return [x - latest_ts_s for x in xs]


def draw_time_plot(ax, series_by_ue, latest_ts_s, field_name, title, ylabel, window_s, cmap, ylim=None):
    ax.clear()
    has_data = False
    for idx, ue_id in enumerate(sorted(series_by_ue.keys(), key=lambda x: int(x))):
        xs, ys = series_by_ue[ue_id].extract(field_name)
        if not xs:
            continue
        has_data = True
        ax.plot(relative_times(xs, latest_ts_s), ys, linewidth=1.8, color=cmap(idx % cmap.N), label=f"UE {ue_id}")
    ax.set_title(title)
    ax.set_ylabel(ylabel)
    ax.set_xlim(-window_s, 0)
    if ylim is not None:
        ax.set_ylim(*ylim)
    ax.grid(True, alpha=0.3)
    if has_data:
        ax.legend(loc="upper left", fontsize=8)


def draw_latency_plot_ms(ax, series_by_ue, latest_ts_s, field_name, title, window_s, cmap):
    ax.clear()
    has_data = False
    for idx, ue_id in enumerate(sorted(series_by_ue.keys(), key=lambda x: int(x))):
        xs, ys = series_by_ue[ue_id].extract(field_name)
        if not xs:
            continue
        has_data = True
        ys_ms = [1000.0 * y for y in ys]
        ax.plot(relative_times(xs, latest_ts_s), ys_ms, linewidth=1.8, color=cmap(idx % cmap.N), label=f"UE {ue_id}")
    ax.set_title(title)
    ax.set_ylabel("ms")
    ax.set_xlim(-window_s, 0)
    ax.grid(True, alpha=0.3)
    if has_data:
        ax.legend(loc="upper left", fontsize=8)


def draw_runtime_plot(ax, series, latest_ts_s, fields, labels, title, ylabel, window_s):
    ax.clear()
    has_data = False
    for field_name, label in zip(fields, labels):
        xs, ys = series.extract(field_name)
        if not xs:
            continue
        has_data = True
        ax.plot(relative_times(xs, latest_ts_s), ys, linewidth=1.8, label=label)
    ax.set_title(title)
    ax.set_ylabel(ylabel)
    ax.set_xlim(-window_s, 0)
    ax.grid(True, alpha=0.3)
    if has_data:
        ax.legend(loc="upper left", fontsize=8)


def build_dashboard(state, refresh_ms, window_s):
    fig, axes = plt.subplots(5, 2, figsize=(15, 14), constrained_layout=True)
    cmap = plt.get_cmap("tab10")
    ax_traj = axes[0, 0]
    ax_sinr = axes[0, 1]
    ax_tp_ul = axes[1, 0]
    ax_tp_dl = axes[1, 1]
    ax_lat_ul = axes[2, 0]
    ax_lat_dl = axes[2, 1]
    ax_cong_ul = axes[3, 0]
    ax_cong_dl = axes[3, 1]
    ax_rt_total = axes[4, 0]
    ax_rt_split = axes[4, 1]

    def update(_frame):
        with state.lock:
            latest_ts_s = state.latest_ts_s
            min_ts_s = latest_ts_s - state.window_s
            ue_ids = sorted(state.ue_ids, key=lambda x: int(x))

            ax_traj.clear()
            for idx, ue_id in enumerate(ue_ids):
                state.mobility[ue_id].prune(min_ts_s)
                _, x_vals = state.mobility[ue_id].extract("x")
                _, y_vals = state.mobility[ue_id].extract("y")
                if not x_vals or not y_vals:
                    continue
                n = min(len(x_vals), len(y_vals))
                x_vals = x_vals[:n]
                y_vals = y_vals[:n]
                ax_traj.plot(x_vals, y_vals, linewidth=1.8, color=cmap(idx % cmap.N), label=f"UE {ue_id}")
                ax_traj.plot(x_vals[-1], y_vals[-1], "o", color=cmap(idx % cmap.N))
            ax_traj.plot(0.0, 0.0, "kp", markersize=10, label="gNB")
            ax_traj.set_title("UE Trajectory (2D)")
            ax_traj.set_xlabel("x [m]")
            ax_traj.set_ylabel("y [m]")
            ax_traj.axis("equal")
            ax_traj.grid(True, alpha=0.3)
            if ue_ids:
                ax_traj.legend(loc="upper left", fontsize=8)

            ax_sinr.clear()
            sinr_bins = None
            for idx, ue_id in enumerate(ue_ids):
                samples = []
                for tx_dir in ("ul", "dl"):
                    state.phy[tx_dir][ue_id].prune(min_ts_s)
                    _, vals = state.phy[tx_dir][ue_id].extract("sinr_db_mean")
                    samples.extend(vals)
                if not samples:
                    continue
                values = np.asarray(samples, dtype=float)
                values = values[np.isfinite(values)]
                if values.size == 0:
                    continue
                if sinr_bins is None:
                    vmin = math.floor(float(values.min())) - 1
                    vmax = math.ceil(float(values.max())) + 1
                    if vmax <= vmin:
                        vmax = vmin + 2
                    sinr_bins = np.linspace(vmin, vmax, 25)
                ax_sinr.hist(values, bins=sinr_bins, histtype="step", linewidth=2.0,
                             color=cmap(idx % cmap.N), label=f"UE {ue_id}")
            ax_sinr.set_title("SINR per UE")
            ax_sinr.set_xlabel("SINR [dB]")
            ax_sinr.set_ylabel("Muestras")
            ax_sinr.grid(True, alpha=0.3)
            if ue_ids:
                ax_sinr.legend(loc="upper right", fontsize=8)

            draw_time_plot(ax_tp_ul, state.pdcp["ul"], latest_ts_s, "throughput_mbps_mean",
                           "Throughput UL", "Mbps", window_s, cmap)
            draw_time_plot(ax_tp_dl, state.pdcp["dl"], latest_ts_s, "throughput_mbps_mean",
                           "Throughput DL", "Mbps", window_s, cmap)
            draw_latency_plot_ms(ax_lat_ul, state.pdcp["ul"], latest_ts_s, "latency_s_mean",
                                 "Latency UL", window_s, cmap)
            draw_latency_plot_ms(ax_lat_dl, state.pdcp["dl"], latest_ts_s, "latency_s_mean",
                                 "Latency DL", window_s, cmap)
            draw_time_plot(ax_cong_ul, state.l4s["ul"], latest_ts_s, "ce_rate",
                           "L4S Congestion UL (EC1/total)", "ratio", window_s, cmap, ylim=(0.0, 1.0))
            draw_time_plot(ax_cong_dl, state.l4s["dl"], latest_ts_s, "ce_rate",
                           "L4S Congestion DL (EC1/total)", "ratio", window_s, cmap, ylim=(0.0, 1.0))
            ax_cong_ul.set_xlabel(f"Time, relative [s], ventana={window_s:.0f}s")
            ax_cong_dl.set_xlabel(f"Time, relative [s], ventana={window_s:.0f}s")
            state.runtime.prune(min_ts_s)
            draw_runtime_plot(
                ax_rt_total,
                state.runtime,
                latest_ts_s,
                ["step_time_ms_mean"],
                ["Step total"],
                "Emulator Runtime",
                "ms",
                window_s,
            )
            draw_runtime_plot(
                ax_rt_split,
                state.runtime,
                latest_ts_s,
                ["mac_step_time_ms_mean", "ue_step_time_ms_mean"],
                ["MAC", "UE"],
                "MAC / UE Breakdown",
                "ms",
                window_s,
            )
            ax_rt_total.set_xlabel(f"Time, relative [s], ventana={window_s:.0f}s")
            ax_rt_split.set_xlabel(f"Time, relative [s], ventana={window_s:.0f}s")

            if latest_ts_s > 0:
                fig.suptitle(f"Live Monitoring UDP  t={latest_ts_s:.3f}s", fontsize=14)
            else:
                fig.suptitle("Live Monitoring UDP", fontsize=14)

    animation = FuncAnimation(fig, update, interval=refresh_ms, cache_frame_data=False)
    return fig, animation


def main():
    parser = argparse.ArgumentParser(description="Basic live dashboard for emulator monitoring over Influx UDP.")
    parser.add_argument("--host", default="0.0.0.0", help="UDP bind address")
    parser.add_argument("--port", type=int, default=8096, help="UDP bind port")
    parser.add_argument("--window", type=float, default=30.0, help="Sliding window in seconds")
    parser.add_argument("--refresh-ms", type=int, default=500, help="Plot refresh interval in ms")
    parser.add_argument("--backend", default="auto", help="Matplotlib backend: auto, TkAgg, Qt5Agg, WebAgg, ...")
    args = parser.parse_args()

    selected_backend = configure_matplotlib_backend(args.backend)
    state = DashboardState(window_s=args.window)
    stop_event = threading.Event()
    listener = threading.Thread(target=udp_listener, args=(state, args.host, args.port, stop_event), daemon=True)
    listener.start()

    print(f"[INFO] Listening on UDP {args.host}:{args.port} with sliding window {args.window:.1f}s")
    print(f"[INFO] Matplotlib backend: {selected_backend}")
    if selected_backend.lower() == "webagg":
        print("[INFO] Open the URL shown by Matplotlib in your browser.")

    fig, animation = build_dashboard(state, args.refresh_ms, args.window)
    try:
        plt.show()
    finally:
        stop_event.set()
        time.sleep(0.1)
        _ = fig
        _ = animation


if __name__ == "__main__":
    main()
