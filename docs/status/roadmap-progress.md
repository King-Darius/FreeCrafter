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

### Phase 2 — Geometry & Interaction (implemented)

* **Tool state machine:** Shared tooling flows from `Tool::handle*` helpers that
  arm, activate, and return to idle while wiring Enter/Esc through
  `GLViewport` to call `Tool::commit()`/`Tool::cancel()`. 【F:src/Tools/Tool.cpp†L5-L159】【F:src/GLViewport.cpp†L1247-L1275】
* **Core toolset:** The required select, line, move, rotate, and scale tools are
  implemented with selection gathering, inference-aware point resolution, drag
  previews, and measurement overrides. 【F:src/Tools/SmartSelectTool.cpp†L65-L240】【F:src/Tools/LineTool.cpp†L55-L199】【F:src/Tools/MoveTool.cpp†L13-L194】【F:src/Tools/RotateTool.cpp†L55-L216】【F:src/Tools/ScaleTool.cpp†L40-L233】
* **Inference & axis locking:** Geometry snaps enumerate endpoints, midpoints,
  intersections, and face cues, with `ToolManager` applying sticky locks and
  X/Y/Z axis constraints for manipulation tools. 【F:src/Interaction/InferenceEngine.cpp†L450-L519】【F:src/Tools/ToolManager.cpp†L182-L466】
* **Visual feedback:** Selection renders with distinct colors while the
  inference overlay shows hover glyphs, dashed direction guides, and axis lock
  anchors to confirm the active snap. 【F:src/GLViewport.cpp†L352-L420】【F:src/GLViewport.cpp†L935-L1004】

### Phase 5 — Object Management (implemented)

* **Groups, hierarchy, and components:** `Scene::Document` creates groups,
  re-parents nodes, and tracks component definitions/instances so that the
  outliner tree can be reorganised and instanced content refreshed or made
  unique on demand. 【F:src/Scene/Document.cpp†L132-L275】
* **Tags, visibility, and isolation:** Object tags carry colours, visibility
  toggles, and isolation stacks while `updateVisibility()` applies the resulting
  rules to geometry so hidden items drop out of inference. 【F:src/Scene/Document.cpp†L277-L360】【F:src/Scene/Document.cpp†L640-L704】
* **Scenes & persistence:** Scene snapshots capture camera pose, style toggles,
  tag visibilities, and colour-by-tag state, and can be saved/loaded alongside
  geometry for round-tripping document state. 【F:src/Scene/Document.cpp†L371-L517】

### Upcoming Gaps

* Phases 3–4 still need the expanded drawing and modification tools outlined in
  the roadmap (arcs, follow-me, push/pull pre-pick, etc.).
* Later phases (object management, advanced modeling, file I/O, performance,
  polish, QA) remain untouched in code and should be treated as open backlog
  items despite the UI placeholders.
* The new Phase 11 surface painting & materials milestone is currently a
  documentation plan only and has no engineering work underway.

