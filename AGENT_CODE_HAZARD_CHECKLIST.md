# AGENT Code Hazard & Bug Location Checklist

**Last Reviewed/Updated:** 2025-11-11

This file is for agents (both human and automated) to identify, annotate, and triage persistent code hazards, incomplete features, and architectural problems in FreeCrafter. Use these examples and locations to leave actionable, discoverable comments for future contributors and code-modifying bots.

---

## 1. Viewport Rendering Bugs
- **Location:** src/GLViewport.*, src/MainWindow.*
- **Annotation Example:**
    // TODO: Viewport suffers from redraw glitches and camera drift.
    // See README.md#Current limitations. Debug paint event triggers, camera update logic, and OpenGL state restoration here.

## 2. Desynchronized GUI Layout/Theming
- **Location:** src/MainWindow.cpp, src/dock/*, qss/themes/
- **Annotation Example:**
    // FIXME: Docking/layout state is inconsistent on theme toggle or window resize.
    // See README.md#GUI layout & styling. Standardize QDockWidget restoreState/saveState logic.

## 3. Tool Activation/Actions Broken
- **Location:** src/toolmanager.*, src/actions.*, src/MainWindow.cpp
- **Annotation Example:**
    // TODO: Action → ToolID mapping incomplete or mismatched (AGENTS.md#3.1 Tool activation).
    // Ensure every QAction is mapped to its corresponding ToolID and disables gracefully if not implemented.

## 4. Persistence & Recovery Fragility (Autosave/Undo/Redo)
- **Location:** src/document.*, src/MainWindow.cpp, src/io/*
- **Annotation Example:**
    // CRITICAL: Autosave/undo/redo paths may cause data loss or corrupt transforms (README.md#Persistence & recovery).
    // Audit serialization and command stack integration for missing or mis-ordered operations.

## 5. Testing Gaps & Risk of Regression
- **Location:** tests/, CMakeLists.txt, CI scripts
- **Annotation Example:**
    // TODO: Add integration tests for interactive tools and viewport regression checks (see docs/reviews/freecrafter-reality-check.md).
    // Current coverage only includes primitive creation/undo stack (test_scene_commands.cpp).

## 6. Unimplemented (Stub) Features
- **Location:** Plugin manager, advanced geometry tools, etc.
- **Annotation Example:**
    // STUB: Placeholder for plugin loading (docs/milestones/plugins.md)
    // Feature not yet implemented. Loader currently only enumerates file entries.

## 7. Styling/Theming Inconsistency
- **Location:** src/MainWindow.cpp, resources/styles/dark.qss, resources/styles/light.qss
- **Annotation Example:**
    // TODO: Ensure parity between dark.qss and light.qss per AGENTS.md#3.7 Styling (QSS themes).
    // Test all widget classes for proper theme application, focus rings, and icon swaps.

## 8. Selection/Transform Gizmo Gaps
- **Location:** Likely in src/viewport/tools/, gizmo logic/class.
- **Annotation Example:**
    // FIXME: Gizmo occasionally fails to activate after object creation (AGENTS.md#3.6 Selection & transforms).
    // After doc->addNode, ensure gizmo selection and axis locks respond as expected.

## 9. Error Handling/Stubbed Actions
- **Location:** Tool finish/callbacks, geometry commit routines.
- **Annotation Example:**
    // FIXME: Failure to commit tool (invalid geometry, non-planar face) should show user-clearly inline error (AGENTS.md#3.9 Error handling).
    // Do not silently drop tool; keep active and guide correct action.

---

## General Agent Instructions
- Start all actionable code comments with // CRITICAL:, // FIXME:, // TODO:, or // STUB: per severity.
- Place these comments early in relevant methods, classes, or file headers.
- Keep this checklist up-to-date as issues are fixed or FreeCrafter's architecture evolves.
- See README.md, AGENTS.md, and docs/reviews for more context.

---

Thank you for making FreeCrafter more robust for both human and AI-assisted development!