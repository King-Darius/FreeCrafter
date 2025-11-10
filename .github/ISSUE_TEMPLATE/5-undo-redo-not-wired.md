---
name: "Undo/Redo not wired for many operations"
about: "Ensure all major editing operations support undo/redo"
title: "Undo/Redo not wired for many operations"
labels: ["bug", "priority-high", "area:core", "type:undo"]
assignees: []
---

## Description
A subset of edits are not undoable; redo sometimes replays partial state.

## Suggested Fix / Investigation
- Files: `src/commands/*`, `src/core/CommandStack.*`, tool commit paths
- Introduce `QUndoCommand` subclasses for Create/Transform/Delete/Group/PropertyEdit; ensure every commit path returns a command; fix stack index off-by-one.

## Acceptance Criteria
- [ ] Draw→Extrude→Move→Delete fully undo/redo in order.
- [ ] Command log mirrors user actions 1:1.
- [ ] Regression tests pass for 20 action sequences.
