# FreeCrafter Geometry System Gaps

This note synthesizes observations from mature polygonal modelers and parametric solid modelers to highlight capabilities FreeCrafter still needs to close the usability gap. The goal is to inform the roadmap without duplicating external terminology inside the codebase.

## Topology & Data Structures
- **Persistent adjacency graph:** Leading mesh editors expose per-vertex and per-edge cycles so operators can traverse neighbouring elements without rebuilding buffers. FreeCrafter needs an internal representation that records vertex–edge–face adjacency, loop ordering, and supports n-gons while remaining efficient for non-manifold edits.
- **Per-face-vertex attributes:** Advanced systems store shading normals, UVs, and custom data on a loop-like structure. FreeCrafter should define a lightweight per-corner payload to enable smooth shading, attribute painting, and future deformation tools.
- **Mutable surface cache:** Editing sessions keep GPU buffers in sync with topology deltas. FreeCrafter must introduce a streaming update path instead of fully regenerating draw data after each operation.

## Creation & Editing Tools
- **Operator pipeline:** Modern editors rely on small, composable operators (extrude, inset, bevel) that manipulate topology using adjacency cycles. FreeCrafter’s tools should target the same granularity and operate directly on the persistent mesh graph.
- **Curve-to-surface workflow:** Curve primitives are often evaluated into meshes only when needed. FreeCrafter should support lightweight curve objects that can be sketched, parameterized, and converted into surfaces or solids on demand.
- **Undo-friendly mutations:** To keep tool feedback responsive, edits should batch changes to topology and attribute layers, exposing well-defined deltas for the undo stack.

## Parametric Solid Modelling Parity
- **Separated geometry vs. topology:** Production solid modelers maintain a distinction between geometric definitions (curves, surfaces) and topological entities (vertices, edges, faces) so parameters can be recomputed without loss of connectivity. FreeCrafter should adopt a similar separation to unlock parametric edits and reliable references such as `Edge1` or `Face3` in constraints.
- **Document object recompute:** Features store parameters and rebuild their shapes when dependencies change. FreeCrafter needs a document graph that caches parameters, manages recompute order, and handles failure cases with clear diagnostics.
- **Feature-level booleans & fillets:** Robust kernels supply boolean, fillet, and chamfer operators that operate on the topological model. FreeCrafter’s roadmap should include equivalent feature nodes and error reporting when the solver fails.

## Viewport & Scenegraph Integration
- **Scenegraph handoff:** External reference apps decouple modelling kernels from the scenegraph renderer. FreeCrafter should maintain a translation layer that converts document shapes into renderable scenegraph nodes without leaking kernel details into the UI.
- **Incremental redraw:** When geometry updates, only the affected nodes are dirtied and re-uploaded. FreeCrafter needs dirty-flag propagation and selective buffer updates to keep the viewport interactive.
- **Diagnostics overlays:** Mature systems surface face normals, curvature, and draft analysis overlays derived from the same topology data. FreeCrafter should plan for diagnostic layers that read from the shared adjacency/attribute infrastructure.

## Next Steps for FreeCrafter
1. Define a unified mesh/solid kernel interface that exposes adjacency queries, attribute layers, and parametric rebuild hooks.
2. Update tool implementations to consume the new kernel interface, emitting minimal deltas for undo/redo.
3. Introduce a recompute scheduler in the document model to manage dependencies between parametric features.
4. Refactor the renderer to accept incremental updates from both mesh and solid sources, paving the way for diagnostic overlays.

By translating proven concepts from industry tooling into FreeCrafter-centric requirements, we can prioritize foundational work that unlocks reliable creation, editing, and visualization workflows without copying external implementations verbatim.
