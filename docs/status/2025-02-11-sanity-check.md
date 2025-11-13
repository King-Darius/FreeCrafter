# Sanity Check Report — 11 Feb 2025 (refreshed)

## Summary
- CMake configure fails immediately at `find_package(Qt6)` on a clean environment, blocking the native build and all downstream regression suites until Qt 6 development packages are installed (run `scripts/bootstrap.py` or point `CMAKE_PREFIX_PATH`/`Qt6_DIR` at an existing SDK).
- Because the build stage did not complete, `ctest` and Python smoke suites were not run; their status remains **unknown** pending restoration of the toolchain.
- The codebase still contains implementations for Phases 1–7 from `ROADMAP.md`, but their stability is unverified without a successful build.
- Work continues to be queued for Phases 8–11 (performance, polish, QA & release, surface painting) once the environment is healthy again.

## Test Log
```text
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
# → fails: Qt6Config.cmake not found (Qt 6 SDK missing)
```

With configure failing, no further test executables were built or exercised.

## Roadmap Snapshot
- Phase coverage snapshot:
  - Phase 1 — Core Shell: ☑ implemented & regression-tested.
  - Phase 2 — Geometry & Interaction: ☑ implemented & regression-tested.
  - Phase 3 — Navigation & View: ☑ delivered, pending dynamic cursor visuals (☐).
  - Phase 4 — Drawing & Modification: ☑ implemented & regression-tested.
  - Phase 5 — Object Management: ☑ implemented & regression-tested.
  - Phase 6 — Integrated Advanced Tools: ☑ implemented & regression-tested.
  - Phase 7 — File I/O: ☑ implemented & regression-tested.
- Outstanding backlog focus:
  - Phase 3 cursor HUD artwork remains to be integrated.
  - Phases 8–10 (performance/stability, polish, QA & release) are still open.
  - Phase 11 surface painting is a planning document without code yet.

## Dependency & Packaging Notes
- `scripts/bootstrap.py` orchestrates dependency installation, Qt acquisition, CMake configuration, and bundling; it respects offline caches, CI-friendly flags, and installation prefixes.
- Python package requirements for the bootstrap workflow are tracked in `scripts/requirements.txt` (aqtinstall, PySimpleGUI, pyinstaller).
- The pinned Qt runtime definition lives in `qt/manifest.json`, ensuring consistent module selection.
- CMake/CPack settings in `CMakeLists.txt` already configure NSIS, DragNDrop, and TGZ packaging targets and generate deploy scripts when available.
