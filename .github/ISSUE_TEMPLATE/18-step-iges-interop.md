---
name: "STEP/IGES import/export not supported"
about: "Add STEP and IGES interoperability"
title: "STEP/IGES import/export not supported"
labels: ["enhancement", "priority-medium", "area:file-io", "area:interop"]
assignees: []
---

## Description
Only mesh formats (OBJ/STL/glTF) available; cannot exchange parametric CAD.

## Suggested Fix / Investigation
- Files: `src/io/ImportExport.*`
- Integrate OCCT STEP/IGES readers/writers; unit & orientation handling; progress & error reporting.

## Acceptance Criteria
- [ ] Import STEP from SpaceClaim → valid solids.
- [ ] Export IGES opened in another CAD without errors.
- [ ] Round-trip preserves topology within tolerance.
