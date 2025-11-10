---
name: "Push/Pull produces broken topology / no through-cuts"
about: "Repair push/pull tool output to create valid manifolds and perforations"
title: "Push/Pull produces broken topology / no through-cuts"
labels: ["bug", "priority-critical", "area:modeling", "type:geometry"]
assignees: []
---

## Description
Extruding faces sometimes yields non-manifold walls; inward pushes don’t cut through as expected.

## Steps to Reproduce
1. Push inward a wall until it should perforate.
2. Attempt outward push adjoining another solid.

## Expected Behavior
Clean side faces, capping, and through-cut when distance ≥ thickness.

## Actual Behavior
Open borders or silent no-op on intersections.

## Suggested Fix / Investigation
- Files: `src/Phase6/PushPullTool.*`, `src/geometry/GeometryKernel.*`, `src/geom/PushANDPull.*`
- Build side faces from boundary loop; add cap; weld & heal; implement perforation heuristic; fallback to boolean subtract when intersecting.

## Acceptance Criteria
- [ ] Outward push adds volume; inward push perforates and removes opposite wall.
- [ ] Mesh validator reports manifold output.
- [ ] Undo/redo supported.
