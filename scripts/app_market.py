#!/usr/bin/env python3
"""Generate and validate ESP App Market release manifests."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_TEMPLATE = ROOT / "app-market.template.json"
VERSION_FILE = ROOT / "src" / "config.h"
SEMVER_RE = re.compile(
    r"^(0|[1-9]\d*)\.(0|[1-9]\d*)\.(0|[1-9]\d*)"
    r"(?:-([0-9A-Za-z-]+(?:\.[0-9A-Za-z-]+)*))?"
    r"(?:\+([0-9A-Za-z-]+(?:\.[0-9A-Za-z-]+)*))?$"
)
SHA256_RE = re.compile(r"^[0-9a-f]{64}$")
OFFSET_RE = re.compile(r"^0x[0-9a-fA-F]+$")


class ManifestError(ValueError):
    """Raised when release input or a manifest is invalid."""


def normalize_version(value: str) -> str:
    value = value.strip()
    if value.startswith("refs/tags/"):
        value = value[len("refs/tags/") :]
    if value.startswith("v"):
        value = value[1:]
    if not SEMVER_RE.fullmatch(value):
        raise ManifestError(f"invalid SemVer version: {value!r}")
    return value


def is_prerelease(version: str) -> bool:
    match = SEMVER_RE.fullmatch(normalize_version(version))
    return bool(match and match.group(4))


def source_version() -> str:
    ref = os.environ.get("GITHUB_REF_NAME", "")
    if ref.startswith("v"):
        return normalize_version(ref)

    try:
        tag = subprocess.run(
            ["git", "describe", "--tags", "--exact-match", "--match", "v*"],
            cwd=ROOT,
            check=True,
            capture_output=True,
            text=True,
        ).stdout.strip()
        return normalize_version(tag)
    except (OSError, subprocess.CalledProcessError, ManifestError):
        pass

    text = VERSION_FILE.read_text(encoding="utf-8")
    match = re.search(r'^#define\s+VERSION\s+"([^"]+)"', text, re.MULTILINE)
    if not match:
        raise ManifestError(f"VERSION not found in {VERSION_FILE}")
    return normalize_version(match.group(1))


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def discover_artifact(source: str, root: Path = ROOT) -> Path:
    path = Path(source)
    if not path.is_absolute():
        path = root / path
    if not path.is_file():
        raise ManifestError(f"expected artifact is missing: {path}")
    return path


def validate_ranges(partitions: list[dict], sizes: dict[str, int]) -> None:
    ranges = []
    for partition in partitions:
        asset = partition["asset"]
        start = int(partition["offset"], 16)
        end = start + sizes[asset]
        ranges.append((start, end, partition["name"]))
    ranges.sort()
    for previous, current in zip(ranges, ranges[1:]):
        if current[0] < previous[1]:
            raise ManifestError(
                f"flash ranges overlap: {previous[2]} ends at 0x{previous[1]:x}, "
                f"but {current[2]} starts at 0x{current[0]:x}"
            )


def generate(template_path: Path, output: Path, assets_dir: Path | None, version: str) -> dict:
    template = json.loads(template_path.read_text(encoding="utf-8"))
    partitions = []
    sizes: dict[str, int] = {}

    for definition in template["release"]["partitions"]:
        source = discover_artifact(definition["source"])
        asset = definition["asset"]
        if Path(asset).name != asset:
            raise ManifestError(f"asset filename must not contain a path: {asset}")
        sizes[asset] = source.stat().st_size
        partitions.append(
            {
                "name": definition["name"],
                "asset": asset,
                "offset": definition["offset"],
                "sha256": sha256_file(source),
            }
        )
        if assets_dir:
            assets_dir.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(source, assets_dir / asset)

    validate_ranges(partitions, sizes)
    manifest = {
        "schemaVersion": template["schemaVersion"],
        "app": template["app"],
        "release": {"version": normalize_version(version), "partitions": partitions},
    }

    if assets_dir:
        for definition in template.get("releaseAssets", []):
            source = discover_artifact(definition["source"])
            asset = definition["asset"]
            if Path(asset).name != asset:
                raise ManifestError(f"asset filename must not contain a path: {asset}")
            shutil.copyfile(source, assets_dir / asset)

    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(manifest, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    return manifest


def validate(manifest_path: Path, assets_dir: Path) -> dict:
    assets_dir = assets_dir.resolve()
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    if manifest.get("schemaVersion") != 1:
        raise ManifestError("schemaVersion must be 1")
    app = manifest.get("app")
    release = manifest.get("release")
    if not isinstance(app, dict) or not isinstance(release, dict):
        raise ManifestError("manifest must contain app and release objects")
    required_app = {"id", "name", "description", "supportedDevices", "tags", "icon"}
    if not required_app.issubset(app):
        raise ManifestError("app object is missing required fields")
    normalize_version(release.get("version", ""))
    partitions = release.get("partitions")
    if not isinstance(partitions, list) or not partitions:
        raise ManifestError("release.partitions must be a non-empty array")

    sizes: dict[str, int] = {}
    for partition in partitions:
        if set(partition) != {"name", "asset", "offset", "sha256"}:
            raise ManifestError("partition fields must be name, asset, offset, and sha256")
        asset = partition["asset"]
        if Path(asset).name != asset:
            raise ManifestError(f"asset filename must not contain a path: {asset}")
        if not OFFSET_RE.fullmatch(partition["offset"]):
            raise ManifestError(f"invalid offset for {asset}: {partition['offset']}")
        if not SHA256_RE.fullmatch(partition["sha256"]):
            raise ManifestError(f"invalid SHA-256 for {asset}")
        path = discover_artifact(str(assets_dir / asset))
        sizes[asset] = path.stat().st_size
        actual = sha256_file(path)
        if actual != partition["sha256"]:
            raise ManifestError(f"SHA-256 mismatch for {asset}: expected {partition['sha256']}, got {actual}")

    icon = app["icon"]
    if "://" not in icon:
        discover_artifact(str(assets_dir / icon))
    validate_ranges(partitions, sizes)
    return manifest


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)
    generate_parser = subparsers.add_parser("generate")
    generate_parser.add_argument("--template", type=Path, default=DEFAULT_TEMPLATE)
    generate_parser.add_argument("--output", type=Path, default=ROOT / "app-market.json")
    generate_parser.add_argument("--assets-dir", type=Path)
    generate_parser.add_argument("--version", default=None)
    validate_parser = subparsers.add_parser("validate")
    validate_parser.add_argument("--manifest", type=Path, default=ROOT / "app-market.json")
    validate_parser.add_argument("--assets-dir", type=Path, required=True)
    args = parser.parse_args()

    try:
        if args.command == "generate":
            manifest = generate(args.template, args.output, args.assets_dir, args.version or source_version())
            print(f"Generated {args.output} for version {manifest['release']['version']}")
        else:
            validate(args.manifest, args.assets_dir)
            print(f"Validated {args.manifest}")
    except (KeyError, OSError, json.JSONDecodeError, ManifestError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
