# FreeCrafter <img src="docs/media/freecrafter-logo.svg" alt="FreeCrafter logo" height="48" align="top" />

> **Status snapshot:** FreeCrafter is still in a **pre-alpha** state. The Qt 6 shell boots, but major UI layout, viewport, and styling bugs remain before the editor is usable for day-to-day work.

FreeCrafter is a cross-platform, tool-driven 3D modeling sandbox that blends CAD-style precision with a lightweight, top-down workspace. The editor pairs a modern Qt 6 desktop experience with a hardware-accelerated OpenGL viewport, rich inference, and a growing catalog of advanced modeling tools.

<div align="center">
  <img width="218.5" height="328.5" alt="FreeCrafter interface preview" src="https://github.com/user-attachments/assets/27a4e54b-a6d0-4513-907d-960175ea2e24" />
</div>

## Project snapshot
- ✅ **Roadmap coverage:** The foundations for Phases 1–7 (core shell, geometry/inference, navigation/view, drawing, object management, advanced tools, and file I/O) exist in the codebase but require polish.
- 🚧 **In progress:** Phase 7.5 (comprehensive bug sweep + UX polish) is focused on stabilizing the View menu, undo/redo, autosave, and accessibility/usability audits.
- ⚠️ **Reality check:** Current desktop builds expose severe regressions in the viewport, docking/layout, and theming pipelines. Expect crashes, broken painting, and inconsistent tool activation until the stabilization work lands.
- 📅 **Next up:** Phases 8–11 (performance, polish, QA, release, and surface painting) remain on the long-term backlog.

## Current limitations
- **Viewport rendering:** Clipping, missing redraws, and camera drift make the main canvas unreliable for production work.
- **GUI layout & styling:** Dock stacks, toolbars, and theme toggles frequently desync; widgets clip or overlap on smaller displays.
- **Tool activation:** The action → tool wiring is incomplete, so several modeling tools fail silently or leave the app in an unusable state.
- **Persistence & recovery:** Autosave/undo/redo paths are fragile. Saving and reopening complex scenes often loses materials or corrupts transforms.
- **Testing coverage:** Automated smoke tests are still being written, so regressions may slip in between builds until we expand the suite.

## Feature goals & active work
The following features are in various stages of implementation. Many ship behind feature flags or require bug fixes before they are production ready.

### Tool-driven workspace
- Multi-toolbar QMainWindow layout with persistent docking, status hints, and a measurement/VCB widget for quick numeric overrides.
- Hotkey manager aligned with the toolbar/menu actions, keeping tool activation and tooltips in sync.

### Modeling & editing depth
- Half-edge geometry core with healing, triangulation, and normals ensures clean solids and faces for subsequent operations.
- Core toolset (Select, Line, Move, Rotate, Scale, Push/Pull, Offset, Follow-Me, Arc/Circle/Polygon, Text, Paint) mirrors familiar CAD workflows with inference-aware previews and typed measurements.
- Integrated advanced suites—Round Corner, CurveIt, PushANDPull, Surface, BezierKnife, QuadTools, SubD, Weld, Vertex Tools, Clean, ClothEngine, and CAD Designer—cover filleting, lofting, subdivision, cleanup, simulation, and solid modeling needs.

### Navigation & visualization
- Orbit/pan/zoom navigation with zoom extents, standard view presets, and axis gizmo support.
- Style system for wireframe, shaded, shaded+edges, hidden line, and monochrome modes, plus sections, solar shadows, and HUD diagnostics (FPS, frame time, draw calls).

### Object management & scenes
- Group/component authoring with make/edit unique flows, tag-driven visibility and colour-by-tag rendering, and an outliner that supports drag-and-drop hierarchy editing.
- Scene snapshots capture camera poses, styles, section states, and tag visibility for rapid context switching.

### File I/O & packaging
- Import/export pipelines for OBJ, STL, glTF, and optional SKP/FBX/DAE formats when the Assimp integrations are enabled.
- CMake + CPack configuration targets NSIS installers, while the Python bootstrap tooling automates Qt acquisition, builds, and GUI packaging.

## Getting started quickly
1. Review the [getting started and troubleshooting guide](docs/getting_started_and_troubleshooting.md) for environment setup tips, bootstrap walkthroughs, and common fixes.
2. Run the bootstrap helper to fetch Qt, configure CMake, build the project, and stage runnable binaries:
   ```bash
   python scripts/bootstrap.py
   ```
3. Need a GUI wrapper? Launch `python scripts/gui_bootstrap.py` for a click-to-install experience or `python scripts/package_gui_bootstrap.py` to bundle a distributable executable.

## Build & test from source
If you maintain your own Qt install, point CMake at it and rerun the bootstrap:
```bash
export CMAKE_PREFIX_PATH=/path/to/Qt/6.x/gcc_64
# or
export Qt6_DIR=/path/to/Qt/6.x/gcc_64/lib/cmake/Qt6
python scripts/bootstrap.py
```

Update the pinned Qt manifest when bumping runtimes:
```bash
python scripts/fetch_qt_runtime.py
```

Run the native build and test loop manually:
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
ctest --test-dir build --output-on-failure
```

> **Note:** IDE language servers may warn about `aqtinstall` imports until the dependencies in [`scripts/requirements.txt`](scripts/requirements.txt) are installed for the selected Python interpreter.

## Run locally

```bash
# POSIX
./scripts/run_local.sh
# PowerShell
./scripts/run_local.ps1
```

No account or email is required. Telemetry is disabled via config/env.

## Documentation & roadmap
- [ROADMAP.md](ROADMAP.md) — long-range plan broken into phases and milestone tracks.
- [docs/status/roadmap-progress.md](docs/status/roadmap-progress.md) — running implementation snapshot mapped to the roadmap.
- [docs/status/2025-02-11-sanity-check.md](docs/status/2025-02-11-sanity-check.md) — latest smoke test summary and pass/fail log.
- [docs/getting_started_and_troubleshooting.md](docs/getting_started_and_troubleshooting.md) — full bootstrap, packaging, and troubleshooting manual.

## Contributing
Please read [CONTRIBUTING.md](CONTRIBUTING.md) for coding standards, contribution flow, and review expectations. Bug triage and release automation notes live in `docs/process/`.

## License
FreeCrafter is released under the terms of the [MIT License](LICENSE).
