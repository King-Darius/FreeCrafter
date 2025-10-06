# Phase 6 — Integrated Advanced Tools

Phase 6 builds on the solid foundation from Phases 1–5 by delivering the advanced modeling workflows needed for production-grade CAD and archviz. Each initiative expands FreeCrafter's tooling depth while honoring the direct modeling feel established in earlier phases.

## Round Corner
- Interactive fillet and chamfer creation with live previews.
- Adjustable corner resolution controls with per-corner overrides.
- Toggle for tagging resulting edges as hard or soft to match downstream shading.

## CurveIt
- Rail-based lofting, skinning, and bridging between open or closed profile loops.
- Optional subdivision smoothing for seamless transitions across generated surfaces.
- Support for profile reordering, twist management, and symmetric pairing.

## PushANDPull
- Normal-based surface thickening with automatic capping.
- General surface offsetting that exceeds the existing Push/Pull tool's planar scope.
- Thickness presets and constraint aids for mechanical-style workflows.

## Surface
- Direct drawing and offsetting on curved faces.
- Remeshing helpers that maintain usable topology for subsequent edits.
- Snapping controls that respect existing curvature and feature edges.

## BezierKnife
- Precision cutting along Bézier, NURBS, or polyline paths.
- On-screen grip manipulators for adjusting control points and tangents.
- Controls for curve degree, knot structure, and projection behavior.

## QuadTools
- Utilities for tagging and validating quad-based topology.
- Loop and ring selection accelerators with preview overlays.
- Automated grid filling for patching holes or irregular areas.

## SubD
- Catmull–Clark subdivision surfaces with crease management.
- Multi-level preview system with cage overlays for intuitive edits.
- Non-destructive toggling between cage and subdivided representations.

## Weld
- Direction-aware vertex welding and unwelding for seam repair.
- Support for toleranced selection sets and interactive feedback.
- Batch operations with undo-friendly grouping.

## Vertex Tools
- Soft selection transforms with customizable falloff profiles.
- On-screen gizmos for translation, rotation, and scaling.
- Falloff visualization to guide nuanced deformations.

## Clean
- Automated cleanup passes that purge unused data.
- Merging of coplanar faces and removal of stray edges.
- Configurable tolerance thresholds for aggressive or conservative cleanup.

## ClothEngine
- Position-based dynamics (PBD) solver for cloth behavior.
- Pinning and weight maps to control constraints and influence.
- Collision handling against solid geometry with stabilized iterations.

## CAD Designer
- Consolidated suite for Pull+, shelling, splitting, mirroring, and patterning.
- Revolve and sweep operations driven by sketches or guide curves.
- Imprint and project tools for bridging CAD-style workflows into direct modeling.

## Integration & Roadmap Notes
- Tool implementations will share consistent UI scaffolding via the ToolManager framework established in Phases 1–4.
- Advanced operations must interoperate with Phase 5's object management features, ensuring groups/components maintain histories and metadata.
- Final integration reviews will confirm interoperability with downstream phases, particularly Phase 7's file I/O expectations and Phase 8's performance targets.

