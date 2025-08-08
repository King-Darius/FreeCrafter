#!/usr/bin/env python3
"""Fetch a Qt runtime, build FreeCrafter, and bundle Qt libraries.

After cloning the repository, run this script with Python 3 to obtain a
minimal Qt installation using the `aqtinstall` utility, configure CMake with
that Qt, build the project, and run the appropriate deployment tool so the
resulting executable includes the required Qt libraries next to it.

The user only needs Python and a compiler toolchain; no manual Qt
installation steps are required.
"""

import os
import platform
import subprocess
import sys
from pathlib import Path

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
bin_dir = install_dir / "bin"


def ensure_aqt():
    try:
        import aqtinstall  # noqa: F401
    except ImportError:
        subprocess.check_call([sys.executable, "-m", "pip", "install", "--user", "aqtinstall"])


def ensure_qt():
    if install_dir.exists():
        return
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


def run_cmake():
    build_dir = root / "build"
    build_dir.mkdir(exist_ok=True)
    env = os.environ.copy()
    env["PATH"] = str(bin_dir) + os.pathsep + env.get("PATH", "")
    prefix_arg = f"-DCMAKE_PREFIX_PATH={install_dir}"
    subprocess.check_call(["cmake", "-S", str(root), "-B", str(build_dir), prefix_arg], env=env)
    subprocess.check_call(["cmake", "--build", str(build_dir)], env=env)


def main():
    ensure_qt()
    run_cmake()
    print("Build complete. Run the executable in the 'build' directory.")


if __name__ == "__main__":
    main()
