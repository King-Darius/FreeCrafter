"""Package the GUI bootstrap script into a standalone executable.

Run this script to generate a self-contained executable using PyInstaller.
"""
from pathlib import Path

import PyInstaller.__main__


def main():
    gui_script = Path(__file__).with_name("gui_bootstrap.py")
    PyInstaller.__main__.run([
        str(gui_script),
        "--onefile",
        "--name",
        "FreeCrafterInstaller",
    ])


if __name__ == "__main__":
    main()
