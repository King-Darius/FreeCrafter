---
name: "Cursor overlay & inference glyphs incomplete"
about: "Complete the cursor overlay and inference glyph system"
title: "Cursor overlay & inference glyphs incomplete"
labels: ["bug", "priority-high", "area:viewport", "type:ux"]
assignees: []
---

## Description
No endpoint/midpoint/axis-lock glyphs; custom cursor overlay flickers or is misaligned.

## Suggested Fix / Investigation
- Files: `src/view/GLViewport.*` (overlay), `src/tools/CursorDescriptor.*`, `src/inference/InferenceEngine.*`
- Blank OS cursor; draw GL overlay (crosshair, tool icon, snap glyphs, modifier hints); DPI-aware screen→GL mapping.

## Acceptance Criteria
- [ ] Endpoint/midpoint/center/tangent snaps visually indicated.
- [ ] Overlay never flickers and tracks cursor precisely.
- [ ] Performance cost < 0.2 ms/frame.
