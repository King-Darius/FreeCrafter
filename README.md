# FreeCrafter  <img width="30" height="30" alt="FreeCrafter Logo" src="https://github.com/user-attachments/assets/4fb35500-bc0c-4275-97a9-96ad20268567" />

> **Status:** Pre-alpha. Most roadmap features are missing and the application is not yet usable for real work.  Also, like all open source code, use at your own risk.

FreeCrafter is an experimental 3D modeling sandbox built with Qt and OpenGL. At this stage it only offers a minimal interface and a few prototype tools; many menu items are placeholders and the program may crash.

## About FreeCrafter

<div align="center">
<img width="218.5" height="328.5" alt="image" src="https://github.com/user-attachments/assets/27a4e54b-a6d0-4513-907d-960175ea2e24" />
</div>


FreeCrafter is a versatile 3D modeling tool designed for designers, engineers, and creative professionals. With an intuitive, user-friendly interface and a range of powerful modeling tools, FreeCrafter enables you to create 3D models for a wide array of applications—whether you're working on product prototypes, building designs, or creative projects. FreeCrafter is designed for users of all experience levels, offering the flexibility to turn ideas into detailed 3D models.
Key Features:

    Intuitive 3D Modeling: Use simple tools to draw, shape, and modify 3D objects, making it easy to bring your designs to life, regardless of your experience with 3D software.

    Customizable Models: Build your own models from scratch or modify existing ones to suit your needs, giving you complete control over your design process.

    Precision Tools: Access tools for accurate measurement and alignment, ensuring your models meet exact specifications.

    Multi-Platform Support: FreeCrafter is available for Windows, macOS, and likely Linux, providing compatibility across major desktop operating systems.

Use Cases:

    Building & Architecture: Design 3D models of buildings and architectural elements. While FreeCrafter doesn't have a dedicated floor plan feature, it's perfect for creating detailed models of structures, facades, and interiors.

    Product Design: Quickly prototype and visualize products, from small objects to complex designs, for testing, manufacturing, or presentation.

    Engineering & Manufacturing: Model mechanical components, assemblies, or other engineering designs, ideal for analysis, simulation, or prototyping.

    Creative Projects: Bring your artistic ideas to life, whether for visual arts, conceptual designs, or other creative pursuits.

FreeCrafter provides an approachable yet powerful environment for creating 3D models, offering flexibility for a variety of projects while maintaining simplicity in its design and workflow.


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

FreeCrafter ships automation that downloads Qt, configures CMake, runs the
build, and even provisions the right environment for platform-specific test
runs. The following sections summarize the most common entry points; deeper
automation notes live in [docs/testing.md](docs/testing.md).

### Bootstrap the toolchain

The repository includes a helper script that fetches a minimal Qt runtime and
builds the project for you. After cloning, simply run the bootstrap script:

```bash
python scripts/bootstrap.py
```

If you already have Qt 6 installed, set the `CMAKE_PREFIX_PATH` or `Qt6_DIR`
environment variable so the script can use it:

```bash
export CMAKE_PREFIX_PATH=/path/to/Qt/6.5.3/gcc_64
# or
export Qt6_DIR=/path/to/Qt/6.5.3/gcc_64/lib/cmake/Qt6
python scripts/bootstrap.py
```

The script also searches common install locations such as `~/Qt` on Unix
systems or `C:/Qt` on Windows and falls back to downloading Qt if none is
found.

If you prefer a graphical installer, launch:

```bash
python scripts/gui_bootstrap.py
```

This GUI streams progress from `bootstrap.py` and provides an "Install" button
to start the build. To create a standalone executable that users can
double‑click, run:

```bash
python scripts/package_gui_bootstrap.py
```

The command uses [PyInstaller](https://pyinstaller.org/) and writes the bundled
executable to the `dist` directory.

> **Note:** IDEs or language servers may report `Import "aqtinstall" could not
> be resolved` until the packages listed in
> [`scripts/requirements.txt`](scripts/requirements.txt) are installed for the
> Python interpreter they use. Creating a virtual environment and ensuring `pip`
> points to that interpreter usually resolves the warning.

The script installs its Python dependencies (including `aqtinstall`) from
[PyPI](https://pypi.org/project/aqtinstall/) if they are missing, downloads the
necessary Qt libraries from the official Qt servers, configures CMake with that
Qt, compiles FreeCrafter, and runs the appropriate deployment utility so the
resulting executable already includes the Qt runtime.

The built program can be found in the `build` directory and should run on a
machine without any additional setup.

Though we hope to skip even this step in future.

### Windows test helper

When running the renderer regression tests on Windows, prefer the PowerShell
wrapper in `scripts/run_tests_with_qt_env.ps1` instead of invoking `ctest`
directly. The script extends `PATH` so the bundled Qt runtime can be found,
sets diagnostic environment variables, and exports
`FREECRAFTER_RENDER_SKIP_COVERAGE=1` to keep the tests stable on machines that
fall back to the software WARP renderer.

```powershell
powershell -ExecutionPolicy Bypass -File scripts/run_tests_with_qt_env.ps1 -UseCTest
```

Use raw `ctest` only if you have already bootstrapped a shell where Qt's
`bin` directory precedes everything else on `PATH` and you are confident the
default platform plugin works in your environment. Additional logging options
and troubleshooting tips are collected in
[docs/testing.md](docs/testing.md#running-the-windows-regression-test).

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
See [docs/testing.md](docs/testing.md) for regression test notes.

### Troubleshooting Qt plugin discovery on Windows

When running the automated test suite on Windows, prefer invoking the helper
script with the `-UseCTest` flag:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/run_tests_with_qt_env.ps1 -UseCTest
```

The script now forces `QT_DEBUG_PLUGINS=1` for the CTest invocation and extends
the process environment so Qt can locate its `platforms`, `styles`, and other
plugin directories that ship alongside the bootstrapped runtime. The resulting
log output is written directly to the console, making it easier to diagnose any
future plugin loading failures without additional setup.


