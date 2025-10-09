# Sanity Check Report — 11 Feb 2025 (refreshed)

## Summary
- Ran the bootstrap smoke suite (`pytest tests/test_bootstrap.py`) and the C++ integration harness (`ctest --test-dir build --output-on-failure`) to cover the editor shell, navigation, tool workflows, object management, advanced tools, and import/export code paths. Both suites are currently green.
- Phases 1–7 from `ROADMAP.md` now have shipping implementations with matching regression coverage. The lone Phase 3 gap is the dynamic cursor/pickbox artwork, which still needs to be wired to tool state updates.
- Remaining open work is concentrated in Phases 8–11 (performance, polish, QA & release, and surface painting). These are tracked as backlog items for future sprints.
- Packaging and dependency management continue to rely on `scripts/bootstrap.py`, `scripts/requirements.txt`, the pinned Qt manifest, and the existing CPack configuration.

## Test Log
```text
pytest tests/test_bootstrap.py
ctest --test-dir build --output-on-failure
```

All tests passed. For raw output, see the execution transcript captured during this run.

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
