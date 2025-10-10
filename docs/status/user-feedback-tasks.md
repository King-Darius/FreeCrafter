# User Feedback Tasks

These tasks capture recent user feedback that needs follow-up work.

:::task-stub{title="Define default object colour palette options"}
1. Audit the current default material definitions in `src/GLViewport.cpp` and `styles/dark.qss` to confirm which colours are applied to unselected, selected, monochrome, and hidden-line renders (mirror updates in `styles/light.qss`).
2. Prototype at least three pastel-inspired palettes (e.g., light green, light purple, light blue) and document their RGBA values alongside selection highlight complements.
3. Add a settings proposal outlining how users would choose a default palette per project (e.g., preferences dialog, project metadata), including persistence requirements.
4. Socialize the palette options with design/UX for sign-off, then break down implementation tasks for updating shaders, UI previews, and configuration storage.
:::

:::note
**Update:** Soft Green, Lavender, and Powder Blue presets are now selectable from **View ▸ Surface Palette**, apply immediately in the viewport, and persist with each `.fcm` project. The active choice is also stored in user settings to seed new documents, matching the design backlog workflow. Follow-up validation (Qt-dependent build/tests and in-app smoke checks) remains outstanding and should be rerun once the toolchain is available.
:::

:::task-stub{title="Write a troubleshooting guide for launching FreeCrafter"}
1. Expand the README/CONTRIBUTING build sections with a step-by-step Windows walkthrough covering bootstrap, required Qt dependencies, and PATH setup, linking to automation scripts where available.
2. Document the most common startup failures (missing Qt DLLs, CMake not on PATH, outdated Visual Studio redistributables) and add quick diagnostics/commands for each.
3. Provide guidance for macOS/Linux users on locating Qt frameworks or using platform-specific helper scripts, noting any differences from Windows instructions.
4. Include a checklist for validating a fresh install (first launch smoke test, logging verification) and reference future packaging docs so the guidance stays in sync with release artifacts.
:::
