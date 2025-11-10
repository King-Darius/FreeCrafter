---
name: "Solid booleans (Union/Difference/Intersect) missing"
about: "Implement robust solid boolean operations"
title: "Solid booleans (Union/Difference/Intersect) missing"
labels: ["enhancement", "priority-critical", "area:kernel", "type:feature"]
assignees: []
---

## Description
No robust boolean operators; users cannot combine or cut solids.

## Suggested Fix / Investigation
- Files: `src/geometry/boolean_*` (new), `docs/geometry_system_gaps.md`, `ROADMAP.md`
- Integrate OCCT/CGAL/Geogram booleans; tolerance wrapper, auto-heal; diagnostics on failure.

## Acceptance Criteria
- [ ] Union/Diff/Intersect for overlapping boxes, coplanar faces, touching shells.
- [ ] Results watertight; no inverted normals.
- [ ] Unit tests cover 12 canonical cases.
