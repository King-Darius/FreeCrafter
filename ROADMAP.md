# FreeCrafter Roadmap

A fully expanded, stepwise roadmap to Popular CAD/Archviz parity and beyond. Each phase builds on the last for an intuitive, professional CAD/modeling experience.

---

## High-Level Milestones

The following tracked milestones correspond to major roadmap items A–H:

- [ ] [A. Rendering](docs/milestones/rendering.md)
- [x] [B. Tools](docs/milestones/tools.md)
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

- [x] Vertex/Edge/Face; manifold checks; face triangulation; normals
- [x] Healing: orphan edge purge; tiny edge collapse; duplicate vert weld on curve/solid builds

### 2.2 Inference engine

- [x] Snap database (kd‑trees for verts, midpoints, face centers)
- [x] Inference types: endpoint, midpoint, intersection, on‑edge, on‑face, axis (RGB), parallel, perpendicular
- [x] Locking: Shift (sticky), Arrow keys (R/G/B)
- [x] Visuals: colored glyphs + dashed guides; hover latency < 12ms

### 2.3 Interaction layer

- [x] Tools run as state machines; cancel (Esc), commit (Enter), modifier keys
- [x] VCB: dynamic prompts and typed overrides (e.g., 12'6", 8') — measurement entry widget drives active tool updates

### 2.4 Core tools

- [x] Line (continuous), Smart Select (click/drag, window/crossing), Move/Stretch (axis‑aware), Rotate, Scale
- [x] Ghost previews; hover/selection highlighting; inference overlays

---

## Phase 3 — Navigation & View

- [x] Mouse mapping: MMB=Orbit, Shift+MMB=Pan, Wheel=Zoom; O/H/Z tool keys; plus Zoom Extents/Selection (current build supports orbit/pan/zoom via mouse buttons but lacks remapping + view hotkeys)
- [x] Standard views; perspective/parallel; FOV dialog; axis gizmo
- [x] Styles: Wireframe/Shaded/Shaded+Edges/HiddenLine/Monochrome; hidden geometry
- [x] Sections: multiple planes; active cut; fills; per‑scene state
- [x] Shadows: solar model; date/time sliders; lat/long; basic projected → shadow‑map upgrade later
- [ ] Dynamic cursor system with context-sensitive crosshair, pickbox, pointer, and pencil visuals that reflect tool state

---

## Phase 4 — Drawing & Modification

- [x] Arc (2‑pt, 3‑pt), Circle, Polygon, Rotated Rectangle, Freehand
- [x] Offset (faces/edges), Follow‑Me, Push/Pull with pre‑pick & hover‑pick
- [x] Paint Bucket (materials panel), Text, Dimensions
- [x] Guides: Tape Measure (guide lines/points), Protractor, Axes tool

---

## Phase 5 — Object Management

- [x] Groups/Components: make/edit; component definitions; Make Unique; replace/reload; gluing & cutting behavior later
- [x] Tags: create, color, toggle, color‑by‑tag; hidden objects excluded from inference
- [x] Outliner: drag‑drop hierarchy; isolate/edit‑in‑context UX
- [x] Scenes: capture camera, styles, section states, tag visibilities

---

## Phase 6 — Integrated Advanced Tools

Reference: [Phase 6 planning brief](docs/planning/phase-6-integrated-advanced-tools.md)

- [x] Round Corner — interactive fillet/chamfer creation with preview, resolution, and edge tagging controls
- [x] CurveIt — rail-driven loft, skin, and bridge workflows with subdivision smoothing support
- [x] PushANDPull — surface thickening and offsetting along normals with automatic capping
- [x] Surface — direct drawing/offsetting on curved faces with remesh helpers
- [x] BezierKnife — precision Bézier/NURBS/polyline cutting with manipulators and curve controls
- [x] QuadTools — quad topology tagging, loop/ring selection, and grid filling utilities
- [x] SubD — Catmull–Clark subdivision with crease management, level previews, and cage overlays
- [x] Weld — direction-aware vertex weld/unweld operations with tolerant selection
- [x] Vertex Tools — soft-selection transforms with customizable falloff and gizmo controls
- [x] Clean — automated cleanup passes for unused data, coplanar merges, and stray edge removal
- [x] ClothEngine — PBD cloth simulation with pin/weight maps and solid collision handling
- [x] CAD Designer — Pull+, shell, split, mirror, pattern, revolve, sweep, and imprint/project suite

---

## Phase 7 — File I/O

- [x] Import pipeline landed: OBJ, STL, glTF (FBX/DAE/3DS/DXF/DWG surface when optional Assimp integrations are enabled)
- [x] Export pipeline landed: OBJ, STL, glTF (FBX/DAE routed through Assimp when present; UI hides formats otherwise)
- [x] SKP via SDK if allowed; otherwise robust DAE/glTF fallback documented and gated

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

## Phase 11 — Surface Painting & Materials

- [ ] SurfacePaint tool: brush-based 3D painting with pressure/tilt-aware strokes, live masking, and viewport-aligned cursor feedback so color application matches the artist's hand movement.
- [ ] Adaptive SmartWrap: auto-seam detection and multi-projection blending that locks texture pixels to geometry with per-face accuracy; manual seam and projection overrides for edge cases.
- [ ] Layered material stack: base color/roughness/normal/displacement layers with blend modes, masks, and history-backed adjustments for non-destructive editing.
- [ ] Palette & swatch UX: color stories, eyedropper sampling from scene/reference, quick-favorite decks, and per-project libraries synced with the Materials panel.
- [ ] Brush & asset presets: save/load brush engines (tip/spacing/flow), drag-and-drop texture stamps, HDR lighting previews, and instant undo/redo with replay for confidence.
- [ ] Immersive paint workspace UX: split-view material inspector, gesture-friendly HUD, contextual shortcuts, and onboarding tours that teach brush logic without breaking flow.

---

**How to contribute:**
- If you want to help with a feature, check the [issues](../../issues) and mention the corresponding roadmap item in your PR or discussion.
- For suggestions or clarifications, open a [discussion](../../discussions).

# Phase Δ — Artist‑First NURBS & Direct‑Modeling

**Goal:** Fast, creativity‑centric direct modeling (history‑light) with NURBS surfacing, robust booleans, high‑quality fillets, precision snapping, and a Live Blender Bridge.

**Legend:** ☐ To do ☑ Done ➖ In progress

## Objectives

* ☐ Direct NURBS toolset covering curves and surfaces (create/edit; loft, sweep, revolve, network; shell/thicken; trim, split, project, offset).
* ☐ Reliable booleans (union/difference/intersection) with tolerance controls, auto‑heal of small gaps, watertight checks, and non‑manifold detection.
* ☐ Fillets & blends: constant and variable radius; G1/G2 continuity options; zebra/curvature‑comb previews.
* ☐ Artist‑first interaction: manipulator/gumball with inline numeric micro‑entry, radial menu, construction planes from view/selection/3‑point, precision snapping.
* ☐ History‑light action stack: reorder/disable recent operations without heavy parametrics.
* ☐ Interop: Freecrafter ↔ Blender Live Link (one‑click send, preservation of names/layers/material identifiers).
* ☐ Learnability: command palette, inline help, printable shortcuts.

---

## Engineering Issue Checklist (with Milestones & Labels)

**Milestones**
• M1 — Kernel & Reliability (4–6 weeks)
• M2 — Surfacing Toolkit (3–4 weeks)
• M3 — Interaction Layer (3 weeks)
• M4 — Blender Bridge (2–3 weeks)
• M5 — Polish & Learnability (2 weeks)

**Labels**
`area:kernel` `area:surfacing` `area:ux` `area:interop` `area:docs` `area:testing`
`type:feature` `type:bug` `type:perf` `type:spec` `type:qa`
`priority:P0` `priority:P1` `priority:P2`
`risk:high` `risk:med` `risk:low`
`blocked` `needs design` `help wanted`

### M1 — Kernel & Reliability (`area:kernel`, `priority:P0`)

* ☐ Boolean reliability wrapper: tolerant façade over boolean ops; sew/solidify; small‑gap auto‑heal; actionable failure reporting.
* ☐ Non‑manifold detection and auto‑fix where possible.
* ☐ Regression suite: 50+ boolean stress cases; watertight shell assertions.
* ☐ Fillet/Blend enhancements: constant and variable radius; edge‑chain selection; selectable G1/G2; continuity report in UI.
* ☐ Curvature diagnostics: zebra and curvature‑comb overlays in viewport.
* ☐ Geometry health & tolerances: centralized tolerance profile (length/angle/merge) with “Pro” panel exposure.
* ☐ Performance & stability: async previews and cancelation for heavy operations; CI geometry tests with time budgets.

### M2 — Surfacing Toolkit (`area:surfacing`, `priority:P1`)

* ☐ Curves: BSpline creation/editing (degree, knots, end conditions).
* ☐ Surfaces: loft/sweep/network with robust rail/profile handling and self‑intersection checks.
* ☐ Project/Trim/Split; Offset (2D/3D); Shell/Thicken.
* ☐ Surface‑to‑surface blend with live preview and continuity controls.
* ☐ Unit and visual tests with numeric continuity thresholds.

### M3 — Interaction Layer (`area:ux`, `priority:P1`)

* ☐ Manipulator/gumball: translate/rotate/scale; inline numeric micro‑entry; axis/plane constraints; incremental snaps.
* ☐ Radial menu: context‑aware actions; user‑editable mappings.
* ☐ Construction planes: create from view/face/three‑point; named planes; quick toggle.
* ☐ Precision snapping: endpoint, midpoint, center, tangent, perpendicular; smart pick filters.
* ☐ Action stack: reorder/disable recent operations (non‑parametric, history‑light).

### M4 — Blender Bridge (`area:interop`, `priority:P0`)

* ☐ Built‑in socket server: JSON over WebSocket (TCP fallback), versioned handshake, optional token; endpoints include ping, get_scene, push_scene, subscribe, apply_ops, error.
* ☐ Blender add‑on (MVP): preferences for host/port/token; “Send Selection” operator; Live Link toggle for subscriptions; preserves object names, layers, material identifiers.
* ☐ Transfer formats: OBJ inline (MVP) and/or GLTF; NURBS transferred as tessellated proxy for MVP; optional STEP export for later.
* ☐ Acceptance: round‑trip push of ~100k faces in under one second on localhost, with correct names/layers/materials and lossless transforms.

### M5 — Polish & Learnability (`area:docs`, `area:ux`, `priority:P2`)

* ☐ Command palette with fuzzy search and inline command help.
* ☐ Tooltips for major operations and printable shortcuts (PDF).
* ☐ Example scenes for QA (booleans, fillets, surfacing edge cases).
* ☐ Documentation: “Artist‑first Modeling” quickstart and troubleshooting.

---

## Minimal Spec — Freecrafter ↔ Blender Live Link (No Code)

**Transport & Session**
• Transport: WebSocket preferred; TCP fallback. Messages are JSON.
• Default endpoint: localhost on a configurable port.
• Handshake: client identifies itself (e.g., Blender), version, and optional token. Server acknowledges with capabilities (mesh, transforms, materials).
• Heartbeat: lightweight ping/pong to keep the connection alive.
• Security: bind to localhost by default; optional static token; warn if binding publicly; plan for stronger auth later.

**Client Requests (Blender → Freecrafter)**
• Ping: liveness check.
• Get Scene: returns a compact snapshot (whole scene or selection only).
• Push Scene: uploads one or more objects with transforms, layer/group info, and material identifiers.
• Subscribe: registers for server events (model changes, selection changes).
• Apply Ops: asks Freecrafter to apply operations (e.g., transform an object, assign a material) — future‑ready and versioned.

**Server Events (Freecrafter → Blender)**
• Hello OK: handshake success and capability advertisement.
• Model Changed: indicates that specific objects or the scene have changed; includes reason (e.g., user edit, push).
• Selection Changed: provides the current selection set identifiers.
• Error: returns a code, message, and optional context describing what failed.
• Log: debug/information messages gated by a verbosity setting.

**Data Model (Conceptual)**
• Scene Identifier: a token naming the current scene/session.
• Object: id, type (mesh or future NURBS/solid), transform (4×4 flattened), layer/tag, material slots (names and simple attributes), and mesh payload.
• Mesh Payload (MVP): OBJ serialized inline for simplicity. GLTF supported as an alternative; for GLTF, either inline (encoded) or referenced by a local file URI.
• NURBS/STEP (Later): surfaces/solids provided via STEP or a native BREP container; MVP sends tessellated proxies only.

**Event Flows (Narrative)**
• Push Flow: Blender connects and handshakes → user sends selection → Freecrafter imports/updates objects → Freecrafter emits a Model Changed event (and updates any subscribers).
• Live Update Flow (future): On edit/transform in Freecrafter, a Model Changed event lists the affected object ids; Blender pulls deltas via Get Scene and updates in place.

**Acceptance & Diagnostics**
• Performance: pushing a ~100k‑face mesh completes under one second on localhost.
• Correctness: object names, layer mappings, and world transforms preserved; material slot names are stable across a round‑trip.
• Robustness: invalid payloads and non‑manifold meshes produce an Error event with a clear, actionable code and context.
• Logging: server and add‑on write JSON‑line logs suitable for capture and replay; verbosity is user‑configurable.

**Folder Layout (Descriptive, not code)**
• Interop/Server: lightweight socket server source, JSON handling, WebSocket wrapper, build files.
• Blender Add‑on: single add‑on package with preferences, panel, and send operator.
• Tests/Interop: boolean stress meshes and case descriptors for automated checks.
• Docs: protocol overview and usage instructions.

---

**How to contribute (Phase Δ):**
Please open issues using the labels above and reference **Phase Δ** and the relevant milestone (M1–M5). For interop changes, attach a brief log excerpt showing the handshake and a push/acknowledge cycle, or a short screen capture demonstrating the round‑trip.


---
