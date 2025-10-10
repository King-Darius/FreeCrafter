# Phase 7.5 Bug Sweep – 2025-10-10

## Overview
- Blocking issue: current `main` fails to build on Windows with MSVC when linking against Qt 6.5.3. The compiler reports syntax errors starting at `src/MainWindow.cpp:1146` because the `View` menu construction code was truncated and now sits outside any function. The crash cascades into missing definitions for `onUndo()`/`onRedo()` and invalid forward declarations in `RightTray`.
- Qt toolchain: requires `qt/6.5.3/msvc2019_64`. Previous bootstrap attempts auto-selected `C:\Qt\6.9.3\mingw_64`, leading to incompatible headers and `/Zc:__cplusplus` failures.

## Findings
### UI & Tooling
- `MainWindow::createMenus()` terminates prematurely, leaving `actionViewBottom` … `gridAction` outside the function. Undo/redo slot definitions and the remainder of the View menu never compile.
- `View ▸ Toggle Theme` diverges from the spec:
  * Label should be `Toggle Dark Mode`.
  * Action is not checkable and does not reuse a shared QAction for toolbar/menu sync.
  * Stylesheet swap uses `:/styles/app.qss` and `app_light.qss` instead of the contract’s `dark.qss`/`light.qss`.
- `RightTray` is instantiated with a `nullptr` `QUndoStack`, so the History panel can never surface undo data.
- `EnvPanel` forward declaration in `RightTray.h` collides with the `using EnvPanel = EnvironmentPanel;` alias in `EnvPanel.h`, preventing compilation and blocking dynamic casts.

### Undo/Redo, Autosave, Recovery
- No `QUndoStack`, `QUndoCommand`, or equivalent command objects exist in the project outside the UI stubs.
- `MainWindow::onUndo()` / `onRedo()` functions are missing in the translation unit because of the corrupted View menu block.
- No autosave manager, timer, or recovery prompt is present; searches for `autosave`, `AutoSave`, and `restoreAutosave` return docs only.
- Crash recovery pathways (checkpoint files, session resume) are absent from both `src/` and `scripts/`.

### Testing
- `cmake --build build --config Release` fails before any executable is produced, so `ctest` cannot be executed. The failure is deterministic.
- Existing CMake test targets (`test_render`, `test_phase4`, `test_phase5`, etc.) remain unverified on this commit due to the build failure.

## Recommended Actions
1. Repair `MainWindow.cpp` by moving the full `View` menu construction (Bottom/Front/Back/Left/Right, projection toggle, theme toggle, render style menu) back inside `createMenus()` and reintroduce concrete slot definitions for undo/redo.
2. Replace the `View ▸ Toggle Theme` action with a single shared QAction labelled `Toggle Dark Mode`, mark it checkable, persist its state, and wire it to `applyThemeStylesheet()` with the Light/Dark QSS pair.
3. Instantiate a real `QUndoStack` in `MainWindow`, pass it to `RightTray`, and verify the History panel updates on command pushes.
4. Introduce an autosave manager per the contract: timer-based saves to `autosave/`, crash detection, and restore prompt on launch.
5. After the build is green, execute `ctest --output-on-failure` via `scripts/run_tests_with_qt_env.ps1 -UseCTest` and capture the transcript for this milestone.

## Blockers for Regression Pass
- Build failure prevents automated or manual regression testing.
- Missing undo/redo/autosave functionality violates Phase 7.5 exit criteria and the top-down UI contract.
- Theme toggle spec mismatch requires design-side sign-off before closing UI polish tasks.
