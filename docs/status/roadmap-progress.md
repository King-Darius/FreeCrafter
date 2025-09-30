# Roadmap Progress Snapshot

This document cross-references the current FreeCrafter implementation with the
items tracked in `ROADMAP.md` so contributors can quickly see which roadmap
milestones already have code backing them and which ones are still open.

## Release Packaging Readiness

* The Windows release workflow builds the project with CMake, packages it with
  CPack's NSIS generator, and now uploads any generated installer that matches
  `build/*.exe` by enabling globbing on the upload step. This guarantees that a
  `.exe` asset is attached to GitHub releases that are triggered from tags.
  【F:.github/workflows/release-windows.yml†L46-L53】
* The root `CMakeLists.txt` already configures NSIS-specific metadata (display
  name, Start Menu shortcut, prerequisite installer macro) and enables CPack,
  so the workflow's `cpack -G NSIS` invocation produces a Windows installer.
  【F:CMakeLists.txt†L44-L64】

## Roadmap Alignment Highlights

### Phase 1 — Core Shell (implemented)

* **QMainWindow scaffolding:** Multi-tab document bar, a persistent OpenGL
  viewport, toolbar ribbon, and right-side dock panels are fully wired up in
  `MainWindow`. 【F:src/MainWindow.cpp†L35-L274】
* **Status bar & measurement widget:** Live cursor/selection feedback and the
  measurement (VCB-style) widget emit committed values for future tool usage.
  【F:src/MainWindow.cpp†L276-L294】【F:src/ui/MeasurementWidget.cpp†L7-L34】
* **Hotkey map:** Actions for file, view, and tool commands are registered with
  `HotkeyManager` and kept in sync with toolbar tooltips. 【F:src/MainWindow.cpp†L296-L348】
* **Viewport placeholder:** The OpenGL viewport renders the reference grid,
  axes, curve/solid previews, and a HUD showing FPS, frame time, and draw calls
  — matching the roadmap's frame budget HUD milestone. 【F:src/GLViewport.cpp†L57-L199】

### Phase 2 — Geometry & Interaction (early prototype)

* **Line authoring:** The `LineTool` runs as a state machine, projecting screen
  positions onto the ground plane with inference-aware snapping and committing
  continuous polylines to the geometry kernel. 【F:src/Tools/LineTool.cpp†L1-L137】
* **Manipulation prototypes:** Move/Rotate/Scale operate on selected geometry
  using axis-aware inference, with transient previews rendered in the viewport.
  【F:src/Tools/MoveTool.cpp†L1-L142】【F:src/Tools/RotateTool.cpp†L1-L144】【F:src/Tools/ScaleTool.cpp†L1-L163】
* **Extrusion:** The `ExtrudeTool` turns the most recent curve into a simple
  prismatic solid, demonstrating the start of Push/Pull style workflows.
  【F:src/Tools/ExtrudeTool.cpp†L1-L13】
* **Viewport interaction:** Orbit, pan, and zoom gestures are active, with
  cursor world-position projection feeding the status bar updates.
  【F:src/GLViewport.cpp†L200-L287】

### Upcoming Gaps

* Phases 2–4 still need the full inference engine, tool state machines, and the
  richer toolset outlined in the roadmap. The current implementations are
  prototypes that should be expanded with snapping databases, typed overrides,
  and additional drawing/modify tools.
* Later phases (object management, advanced modeling, file I/O, performance,
  polish, QA) remain untouched in code and should be treated as open backlog
  items despite the UI placeholders.

