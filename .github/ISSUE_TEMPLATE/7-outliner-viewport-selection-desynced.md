---
name: "Outliner ⇄ Viewport selection desynced"
about: "Synchronize selection state between the outliner and the viewport"
title: "Outliner ⇄ Viewport selection desynced"
labels: ["bug", "priority-high", "area:ui", "type:selection"]
assignees: []
---

## Description
Selecting in Outliner doesn’t always highlight in viewport and vice versa; drag-reparent not undoable.

## Suggested Fix / Investigation
- Files: `src/ui/OutlinerPanel.*`, `src/scene/Document.*`, `src/ui/InspectorPanel.*`
- Two-way selection bus; implement `ReparentCommand`; context menu: Group/Explode/Isolate.

## Acceptance Criteria
- [ ] Selecting either side reflects immediately.
- [ ] Drag reparent has undo/redo.
- [ ] No selection leaks across document switches.
