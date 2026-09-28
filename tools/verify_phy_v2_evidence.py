#!/usr/bin/env python3
"""Verify hashes in the committed PHY Model V2 evidence manifest."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_MANIFEST = (
    ROOT / "docs" / "baselines" / "phy-model-v2-evidence-manifest.json"
)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def check(path_text: str, expected: str) -> None:
    path = ROOT / path_text
    if not path.is_file():
        raise ValueError(f"evidence file is missing: {path_text}")
    actual = sha256(path)
    if actual != expected:
        raise ValueError(
            f"evidence hash mismatch for {path_text}: "
            f"expected {expected}, got {actual}")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    args = parser.parse_args()
    manifest = json.loads(args.manifest.resolve().read_text())
    checked = 0

    for key in ("production_catalog", "production_manifest"):
        artifact = manifest["map_generation"][key]
        check(artifact["path"], artifact["sha256"])
        checked += 1

    for profile in manifest["canonical_profiles"]:
        check(profile["path"], profile["validated_sha256"])
        checked += 1

    for batch in manifest["run_batches"]:
        for path_key, hash_key in (
            ("manifest", "manifest_sha256"),
            ("summary", "summary_sha256"),
            ("paired_summary", "paired_summary_sha256"),
        ):
            if path_key in batch:
                check(batch[path_key], batch[hash_key])
                checked += 1
        if "manifest" in batch:
            run_manifest = json.loads(
                (ROOT / batch["manifest"]).read_text())
            for run in run_manifest.get("runs", []):
                check(
                    run["rendered_config"],
                    run["rendered_config_sha256"],
                )
                checked += 1

    identifiers = set()
    for artifact in manifest["committed_evidence"]:
        identifier = artifact["id"]
        if identifier in identifiers:
            raise ValueError(f"duplicate evidence identifier: {identifier}")
        identifiers.add(identifier)
        check(artifact["path"], artifact["sha256"])
        checked += 1

    if manifest["pending_evidence"]:
        raise ValueError(
            f"evidence remains pending: {manifest['pending_evidence']}")
    print(f"verified {checked} PHY Model V2 evidence artifacts")


if __name__ == "__main__":
    main()
