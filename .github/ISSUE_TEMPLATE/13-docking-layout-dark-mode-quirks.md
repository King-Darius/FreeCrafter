---
name: "Docking/layout persistence & dark-mode quirks"
about: "Persist dock layouts and ensure theme toggles update all widgets"
title: "Docking/layout persistence & dark-mode quirks"
labels: ["bug", "priority-medium", "area:ui"]
assignees: []
---

## Description
Panels overlap/disappear on resize; theme toggle leaves mixed light/dark widgets.

## Suggested Fix / Investigation
- Files: `src/ui/MainWindow.*`, `src/ui/ThemeManager.*`
- Persist dock geometry via `QSettings`; fix size policies; apply palette & repolish on theme switch; provide high-DPI icons.

## Acceptance Criteria
- [ ] Layout persists across restarts.
- [ ] Zero overlap on 720p–4K.
- [ ] Theme switch fully updates all widgets/icons.
