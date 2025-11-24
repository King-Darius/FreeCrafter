# AGENTS.md — FreeCrafter

> Minimal, strict guidance for coding agents (and humans) contributing to **FreeCrafter**.  
> Objectives: restore **day‑to‑day usability**, keep the **GUI top‑down look**, and enforce **consistent naming** across tools/actions.  
> Documentation map (canonical references + where to place new guidance): `docs/documentation_map.md`.
> Full GUI/tool behaviour spec: see `docs/gui_tool_behaviour_spec.md`.
> Stack-agnostic contract: see `docs/codex_master_ui_tools_spec.md` (keep in lockstep with `docs/gui_tool_behaviour_spec.md`).

---

## 1) Project overview
**FreeCrafter** is a cross‑platform 3D modeling sandbox built with **C++17** and **Qt 6**. The app uses a tool‑driven interaction model (Select, Line, Rectangle, Circle, Move, Rotate, Scale, Push/Pull, etc.), a document model for scene graph, and a QWidget/QOpenGLWidget viewport.

---

## 2) UI alignment with the Figma concept (top‑down contract)
Adhere to the concept’s **top‑down** frame so the app looks and behaves as designed:

1. **MenuBar** — File, Edit, View, Tools, Window, Help  
2. **Top Toolbar** — context/tool actions (short tooltips; no text by default)  
3. **Left Toolbar** — primary tools (Select, Line, Arc, Rectangle, Push/Pull, Move, Rotate, Scale, Paint, …)  
4. **Viewport (central)** — sacred; overlays (grid, nav ball, horizon) are opt‑in and **must not occlude** interaction by default  
5. **Right Dock Stack** — Inspector/Properties, Layers/Outliner, Materials  
6. **Status Bar** — dynamic hints, coordinates, selection badges

**Naming & wiring:** UI actions must activate tools via the **exact** tool ID string (see §5). Buttons are checkable and grouped exclusively. **Always load toolbar button icons from the bundled assets under `resources/icons` so the left and top toolbars match the shipped GUI graphics.**

**Theme toggle (Light/Dark) — overview:** The GUI exposes **View ▸ Toggle Dark Mode**. Theme choice applies immediately to all widgets (docks/tabs/inputs/trees/status bar) and **persists**. Details and code in **§3.7 Styling (QSS themes)**.

---

## 3) Implementation guidance (minimum viable creation & edit)
### 3.1 Tool activation (single source of truth)
Create a map `{Action → ToolID}`. Every toolbar/menu slot calls `toolManager->activateTool(id)` and sets a clear status hint. Disable non‑implemented tools and show a small toast (“Coming soon”).

### 3.2 Event pipeline (strict order)
Forward events from the viewport in this order: **active tool → selection → camera** (unless a navigation tool is active). Consider a tiny **cursor HUD** showing current tool + mode.

### 3.3 Commit to the document
On creation finish: build mesh/face → `doc->addNode(...)` → push **Undo** → emit change → **select** the new node. Recompute scene bbox and `maybeFrameNew(bbox)` on the first entity.

### 3.4 Primitives
Provide **Box, Plane, Cylinder, Sphere** (Insert menu + toolbar). Support numeric dialog + drag variant. Assign default material/tag. Place at origin or under cursor with grid snap.

### 3.5 Sketch→Face bridge
Detect closed edge loops → create planar face (triangulate n‑gons). Highlight face; allow **Push/Pull** immediately.

### 3.6 Selection & transforms
After creation, auto‑select new node; gizmo appears; axis locks respond to keys.

### 3.7 Styling (QSS themes) — **Dark Mode Addendum**
**Goal:** user‑selectable Light/Dark mode consistent with the concept. The action lives under **View ▸ Toggle Dark Mode**, is checkable, applies immediately, and **persists**.

**Startup load (persisted):**
```cpp
// On app startup
QSettings settings("FreeCrafter", "FreeCrafter");
bool dark = settings.value("ui/darkMode", true).toBool();
auto loadTheme = [&](bool dark){
    QFile f(dark ? ":/styles/dark.qss" : ":/styles/light.qss");
    if (f.open(QIODevice::ReadOnly))
        qApp->setStyleSheet(QString::fromUtf8(f.readAll()));
};
loadTheme(dark);
```

**View menu action:**
```cpp
QAction* actDark = viewMenu->addAction(tr("Toggle Dark Mode"));
actDark->setCheckable(true);
actDark->setChecked(dark);
connect(actDark, &QAction::toggled, this, [=](bool on){
    settings.setValue("ui/darkMode", on);
    loadTheme(on);
    // Optional: swap theme-specific toolbar icons (sun/moon) here
});
```

**Notes:**
- Keep **parity** between `dark.qss` and `light.qss` (same metrics/widgets; colors differ).
- Ensure tooltip contrast, focus rings, and selection highlights meet accessibility contrast in both themes.
- If you add a toolbar Theme button, bind it to the same action so menu/toolbar stay in sync.

### 3.8 Render & visibility defaults
Beginner‑safe defaults: **Shaded + Edges**, grid ON, axes ON; default tag visible; perspective FOV sensible. Add **View ▸ Reset Camera & Style** to recover from bad states.

### 3.9 Error handling
If a tool can’t commit (invalid loop/zero area), show a clear inline error and keep the tool active.

---

## 4) Build & test (canonical)
Prereqs: Qt 6.6+, CMake 3.24+, Python 3.10+, a C++17 compiler.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
ctest --test-dir build --output-on-failure
```

If Qt is not found, set one of:
```bash
# Linux/macOS
export CMAKE_PREFIX_PATH=/path/to/Qt/6.x/gcc_64
# or
export Qt6_DIR=/path/to/Qt/6.x/gcc_64/lib/cmake/Qt6

# Windows (PowerShell)
$env:CMAKE_PREFIX_PATH="C:/Qt/6.x/msvc2019_64"
```

---

## 5) Code style & naming (follow what exists)
- **Language:** C++17, Qt 6. Prefer RAII, smart pointers; QObject parentage for ownership.
- **Classes:** `PascalCase` (e.g., `MoveTool`, `RectangleTool`).  
- **Methods/vars:** `camelCase()`, members lower camelCase. Constants often `kLikeThis`.
- **UI text:** wrap user‑visible strings in `tr("…")`.
- **Tool IDs (critical):** Use the **exact** `getName()` string for activation. Historical mix exists:
  - Some tools: `"MoveTool"`, `"RotateTool"`, `"SmartSelectTool"` (include `Tool`).
  - Others: `"Rectangle"`, `"Circle"`, `"Arc"`, `"Offset"` (no suffix).
  Always match the existing tool’s `getName()` across code, actions, and bindings.
- **Actions & icons:** Title‑case action text (e.g., *Move*, *Push/Pull*). Prefer existing SVG icons; add new ones with logical names (e.g., `:/icons/offset.svg`) and register in `resources.qrc`.

---

## 6) Usability triage (quick reference)
**Symptoms:** tools don’t create geometry; edits can’t select/transform; Zoom Extents frames nothing.  
**Likely causes:** activation not wired; event routing gap; no document commit/signal; visibility filter; missing sketch→face; projection/clipping.  
**Diagnostics:** log tool activation; tap viewport events to the active tool; print `nodeCount()`/bbox after create; enforce **Shaded + Edges**; auto‑face closed loops; frame new bbox.  
**Smoke tests:** Box; Circle→Face→Push/Pull; Rect array with copy `x5`; Measure; Hide/Unhide; Tags & Color‑by‑Tag; Undo/Redo; Zoom Extents; Reset Camera & Style; Dark/Light toggle.

---

## 7) Task scope & PR checklist
Keep diffs small and local to target modules. Avoid sweeping refactors.

**PR messaging policy (automation + humans):**
- **Never include** links to agent conversations, chat logs, or `https://chatgpt.com/...` style URLs in the PR summary or description.

**PR must include:**
- Repro steps (bug) or acceptance criteria (feature).
- Build + test transcript (or CI green).
- Before/after screenshots for UI changes.

**Definition of Done:**
- Clean build; tests pass for non‑trivial changes.
- Tool activation and UI naming remain consistent.
- GUI respects top‑down contract; viewport not occluded by default.

---

## 8) Modules & boundaries
```
src/
  app/        # QApplication, MainWindow wiring
  ui/         # Widgets, menus, toolbars, docks, overlays (no heavy logic)
  core/       # Geometry/data models/utilities (no UI deps)
  graphics/   # Rendering glue; minimal surface
  io/         # Import/export, serialization
  ops/        # Commands/tools/undo‑redo; thin adapters over core
resources/    # Icons, images, .qrc
tests/        # Unit/integration tests
docs/         # Architecture, conventions, UI design
```
**Rules:** `core` must not depend on `ui`. Rendering state is isolated in `graphics`. `ops` remains testable.

---

## 9) Common pitfalls
- Mismatching tool IDs (e.g., using `"ArcTool"` when `getName()` is `"Arc"`).  
- Swallowing viewport events in overlays; ensure the **active tool** receives input first.  
- Creating geometry without committing/selecting/emitting change.  
- Hardcoded colors/margins; prefer existing styles/patterns.  
- Blocking work on the UI thread; offload heavy ops.

---

## 10) Ownership
- **Owner:** @King-Darius
- When in doubt: open a small draft PR with questions at the top.

1. Baseline: where FreeCrafter stands

FreeCrafter already has:

Qt 6 desktop shell with dockable toolbars and measurement / value-control box.

Hardware-accelerated OpenGL viewport.

A half-edge geometry core with healing, triangulation, and normals.

Core tools: Select, Line, Move, Rotate, Scale, Push/Pull, Offset, Follow-Me, Arc/Circle/Polygon, Text, Paint.

Groups/components with make-unique flows, tag-based visibility, and an outliner.

Basic import/export (OBJ, STL, glTF, etc.).

The missing pieces for parity are mostly about:

A strict edge → loop → face pipeline that always keeps faces in sync with edges.

A workplane-centric inference engine with all subtle snaps and guides.

Component glue/cut semantics + safe selection logic around them.

A hardened GPU viewport + nav gizmo that never lies about planarity or closure.

Topology-aware selection and undo that protects dependent faces and glued components.

2. Geometry core & automatic face creation
2.1 Parity requirements

The environment must maintain a canonical topology:

Vertex

3D position (x, y, z); merged using distance tolerance ε.

Edge

Ordered pair of vertices (v₁, v₂).

Adjacency to 0/1/2 faces.

Face

One outer boundary loop (closed polygon).

Zero or more inner loops (holes).

A plane equation and consistent orientation.

After any topological edit:

Group edges by coplanar sets.

Extract closed, non-self-intersecting loops in each plane.

Create/update faces from these loops.

Nest loops to determine outer boundary vs. holes.

2.2 To-dos for FreeCrafter

Central face solver module

Implement a single module (e.g. FaceSolver / LoopBuilder) that:

Takes modified edges as input.

Groups them into coplanar sets.

Finds closed loops.

Creates/updates faces accordingly.

All tools (Line, Arc, Move, Push/Pull, Offset, Follow-Me, Booleans) must call this module.

Tools must not create faces directly.

Planarity + tolerance discipline

Define global constants:

EPS_DIST for positional tolerance.

EPS_ANGLE for angular tolerance.

All “coplanar?” and “equal point?” decisions use these values.

When necessary:

Either project slightly off-plane edges to the plane, or

Treat them as non-coplanar and exclude them from the face.

Hole vs separate face semantics

In a single plane:

Outer loop: face boundary.

Inner loops: holes/windows.

Use loop nesting and orientation to classify outer vs inner.

Healing and loop consistency

When edges/vertices are deleted or merged:

Destroy faces whose loops are broken.

Rebuild faces from whatever valid loops remain.

All such updates are routed through the face solver.

Agent note: treat “FaceSolver v1” as a core engine feature. Refactor existing tools so they only manipulate edges/vertices and then call the solver to derive faces.

3. Workplanes and projection
3.1 Parity requirements

The modeling workflow is workplane-centric:

Maintain explicit WorkPlane objects with:

Plane equation in world space.

Local 2D coordinates (u, v) for drawing.

Tools project cursor rays to the active workplane.

All sketching is done in (u, v) and then mapped back to 3D.

This keeps loops closed and coplanar even in the presence of camera distortion.

3.2 To-dos for FreeCrafter

Introduce WorkPlane and WorkPlaneManager

Types of workplanes:

World planes (ground, vertical planes).

Face-attached planes (drawing directly on a face).

User-defined construction planes.

Manager tracks which plane is currently active.

Make drawing tools plane-aware

Line/Arc/Circle tools:

Operate in 2D (u, v) on the workplane.

Store 3D results by mapping (u, v) back via the plane transform.

Distances and angles for inference are computed in (u, v), not screen space.

Plane-aware snapping default

If cursor hovers over a face: project onto that face’s plane by default.

If no face under cursor: fall back to the nearest world plane consistent with the tool.

Agent note: this is a structural change—expect to touch input handling, tools, and geometry creation to ensure all edges originate from a workplane, not arbitrary per-frame camera projections.

4. Inference system & snapping (major gap)
4.1 Required inference features

The inference engine must support at least:

Point-based snaps

Vertex endpoints.

Edge midpoints and other parametric fractions.

Component origins, bounding-box corners, and centers.

Curve-based snaps

Circle/arc centers.

Quadrant points (0°, 90°, 180°, 270°).

Tangent points where a new line is tangent to a circle/arc.

Nearest point on edge or curve.

Directional constraints

Axis alignment (along world X, Y, Z).

Parallel to an existing edge.

Perpendicular to an existing edge or face normal.

Metric constraints

Equal length to a reference segment.

Fixed angle relative to a reference axis or edge.

“On plane” (on active face or workplane).

Special arc cues

Acute-angle arcs:

Show two dotted lines from arc endpoints toward the implied circle center.

Right-angle quarter arcs:

Show the same two dotted radii.

Show a small square marker at the 90° corner.

Arc prefers snapping to exactly 90° within an angular tolerance.

Visual style and colours

Dotted or faint guides for:

Extensions of existing edges.

Projection lines (parallel/perpendicular).

Axis-aligned segments:

Use the colour of the corresponding axis.

Perpendicular-to-surface/edge construction:

Use a dedicated highlight colour (e.g. purple).

Tangent, midpoint, on-edge, on-face:

Each has a consistent, subtle styling and optional text tag (“Midpoint”, “On edge”, “On face”, “Tangent”, etc.).

4.2 To-dos for FreeCrafter

Inference target index

Data structure that supports fast queries:

Nearby vertices.

Nearby edge segments.

Nearby circle/arc primitives.

Active axes and workplanes.

Must be efficient enough for per-frame updates during mouse moves.

Candidate generation + scoring

For each potential snap:

Detect type (endpoint, midpoint, center, tangent, parallel, perpendicular, equal length, etc.).

Compute distance and/or angular error.

Score candidates:

Stronger snap types win (endpoint > midpoint > on-edge > on-plane).

Use proximity and type priority.

Choose the best candidate that passes thresholds.

Constraint resolution

Once a snap is selected:

Solve the exact snap point:

Axis/parallel/perpendicular: fix direction vector, allow movement along it.

Equal length: project along direction until distance matches reference.

Tangent: solve for the tangent point on circle/arc.

Provide a unified API:

e.g., InferenceResult { snapPoint, snapType, snapDirection, debugGuides[] }.

Overlay rendering

Implement a GPU overlay layer dedicated to inference:

Dotted extension lines.

Dotted radii to arc centers.

Right-angle square markers.

Short text labels near the cursor.

Enforce consistent colour mapping:

Axis colours.

Perpendicular (e.g. purple).

Tangent/center cues.

Integration with tools

Line, Arc, Circle, Move, Rotate:

Query inference each frame while active.

Update their previews and constraints accordingly.

Move/Rotate/Scale:

Support snapping of reference points and angles to the same inference targets.

Agent note: treat “Inference v1.0” as its own subsystem with clear boundaries:

InferenceIndex (geometry queries).

InferenceSolver (scoring + constraints).

InferenceOverlay (rendering).
Wire all creation/edit tools to this pipeline.

5. Push/Pull and boolean behaviour
5.1 Parity requirements

Push/Pull must:

Extrude a selected face along its normal.

Add or remove material based on context:

On an open surface: create thickness/volume.

On a solid: remove material when pushing inward.

Preserve:

Shared edges and neighbouring faces.

Inner loops (holes) turning into tunnels/holes in the resulting solid.

Always be reconciled by the face solver afterward.

5.2 To-dos for FreeCrafter

Canonical extrusion kernel

Implement extrude_face(face_id, distance):

Duplicate boundary loops offset along face normal.

Connect original and new loops with side faces.

Propagate inner loops correctly (holes).

Context-aware boolean

Distinguish:

Simple surfaces (no enclosed volume).

Closed solids.

On closed solids:

Pushing until coincident with back face → clean subtraction (hole).

Pushing beyond → through-cut plus appropriate removal.

Healing + re-facing

After boolean/extrusion:

Call the central face solver to:

Merge co-linear edges.

Remove degenerate faces.

Rebuild any disrupted faces.

UI + inference integration

During Push/Pull:

Show distance guides (e.g. snapping to lengths of existing edges or previous push distances).

Show snap when the pushed face aligns with another face’s plane.

Agent note: separate the user-level Push/Pull tool from the geometry kernel. The tool calls extrude_face() and boolean helpers, handles previews and inference, and then commits a single compound undo step.

6. Components: glue, cutting, and reuse
6.1 Parity requirements

Components must support:

Definitions:

Local geometry, local axes.

Instances:

Transform matrix.

Behaviour flags:

glues_to_plane (align glue face to a host face).

cuts_host (introduce holes in host faces).

free (no special behaviour).

Placement workflow:

User chooses a component and a host face.

System computes an alignment transform:

Component’s glue face normal → host face normal.

Respect a notion of “up” axis if defined.

Instance is glued to host plane.

If cutting is enabled:

Project cutting profile onto host face.

Modify host face’s loops to add a hole.

Selection/deletion must understand:

Components glued to faces.

Faces that have glued components.

Edges and vertices that act as boundaries for glued elements.

6.2 To-dos for FreeCrafter

Component behaviour extension

Extend component definitions:

glueFaceId or local plane descriptor.

cutsHost flag.

Optional cutProfile (loop in local space).

Keep backwards-compatible with existing groups/components.

Placement solver

Given (componentDef, hostFace):

Compute transform aligning glue face to host face.

Apply optional rotation to respect “up”.

Position so glue face coincides with host plane.

Hole-cutting logic

On placing/updating a cutting component:

Project cutProfile loop into host face plane.

Integrate it as an inner loop into host face.

Call face solver to rebuild.

Selection & dependency checks

Before transforming or deleting:

If a selected face has glued components → select or warn about them.

If a selected component is glued to an unselected face → select or warn about the host.

Answer questions like:

“Are there glued components outside the selection that depend on faces inside?”

“Are there faces outside the selection that rely on edges/vertices inside?”

Agent note: this is a behaviour layer on top of existing components. Implement glue/cut relationships as explicit references in the data model and ensure they’re considered during selection, edits, and undo.

7. Selection model, safety, and undo/redo
7.1 Parity requirements

Selection and undo should be topology-aware:

Distinguish:

Loose geometry (edges/vertices).

Faces.

Closed solids.

Component instances vs definitions.

Before destructive operations:

Check for dependent faces/components.

Either expand the selection or warn.

Undo/redo:

One user-level action = one undo step (even if many low-level changes).

7.2 To-dos for FreeCrafter

Selection typing

Assign types to selections:

SelectionType::Vertex, Edge, Face, Solid, ComponentInstance, ComponentDefinition, etc.

Provide utilities for:

“Select face’s boundary edges”.

“Select solid that contains this face”.

Preflight analysis

For operations like delete/move/extrude:

Check for:

Faces that will lose boundary edges.

Glued components with hosts inside/outside selection.

Host faces with components inside/outside selection.

Solids that will be split.

Decide:

Auto-expand selection to include dependents, or

Expose hooks for future UI warnings.

Undo/redo granularity

Group low-level edits into logical operations:

Drawing a line that creates an edge and a face = one command.

Push/Pull that modifies multiple faces and edges = one command.

Ensure geometry + component graphs restore faithfully.

Agent note: build a “SelectionAnalysis” helper that can be reused by all tools before committing any destructive change. Tie its outputs into the undo system.

8. Viewport, GPU path, and navigation gizmo
8.1 Parity requirements

Viewport must:

Always reflect true topology:

Closed loops look closed.

Faces appear wherever the solver says they exist.

Provide a stable navigation gizmo:

Axis colours and directions always match world axes.

Clicking gizmo axes yields consistent camera orientations.

Support inference overlays without flicker or lag.

8.2 To-dos for FreeCrafter

Separate model vs preview vs HUD

Three rendering layers:

Model: persistent geometry from the half-edge core.

Preview: current tool’s temporary geometry (in-progress edges, proposed faces).

HUD/Inference: guides, texts, nav gizmo.

Each layer can be updated independently without forcing full rebuilds.

Unified GPU batching

Efficient use of VBOs/IBOs or equivalent:

Solid faces and edges in the model layer.

Overlay lines and symbols in the preview/HUD layers.

Avoid per-frame reallocation for large models.

Navigation gizmo correctness

Gizmo always reflects:

Current world axes.

Current camera orientation.

Aligning a line to an axis:

Line preview uses the same axis colour.

Gizmo visually indicates the active axis.

Tool activation state machine

One active tool at a time.

Tools get:

onEnter, onExit, onMouseDown, onMouseMove, onMouseUp, onKey events.

Viewport input routing depends on current tool.

Agent note: treat viewport as a subsystem with well-defined APIs for “draw model”, “draw preview”, and “draw HUD”. Ensure state transitions between tools don’t leave stale buffers or event handlers.

9. Concrete agent-friendly checklist

For use directly in AGENTS task breakdown:

Global tolerances + WorkPlane abstraction
