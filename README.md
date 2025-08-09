# FreeCrafter  <img width="30" height="30" alt="FreeCrafter Logo" src="https://github.com/user-attachments/assets/4fb35500-bc0c-4275-97a9-96ad20268567" />

Still very early WIP. Nothing "works" just yet.

FreeCrafter is an experimental 3D modeling sandbox built with Qt and OpenGL. It currently provides a minimal interface and a handful of prototype tools for sketching and extruding simple geometry.

For a breakdown of planned milestones and features, see the [ROADMAP](ROADMAP.md).

## Features
- Qt-based desktop interface with a custom OpenGL viewport
- Camera controller supporting orbit, pan, and zoom
- Geometry kernel that stores curves and extruded solids
- Tools for selection, freehand sketching on the ground plane, and basic extrusion

## Building

The repository ships with a helper script that fetches a minimal Qt runtime and
builds the project for you. After cloning, simply run the bootstrap script:

```bash
python scripts/bootstrap.py
```

The script installs its Python dependencies (including `aqtinstall`) if they are
missing, downloads the necessary Qt libraries, configures CMake with that Qt,
compiles FreeCrafter, and runs the appropriate deployment utility so the
resulting executable already includes the Qt runtime.

The built program can be found in the `build` directory and should run on a
machine without any additional setup.

Though we hope to skip even this step in future.

### Installing or Packaging

After configuring and building, the project can be staged for distribution.

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
