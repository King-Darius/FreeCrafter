# Sanity Check Report — 11 Feb 2025 (refreshed)

## Summary
- `python scripts/bootstrap.py` now reuses the apt-provided Qt 6 toolchain at `/usr`, rebuilds every target, and installs the deliverables into `dist/bin/FreeCrafter` without further intervention.【074f9f†L1-L11】【F:scripts/bootstrap.py†L320-L349】
- `ctest --test-dir build --output-on-failure` executes all 18 suites; 11 pass while 7 fail. GUI-driven suites (`render_regression`, `viewport_depth_range`, `tool_activation`, `cursor_overlay`, `undo_stack_resets`) still crash in this headless environment because Qt cannot create an OpenGL context, and the regression suites for Phase 4 and Phase 6 (`phase4_tools`, `phase6_advanced_tools`) continue to assert on offset/follow-me/surface behaviours.【5df591†L1-L68】【324942†L1-L15】 These regressions keep the roadmap verification status at ⚠️ despite the restored build.
- This status snapshot remains synchronized with the [README “Status snapshot”](../../README.md#status-snapshot) messaging so future refreshes stay aligned.

## Execution Log
```text
python scripts/bootstrap.py
  ...
  Using existing Qt at /usr
  Running: cmake --build /workspace/FreeCrafter/build
  ...
  Running: cmake --install /workspace/FreeCrafter/build --prefix /workspace/FreeCrafter/dist
  -- Install configuration: ""
  -- Installing: /workspace/FreeCrafter/dist/bin/FreeCrafter
```

```text
ctest --test-dir build --output-on-failure
  ...
  1/18 render_regression           ***Exception: SegFault (OpenGL context missing)
  2/18 viewport_depth_range        ***Failed (OpenGL context missing)
 10/18 phase4_tools                Subprocess aborted (Offset tool assertion)
 12/18 phase6_advanced_tools       Subprocess aborted (Surface workflow assertion)
 13/18 tool_activation             ***Exception: SegFault (OpenGL context + resource wiring)
 14/18 cursor_overlay              ***Exception: SegFault (OpenGL context missing)
 17/18 undo_stack_resets           ***Exception: SegFault (OpenGL context + resource wiring)
 11/18 suites passed
```

## Roadmap Snapshot
- Phase coverage snapshot (terminology matches the checkboxes in `ROADMAP.md`):
- Phase 1 — Core Shell: ☑ implementation exists; verification currently **fails** because `tool_activation` and `undo_stack_resets` crash without an OpenGL context or packaged icons in the test harness.【5df591†L35-L68】【5df591†L73-L88】【098236†L1-L5】【213b07†L1-L3】 Address GPU availability or supply mock resources before closing the checkbox.
- Phase 2 — Geometry & Interaction: ☑ implementation exists, and the CLI/unit suites pass, but viewport-driven regression tests remain **blocked** by the same headless limitations affecting Phase 1.【5df591†L1-L33】【098236†L1-L5】【213b07†L1-L3】
- Phase 3 — Navigation & View: ☑ implementation exists with a known ☐ cursor HUD gap; verification still depends on GPU-backed widgets, so maintain the ⚠️ status until the render suites can run headless.【5df591†L1-L24】【213b07†L1-L3】
- Phase 4 — Drawing & Modification: ☑ implementation exists, but `phase4_tools` asserts when exercising offset/push-pull in tests, so the milestone stays at ⚠️ pending a fix.【5df591†L25-L33】【098236†L1-L5】 
- Phase 5 — Object Management: ☑ implementation exists; automated checks pass in this run, yet overall roadmap sign-off remains contingent on resolving the GPU-dependent suites shared with other phases.【5df591†L21-L33】【098236†L1-L5】
- Phase 6 — Integrated Advanced Tools: ☑ implementation exists; `phase6_advanced_tools` still fails on the surface workflow assertion and needs follow-up before the checkbox can be considered verified.【5df591†L25-L33】【098236†L1-L5】
- Phase 7 — File I/O: ☑ implementation exists; regression coverage (e.g., `file_io_exporters`) passes, but the overall release gate stays at ⚠️ until the remaining GUI regressions are addressed.【5df591†L61-L68】【098236†L1-L5】
- Outstanding backlog focus:
  - Phase 3 cursor HUD artwork remains to be integrated (☐ in `ROADMAP.md`).
  - Phases 8–10 (performance/stability, polish, QA & release) stay open backlog work (☐ in `ROADMAP.md`).
  - Phase 11 surface painting is still a planning document without code yet (☐ in `ROADMAP.md`).

## Dependency & Packaging Notes
- `scripts/bootstrap.py` now detects both `aqtinstall` and `aqt`, strips unreachable proxy settings, reuses an existing Qt installation (e.g., `/usr`), and ensures the install prefix exists before calling `cmake --install`, which lets the bootstrap finish end-to-end on this environment.【F:scripts/bootstrap.py†L127-L221】【F:scripts/bootstrap.py†L320-L349】【074f9f†L1-L11】
- Python package requirements remain in `scripts/requirements.txt`; confirm `aqt` CLI availability before the next rerun.
- The pinned Qt runtime definition in `qt/manifest.json` and existing CMake/CPack packaging remain unchanged.
