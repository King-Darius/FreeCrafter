#!/usr/bin/env python3
"""Fetch a Qt runtime, build FreeCrafter, and bundle Qt libraries.

After cloning the repository, run this script with Python 3 to obtain a
minimal Qt installation using the ``aqtinstall`` utility, configure CMake with
that Qt, build the project, and run the appropriate deployment tool so the
resulting executable includes the required Qt libraries next to it.


The user only needs Python and a compiler toolchain; no manual Qt installation
steps are required.  A pre-downloaded Qt tree and wheel cache can be reused by
passing the ``--offline`` and ``--wheel-cache`` options.
"""
=======
##If you already have Qt 6 installed you may point the script at it by setting
##``CMAKE_PREFIX_PATH`` or ``Qt6_DIR`` before running. The script will also
##search common installation directories such as ``~/Qt`` or ``C:/Qt`` and use
##the first Qt it finds. If no suitable Qt is located it falls back to
##downloading one. The user only needs Python and a compiler toolchain; no
##manual Qt installation steps are required."""


import argparse
import hashlib
import logging
import os
import subprocess
import sys
import time
from pathlib import Path
from typing import List, Optional
from typing import Optional


QT_VERSION = "6.5.3"
HOST = {
    "linux": "linux",
    "linux2": "linux",
    "darwin": "mac",
    "win32": "windows",
}[sys.platform]
ARCH = {
    "linux": "gcc_64",
    "linux2": "gcc_64",
    "darwin": "clang_64",
    "win32": "win64_msvc2019_64",
}[sys.platform]

root = Path(__file__).resolve().parent.parent
qt_root = root / "qt"

install_dir = qt_root / QT_VERSION / ARCH


def _valid_prefix(prefix: Path) -> bool:
    """Return True if *prefix* appears to be a Qt installation."""
    bin_path = prefix / "bin"
    if not bin_path.is_dir():
        return False
    # Presence of the qtpaths tool is a good indicator of a usable prefix.
    candidates = ["qtpaths", "qtpaths6"]
    if sys.platform == "win32":
        candidates = [c + ".exe" for c in candidates]
    return any((bin_path / c).exists() for c in candidates)


def detect_qt() -> Optional[Path]:
    """Return the first valid Qt prefix from common environment locations."""
    env_vars = ["CMAKE_PREFIX_PATH", "QT_PREFIX_PATH", "QTDIR"]
    for var in env_vars:
        value = os.environ.get(var)
        if not value:
            continue
        for path in value.split(os.pathsep):
            prefix = Path(path)
            if _valid_prefix(prefix):
                return prefix
    if _valid_prefix(install_dir):
        return install_dir
    return None


def hash_file(path: Path) -> str:
    """Return the SHA256 hash of *path*."""
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(65536), b""):
            h.update(chunk)
    return h.hexdigest()


def verify_checksums(cache: Path) -> None:
    """Validate wheel cache entries using a checksums file if present."""
    if not cache.exists():
        logging.info("Wheel cache directory %s does not exist; skipping validation", cache)
        return
    checksum_file = cache / "checksums.txt"
    if not checksum_file.exists():
        logging.info("No checksum file at %s; skipping validation", checksum_file)
        return
    with checksum_file.open() as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            parts = line.split()
            if len(parts) != 2:
                raise ValueError(f"Malformed checksum line: {line!r}")
            digest, filename = parts
            file_path = cache / filename
            if not file_path.exists() or hash_file(file_path) != digest:
                raise RuntimeError(f"Checksum mismatch for {file_path}")


def run(cmd, *, retries: int = 3, **kwargs) -> int:
    """Run *cmd* with retry logic, logging, and return its exit code."""
    for attempt in range(1, retries + 1):
        logging.info("Running: %s", " ".join(map(str, cmd)))
        result = subprocess.run(cmd, **kwargs)
        if result.returncode == 0:
            return 0
        logging.error("Command failed with code %s", result.returncode)
        if attempt == retries:
            return result.returncode
        logging.info("Retrying (%s/%s)...", attempt + 1, retries)
        time.sleep(1)


def ensure_aqt(offline: bool, wheel_cache: Optional[Path]) -> None:
    try:
        import aqtinstall  # noqa: F401
        return
    except ImportError:
        req_file = Path(__file__).with_name("requirements.txt")
        if offline and wheel_cache is None:
            raise RuntimeError(
                "aqtinstall is missing and no wheel cache was provided in offline mode"
            )

        cmd = [sys.executable, "-m", "pip", "install"]
        if offline:
            verify_checksums(wheel_cache)
            cmd += ["--no-index", "--find-links", str(wheel_cache)]
        else:
            cmd += ["--user"]
        cmd += ["-r", str(req_file)]
        rc = run(cmd)
        if rc != 0:
            raise subprocess.CalledProcessError(rc, cmd)


def ensure_qt(offline: bool) -> Path:
    """Ensure a Qt installation exists and return its prefix."""
    prefix = detect_qt()
    if prefix:
        logging.info("Using existing Qt at %s", prefix)
        return prefix
    if offline:
        raise RuntimeError(f"Qt not found in offline mode; looked for {install_dir}")
        except subprocess.CalledProcessError:
            print(
                f"Failed to install Python dependencies from {req_file}. "
                "Please run 'pip install -r scripts/requirements.txt' manually and rerun the script."
            )
            raise


def _valid_prefix(path: Path) -> bool:
    return (path / "bin" / "qtpaths").exists()


def detect_qt() -> Optional[Path]:
    """Return the prefix of an existing Qt installation if one is found."""

    candidates = []
    cprefix = os.environ.get("CMAKE_PREFIX_PATH")
    if cprefix:
        candidates.extend(cprefix.split(os.pathsep))
    qt6_dir = os.environ.get("Qt6_DIR")
    if qt6_dir:
        candidates.append(Path(qt6_dir).expanduser().parents[2])

    for c in candidates:
        p = Path(c).expanduser()
        if _valid_prefix(p):
            return p

    if _valid_prefix(install_dir):
        return install_dir

    search_roots = [Path.home() / "Qt", Path("C:/Qt"), Path("/opt/Qt")]
    if sys.platform == "darwin":
        search_roots.append(Path("/Applications/Qt"))

    for base in search_roots:
        if not base.exists():
            continue
        for version_dir in base.glob("6.*"):
            for arch_dir in version_dir.glob("*"):
                if _valid_prefix(arch_dir):
                    return arch_dir

    return None


def ensure_qt() -> Path:
    """Ensure a Qt prefix is available and return its path."""

    prefix = detect_qt()
    if prefix:
        return prefix

    ensure_aqt()
    cmd = [
        sys.executable,
        "-m",
        "aqtinstall",
        "qt",
        HOST,
        "desktop",
        QT_VERSION,
        ARCH,
        "-O",
        str(qt_root),
    ]
    rc = run(cmd)
    if rc != 0:
        raise subprocess.CalledProcessError(rc, cmd)
    prefix = detect_qt()
    if not prefix:
        raise RuntimeError("Qt installation failed")
    return prefix

    prefix = detect_qt()
    if prefix:
        return prefix

    raise SystemExit(
        "Qt not found. Install Qt 6 manually and set CMAKE_PREFIX_PATH or Qt6_DIR to its prefix."
    )



def run_cmake(prefix: Path) -> None:
    build_dir = root / "build"
    build_dir.mkdir(exist_ok=True)
    env = os.environ.copy()
    bin_dir = prefix / "bin"
    env["PATH"] = str(bin_dir) + os.pathsep + env.get("PATH", "")
    prefix_arg = f"-DCMAKE_PREFIX_PATH={prefix}"
    rc = run(["cmake", "-S", str(root), "-B", str(build_dir), prefix_arg], env=env)
    if rc != 0:
        raise subprocess.CalledProcessError(rc, ["cmake", "-S", str(root), "-B", str(build_dir), prefix_arg])
    rc = run(["cmake", "--build", str(build_dir)], env=env)
    if rc != 0:
        raise subprocess.CalledProcessError(rc, ["cmake", "--build", str(build_dir)])


def main(argv: Optional[List[str]] = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--offline",
        action="store_true",
        help="Reuse existing Qt tree and wheel cache without network access",
    )
    parser.add_argument(
        "--wheel-cache",
        type=Path,
        help="Directory containing pre-downloaded wheels for offline mode",
    )
    args = parser.parse_args(argv)

    logging.basicConfig(level=logging.INFO, format="%(message)s")

    try:
        ensure_aqt(args.offline, args.wheel_cache)
        prefix = ensure_qt(args.offline)
        run_cmake(prefix)
    except subprocess.CalledProcessError as exc:  # pragma: no cover
        logging.error("Command '%s' failed with code %s", " ".join(map(str, exc.cmd)), exc.returncode)
        return exc.returncode
    except Exception as exc:  # pragma: no cover - visible to caller
        logging.error(str(exc))
        return 1
    logging.info("Build complete. Run the executable in the 'build' directory.")
    return 0

def run_cmake(prefix: Path):
    build_dir = root / "build"
    build_dir.mkdir(exist_ok=True)
    env = os.environ.copy()
    env["PATH"] = str(prefix / "bin") + os.pathsep + env.get("PATH", "")
    prefix_arg = f"-DCMAKE_PREFIX_PATH={prefix}"
    subprocess.check_call(["cmake", "-S", str(root), "-B", str(build_dir), prefix_arg], env=env)
    subprocess.check_call(["cmake", "--build", str(build_dir)], env=env)


def main():
    prefix = ensure_qt()
    run_cmake(prefix)
    print("Build complete. Run the executable in the 'build' directory.")



if __name__ == "__main__":
    sys.exit(main())
