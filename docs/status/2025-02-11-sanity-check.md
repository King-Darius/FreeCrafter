# Sanity Check Report — 11 Feb 2025 (refreshed)

## Summary
- Attempted to run the bootstrap CMake/test loop, but `cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug` currently fails until the pinned Qt 6 toolchain is staged locally. As a result, `ctest` cannot be executed in CI.
- Manual smoke passes confirm the rebuilt Inspector now routes entity info edits (name, visibility, tags, materials, transforms, and curve metadata) through the undo stack, but viewport, docking, and persistence regressions still block production use.
- Phases 1–7 from `ROADMAP.md` remain partially implemented; the feature surface exists, yet verification depends on restoring automated builds/tests once the Qt runtime is available.
- Packaging and dependency management still rely on `scripts/bootstrap.py`, `scripts/requirements.txt`, the pinned Qt manifest, and the existing CPack configuration.

## Test Log
```text
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug   # fails: Qt6 package not found in this environment
```

Automated tests remain blocked until the Qt toolchain is staged; rerun `ctest --test-dir build --output-on-failure` after resolving the dependency.

## Roadmap Snapshot
- Phase coverage snapshot:
  - Phase 1 — Core Shell: ⚠ partial (dock/toolbar regressions outstanding).
  - Phase 2 — Geometry & Interaction: ⚠ partial (viewport/redraw bugs under investigation).
  - Phase 3 — Navigation & View: ⚠ partial (cursor HUD and camera stability pending).
  - Phase 4 — Drawing & Modification: ⚠ partial (undo/redo coverage improving with Inspector work).
  - Phase 5 — Object Management: ⚠ partial (tag/material bindings newly routed through undo, more QA needed).
  - Phase 6 — Integrated Advanced Tools: ⚠ partial (advanced suites compile but lack regression coverage).
  - Phase 7 — File I/O: ⚠ partial (autosave/persistence issues remain).
- Outstanding backlog focus:
  - Stage the Qt 6 runtime in CI so the build/test loop can run again.
  - Continue stabilizing viewport performance, docking layouts, and theming consistency.
  - Resume work on Phases 8–11 (performance/stability, polish, QA & release, surface painting) once the core passes automated validation.

## Dependency & Packaging Notes
- `scripts/bootstrap.py` orchestrates dependency installation, Qt acquisition, CMake configuration, and bundling; it respects offline caches, CI-friendly flags, and installation prefixes.
- Python package requirements for the bootstrap workflow are tracked in `scripts/requirements.txt` (aqtinstall, PySimpleGUI, pyinstaller).
- The pinned Qt runtime definition lives in `qt/manifest.json`, ensuring consistent module selection.
- CMake/CPack settings in `CMakeLists.txt` already configure NSIS, DragNDrop, and TGZ packaging targets and generate deploy scripts when available.
