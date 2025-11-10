---
name: "Save/Load integrity + Windows .exe packaging unreliable"
about: "Stabilize document I/O and Windows packaging pipeline"
title: "Save/Load integrity + Windows .exe packaging unreliable"
labels: ["bug", "priority-high", "area:file-io", "area:build"]
assignees: []
---

## Description
Scenes occasionally reload corrupted/partial; PyInstaller/Nuitka builds fail to produce runnable `.exe`.

## Suggested Fix / Investigation
- Files: `src/io/DocumentIO.*`, `build/build_pyinstaller.bat`, `build/deploy_pyside6.bat`, `installer/SketchMiniApp.iss`
- Atomic save (temp → rename), versioned headers, strict validation on load; CI job builds artifact; ensure Qt platform plugins & OpenGL fallback included.

## Acceptance Criteria
- [ ] 100 save/load cycles diff-equal.
- [ ] CI publishes runnable `.exe` artifact.
- [ ] Missing prereqs produce clear error messages.
