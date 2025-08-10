# FreeCrafter  <img width="30" height="30" alt="FreeCrafter Logo" src="https://github.com/user-attachments/assets/4fb35500-bc0c-4275-97a9-96ad20268567" />

> **Status:** Pre-alpha. Most roadmap features are missing and the application is not yet usable for real work.

FreeCrafter is an experimental 3D modeling sandbox built with Qt and OpenGL. At this stage it only offers a minimal interface and a few prototype tools; many menu items are placeholders and the program may crash.


## Current Capabilities
For a breakdown of planned milestones and features, see the [ROADMAP](ROADMAP.md).

## Features

- Qt-based desktop interface with a custom OpenGL viewport
- Camera controller supporting orbit, pan, and zoom
- Minimal geometry kernel that stores curves and extruded solids
- Prototype tools for selection and freehand sketching on the ground plane with basic extrusion

## Planned Features
- Robust sketching and constraint tools
- File operations (new, open, save) and persistent projects
- Packaging into ready-to-run installers or app bundles
- Undo/redo, layers, and many other modeling functions

## Building

The repository ships with a helper script that fetches a minimal Qt runtime and
builds the project for you. After cloning, simply run the bootstrap script:

```bash
python scripts/bootstrap.py
```

If you prefer a graphical installer, launch:

```bash
python scripts/gui_bootstrap.py
```

This GUI streams progress from `bootstrap.py` and provides an "Install" button to start the build. To create a standalone executable that users can double‑click, run:

```bash
python scripts/package_gui_bootstrap.py
```

The command uses [PyInstaller](https://pyinstaller.org/) and writes the bundled executable to the `dist` directory.


> **Note:** IDEs or language servers may report `Import "aqtinstall" could not be resolved`
> until the packages listed in [`scripts/requirements.txt`](scripts/requirements.txt) are
> installed for the Python interpreter they use. Creating a virtual environment and
> ensuring `pip` points to that interpreter usually resolves the warning.

The script installs its Python dependencies (including `aqtinstall`) from
[PyPI](https://pypi.org/project/aqtinstall/) if they are missing, downloads the
necessary Qt libraries from the official Qt servers, configures CMake with that
Qt, compiles FreeCrafter, and runs the appropriate deployment utility so the
resulting executable already includes the Qt runtime.

The built program can be found in the `build` directory and should run on a
machine without any additional setup.

Though we hope to skip even this step in future.

### Installing or Packaging

Official releases ship ready-to-run installers for each platform.  Download the
appropriate package from the project's GitHub Releases page and double-click to
run:

| Platform | File |
| --- | --- |
| **Windows** | `FreeCrafter-<version>.exe` |
| **macOS** | `FreeCrafter-<version>.dmg` |
| **Linux** | `FreeCrafter-<version>.AppImage` |

The `<version>` placeholder should match the release version number you're
downloading (for example, `FreeCrafter-0.1.0.dmg`).
=======
- `FreeCrafter-<version>.exe` for Windows
- `FreeCrafter-<version>.dmg` for macOS
- `FreeCrafter-<version>.AppImage` for Linux

After configuring and building locally, the project can be staged for distribution.

To copy the app bundle into a separate directory, run:

```bash
cmake --install build --prefix dist
# or just
cmake --install build
```

The install step writes the executable or bundle into the chosen prefix. For
example, using the `dist` prefix above yields `dist/FreeCrafter.exe` on Windows
or `dist/FreeCrafter.app` on macOS.

From within the `build` directory you can also create an installer with
[`cpack`](https://cmake.org/cmake/help/latest/module/CPack.html):

```bash
cpack
```

On Windows this generates an NSIS `.exe` installer; on macOS it creates a
Drag’n’Drop `.dmg` containing the app bundle. The resulting package is written
to the `build` directory.

## Resources
Icon assets are stored under `resources/icons` and bundled using Qt's resource system.  They are currently only dummy assets.

## License
This project is released under the terms of the MIT License. See [LICENSE](LICENSE) for details.
