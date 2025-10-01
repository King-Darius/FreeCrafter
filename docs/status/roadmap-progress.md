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

### Phase 4 — Drawing & Modification (implemented)

* **Planar curve tools:** Added dedicated arc, circle, polygon, rotated
  rectangle, and freehand tools that author `Curve` geometry directly on the
  ground plane while honoring inference snaps and measurement overrides.
  【F:src/Tools/ArcTool.cpp†L1-L220】【F:src/Tools/CircleTool.cpp†L1-L182】【F:src/Tools/PolygonTool.cpp†L1-L210】【F:src/Tools/RotatedRectangleTool.cpp†L1-L192】【F:src/Tools/FreehandTool.cpp†L1-L94】

* **Surface modifiers:** Push/Pull now preselects faces, supports distance
  overrides, and previews resulting geometry, while Offset and Follow Me create
  additional curves or solids from existing selections. 【F:src/Tools/ExtrudeTool.cpp†L1-L164】【F:src/Tools/OffsetTool.cpp†L1-L231】【F:src/Tools/FollowMeTool.cpp†L1-L81】
* **Presentation aids:** Paint Bucket assigns per-object colors, and the Text
  and Dimension tools register annotations in the active document while honoring
  measurement overrides. 【F:src/Tools/PaintBucketTool.cpp†L1-L103】【F:src/Tools/TextTool.cpp†L1-L42】【F:src/Tools/DimensionTool.cpp†L1-L74】
* **Guides:** Tape Measure, Protractor, and Axes tools capture linear, angular,
  and orientation guides in the scene for precision modeling. 【F:src/Tools/TapeMeasureTool.cpp†L1-L64】【F:src/Tools/ProtractorTool.cpp†L1-L132】【F:src/Tools/AxesTool.cpp†L1-L74】【F:src/Scene/Document.cpp†L9-L63】

### Upcoming Gaps

* **Phase 5 — Object Management:** Groups/components, tags, the outliner, and
  scenes are still roadmap items without code coverage.
* **Phases 6–10:** Advanced modeling workflows, file interchange, performance
  investments, polish, and release readiness tasks have not been started in the
  repository and remain future backlog.

