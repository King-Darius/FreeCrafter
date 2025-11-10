---
name: "Pattern/array tools lack parametric editability"
about: "Add parametric editing support for pattern and array tools"
title: "Pattern/array tools lack parametric editability"
labels: ["enhancement", "priority-medium", "area:modeling"]
assignees: []
---

## Description
Linear/circular arrays cannot be edited after creation (count/spacing/angle).

## Suggested Fix / Investigation
- Files: `src/Phase6/PatternTool.*`, `src/scene/PatternInstance.*`
- Store pattern as document node with parameters; instance rendering; Inspector exposes params; apply delta updates.

## Acceptance Criteria
- [ ] Create 6-hole pattern → change to 8 via Inspector.
- [ ] Instances update instantly; undo/redo works.
- [ ] Per-instance suppress supported.
