#!/usr/bin/env python3
"""Download or refresh the Qt runtime recorded in ``qt/manifest.json``."""

from __future__ import annotations

import argparse
import datetime as _dt
import json
import logging
import shutil
import subprocess
import sys
from pathlib import Path
from typing import List

SCRIPT_DIR = Path(__file__).resolve().parent
REPO_ROOT = SCRIPT_DIR.parent
sys.path.insert(0, str(SCRIPT_DIR))

import bootstrap  # noqa: E402


def _read_manifest(path: Path) -> dict:
    if not path.exists():
        return {}
    try:
        with path.open("r", encoding="utf-8") as f:
            return json.load(f)
    except (OSError, json.JSONDecodeError) as exc:
        logging.warning("Unable to parse %s: %s", path, exc)
        return {}


def _write_manifest(path: Path, data: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8") as f:
        json.dump(data, f, indent=2)
        f.write("\n")


def _normalise_modules(modules: List[str]) -> List[str]:
    seen = set()
    ordered: List[str] = []
    for module in modules:
        key = module.strip()
        if not key or key in seen:
            continue
        ordered.append(key)
        seen.add(key)
    return ordered


def _install_qt(version: str, modules: List[str], *, force: bool) -> None:
    target_dir = bootstrap.qt_root / version / bootstrap.ARCH
    if target_dir.exists():
        if force:
            logging.info("Removing existing Qt runtime at %s", target_dir)
            shutil.rmtree(target_dir)
        else:
            logging.info(
                "Qt runtime already present at %s; skipping download (use --force to reinstall)",
                target_dir,
            )
            return

    bootstrap.ensure_aqt(offline=False, wheel_cache=None)
    cmd = [
        sys.executable,
        "-m",
        "aqtinstall",
        "qt",
        bootstrap.HOST,
        "desktop",
        version,
        bootstrap.ARCH,
        "-O",
        str(bootstrap.qt_root),
    ]
    modules = [m for m in modules if m]
    if modules:
        logging.info("Requesting Qt modules: %s", ", ".join(modules))
        cmd += ["--modules", *modules]
    rc = bootstrap.run(cmd)
    if rc != 0:
        raise subprocess.CalledProcessError(rc, cmd)


def main(argv: List[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--version",
        help="Override the Qt version specified in qt/manifest.json",
    )
    parser.add_argument(
        "--modules",
        nargs="+",
        help="Override the modules listed in qt/manifest.json (space separated)",
    )
    parser.add_argument(
        "--force",
        action="store_true",
        help="Reinstall Qt even if the target directory already exists",
    )
    parser.add_argument(
        "--update-manifest-only",
        action="store_true",
        help="Rewrite qt/manifest.json without downloading Qt",
    )
    parser.add_argument(
        "--verbose",
        action="store_true",
        help="Enable verbose logging",
    )
    args = parser.parse_args(argv)

    logging.basicConfig(level=logging.DEBUG if args.verbose else logging.INFO, format="%(message)s")

    manifest = _read_manifest(bootstrap.manifest_path)
    version = args.version or manifest.get("version") or bootstrap.DEFAULT_QT_VERSION
    modules = args.modules or manifest.get("modules") or bootstrap.DEFAULT_QT_MODULES
    modules = _normalise_modules(modules)

    manifest_update = {
        "version": version,
        "modules": modules,
        "last_updated": _dt.datetime.utcnow().replace(microsecond=0).isoformat() + "Z",
    }

    if not args.update_manifest_only:
        try:
            _install_qt(version, modules, force=args.force)
        except subprocess.CalledProcessError as exc:
            logging.error(
                "Command '%s' failed with exit code %s",
                " ".join(map(str, exc.cmd)),
                exc.returncode,
            )
            return exc.returncode

    _write_manifest(bootstrap.manifest_path, manifest_update)
    logging.info("Updated %s", bootstrap.manifest_path.relative_to(REPO_ROOT))
    return 0


if __name__ == "__main__":
    sys.exit(main())
