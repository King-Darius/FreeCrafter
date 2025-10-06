# Legal & Technical Review — Phase 0

## Licensing Summary
- **Project license:** MIT (retained in repository root). Compatible with permissive dependencies.
- **Qt 6 (LGPL/commercial):** Using LGPL build. We statically link only permissible modules (Widgets, OpenGL, Svg). Dynamic linking maintained with deployment instructions in installer scripts.
- **OpenGL:** Core spec licensing is permissive; ensure attribution in docs if using sample shaders from Khronos.
- **Geometry libraries:** Planned integration with CGAL or libIGL. CGAL dual-license (GPL/commercial); prefer libIGL (MPL2) for base features and isolate CGAL-dependent features behind optional module pending commercial license decision.
- **File formats:** Native `.fcm` uses MIT-licensed serializer. Import/export for OBJ/STL/FBX/DAE rely on Assimp (BSD). DXF/DWG import requires Teigha/ODA (commercial) — scheduled for Phase 7 research.

## Patent/IP Considerations
- Review SketchUp patent portfolio for push/pull and inference behaviors; implementation must be novel (state machines and inference events rather than patent-restricted algorithms).
- Ensure plugin API avoids embedding proprietary format logic contributed by third parties without CLA.

## Technical Risks & Mitigations
| Area | Risk | Mitigation |
| --- | --- | --- |
| Qt Dock persistence | Corrupted layout states on version bump | Version key in QSettings; fallback to defaults when schema mismatch detected. |
| Hotkey rebinding | Conflicting shortcuts | Validation in shortcut editor prevents duplicates and surfaces conflicts in UI. |
| OpenGL compatibility | macOS core profile dropping fixed pipeline | Move renderer toward VBO/Shader pipeline by Phase 2; compatibility profile used temporarily. |
| Plugin sandboxing | Untrusted code execution | Plugins run out-of-process for heavy operations; embed allowlist manifest per plugin. |
| Geometry robustness | Degenerate faces & precision | Implement tolerance-aware comparisons and healing passes (Phase 2). |

## Compliance Checklist
- [x] SPDX headers to be added gradually as files are touched (tracked in Phase 8).
- [x] Third-party acknowledgements to live in `/docs/legal/notices.md` (to be created Phase 7).
- [ ] Export compliance review before shipping encryption-capable plugins (Phase 10).
- [ ] Texture/material licensing review for bundled brushes, swatches, and SmartWrap reference assets ahead of the Phase 11 surface painting release.
