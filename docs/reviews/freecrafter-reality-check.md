# FreeCrafter Reality Check

## Core application structure
- `src/main.cpp` configures a core-profile OpenGL context, registers the bundled Qt resources, and instantiates `MainWindow`, which anchors the rest of the UI.
- `MainWindow` owns the `GLViewport`, tool/command infrastructure, and dock layout. The constructor wires up the undo stack, tool manager, and callbacks that keep the viewport, right tray, and status bar synchronized with document changes.
- `GLViewport` subclasses `QOpenGLWidget`, exposing camera/navigation helpers, grid/axes toggles, and the cursor overlay pipeline that routes events toward the active tool before falling back to navigation.

## Functionality that is already implemented
- The command stack delegates to Qt's `QUndoStack`, and `MainWindow` connects the undo/redo actions, status text, and history panel to that stack. Tests such as `tests/test_scene_commands.cpp` exercise primitive creation, guide management, image plane insertion, and external reference linkage through undoable commands.
- The rendering layer batches line and triangle primitives, supports several render styles, and allocates optional sun-shadow resources, giving the viewport a modern shaded-with-edges baseline.
- Scene import/export recognizes OBJ, STL, FBX, DAE, glTF, and SKP containers (with feature flags for optional SDKs). The importer implements STL parsing and glTF node traversal to reconstruct meshes inside the geometry kernel.
- The geometry kernel can add curves, extrude them into solids (with options for capping and directional extrusions), clone/delete geometry objects, and serialize the model to disk.

## Gaps relative to mature 3D/CAD suites
- The plugin manager dialog only enumerates filesystem entries and does not load or execute plugins. The roadmap still tracks plugin support as an unfinished milestone with unchecked acceptance criteria (discovery, registration, sandboxing, sample plugin).
- There is no embedded scripting runtime; no Python/Lua bindings or automation hooks exist outside the C++ codebase, and no modules expose a public API for scripts.
- Geometry operations focus on extrusion and mesh cloning. The kernel does not surface boolean solids, NURBS, or parametric constraints that CAD users expect.
- Rendering stops at rasterized shading. There is no ray-traced or PBR renderer, technical drawing generator, or baking pipeline comparable to the engines bundled with Blender or FreeCAD.
- File interchange is narrower than production tools. High-end CAD staples like STEP/IGES/DWG/DXF are absent, so workflows that depend on these formats would require external converters.
- Automated coverage is modest. The existing tests hit command stack plumbing and a few scene actions, but there is no integration test suite for the interactive tools, complex geometry operations, or viewport regression scenarios.

## Suggested next validation steps
1. Flesh out the plugin loader so it can detect manifests, enforce sandboxing, and register contributed tools or actions.
2. Expand the geometry kernel's operation set (boolean ops, lofts, fillets, constraints) and add regression tests that guard against topology corruption.
3. Broaden the importer/exporter surface to cover STEP and DXF, or document how to round-trip with external utilities.
4. Add integration tests that drive tools through the command stack, confirming undo/redo safety and viewport synchronization during multi-step modeling sessions.
