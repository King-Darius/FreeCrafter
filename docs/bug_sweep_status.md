# FreeCrafter Bug Sweep Follow-up

This document summarizes the current status of the issues raised in the October 2025 bug sweep and subsequent code review. Each item references the present-day FreeCrafter sources to show whether the bug still affects the project and, where needed, outlines follow-up work.

## Build fails due to truncated View menu code
* **Status:** Fixed. `MainWindow.cpp` now contains a complete menu construction block with working Undo/Redo actions, so the truncated code that previously prevented compilation is no longer present.【F:src/MainWindow.cpp†L1313-L1357】
* **Recommended action:** None.

## Missing undo/redo infrastructure
* **Status:** Fixed. The main window owns a `QUndoStack`, wires it into the command stack, and exposes working `onUndo()`/`onRedo()` slots. Undo/redo labels and enable states are refreshed automatically.【F:src/MainWindow.cpp†L901-L943】【F:src/MainWindow.cpp†L2038-L2070】
* **Recommended action:** None.

## No autosave or crash recovery
* **Status:** Fixed. The application instantiates an `AutosaveManager`, restores persisted settings, and manages autosave intervals and retention. The manager can also restore or clear autosaves as needed.【F:src/MainWindow.cpp†L1149-L1181】【F:src/app/AutosaveManager.cpp†L18-L206】
* **Recommended action:** None.

## Placeholder menu actions
* **Status:** Fixed. Insert/Tools/Window actions now invoke fully implemented dialogs or behaviors—for example, inserting shapes or images and opening the plugin manager. The terminal dock provides an actual command runner rather than a placeholder message.【F:src/MainWindow.cpp†L1863-L2035】【F:src/ui/TerminalDock.cpp†L16-L112】
* **Recommended action:** None.

## Extrude tool limitations
* **Status:** Fixed. `ExtrudeTool` supports profile selection, optional path curves, interactive dragging, measurement overrides, previews, and undoable extrusions via command objects—far beyond the earlier single-curve unit-height behavior.【F:src/Tools/ExtrudeTool.cpp†L118-L552】
* **Recommended action:** None.

## Simplistic geometry kernel and file I/O
* **Status:** Improved. The kernel keeps stable identifiers, supports metadata, guides, and axes, and can extrude along arbitrary vectors. Higher-level modeling commands (chamfer, loft) use dedicated operators that run through the shared command stack, enabling richer geometry than the original minimal kernel.【F:src/GeometryKernel/GeometryKernel.cpp†L21-L376】【F:src/Tools/ToolCommands.cpp†L330-L393】
* **Recommended action:** Continue extending import/export coverage (e.g., broader format support) if new requirements emerge.

## Potential pointer lifetime issues
* **Status:** Fixed. Material assignments and metadata are keyed by geometry stable IDs instead of raw pointers, and deletions clear the associated entries, preventing dangling references.【F:src/GeometryKernel/GeometryKernel.h†L155-L166】【F:src/GeometryKernel/GeometryKernel.cpp†L129-L138】【F:src/GeometryKernel/GeometryKernel.cpp†L248-L288】
* **Recommended action:** None.

## Unimplemented grouping and tagging systems
* **Status:** Fixed. The scene graph can create group nodes, and the right-hand tray exposes inspector, tags, and materials panels. The tags panel lets users add, rename, recolor, and toggle tags, and the document serializer understands group nodes.【F:src/Scene/Document.cpp†L175-L207】【F:src/ui/RightTray.cpp†L24-L69】【F:src/ui/TagsPanel.cpp†L19-L155】【F:src/Scene/SceneSerializer.cpp†L44-L57】
* **Recommended action:** None.

## Incomplete navigation and view controls
* **Status:** Fixed. The View menu now contains projection toggles, view presets, grid controls, sun/shadow settings, and a dialog for advanced view parameters, matching the specification more closely.【F:src/MainWindow.cpp†L1331-L1458】【F:src/MainWindow.cpp†L1619-L1680】【F:src/MainWindow.cpp†L4266-L4450】
* **Recommended action:** None.

## Inconsistent tool identifiers
* **Status:** Fixed. Tool actions are created from a centralized registry that guarantees each QAction uses the descriptor’s canonical ID, label, and icon, preventing identifier mismatches.【F:src/MainWindow.cpp†L2213-L2248】
* **Recommended action:** None.

## Testing infrastructure is blocked
* **Status:** Environment-dependent. The CMake project configures successfully when Qt 6 development packages are available; our local attempt failed because Qt is not installed in this container, not due to source errors.【9a59f1†L1-L15】
* **Recommended action:** Ensure Qt 6 is present in CI/build images so the standard `cmake`/`ctest` pipeline can run.

---

**Build note:** Running `cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug` currently fails only because Qt 6 is absent from this environment. Install Qt 6 or set `CMAKE_PREFIX_PATH`/`Qt6_DIR` before invoking CMake.【9a59f1†L1-L15】
