---
name: "Autosave & crash recovery missing"
about: "Implement autosave and crash recovery flow"
title: "Autosave & crash recovery missing"
labels: ["enhancement", "priority-high", "area:core", "type:reliability"]
assignees: []
---

## Description
No periodic backups; session loss on crash.

## Suggested Fix / Investigation
- Files: `src/app/AutosaveManager.*`, `src/io/DocumentIO.*`
- Idle-aware autosave (config interval) to `~/.freecrafter/autosaves`; restore prompt on startup; retention policy.

## Acceptance Criteria
- [ ] Kill process mid-edit → restart → “Restore autosave?” works.
- [ ] No autosave while modal or during drag.
- [ ] Autosave files pruned per policy.
