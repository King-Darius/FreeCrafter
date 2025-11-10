---
name: "Fillet/Chamfer lacks edge-chain & variable radius"
about: "Extend fillet and chamfer tools with chain selection and variable radii"
title: "Fillet/Chamfer lacks edge-chain & variable radius"
labels: ["enhancement", "priority-medium", "area:modeling"]
assignees: []
---

## Description
Only single-corner ops; no chain selection, no variable radii, no continuity options.

## Suggested Fix / Investigation
- Files: `src/Phase6/RoundTool.*`, `src/geometry/RoundingEngine.*`
- Edge-chain traversal; per-edge radius map; optional G1/G2; failure diagnostics.

## Acceptance Criteria
- [ ] Apply chain fillet around a box edge loop.
- [ ] Variable radii validated.
- [ ] Error messages for impossible blends.
