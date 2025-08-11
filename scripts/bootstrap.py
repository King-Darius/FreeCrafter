#!/usr/bin/env python3
"""Fetch a Qt runtime, build FreeCrafter, and bundle Qt libraries.

After cloning the repository, run this script with Python 3 to obtain a
minimal Qt installation using the `aqtinstall` utility, configure CMake with
that Qt, build the project, and run the appropriate deployment tool so the
resulting executable includes the required Qt libraries next to it.

If you already have Qt 6 installed you may point the script at it by setting
``CMAKE_PREFIX_PATH`` or ``Qt6_DIR`` before running. The script will also
search common installation directories such as ``~/Qt`` or ``C:/Qt`` and use
the first Qt it finds. If no suitable Qt is located it falls back to
downloading one. The user only needs Python and a compiler toolchain; no
manual Qt installation steps are required."""

import os
import subprocess
import sys
from pathlib import Path
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


def ensure_aqt():
    try:
        import aqtinstall  # noqa: F401
    except ImportError:
        req_file = Path(__file__).with_name("requirements.txt")
        try:
            subprocess.check_call(
                [
                    sys.executable,
                    "-m",
                    "pip",
                    "install",
                    "--user",
                    "-r",
                    str(req_file),
                ]
            )
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
    subprocess.check_call(cmd)

    prefix = detect_qt()
    if prefix:
        return prefix

    raise SystemExit(
        "Qt not found. Install Qt 6 manually and set CMAKE_PREFIX_PATH or Qt6_DIR to its prefix."
    )


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
    main()
