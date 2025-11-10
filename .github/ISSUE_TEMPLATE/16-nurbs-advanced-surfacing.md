---
name: "NURBS / advanced surfacing toolkit absent"
about: "Introduce NURBS curve and surface authoring tools"
title: "NURBS / advanced surfacing toolkit absent"
labels: ["enhancement", "priority-medium", "area:kernel"]
assignees: []
---

## Description
Loft/sweep are mesh-based; no B-spline/NURBS curves/surfaces edit/evaluate/trim.

## Suggested Fix / Investigation
- Files: `src/geometry/NURBS.*` (new), integration points in CAD Designer
- Add NURBS curves/surfaces (degree/knot editing); OCCT or lightweight NURBS lib; control-point Inspector.

## Acceptance Criteria
- [ ] Create/modify NURBS surface; knots/degree editable.
- [ ] Trim/join maintained; continuity checks pass.
- [ ] Export to STEP retains NURBS (after #18).
