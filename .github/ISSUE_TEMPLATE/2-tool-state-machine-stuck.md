---
name: "Tool state machine can get stuck / wrong active tool"
about: "Ensure rapid tool switching never leaves the tool manager in an invalid state"
title: "Tool state machine can get stuck / wrong active tool"
labels: ["bug", "priority-critical", "area:tools", "type:state"]
assignees: []
---

## Description
Fast switching via hotkeys/toolbar occasionally leaves no tool active or the wrong tool handling input.

## Steps to Reproduce
1. Activate Line.
2. Switch rapidly to Move/Eraser via hotkeys; hit `Esc` mid-gesture.

## Expected Behavior
Single active tool; `Esc` cancels current step then returns to Select.

## Actual Behavior
“Zombie” tool persists or app falls into idle with no tool.

## Suggested Fix / Investigation
- Files: `src/tools/ToolManager.*`, `src/tools/BaseTool.*`, `src/ui/MainWindow.*`
- Centralize `activateTool(id)`; always call `deactivate()` on current tool; unify `cancel()` semantics; add guard transitions Idle→Armed→Preview→Commit/Cancel.

## Acceptance Criteria
- [ ] 500 rapid tool toggles never leave undefined state.
- [ ] `Esc` cancels consistently and returns to Select.
- [ ] Telemetry logs show matched activate/deactivate pairs.
