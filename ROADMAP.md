# FreeCrafter Roadmap

A fully expanded, stepwise roadmap to Popular CAD/Archviz parity and beyond. Each phase builds on the last for an intuitive, professional CAD/modeling experience.

---

## High-Level Milestones

The following tracked milestones correspond to major roadmap items A–H:

- [ ] [A. Rendering](docs/milestones/rendering.md)
- [ ] [B. Tools](docs/milestones/tools.md)
- [ ] [C. DSM](docs/milestones/dsm.md)
- [ ] [D. Plugins](docs/milestones/plugins.md)
- [ ] [E. File I/O](docs/milestones/file-io.md)
- [ ] [F. UI Polish](docs/milestones/ui-polish.md)
- [ ] [G. Performance & Stability](docs/milestones/performance-stability.md)
- [ ] [H. QA & Release](docs/milestones/qa-release.md)

---

## Legend

☐ = To do ☑ = Done ➖ = In progress

---

## Phase 0 — Planning & Foundation

- [x] **Scope lock:** Popular CAD/Archviz parity + additional integrated features
- [x] **UX research:** Catalog micro‑behaviors (sticky inference, axis lock keys, dynamic hints, VCB behaviors, modifier keys)
- [x] **Design:** High‑fidelity wireframes for menus, trays, toolbars; icon language; dark/light themes
- [x] **Legal/Tech:** License review (Qt, OpenGL, CGAL/libIGL); file formats plan

---

## Phase 1 — Core Shell

- [x] **QMainWindow scaffolding:** Multi‑toolbar, docking system, layout persistence
- [x] **Status bar:** Live hints + Measurements (VCB) widget; unit settings
- [x] **Hotkey map import:** Popular CAD/Archviz‑style defaults + rebinding UI
- [x] **Viewport placeholder:** Frame budget HUD (FPS, draw calls)

---

## Phase 2 — Geometry & Interaction

### 2.1 Half‑edge core

- [ ] Vertex/Edge/Face; manifold checks; face triangulation; normals
- [ ] Healing: orphan edge purge; tiny edge collapse; duplicate vert weld

### 2.2 Inference engine

- [ ] Snap database (kd‑trees for verts, midpoints, face centers)
- [ ] Inference types: endpoint, midpoint, intersection, on‑edge, on‑face, axis (RGB), parallel, perpendicular
- [ ] Locking: Shift (sticky), Arrow keys (R/G/B)
- [ ] Visuals: colored glyphs + dashed guides; hover latency < 12ms

### 2.3 Interaction layer

- [ ] Tools run as state machines; cancel (Esc), commit (Enter), modifier keys
- [ ] VCB: dynamic prompts and typed overrides (e.g., 12'6", 8')

### 2.4 Core tools

- [ ] Line (continuous), Smart Select (click/drag, window/crossing), Move/Stretch (axis‑aware), Rotate, Scale
- [ ] Ghost previews; hover/selection highlighting; inference overlays

---

## Phase 3 — Navigation & View

- [ ] Mouse mapping: MMB=Orbit, Shift+MMB=Pan, Wheel=Zoom; O/H/Z tool keys; plus Zoom Extents/Selection
- [ ] Standard views; perspective/parallel; FOV dialog; axis gizmo
- [ ] Styles: Wireframe/Shaded/Shaded+Edges/HiddenLine/Monochrome; hidden geometry
- [ ] Sections: multiple planes; active cut; fills; per‑scene state
- [ ] Shadows: solar model; date/time sliders; lat/long; basic projected → shadow‑map upgrade later

---

## Phase 4 — Drawing & Modification

- [ ] Arc (2‑pt, 3‑pt), Circle, Polygon, Rotated Rectangle, Freehand
- [ ] Offset (faces/edges), Follow‑Me, Push/Pull with pre‑pick & hover‑pick
- [ ] Paint Bucket (materials panel), Text, Dimensions
- [ ] Guides: Tape Measure (guide lines/points), Protractor, Axes tool

---

## Phase 5 — Object Management

- [ ] Groups/Components: make/edit; component definitions; Make Unique; replace/reload; gluing & cutting behavior later
- [ ] Tags: create, color, toggle, color‑by‑tag; hidden objects excluded from inference
- [ ] Outliner: drag‑drop hierarchy; isolate/edit‑in‑context UX
- [ ] Scenes: capture camera, styles, section states, tag visibilities

---

## Phase 6 — Integrated Advanced Tools

- [ ] Round Corner: edge fillet/chamfer; previews; corner resolution; hard/soft edges
- [ ] CurveIt: loft/skin/bridge by rails; closed loops; subdivision smoothing
- [ ] PushANDPull: thicken normals; offset surfaces; thickness with caps
- [ ] Surface: draw/offset on curved faces; remesh helpers
- [ ] BezierKnife: Bezier/NURBS/Polyline; grips; degree/knots
- [ ] QuadTools: quad tagging; loop/ring select; grid fill
- [ ] SubD: Catmull‑Clark; creases; level previews; cage overlay
- [ ] Weld: weld/unweld; preserve direction
- [ ] Vertex Tools: soft selection; falloff; transform gizmo
- [ ] Clean: purge, merge co‑planar faces, erase stray edges
- [ ] ClothEngine: PBD solver; pin/weight maps; collision with solids
- [ ] CAD Designer: Pull+, Shell, Split, Mirror, Pattern, Revolve, Sweep, Imprint/Project

---

## Phase 7 — File I/O

- [ ] Import: OBJ, STL, FBX, DAE, 3DS, DXF/DWG
- [ ] Export: OBJ, STL, FBX, DAE, glTF
- [ ] SKP via SDK if allowed; otherwise robust DAE/glTF

---

## Phase 8 — Performance & Stability

- [ ] BVH for selection/raycast; frustum/occlusion culling; LOD; instancing
- [ ] Multithreaded booleans; background tasks with progress UI
- [ ] Memory pools; geometry compression; autosave & crash recovery

---

## Phase 9 — Polish

- [ ] Final SVG icon family; hi‑DPI
- [ ] Theming; localization; onboarding Instructor panel with tool tips
- [ ] Settings sync; telemetry opt‑in
- [ ] Installer (MSI/Inno/In‑app updater)

---

## Phase 10 — QA & Release

- [ ] Unit tests for geometry ops; golden‑model regression
- [ ] UI tests (recorded user flows)
- [ ] Beta cohort; issue triage; docs/site; 1.0 release

---

**How to contribute:**  
- If you want to help with a feature, check the [issues](../../issues) and mention the corresponding roadmap item in your PR or discussion.
- For suggestions or clarifications, open a [discussion](../../discussions).

---
