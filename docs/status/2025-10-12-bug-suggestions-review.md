# Bug Suggestion Follow-up — 2025-10-12

## Scope
This note reviews the outstanding items from the Phase 7.5 bug sweep checklist to confirm which suggestions have already landed and which remain open for engineering work.

## Completed suggestions
- **View menu reconstruction:** `MainWindow::createMenus()` now owns the full View menu, including the projection toggle, render style submenu, and theme action, so the UI wiring lives inside the factory function again.【F:docs/status/2025-10-10-phase7_5-bug-sweep.md†L24-L27】【F:src/MainWindow.cpp†L1021-L1094】
- **Dark mode toggle parity:** The View menu exposes a checkable “Toggle Dark Mode” action that persists the selection in `QSettings` and reloads the corresponding QSS, matching the spec from the sweep report.【F:docs/status/2025-10-10-phase7_5-bug-sweep.md†L24-L27】【F:src/MainWindow.cpp†L1080-L1094】【F:src/MainWindow.cpp†L1927-L1989】
- **Undo stack wiring:** `MainWindow` connects its `QUndoStack` to the action enablement slots so the History panel can mirror stack state, closing out the third sweep suggestion.【F:docs/status/2025-10-10-phase7_5-bug-sweep.md†L24-L27】【F:src/MainWindow.cpp†L1193-L1204】

## Suggestions still requiring fixes
- **Autosave manager:** A repository-wide search for “autosave” continues to return documentation only, confirming that the autosave timer, persistence path, and crash-recovery prompt have not been implemented yet.【F:docs/status/2025-10-10-phase7_5-bug-sweep.md†L28-L29】【a06d0a†L1-L13】
- **Regression test rerun transcript:** There is no captured execution log for `scripts/run_tests_with_qt_env.ps1 -UseCTest`; the only references remain in onboarding docs, so the sweep’s testing follow-up still needs to be carried out once the build is green.【F:docs/status/2025-10-10-phase7_5-bug-sweep.md†L28-L29】【f8d10c†L1-L21】

## Next steps
1. Implement the autosave subsystem per the UI/tools spec (timer interval, autosave directory management, crash detection, and restore dialog).
2. Once the build is stabilized, execute `scripts/run_tests_with_qt_env.ps1 -UseCTest` (or the Linux/macOS equivalent) and archive the output under `docs/status/` to document the regression coverage run for the milestone.
