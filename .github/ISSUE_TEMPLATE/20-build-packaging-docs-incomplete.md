---
name: "Build & packaging docs incomplete (Windows .exe)"
about: "Document the Windows build and packaging workflow"
title: "Build & packaging docs incomplete (Windows .exe)"
labels: ["docs", "priority-low", "area:build"]
assignees: []
---

## Description
Contributors cannot reliably produce a runnable Windows binary; prereqs unclear; silent failures.

## Suggested Fix / Investigation
- Files: `README.md`, `docs/building_windows.md`, `build/*.bat`, CI yaml
- Write step-by-step GUI-only build guide; preflight checks in scripts; GitHub Actions workflow to publish artifacts.

## Acceptance Criteria
- [ ] Fresh Windows VM following docs yields runnable `.exe`.
- [ ] CI artifacts downloadable and start up.
- [ ] Clear troubleshooting section for common errors.
