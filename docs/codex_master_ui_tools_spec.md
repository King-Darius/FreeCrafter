# Codex Master UI & Tools Spec (Repo-Aligned, Implementation Grade) - v2.0

Prepared: 10 Oct 2025

> Purpose. Convert the FreeCrafter desktop concept and master tool specification into a stack-agnostic contract covering visuals, behaviors, and data flow. Engineering, design, QA, and Codex agents[...] 

---

## Table of Contents
1. Guiding Principles & Interaction Model
2. Application Shell (Layout & Measurements)
3. Panels (Right Tray) & Data Flow
4. Input System: Selection, Snapping/Inference & Measurement HUD
5. Camera, Viewport & Rendering Pipeline
6. Tool Specifications (Exhaustive)
7. Navigation Schemes & Temporary Tools
8. File I/O, Scene State & Persistence
9. Undo/Redo Command Architecture
10. Performance, Robustness & Tolerances
11. Accessibility, Internationalization & Telemetry
12. Assets, Iconography & Theming
13. Roadmap Tools (Pre-spec)
14. QA Acceptance Matrix & Dev Checklist
15. Codex Mapping (IDs, Events, Modules)

---

## 1. Guiding Principles & Interaction Model

- **Predictable state machine** per tool: `Idle -> Armed -> (Preview step N)* -> Commit | Cancel`.
  - `Esc` cancels current step; second `Esc` reverts to Select.
  - `Enter`/`Return` commits current tool step; numeric input also commits when valid.
  - Right-click opens context options without committing or cancelling.
- **Type-to-refine:** Measurement HUD is ghost-focused. Any numeric entry routes to `ActiveTool::applyMeasurementOverride()` without explicit focus.
  - Comma grammar accepts multi-parameter inputs (for example `1200,800`, `2,1,1`).
  - Units accepted: `mm`, `cm`, `m`, `in`, `ft`, double quote `"`, single quote `'`.
  - System remembers last unit per session; bare numbers default to document unit.
- **Sticky axis/snap locks:** Tap `X`, `Y`, `Z` (or Arrow keys) to lock. Hold `Shift` to make the current inference sticky until released.
  - HUD displays lock badges beside cursor and in status bar.
- **Minimal modals:** Prefer context toolbars, inline HUDs, or non-blocking drawers. Only blocking modal allowed for file dialogs or fatal errors.
- **Undoable everything:** Every commit adds a single command with inverse. Array operations collapse to one command object.
- **Non-destructive defaults:** Operations preserve source geometry unless modifiers explicitly request destructive behavior (for example `Ctrl` toggles create-new vs merge).
- **Cross-stack determinism:** All timing constants (hover delays, easing durations), snapping tolerances, and render thresholds must be identical across native and web builds.
- **Logging and telemetry hooks:** Tool activation, completion, cancellation, and error states emit structured events for QA parity.

State transitions (per tool) expressed as ASCII diagram:
```
Idle --activate--> Armed --input--> Preview(n) --commit--> Idle
                   |                     |
                   |--cancel/Esc---------|
Preview(n) --next step--> Preview(n+1)
Preview(n) --context menu--> Preview(n) (no transition)
```

---

## 2. Application Shell (Layout & Measurements)

```
+ Menu Bar ------------------------------------------------------------ + 40 px (H)
| File | Edit | View | Tools | Window | Help                           |
+ Top Toolbar (Primary) ---------------------------------------------- + 48 px (H)
| New/Open/Save - Undo/Redo - Style - Projection - Shadows - Presets - |
| Theme toggle                                                         |
+ Context Toolbar (tool-active only) --------------------------------- + 40 px (H)
| Tool options (pagination when >6 controls)                           |
+---------------------------------------------------------------------+
| Left Tool Strip 56 px (W) |               Viewport                   |
| 40x40 buttons, 8 px gaps  |  Infinite canvas, overlays, HUD          |
|                           |                                          |
+---------------------------------------------------------------------+ 24 px (H)
| Status: Hint - Snap/Locks - Selection count - XYZ - Measurement echo |
| - Zoom level - FPS (debug)                                           |
+---------------------------------------------------------------------+
```

### 2.1 Menu Bar

- Height 40 px; top/bottom padding 4 px; left/right frame padding 12 px.
- Font: system default Semibold 13 px, letter spacing 0.
- Menu item row height 32 px; side padding 12 px; submenu arrow inset 8 px.
- Keyboard mnemonics follow platform conventions (Alt accelerators on Windows, underline in textual docs only).
- Menu taxonomy:
  - **File:** New, Open, Save, Save As, Import (OBJ/STL), Export (OBJ/GLTF), Revert, Recent files (max 5), Exit.
  - **Edit:** Undo, Redo, Cut, Copy, Paste, Delete, Select All, Invert Selection, Preferences.
  - **View:** Style (Wireframe, Hidden line, Shaded, Shaded + Edges), Projection (Perspective, Parallel), Camera presets (Front, Back, Left, Right, Top, Bottom, Iso), Grid (Show, Size), Guides, Se[...]
  - **Tools:** Sections mirrored to left tool strip (Selection, Drawing, Modify, Annotate, Section, Navigate) grouped by separators.
  - **Window:** Toggle panels (Outliner, Entity Info, Materials, Tags, Environment, Shortcuts), tray presets (Default, Modeling, Materials, Minimal).
  - **Help:** Documentation, Keyboard Shortcuts, About, Check for Updates (native only), Submit Feedback.

### 2.2 Top Toolbars

- **Primary toolbar:** Height 48 px; background uses theme top-bar color; button size 32x32 px; hit target 40x40 px with invisible padding; inter-group spacing 16 px.
  - Contains core document actions, view toggles, and theme action bound to `View > Toggle Dark Mode`.
  - Buttons display icon only; tooltip (max 32 chars) with accelerator.
- **Context toolbar:** Appears under primary toolbar when a tool exposes options; height 40 px; uses horizontal scroll if >6 controls.
  - Control layout: label 11 px medium weight, input height 28 px, spacing 12 px; toggles align baseline; separators 8 px vertical.
  - Each control bound to tool state via `ToolOptionBinding` interface. Options persist per tool session and survive undo.

### 2.3 Left Tool Strip

- Column width 56 px; background uses theme accent with 8 px inset from viewport.
- Buttons 40x40 px with 8 px vertical spacing and 12 px top/bottom padding.
- Button states:
  - Rest: icon 20 px monochrome.
  - Hover: 2 px outline (theme highlight).
  - Active: filled chip with icon tinted white; exclusive QButtonGroup binding.
- Order (top to bottom):
  1. Select
  2. Eraser
  3. Line
  4. Rectangle
  5. Rotated Rectangle
  6. Circle
  7. Polygon
  8. Arc (3 pt)
  9. Center Arc
  10. Tangent Arc
  11. Bezier
  12. Freehand
  13. Move
  14. Rotate
  15. Scale
  16. Offset
  17. Push/Pull
  18. Extrude
  19. Follow Me
  20. Paint Bucket
  21. Text
  22. Dimension
  23. Tape Measure
  24. Axes
  25. Section Plane
  26. Orbit (toggle)
  27. Pan
  28. Zoom
- Group separators rendered as 16 px vertical gap and 1 px divider line.

### 2.4 Viewport

- Occupies remaining client area; minimum size 640x480 px before layout collapses.
- Grid default on with spacing 1000 mm; axis lines 2 px; minor grid 1 px.
- Overlays (nav ball, inference glyphs, guides) respect safe inset of 16 px from edges; overlays never block pointer events in central region.
- Cursor HUD anchored 24 px right and 20 px above pointer; auto-repositions near screen borders.

### 2.5 Right Tray (Dock Stack)

- Default width 320 px; resizable 260-560 px via splitter handle 6 px wide.
- Dock stack hosts accordion panels; panel header height 28 px; icon 16 px; caret indicates collapse.
- When unpinned, tray auto-hides after 500 ms of pointer exit; reappears when pointer hits 8 px reveal strip.
- Double-click header toggles collapsed state when pinned.
- Tray remembers panel order, expanded state, and width via `ui/layout.json`.

### 2.6 Status Bar

- Height 24 px; baseline alignment center.
- Sections:
  1. Active tool hint (flex min 160 px).
  2. Snap/lock badges (auto width, 8 px gap).
  3. Selection count (min 60 px).
  4. Cursor world coordinates (X,Y,Z with 2 decimal places by default).
  5. Measurement echo (reflects last typed or committed value).
  6. Zoom percentage (rounded to integer).
  7. FPS readout (debug flag; hidden in release).
- Divider between sections uses 1 px vertical line, 8 px padding.

---

## 3. Panels (Right Tray) & Data Flow

### 3.1 Entity Info (Inspector)

- Layout: label column 96 px; input field 180 px minimum; spacing 8 px vertical.
- Fields:
  - Name (line edit, commit on Enter).
  - Tag/Layer (combo with color chip).
  - Visibility toggle, Lock toggle, Hidden, Cast Shadows, Receive Shadows.
  - Materials: Front, Back (dropdown with swatch).
  - Component definition info (readonly if instance).
- Transform section:
  - Position (X,Y,Z) mm default; increments 0.1 mm.
  - Rotation (Heading, Pitch, Bank) degrees.
  - Scale (Sx, Sy, Sz) floats with 3 decimal places.
- Data flow: Edits create `TransformCommand` or `PropertyEditCommand`, routed through `CommandStack`. Inspector listens to `Scene::SelectionChanged` and `Scene::NodeChanged`.
- File references: `src/ui/InspectorPanel.*`, `src/ops/commands/TransformCommand.*`.

### 3.2 Outliner

- Tree view row height 24 px; indent 16 px per level.
- Icons differentiate Group, Component, Face, Edge, Section.
- Eye/Lock toggles inside row with 8 px spacing; Alt-click Eye isolates selection.
- Drag and drop reparenting snaps to highlight bar; preserves world transform by applying inverse parent transform.
- Context menu actions: Make Group, Make Component, Explode, Rename, Duplicate, Delete.
- Data flow: `Scene::Document` exposes hierarchical model; Outliner uses `QAbstractItemModel` adapter in `src/ui/OutlinerModel.*`.

### 3.3 Materials Panel

- Swatch grid 80 px cells, 8 px gutter; supports search bar (height 28 px).
- Import button open file dialog with texture preview.
- Eyedropper command triggered by Paint tool with Alt; panel updates `currentMaterialId`.
- Applying material emits `ApplyMaterialCommand`.
- Files: `src/ui/MaterialsPanel.*`, textures in `resources/materials/`.

### 3.4 Tags/Layers

- List with color pickers; row height 24 px.
- Controls: visibility eye, lock badge, rename inline (F2), color (opens Qt color dialog).
- Assigning tags via inspector or context toolbar.
- Data persistence: `SceneSettings::tags` serialized with document.
- Files: `src/ui/TagsPanel.*`, `src/scene/TagManager.*`.

### 3.5 Environment Panel

- Sections: Sun and Sky, Grid, Background, Section Fill.
- Sun controls: sliders for Date (0-365), Time (0-24), Azimuth (deg), Altitude (deg).
- Grid controls: toggle, spacing (mm), divisions (int), color pickers.
- Data writes to `EnvironmentSettings` and pushes to renderer via `Graphics::EnvironmentBridge`.

### 3.6 Shortcuts & Navigation Preferences

- Table view bound to `resources/config/hotkeys_default.json`.
- Columns: Action, Default, Custom, Context.
- Import/Export buttons open JSON file dialogs.
- Navigation preferences tied to `resources/config/navigation_default.json`, exposing scheme (SketchUp, DSM), orbit sensitivity, pan speed, zoom-to-cursor toggle.

### 3.7 Panel Command Flow

- All panel edits generate commands (or preferences writes) except changes flagged as ephemeral (view toggles).
- Panels observe `UndoStack::indexChanged` to refresh UI state after undo/redo.
- Telemetry event `panel.edit` includes module, field, success flag.

---

## 4. Input System: Selection, Snapping/Inference & Measurement HUD

### 4.1 Selection Model

- Primary hit test uses BVH on render thread; results piped back within 5 ms budget.
- Click selects nearest visible entity; Shift adds, Ctrl/Cmd toggles, Alt subtracts.
- Double-click enters group/component edit context; triple-click selects connected surface.
- Marquee:
  - Left-to-right (drag with primary button) selects entities entirely within box.
  - Right-to-left selects entities intersecting box (crossing).
  - Box displays dashed border 2 px; fill 25 percent alpha.
- Esc sequence: first Esc cancels marquee/preview; second Esc clears selection; third Esc exits edit context if inside component.
- Selection state stored in `SelectionSet` (src/interaction/SelectionSet.*), exposing `nodeIds`, `edgeIds`, etc.

### 4.2 Snapping & Inference

- Snap priority order with weights:
  1. Endpoint (weight 100)
  2. Midpoint (90)
  3. Center (80)
  4. Intersection (70)
  5. On-Edge (60)
  6. On-Face (50)
  7. Axis alignment (40)
  8. Derived constraints (perpendicular, parallel, tangent) (30)
  9. Free cursor (0)
- Search radius 8 px, expanding to 12 px after 120 ms if no hit; fallback reverts after 400 ms.
- Sticky locks triggered via `Shift` or axis key; persists until unlock key pressed again or tool commit.
- Glyph rendering: 16 px icons with 2 px outline for contrast; placed beside cursor with 4 px offset; tinted by inference type.

### 4.3 Measurement HUD

- HUD anchored near cursor; uses monospace font 14 px.
- Primary field shows typed input; secondary line displays hints (e.g., `Type length or press Enter`).
- Backspace removes last character; Delete clears entire input.
- Accepts the following grammar:

```ebnf
input    := scalar | pair | triplet | arrayop | labeled
scalar   := number [unit]
pair     := scalar sep scalar
triplet  := scalar sep scalar sep scalar
sep      := "," | "x"
arrayop  := ('*' | '/') integer
labeled  := ('r=' scalar | 'd=' scalar | 'a=' scalar | 's=' integer)
number   := integer | float
float    := integer "." integer
```

- Example bindings:
  - Rectangle: pair -> width, height.
  - Rotated rectangle: triplet -> width, height, angle.
  - Circle: labeled `r=` or `d=`; fallback scalar as radius.
  - Move/Rotate: scalar -> distance or angle; array operations permitted after commit (e.g., `*6` duplicates array).
- HUD commits input when `Enter` pressed; invalid input flashes red border for 300 ms.

### 4.4 Lock Badges & Status Echo

- When axis locked, status bar shows `[X]`, `[Y]`, `[Z]` badges.
- Snap locks show short text `MID`, `CEN`, `END`, `PAR`, `PERP`, `TAN`.
- Measurement echo in status bar updates after commit or after valid numeric entry.

---

## 5. Camera, Viewport & Rendering Pipeline

### 5.1 Camera Controller

- Projection modes: Perspective (default FOV 50 deg, configurable 25-100 deg) and Parallel (sets ortho height based on viewport).
- Orbit:
  - Input: MMB drag (SketchUp scheme) or RMB drag (DSM scheme).
  - Rotation speed 0.4 deg per pixel; acceleration capped at 3x.
  - Orbit pivot: last selection anchor or world origin; double-clicking entity sets pivot.
- Pan:
  - MMB + Shift (SketchUp) or MMB (DSM).
  - Pan speed scales with distance to pivot (screen-space).
- Zoom:
  - Mouse wheel zoom-to-cursor; default step 1.2x; accelerated when Shift held.
  - Zoom tool (left toolbar) allows drag vertical; double-click zoom tool to fit selection or scene.
- Camera presets (Front, Back, etc.) animate over 240 ms using ease-in-out; pivot maintained.
- `View > Reset Camera & Style` resets camera, projection, view style, overlays, but preserves selection.

### 5.2 Rendering Pipeline

- Pass 1 (Geometry): draws faces with front/back materials, edges with profile weight, hidden edges optional dashed.
- Pass 2 (Overlay): selection outlines, inference glyphs, guides, section planes, measurement HUD, axis indicator.
- Anti-aliasing: MSAA 4x by default; fallback FXAA when MSAA unsupported.
- Depth bias: polygon offset 1e-3 to prevent z-fighting between faces and edges.
- Section cuts: single plane clipping per active section; fill color applied in overlay with 65 percent opacity.
- Renderer modular in `src/graphics/`; view-specific config from `ViewSettings`.

### 5.3 Picking

- Ray casting uses BVH updated incrementally; rebuild triggered when >5 percent nodes changed.
- Hit tolerance 3 px for edges, 5 px for faces; adjustable via preferences.
- Hover hysteresis: candidate must persist 120 ms before tooltip/inference updates to reduce flicker.
- Selection buffer (GPU) fallback for integrated GPUs documented in `graphics/PickerFallback.*`.

---

## 6. Tool Specifications (Exhaustive)

Each tool implements `Tool` interface with methods `activate`, `deactivate`, `onMouseDown`, `onMouseMove`, `onMouseUp`, `onKeyPress`, `applyMeasurementOverride`, and `cancel`. Tools expose metada[...] 

For brevity, shared attributes:
- **Common modifiers:** `Shift` sticky inference, `Ctrl/Cmd` clone or toggle secondary mode, `Alt` alternate snapping where defined.
- **Undo behavior:** Multi-step interactions result in single command per commit unless array operations triggered.
- **Telemetry:** On activation `tool.activate`, on commit `tool.commit`, on cancel `tool.cancel`, with payload containing duration, number of steps, success flag.

Below summary lists tool specifics (IDs must match `Tool::getName()` implementations).

### 6.1 Select (`tools.select`)

- Cursor: arrow; measurement disabled.
- Context options: filters (Faces, Edges, Components), Ignore Backfaces toggle, Through Selection toggle.
- Hotkeys: `S`, `Space`.
- Edge cases: respects visibility/lock; double-click enters edit; `Esc` twice returns to root context.

... (TRUNCATED FOR BREVITY IN THIS QUERY) ...

---

Scale-based Tolerances (industry standard presets)

* Presets for common modeling scales (provided as recommended defaults; all values are configurable in Preferences):
  * High precision (mm-scale preset):
    * Merge tolerance: 1e-6 m (0.001 mm)
    * Coplanarity tolerance: 1e-5 m (0.01 mm)
  * Medium precision (cm-scale / general modeling preset):
    * Merge tolerance: 1e-5 m (0.01 mm)
    * Coplanarity tolerance: 1e-4 m (0.1 mm)
  * Low precision (m-scale / architectural preset):
    * Merge tolerance: 1e-4 m (0.1 mm)
    * Coplanarity tolerance: 1e-3 m (1 mm)

* These presets align with common, published modeling-tool tolerances used for different model scales and aim to reflect industry standard behavior while remaining configurable per-document and per-project.

Note: These appended directives are subject to further refinement to reflect industry standards.

---