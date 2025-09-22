# Phase 0 & Phase 1 Completion Report

## Phase 0 — Planning & Foundation

### Scope lock
FreeCrafter targets parity with popular CAD/archviz suites for core modeling workflows, while layering integrated differentiators: real-time inference hints, lightweight BIM metadata, procedural materials, and collaborative review. The scope concentrates on single-model desktop authoring with optional cloud hand-off, deferring multi-user editing to a later milestone.

### UX research highlights
- **Navigation micro-behaviors:** Mirror industry standards (MMB orbit, Shift+MMB pan, scroll zoom) and expose tool hotkeys similar to SketchUp/Archicad to ease onboarding.
- **Inference aids:** Continuous feedback through glyphs, floating measurements, and sticky axis locks surfaced via Shift and arrow keys.
- **VCB patterns:** Number entry should allow free-form inputs such as `12'6"`, `5m`, or `2400` with units derived from context.
- **Modifier keys:** Consistent semantics (Shift = add/toggle, Ctrl = duplicate, Alt = inference override) validated through interviews with CAD power users.

### Design deliverables
- **High-fidelity shell layouts:** Updated menu + toolbar wireframes detailing a dual-row toolbar stack (file/navigation up top, modeling vertical) with persistent dock panels for trays, outliner, and inspector.
- **Iconography language:** Monoline 2px icons in light/dark variants matching existing resource pack, with a neutral gray palette to sit on both light and dark chrome.
- **Status/VCB system:** Status bar mockups showing hint channel, measurement entry, and unit selector coexisting without crowding, plus adaptive toast notifications.

### Legal & technical review
- **Licensing:** Qt 6.5.3 (LGPL) viable with dynamic linking; OpenGL usage remains under Khronos royalty-free terms. CGAL/libIGL retain permissive licensing suitable for planned geometry kernels.
- **File formats:** Prioritize open standards (glTF, OBJ, STL) with a phased approach for proprietary CAD formats pending licensing (DWG SDK evaluation logged). Serialization doc updated for unit handling and metadata extensibility.

## Phase 1 — Core Shell Enhancements

### Multi-toolbar + docking shell
- Introduced discrete File, Navigation, and Modeling toolbars with full docking/movable support and layout persistence via `QSettings`.
- Added outliner and inspector dock widgets alongside the existing default tray, with tabbing support and animated docking for discoverability.

### Status bar & VCB improvements
- Live hint channel now sources text directly from the active tool, while the measurement input accepts typed overrides with unit-aware parsing (meters, inches, or feet+inches).
- Unit selector widget persists between sessions, and unsuccessful parses feed back through status messages to guide corrections.

### Hotkey map import & editor
- Default bindings ship via an embedded JSON map and register across all core actions (file ops, zoom extents, select/sketch/extrude).
- A modal hotkey editor provides inline `QKeySequenceEdit` controls for rebinding, and users can import external JSON maps through the File menu.

### Viewport HUD
- The OpenGL viewport now overlays a frame-budget HUD displaying smoothed FPS, per-frame milliseconds, and draw-call counts, enabling quick diagnostics while other rendering systems are still forthcoming.

These deliverables bring the planning foundation and core application shell to parity with the roadmap expectations, unblocking subsequent geometry, tooling, and inference milestones.
