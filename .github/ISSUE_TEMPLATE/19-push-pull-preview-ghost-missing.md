---
name: "Push/Pull preview ghost missing / slow"
about: "Restore fast push/pull preview while dragging"
title: "Push/Pull preview ghost missing / slow"
labels: ["bug", "priority-low", "area:viewport", "type:performance"]
assignees: []
---

## Description
No live preview while dragging, or preview stalls on large faces.

## Suggested Fix / Investigation
- Files: `src/view/PreviewRenderer.*`, `src/Phase6/PushPullTool.*`
- As-you-drag lightweight triangulation; off-thread build; wireframe/transparent ghost; commit builds final mesh.

## Acceptance Criteria
- [ ] Preview appears < 100 ms on 100k-tri scene.
- [ ] No UI jank during drag.
- [ ] Final commit matches preview.
