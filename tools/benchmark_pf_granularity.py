#!/usr/bin/env python3
"""Benchmark PF reranking across scheduling aggregation modes."""

from __future__ import annotations

import argparse
import csv
import itertools
import json
import math
import platform
import socket
import statistics
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

GRID_PROFILES = {
    "20mhz_mu1": (20_000_000, 1),
    "100mhz_mu1": (100_000_000, 1),
    "400mhz_mu3": (400_000_000, 3),
}

RF_BANDWIDTH = {
    20_000_000: 18_000_000.0,
    100_000_000: 98_280_000.0,
}


def rbg_size(frequency_rbs: int, scheduling_type: int) -> int:
    if scheduling_type != 0:
        return 1
    if frequency_rbs < 37:
        return 4
    if frequency_rbs < 73:
        return 8
    return 16


def decision_count(bandwidth: int, numerology: int, time_mode: int, frequency_mode: int) -> int:
    usable = RF_BANDWIDTH.get(bandwidth, bandwidth * 0.9)
    spacing = 15_000 * (2**numerology)
    frequency_rbs = math.floor(usable / (spacing * 12))
    frequency_groups = frequency_rbs // rbg_size(frequency_rbs, frequency_mode)
    time_groups = 2**numerology if time_mode == 0 else 1
    return frequency_groups * time_groups


def config_text(
    ue_count: int,
    bandwidth: int,
    numerology: int,
    time_mode: int,
    frequency_mode: int,
    reranking: str,
) -> str:
    return f"""[Global]
duration: 1
period: -1
multithreading: false
threads: 0
verbose: false
progress_log_period_s: -1

[UE]
ue_id: benchmark
ue_type: 1
n_ues: {ue_count}
n_antennas: 1
set_ul_pow: true
tx_power_ul: 23
cqi_period: 5
ri_period: 5
random_v: false
traffic_type: 0
ul_target: 100000
dl_target: 100000
var_perc: 0
pkt_size: 12000
mobility_type: 0
pos_x: 200
pos_y: 0
random_init: false
speed: 0
max_distance: 1000
priority: 1
pkt_delay_budget: 10
ue_height: 1.5
ue_location_type: outdoor

[Scenario]
scenario_type: 1

[eNBConfig]
modulation_m: 1
target_ber: 0.00005
cqi_mode: 1
tx_power: 46
eNB_gain: 8.7
UT_gain: 0
frequency: 3500000000
bandwidth: {bandwidth}

[MACLayer]
metric_type: 6
pf_alpha: 1
pf_time_window_ms: 100
pf_intra_tti_update: {reranking}
mimo_layers: 1
n_ofdm_syms: 14
n_re_freq: 12
numerology: {numerology}
mcs_tables: true
scheduling_mode: {time_mode}
scheduling_type: {frequency_mode}
scheduling_config: 1
duplexing_type: 0
n_dl_slots: 7
n_ul_slots: 3
transition_c: 54

[PHYLayer]
interference_ues: 0
interference_eNBs: 0
interfered_bandwidth_ratio: 0
thermal_noise: -174
enb_noise_figure: 2
ut_noise_figure: 9
"""


def run_case(binary: Path, config: Path, steps: int) -> dict:
    completed = subprocess.run(
        [str(binary), str(config), str(steps)],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=True,
    )
    for line in reversed(completed.stdout.splitlines()):
        if line.startswith("BENCHMARK "):
            return json.loads(line.removeprefix("BENCHMARK "))
    raise RuntimeError(f"benchmark output missing for {config}")


def percentile(values: list[float], probability: float) -> float:
    ordered = sorted(values)
    if len(ordered) == 1:
        return ordered[0]
    position = probability * (len(ordered) - 1)
    lower = math.floor(position)
    upper = math.ceil(position)
    if lower == upper:
        return ordered[lower]
    fraction = position - lower
    return ordered[lower] * (1.0 - fraction) + ordered[upper] * fraction


def markdown_table(rows: list[dict]) -> str:
    columns = [
        "grid",
        "ues",
        "time_mode",
        "frequency_mode",
        "reranking",
        "decisions_per_tti",
        "us_per_tti_p50",
        "us_per_tti_p95",
        "us_per_tti_p99",
        "dl_total_mbps",
        "dl_jain",
        "dl_max_service_gap_ttis",
    ]
    lines = [
        "| " + " | ".join(columns) + " |",
        "|" + "|".join("---" for _ in columns) + "|",
    ]
    for row in rows:
        lines.append(
            "| "
            + " | ".join(str(row[column]) for column in columns)
            + " |"
        )
    return "\n".join(lines)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--steps", type=int, default=50)
    parser.add_argument("--repeats", type=int, default=2)
    parser.add_argument("--full", action="store_true")
    parser.add_argument(
        "--envelope-modes",
        action="store_true",
        help="run localized/grouped and distributed/per-RB modes only",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=ROOT / "results" / "pf-granularity",
    )
    args = parser.parse_args()

    binary = ROOT / "bin" / "pf_granularity_benchmark"
    if not binary.is_file():
        raise SystemExit(f"build benchmark first: {binary}")

    ue_counts = [1, 16, 64, 256] if args.full else [16, 64]
    modes = (
        [(1, 0), (0, 1)]
        if args.envelope_modes
        else list(itertools.product([1, 0], [0, 1]))
    )
    reranking_modes = ["none", "allocation_unit"]
    output = args.output.resolve()
    config_dir = output / "configs"
    config_dir.mkdir(parents=True, exist_ok=True)
    rows: list[dict] = []
    sample_rows: list[dict] = []

    for grid_name, ue_count, (time_mode, frequency_mode), reranking in itertools.product(
        GRID_PROFILES,
        ue_counts,
        modes,
        reranking_modes,
    ):
        bandwidth, numerology = GRID_PROFILES[grid_name]
        case_name = (
            f"{grid_name}_n{ue_count}_t{time_mode}_f{frequency_mode}_{reranking}"
        )
        config = config_dir / f"{case_name}.ini"
        config.write_text(
            config_text(
                ue_count,
                bandwidth,
                numerology,
                time_mode,
                frequency_mode,
                reranking,
            )
        )
        samples = [
            run_case(binary, config, args.steps)
            for _ in range(args.repeats)
        ]
        for repeat, sample in enumerate(samples):
            sample_rows.append(
                {
                    "case": case_name,
                    "repeat": repeat,
                    "steps": args.steps,
                    "us_per_tti": sample["us_per_tti"],
                }
            )
        representative = samples[0]
        runtime_samples = [
            sample["us_per_tti"] for sample in samples]
        row = {
            "grid": grid_name,
            "ues": ue_count,
            "time_mode": "localized" if time_mode == 1 else "distributed",
            "frequency_mode": "grouped" if frequency_mode == 0 else "per_rb",
            "reranking": reranking,
            "decisions_per_tti": decision_count(
                bandwidth, numerology, time_mode, frequency_mode
            ),
            "us_per_tti": round(
                statistics.median(runtime_samples), 3
            ),
            "us_per_tti_p50": round(
                percentile(runtime_samples, 0.50), 3),
            "us_per_tti_p95": round(
                percentile(runtime_samples, 0.95), 3),
            "us_per_tti_p99": round(
                percentile(runtime_samples, 0.99), 3),
            "dl_total_mbps": round(representative["dl_total_mbps"], 3),
            "ul_total_mbps": round(representative["ul_total_mbps"], 3),
            "dl_jain": round(representative["dl_jain"], 5),
            "ul_jain": round(representative["ul_jain"], 5),
            "dl_max_service_gap_ttis": representative[
                "dl_max_service_gap_ttis"
            ],
            "ul_max_service_gap_ttis": representative[
                "ul_max_service_gap_ttis"
            ],
        }
        rows.append(row)
        print(case_name, row["us_per_tti"])

    csv_path = output / "results.csv"
    with csv_path.open("w", newline="") as handle:
        writer = csv.DictWriter(
            handle, fieldnames=list(rows[0]), lineterminator="\n")
        writer.writeheader()
        writer.writerows(rows)
    with (output / "runtime-samples.csv").open("w", newline="") as handle:
        writer = csv.DictWriter(
            handle, fieldnames=list(sample_rows[0]), lineterminator="\n")
        writer.writeheader()
        writer.writerows(sample_rows)

    report = [
        "# PF Granularity Benchmark",
        "",
        f"Steps per sample: {args.steps}; repeats: {args.repeats}.",
        "",
        f"Host: `{socket.gethostname()}`; platform: `{platform.platform()}`.",
        "",
        "Time modes: `localized` uses one time allocation group per 1 ms; "
        "`distributed` uses one group per numerology slot.",
        "",
        "Frequency modes: `grouped` uses configured RBGs; `per_rb` uses one "
        "PRB per allocation unit.",
        "",
        markdown_table(rows),
        "",
    ]
    (output / "report.md").write_text("\n".join(report))
    source_sha = subprocess.check_output(
        ["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip()
    source_dirty = bool(
        subprocess.check_output(
            ["git", "status", "--porcelain"], cwd=ROOT, text=True
        ).strip()
    )
    metadata = {
        "schema_version": 1,
        "source_sha": source_sha,
        "source_dirty": source_dirty,
        "host": {
            "hostname": socket.gethostname(),
            "platform": platform.platform(),
            "python": platform.python_version(),
        },
        "steps": args.steps,
        "repeats": args.repeats,
        "ue_counts": ue_counts,
        "modes": modes,
        "grids": GRID_PROFILES,
    }
    (output / "metadata.json").write_text(
        json.dumps(metadata, indent=2, sort_keys=True) + "\n")
    print(output)


if __name__ == "__main__":
    main()
