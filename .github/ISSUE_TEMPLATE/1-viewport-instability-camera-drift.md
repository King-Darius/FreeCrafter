---
name: "Viewport instability / camera drift"
about: "Fix camera pivot drift and incorrect clipping behaviour"
title: "Viewport instability / camera drift"
labels: ["bug", "priority-critical", "area:viewport", "type:stability"]
assignees: []
---

## Description
Orbit/pan occasionally leaves the camera pivot drifting or snapping; near/far clipping feels wrong.

## Steps to Reproduce
1. Open any scene with a medium object.
2. Orbit → pan → orbit repeatedly, then release mouse.
3. Observe pivot drift and sporadic clipping.

## Expected Behavior
Stable pivot (last pick/hover) and no autonomous motion; adaptive near/far based on scene bounds.

## Actual Behavior
Camera continues to move slightly; geometry intermittently clips.

## Suggested Fix / Investigation
- Files: `src/view/CameraController.*`, `src/view/GLViewport.*`
- Lock pivot to last pick; store & reuse pivot on tool transitions.
- Recompute near/far from scene AABB; guard against NaN/INF matrices; clamp FOV.

## Acceptance Criteria
- [ ] No drift after releasing orbit/pan.
- [ ] No unexpected clipping across 1e-3…1e+5 unit scales.
- [ ] Automated camera stress test passes (orbit/pan loops → stable matrices).
