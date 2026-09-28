#!/usr/bin/env python3
"""Verify hashes in the committed PHY Model V2 evidence manifest."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import subprocess
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


def check_git_blob(commit: str, path_text: str, expected: str) -> None:
    payload = subprocess.check_output(
        ["git", "show", f"{commit}:{path_text}"],
        cwd=ROOT,
    )
    actual = hashlib.sha256(payload).hexdigest()
    if actual != expected:
        raise ValueError(
            f"git blob hash mismatch for {commit}:{path_text}: "
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

    map_manifest_path = (
        ROOT / manifest["map_generation"]["production_manifest"]["path"]
    )
    map_manifest = json.loads(map_manifest_path.read_text())
    for entry in map_manifest["maps"]:
        check(
            f"include/maps_scenarios/{entry['file']}",
            entry["sha256"],
        )
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
            if run_manifest.get("source_sha") != batch["source_sha"]:
                raise ValueError(
                    f"run source mismatch for {batch['id']}")
            for run in run_manifest.get("runs", []):
                check(
                    run["rendered_config"],
                    run["rendered_config_sha256"],
                )
                checked += 1
                if "map_source_git_commit" in run:
                    check_git_blob(
                        run["map_source_git_commit"],
                        run["map_source_git_path"],
                        run["map_sha256"],
                    )
                else:
                    check(
                        f"include/maps_scenarios/{run['map_file']}",
                        run["map_sha256"],
                    )
                checked += 1

    analysis_metadata = json.loads(
        (
            ROOT
            / "docs/baselines/phy-v2-production-analysis-metadata.json"
        ).read_text()
    )
    if (
        analysis_metadata.get("analyzer_source_sha")
        != manifest["source"]["analyzer_source_sha"]
    ):
        raise ValueError("analysis metadata source SHA is stale")

    manuscript = (
        ROOT / "docs/phy-model-evolution-external-review.md"
    ).read_text()
    implementation_match = re.search(
        r"\*\*Model implementation source:\*\* `([0-9a-f]{40})`",
        manuscript,
    )
    profile_match = re.search(
        r"\*\*Packet-profile source:\*\* `([0-9a-f]{40})`",
        manuscript,
    )
    runtime_match = re.search(
        r"\*\*Runtime-benchmark source:\*\* `([0-9a-f]{40})`",
        manuscript,
    )
    if (
        implementation_match is None
        or implementation_match.group(1)
        != manifest["source"]["implementation_source_sha"]
    ):
        raise ValueError("manuscript implementation source SHA is stale")
    if (
        profile_match is None
        or profile_match.group(1)
        != manifest["source"]["packet_profile_source_sha"]
    ):
        raise ValueError("manuscript profile source SHA is stale")
    if (
        runtime_match is None
        or runtime_match.group(1)
        != manifest["source"]["runtime_benchmark_source_sha"]
    ):
        raise ValueError("manuscript runtime source SHA is stale")
    subprocess.check_call(
        [
            "git",
            "cat-file",
            "-e",
            f"{manifest['source']['map_catalog_source_sha']}^{{commit}}",
        ],
        cwd=ROOT,
    )

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
