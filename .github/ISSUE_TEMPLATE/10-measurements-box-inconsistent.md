---
name: "Measurements box (VCB) inconsistent / unit parsing flaky"
about: "Fix unit parsing and tool handoff from the measurements box"
title: "Measurements box (VCB) inconsistent / unit parsing flaky"
labels: ["bug", "priority-high", "area:input", "type:units"]
assignees: []
---

## Description
Typed overrides ignored or mis-parsed (e.g., `1m 20cm`, `1'6"`); tools don’t receive values.

## Suggested Fix / Investigation
- Files: `src/ui/MeasurementsBox.*`, `src/tools/ToolManager.*`, `src/units/UnitParser.*`
- Normalize locale; robust mixed-unit parser; `Tool::getMeasurementKind()` drives prompt; Enter routes to active tool.

## Acceptance Criteria
- [ ] Rectangle second corner accepts `1m 20cm`.
- [ ] Rotate accepts degrees/radians.
- [ ] Invalid input shows non-blocking error.
