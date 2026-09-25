#!/usr/bin/env python3
"""Build a deterministic provenance manifest for shipped FikoRE maps."""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
DEFAULT_MAP_DIR = ROOT / "include" / "maps_scenarios"
DEFAULT_OUTPUT = DEFAULT_MAP_DIR / "MANIFEST.json"
MAP_NAME = re.compile(
    r"macroscopic_fading_map_(?P<scenario>[A-Z_]+)_(?P<frequency>[0-9.]+)\.json"
)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def summarize_map(path: Path) -> dict:
    match = MAP_NAME.fullmatch(path.name)
    if match is None:
        raise ValueError(f"unsupported map filename: {path.name}")
    payload = json.loads(path.read_text())
    cell_number = int(payload["cell_number"])
    cell_size = float(payload["cell_size"])
    values = payload["map"]
    if len(values) != cell_number:
        raise ValueError(f"{path.name}: row count does not match cell_number")

    minimum = math.inf
    maximum = -math.inf
    total = 0.0
    samples = 0
    for row in values:
        if len(row) != cell_number:
            raise ValueError(f"{path.name}: map is not square")
        for raw_value in row:
            value = float(raw_value)
            if not math.isfinite(value):
                raise ValueError(f"{path.name}: map contains a non-finite value")
            minimum = min(minimum, value)
            maximum = max(maximum, value)
            total += value
            samples += 1

    return {
        "file": path.name,
        "sha256": sha256(path),
        "scenario": match.group("scenario"),
        "frequency_ghz": float(match.group("frequency")),
        "cell_number": cell_number,
        "cell_size_m": cell_size,
        "samples": samples,
        "minimum_db": minimum,
        "mean_db": total / samples,
        "maximum_db": maximum,
        "schema": "legacy-v1",
        "generator": "legacy-matlab-unseeded",
        "seed": None,
        "provenance_status": "historical-realization",
    }


def build_manifest(map_dir: Path) -> dict:
    map_paths = sorted(map_dir.glob("macroscopic_fading_map_*.json"))
    if not map_paths:
        raise ValueError(f"no FikoRE maps found in {map_dir}")
    return {
        "schema_version": 1,
        "map_schema": "legacy-v1",
        "map_count": len(map_paths),
        "notes": [
            "Map arrays are historical stochastic realizations.",
            "Original RNG seeds and exact generator commits are not available.",
            "No map bytes are changed by this manifest.",
        ],
        "maps": [summarize_map(path) for path in map_paths],
    }


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--map-dir", type=Path, default=DEFAULT_MAP_DIR)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()

    manifest = build_manifest(args.map_dir.resolve())
    rendered = json.dumps(manifest, indent=2, sort_keys=True) + "\n"
    output = args.output.resolve()
    if args.check:
        if not output.is_file() or output.read_text() != rendered:
            raise SystemExit(
                f"map manifest is stale; regenerate with {Path(__file__).name}")
        print(f"map manifest is current: {output}")
        return
    output.write_text(rendered)
    print(output)


if __name__ == "__main__":
    main()
