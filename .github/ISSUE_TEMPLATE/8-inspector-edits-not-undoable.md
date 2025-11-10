---
name: "Inspector edits not undoable; missing basics"
about: "Route inspector property changes through undo and add core fields"
title: "Inspector edits not undoable; missing basics"
labels: ["bug", "priority-high", "area:ui", "type:properties"]
assignees: []
---

## Description
Changing name/radius/segments applies instantly but cannot be undone; lacks tag/lock/visibility.

## Suggested Fix / Investigation
- Files: `src/ui/InspectorPanel.*`, `src/commands/PropertyEditCommand.*`, `src/scene/Document.*`
- Route all setters through `PropertyEditCommand`; add Name/Tag/Visible/Locked fields.

## Acceptance Criteria
- [ ] Property changes are undoable.
- [ ] Outliner reflects name/visibility immediately.
- [ ] Invalid values rejected with message.
