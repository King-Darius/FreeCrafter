---
name: "Plugin runtime & scripting API missing"
about: "Add plugin runtime and scripting API for extensibility"
title: "Plugin runtime & scripting API missing"
labels: ["enhancement", "priority-medium", "area:extensibility"]
assignees: []
---

## Description
No runtime plugin loading or scripting; cannot automate or extend tools.

## Suggested Fix / Investigation
- Files: `src/plugins/*` (new), `docs/plugins.md`
- Embedded Python (or C++ ABI) with safe API: scene graph, creation, transforms, I/O; plugin manager UI; manifest & sandbox.

## Acceptance Criteria
- [ ] Example plugin adds “Create Cube” action.
- [ ] Plugin enable/disable without restart.
- [ ] Script console runs `create_primitive()` successfully.
