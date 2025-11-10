---
name: "Delete/Eraser leaves ghosts / no undo"
about: "Fix deletion workflows so scene graph stays consistent and undoable"
title: "Delete/Eraser leaves ghosts / no undo"
labels: ["bug", "priority-high", "area:scene", "type:object-lifecycle"]
assignees: []
---

## Description
Deleting creates stale outliner entries; some deletions irreversible.

## Suggested Fix / Investigation
- Files: `src/tools/EraserTool.*`, `src/ui/EditActions.*`, `src/scene/Document.*`
- Implement `DeleteObjectCommand` (snapshot); prevent deletion of locked; cascade or reparent children deterministically; emit model signals.

## Acceptance Criteria
- [ ] Outliner updates instantly; no ghost nodes.
- [ ] Undo restores objects and hierarchy.
- [ ] Bulk delete is stable.
