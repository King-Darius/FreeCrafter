# Scope Lock — Phase 0

## Vision
FreeCrafter targets feature parity with popular CAD and archviz tools while adding an opinionated command system, plugin surface, and modern collaboration. The MVP focus is:

- Parametric solid and surface modeling workflows familiar to SketchUp, Rhino, and Revit users.
- Fast navigation with inferencing and sticky modifier behaviors from pro CAD suites.
- Integrated right-side panels for inspector, explorer, and history states that align with the Figma reference.
- A plugin surface that allows tooling, panels, and command palette extensions without destabilising the core shell.

## Deliverables Locked for MVP
1. **Core Shell (Phase 1)** – multi-toolbar frame, tabbed viewport host, persisted docking layout, live status readouts.
2. **Geometry Kernel (Phase 2)** – half-edge topology with healing, inference engine, and interactive tool state machines.
3. **Navigation & View (Phase 3)** – pro-grade camera mappings, style toggles, shadow previews, section planes.
4. **Drawing & Modification (Phase 4)** – line/arc/polygon suite, push/pull + follow-me, guide tools, paint materials.
5. **Object Management (Phase 5)** – groups/components, tags, outliner, scenes.
6. **Advanced Tools (Phase 6)** – loft, fillet, subdivision, and sculpt workflows.
7. **File I/O (Phase 7)** – full import/export stack with glTF as baseline and SKP stretch goal.
8. **Performance & QA (Phases 8–10)** – BVH selection acceleration, autosave/recovery, localization, installer, automated QA.

## Out of Scope for MVP
- Cloud syncing, multi-user editing, and shared scenes.
- VR/AR presentation modes.
- GPU-based path tracing beyond real-time raster preview.
- Proprietary format authoring (DWG/DXF write) until licensing completed.

## Success Criteria
- 1440×900 desktop layout pixel parity with the design brief and responsive behavior down to 1280×800.
- Tool workflows map 1:1 with published shortcuts and inference hints.
- Stable Windows/macOS/Linux builds with reproducible packaging scripts.
- Plugin API that supports community tooling without modifying the core executable.
