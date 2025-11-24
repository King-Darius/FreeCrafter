# Codex Master UI & Tools Spec (Repo-Aligned, Implementation Grade) - v2.0

Prepared: 10 Oct 2025

> Purpose. Convert the FreeCrafter desktop concept and master tool specification into a stack-agnostic contract covering visuals, behaviors, and data flow. Engineering, design, QA, and Codex agents must treat this document as the authoritative reference. All dimensions, time constants, input grammar, and identifiers herein are binding across native, web, and WASM implementations.

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
  - **View:** Style (Wireframe, Hidden line, Shaded, Shaded + Edges), Projection (Perspective, Parallel), Camera presets (Front, Back, Left, Right, Top, Bottom, Iso), Grid (Show, Size), Guides, Section Cuts, Shadows, Axes, Face style, Edge style.
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

Each tool implements `Tool` interface with methods `activate`, `deactivate`, `onMouseDown`, `onMouseMove`, `onMouseUp`, `onKeyPress`, `applyMeasurementOverride`, and `cancel`. Tools expose metadata: `id`, `displayName`, `iconPath`, `cursorId`, `defaultHotkeys`, `contextOptions`.

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

### 6.2 Eraser (`tools.eraser`)

- Cursor: eraser icon; measurement disabled.
- Modifiers: `Shift` smooth/soften edge; `Ctrl` hide rather than delete; `Alt` swap soften/hide roles.
- Commit: `DeleteEdgesCommand` aggregated per stroke; smoothing recorded as `EdgePropertyCommand`.

### 6.3 Line (`tools.line`)

- Steps: start point -> second point; continues chaining until `Esc`.
- Context toolbar: Continue on Enter (bool), Length Lock (float for next segment), Auto-heal faces toggle.
- Measurement: scalar length.
- Edge cases: closing loop triggers face creation; 0 length ignored with error toast.

### 6.4 Rectangle (`tools.rectangle`)

- Steps: corner1 -> corner2.
- Context: Plane lock (auto/from face), Inflate from center toggle.
- Measurement: pair width,height; auto-swap if user typed height first (respects order typed).
- Commit: adds edges + single face; auto-select face.

### 6.5 Rotated Rectangle (`tools.rotated_rectangle`)

- Steps: base point -> direction -> second corner.
- Measurement: triplet width,height,angle.
- Context: Snap angle step (default 15 deg).

### 6.6 Circle (`tools.circle`)

- Steps: center -> radius.
- Context: Sides (int spinner 3-96, default 24), Plane lock options.
- Measurement: labeled `r=` or `d=`; fallback scalar as radius.

### 6.7 Polygon (`tools.polygon`)

- Similar to circle but `sides` required; measurement accepts `s=` `r=` `d=`.

### 6.8 Arc (`tools.arc`)

- Steps: start -> end -> bulge.
- Context: Sides (for tessellation), Keep edges on face toggle.
- Measurement: scalar radius or angle; labeled `r=` or `a=`.

### 6.9 Center Arc (`tools.center_arc`)

- Steps: center -> start -> end; measurement `a=` or `r=`.

### 6.10 Tangent Arc (`tools.tangent_arc`)

- Steps: select tangent edge -> end point; measurement same as arc; preview indicates tangent edge highlight.

### 6.11 Bezier (`tools.bezier`)

- Steps: place anchor points; handles appear on drag.
- Context: Mode (Smooth, Corner), Handle lock to axis toggle, Simplify on commit tolerance slider.
- Commit: generates polyline with metadata for editing.

### 6.12 Freehand (`tools.freehand`)

- Continuous sampling at 120 Hz; simplifies with Douglas-Peucker tolerance 0.25 mm default.
- Context: Tolerance slider 0.05-5 mm.

### 6.13 Move (`tools.move`)

- Steps: pick grip -> pick target (drag or type).
- Modifiers: `Ctrl` duplicate; `Alt` toggle inference suppress.
- Measurement: scalar distance; after commit user can type `*n` or `/n` to create arrays (replay last move).
- Context: Copy toggle, Auto-fold geometry toggle.

### 6.14 Rotate (`tools.rotate`)

- Steps: plane resolve -> center -> reference arm -> final arm.
- Measurement: scalar angle; arrays via `*n`.
- Context: Angle snap step, Copy toggle.

### 6.15 Scale (`tools.scale`)

- Modes: uniform, axis, plane (selected by handles or hotkeys).
- Measurement: scalar factor or triplet `sx,sy,sz`.
- Context: Proportional scaling toggle, Uniform bias toggle.

### 6.16 Offset (`tools.offset`)

- Steps: select face -> drag offset.
- Measurement: scalar distance.
- Context: Keep holes toggle, Cap ends toggle, Self-intersection resolution drop-down (Trim, Extend).

### 6.17 Push/Pull (`tools.pushpull`)

- Steps: select planar face -> drag along normal.
- Modifiers: `Ctrl` create new volume, double-click repeats last distance.
- Measurement: scalar distance; negative allowed within tolerance.

### 6.18 Extrude (`tools.extrude`)

- Modes: Linear, Along Path.
- Steps for path mode: select profile -> select path -> set options -> commit.
- Context: Cap ends, Merge faces, Twist mode (None, Frenet, Parallel transport).

### 6.19 Follow Me (`tools.followme`)

- Similar to Extrude path; includes context options for miters, keeps normals, end caps.

### 6.20 Paint Bucket (`tools.paint`)

- Steps: hover preview -> click apply.
- Modifiers: `Alt` eyedropper, `Shift` sample all connected faces.
- Context: Apply to group instances toggle, Replace materials mode.

### 6.21 Text (`tools.text`)

- Modes: Screen label (billboard) or Face label (planar).
- Context: Font family dropdown (system fonts), Height value (mm), Leader toggle.
- Measurement: scalar height when editing.

### 6.22 Dimension (`tools.dimension`)

- Detects type based on picked references.
- Context: Precision combo (1-4 decimal), Unit display (model, force mm, inch), Suppress trailing zeros toggle.

### 6.23 Tape Measure (`tools.tape`)

- Steps: point1 -> point2.
- Context: Create guide (default on), Guide layer drop-down.
- Measurement: scalar length; pressing `Enter` after measurement re-arms for another measurement.

### 6.24 Axes (`tools.axes`)

- Steps: origin -> red direction -> green direction.
- Context: Reset to world button, Align to face toggle.

### 6.25 Section Plane (`tools.section`)

- Steps: place plane -> orientation -> depth adjustment.
- Context: Fill color picker, Display cut edges toggle, Activate section toggle.

### 6.26 Orbit/Pan/Zoom (`tools.orbit`, `tools.pan`, `tools.zoom`)

- Provide explicit tools plus temporary variants via navigation schemes.
- Orbit tool measurement disabled; context includes Invert drag axis toggle.
- Pan tool includes Snap to axis toggle (locks to horizontal/vertical screen axes).
- Zoom tool includes Speed slider (0.5-3.0) and Fit selection button.

---

## 7. Navigation Schemes & Temporary Tools

- Navigation configuration stored in `resources/config/navigation_default.json`.
- Two default schemes:
  - **SketchUp:** Orbit on MMB, Pan on Shift+MMB, Zoom via wheel, Context menu on RMB.
  - **DSM:** Orbit on RMB, Pan on MMB, Zoom via wheel.
- Temporary tools stack: `ToolManager` pushes nav tool on button-down, captures input, pops on button-up to restore previous tool and measurement.
- Keyboard toggles:
  - `O` toggles Orbit tool.
  - `H` toggles Pan.
  - `Z` toggles Zoom.
- Preferences allow adjusting orbit sensitivity, pan speed, zoom acceleration, scroll inversion, and whether zoom-to-cursor is enabled by default (on).

---

## 8. File I/O, Scene State & Persistence

- File format: `.fcscene` JSON + binary mesh data. Stores nodes, materials, tags, scene settings, camera presets.
- Import:
  - OBJ/STL create components named `Imported_<timestamp>`.
  - Unit detection attempts to parse metadata; fallback prompt with default mm.
  - Imports run on worker thread; progress dialog shows percent.
- Export:
  - OBJ available now; GLTF planned. Options include triangulate, embed textures, axis orientation.
  - Exports respect visibility filters unless "Export hidden" checked.
- Scene persistence:
  - Remembers last camera, projection mode, view style, grid visibility, active section, tray layout, theme, recent files.
  - Settings saved via `QSettings` (native) or local storage (web).
- Autosave:
  - Interval default 5 minutes; stores under `autosave/` directory; retains last 5 revisions.
- Recovery:
  - On crash detection, prompts to restore autosave on next launch.

---

## 9. Undo/Redo Command Architecture

- Commands derive from `Command` interface with `apply`, `undo`, `mergeWith`, `id`.
- `CommandStack` allows merge of sequential compatible commands (e.g., push/pull repeats).
- Selection change undo optional (preference); default off.
- Array operations (Move, Rotate, Scale) create derived `ArrayCommand` bundling duplicates.
- Panels create `PropertyEditCommand`; sequential edits to same property within 400 ms merge.
- Tool commits push commands with metadata for telemetry (duration, tool id, data size).
- Undo/redo triggers UI refresh via `UndoStack::indexChanged`.

---

## 10. Performance, Robustness & Tolerances

- Geometry merge tolerance: 1e-6 m.
- Coplanarity tolerance: 1e-5.
- Angle snap default: 15 deg; user adjustable 1-45 deg.
- Viewport target: 60 FPS with 1 million edges on mid-tier GPU; overlays budget <= 2 ms.
- Picking update budget <= 5 ms per frame.
- Tool preview limited to 120 segments for circles/arcs (sides parameter).
- Freehand simplification tolerance default 0.25 mm (adjustable).
- Fallback pipeline logs when GPU features missing; ensures same visuals albeit slower.
- Crash resilience: operations wrapped with `CommandGuard`; errors produce inline toast (max 80 chars) and keep tool active.

---

## 11. Accessibility, Internationalization & Telemetry

- Keyboard navigation: all tool buttons focusable via Tab; arrow keys move within toolbar groups.
- Tooltips, status hints, and measurement HUD mirrored to screen reader via accessible interfaces.
- Contrast ratios:
  - Text vs background >= 4.5:1.
  - Focus ring 2 px, highlight color accessible in both light/dark themes.
- Localization:
  - Strings stored in `resources/strings/*.json`; loaded via `StringBundle`.
  - Decimal comma supported in measurement parser (accepts `;` as alternative separator for locales where `,` is decimal).
- Telemetry events:
  - `app.start`, `app.exit`, `tool.activate`, `tool.commit`, `tool.cancel`, `panel.edit`, `viewport.renderStats`.
  - Telemetry can be disabled; QA uses structured logs for parity tests.

---

## 12. Assets, Iconography & Theming

- Icon sizes:
  - Toolbar icons: 20 px vector.
  - Menu icons: 16 px.
  - Cursor badges: 16 px with 2 px outline.
- Asset locations:
  - Monochrome icons under `resources/icons/`.
  - Colored variants under `resources/icons_colored/` with identical filenames for theme swap.
- Cursors defined in `styles/cursors/`; loader maps tool IDs to cursor pixmaps via `GLViewport::cursorForToolName`.
- Theming:
  - Light and Dark QSS stored in `styles/light.qss`, `styles/dark.qss`.
  - `View > Toggle Dark Mode` action updates QSettings key `ui/darkMode`.
  - Theme switch updates palettes, dock backgrounds, tooltip colors, icon variants if available.
- Typography:
  - Primary UI font: system default (SegUI on Windows, SF Pro on macOS, Noto Sans on Linux).
  - Code/measurement font: monospaced (Consolas, SF Mono, Fira Mono fallback).

---

## 13. Roadmap Tools (Pre-spec)

Advanced tools reside in `src/advanced/` with stub implementations returning "Coming soon" toast until complete. Specifications:

1. **Round Corner** - radius-based filleting with miter options. Requires edge chaining and topology healing.
2. **CurveIt** - convert polylines to splines with continuity controls.
3. **PushAndPull (Thicken)** - shell creation with cap options and collision avoidance.
4. **Surface** - loft, skin, patch tools with rail weights.
5. **BezierKnife** - project bezier onto surfaces and cut topology.
6. **QuadTools** - convert triangulated mesh to quads; includes edge stitch and flip.
7. **SubD** - Catmull-Clark subdivision preview with crease controls.
8. **Weld** - merge collinear edges; specify angle tolerance.
9. **Vertex Tools** - soft selection with falloff, magnet modes, symmetry.
10. **Clean** - remove degenerate geometry, unify normals, report summary.
11. **ClothEngine** - cloth simulation with pins, wind, collisions, bake to mesh.

Each roadmap tool must follow the same state machine, context toolbar, telemetry, and command architecture.

---

## 14. QA Acceptance Matrix & Dev Checklist

### 14.1 QA Matrix (excerpt)

- **Select:** Inside vs Crossing marquee, hidden/locked respect, group entry via double-click, Esc exit sequence.
- **Line:** Axis lock precedence, typed length accuracy, face splitting on intersect.
- **Rectangle/Rotated Rectangle:** Numeric grammar acceptance, on-face projection accuracy, zero-area rejection with warning.
- **Circle/Polygon:** Sides parameter persistence, diameter vs radius input.
- **Move/Rotate/Scale:** Arrays `*n` `/n`, weld correctness, undo merges into single step.
- **Offset:** Keep holes toggle, self-intersection handling, cap options.
- **Push/Pull:** Repeat via double-click, create new vs merge, reverse push preference.
- **Paint:** Component vs face priority, eyedropper accuracy.
- **Section Plane:** Clip fidelity, fill color, persistence after reload.
- **Navigation:** Temporary tool restore, zoom-to-cursor, pivot double-click sets camera focus.

### 14.2 Dev Checklist

1. Confirm tool IDs match `Tool::getName()` and UI actions reference same string.
2. Wire context toolbar controls via shared `ToolOptionBinding`.
3. Ensure measurement grammar handles units, arrays, labeled parameters per tool.
4. Validate theme switch updates all widgets and persists via QSettings.
5. Run `cmake --build build -j` and `ctest --test-dir build --output-on-failure`.
6. Capture before/after screenshots for any UI change.
7. Log telemetry events in debug builds for QA spot checks.
8. Update documentation (README, AGENTS, relevant docs) when behavior changes.
9. Add regression scene or test asset for new geometry features.
10. Verify accessibility (keyboard, screen reader hints) after UI adjustments.

---

## 15. Codex Mapping (IDs, Events, Modules)

### 15.1 Tool ID Map

| Tool | ID string | Module |
|------|-----------|--------|
| Select | `tools.select` | `src/ops/tools/SmartSelectTool.*` |
| Eraser | `tools.eraser` | `src/ops/tools/EraserTool.*` |
| Line | `tools.line` | `src/ops/tools/LineTool.*` |
| Rectangle | `tools.rectangle` | `src/ops/tools/RectangleTool.*` |
| Rotated Rectangle | `tools.rotated_rectangle` | `src/ops/tools/RotatedRectangleTool.*` |
| Circle | `tools.circle` | `src/ops/tools/CircleTool.*` |
| Polygon | `tools.polygon` | `src/ops/tools/PolygonTool.*` |
| Arc | `tools.arc` | `src/ops/tools/ArcTool.*` |
| Center Arc | `tools.center_arc` | `src/ops/tools/CenterArcTool.*` |
| Tangent Arc | `tools.tangent_arc` | `src/ops/tools/TangentArcTool.*` |
| Bezier | `tools.bezier` | `src/ops/tools/BezierTool.*` |
| Freehand | `tools.freehand` | `src/ops/tools/FreehandTool.*` |
| Move | `tools.move` | `src/ops/tools/MoveTool.*` |
| Rotate | `tools.rotate` | `src/ops/tools/RotateTool.*` |
| Scale | `tools.scale` | `src/ops/tools/ScaleTool.*` |
| Offset | `tools.offset` | `src/ops/tools/OffsetTool.*` |
| Push/Pull | `tools.pushpull` | `src/ops/tools/PushPullTool.*` |
| Extrude | `tools.extrude` | `src/ops/tools/ExtrudeTool.*` |
| Follow Me | `tools.followme` | `src/ops/tools/FollowMeTool.*` |
| Paint Bucket | `tools.paint` | `src/ops/tools/PaintTool.*` |
| Text | `tools.text` | `src/ops/tools/TextTool.*` |
| Dimension | `tools.dimension` | `src/ops/tools/DimensionTool.*` |
| Tape Measure | `tools.tape` | `src/ops/tools/TapeTool.*` |
| Axes | `tools.axes` | `src/ops/tools/AxesTool.*` |
| Section Plane | `tools.section` | `src/ops/tools/SectionTool.*` |
| Orbit | `tools.orbit` | `src/ops/tools/OrbitTool.*` |
| Pan | `tools.pan` | `src/ops/tools/PanTool.*` |
| Zoom | `tools.zoom` | `src/ops/tools/ZoomTool.*` |

### 15.2 Event Topics

- `tool.activate` (payload: toolId, timestamp, hotkeyUsed)
- `tool.commit` (payload: toolId, durationMs, selectionCount, measurementInput)
- `tool.cancel` (payload: toolId, reason, durationMs)
- `panel.edit` (payload: panelId, field, oldValue, newValue)
- `viewport.renderStats` (payload: fps, drawCalls, triangles, msCPU, msGPU)
- `document.changed` (payload: changeType, nodeId, commandId)

### 15.3 Module Ownership

- `src/app/` - Application entry, main window, theme handling.
- `src/ui/` - Widgets, toolbars, panels, HUD.
- `src/ops/` - Tools, commands, undo stack.
- `src/graphics/` - Renderer, viewport, camera.
- `src/core/` - Geometry kernels, math utilities.
- `src/io/` - Import/export, serializers.
- `src/advanced/` - Roadmap tools stubs.
- `tests/` - Unit and integration tests; all new features require coverage here when feasible.

### 15.4 Codex Agent Guidelines

- Always read `AGENTS.md` and this spec before modifying UI or tool behavior.
- Reference tool IDs directly from `Tool::getName()`; avoid hardcoded duplicates.
- When adding new assets, register in `resources.qrc` and update this mapping table.
- Update telemetry event schema if payload changes; coordinate with QA dashboards.

---

End of document.

---

**🔧 UI/UX MICRO-BEHAVIORS — EXTENDED INSTRUCTION SET (AI-APPENDABLE ONLY)**

**Directive:** The following behaviors are to be **added to** the existing UI/UX specification files, **not overwritten** or altered. Any AI or contributor integrating these must ensure that no existing directives are deleted or diminished. Append only. Extend existing inference and tool behavior logic to include these micro-operations.

---

**1. Snapping Behavior Enhancements**

* Enable snapping to:

  * Circle, arc, ellipse, spline centers (shown as ring glyphs).
  * Endpoints and midpoints of all edges, curves, and arc segments.
  * Quadrant points on arcs/circles (0°, 90°, 180°, 270° from center).
  * Intersections—including real, virtual (implied), and projected.
  * Perpendicular and tangent positions from curves.
* Virtual intersections (projections across infinite extensions) must be inferred and offered as active snap glyphs.
* Tangent/perpendicular modes must be toggleable (e.g. with Alt/Shift).
* Snap-to behaviors must respect inference priority order: endpoints > intersections > midpoints > projections > axis/angle locks > free cursor.

---

**2. Visual Inference Guides**

* Show dynamic dotted guide lines between arc endpoints, potential closure points, axis-aligned and angle-aligned directions.
* When a loop is nearly closed, display a dashed preview of the face to be formed.
* When axis/angle lock is engaged, show faint guide lines labeled (e.g., “X”, “Y”) anchored from cursor.
* Add symmetry detection: if shapes reflect about an axis, show a faint mirror guide to assist mirroring.

---

**3. Manual Input Behavior**

* Always accept typed input (lengths, radius, angles, dimensions) during drawing operations without requiring field selection.
* Maintain input grammar:

  * For lines: `length`, or `length,angle`.
  * For rectangles: `width,height`, `w,h`, or `WxH`.
  * For circles/arcs: `r=<value>` or `d=<value>`.
  * For rotated shapes: `w,h,angle`.
  * For polygons: `sides,radius`.
* Typing overrides live cursor input until Enter or comma confirms.
* All units must be parsed (`mm`, `cm`, `m`, `in`, `ft`, `”`, etc.).
* Array modifiers (e.g. `*5`, `/2`) remain available for repeated actions.

---

**4. Face Creation & Closure**

* Detect closed loops in real-time as new segments are added.
* Auto-fill a planar face if a closed edge loop forms within merge/coplanar tolerances:

  * Merge tolerance: ~1e-6 m
  * Coplanarity tolerance: ~1e-5 m
* Face creation occurs when:

  * A final point snaps to the loop origin.
  * A rectangle or polygon is completed.
* Show visual face preview when closure is imminent.

---

**5. Edge Extension Behavior**

* When a new edge endpoint lands:

  * Directly on a vertex → merge.
  * On an existing edge → split.
  * Coincident with another edge → weld or collapse.
  * At intersection → create new vertex and join.
* Edge and face updates must occur live and respect clean manifold topology rules.
* Hover, pre-select, and select states must visually update:

  * Hover → soft glow/outline
  * Pre-select → highlight + glyph
  * Select → locked highlight
* Dashed previews are used for in-progress drawing.

---

**6. Axis Lock & Inference Priority**

* Inference priority must continue as:
  `snap > axis lock > angle lock > visual guide > free draw`
* Modifier keys:

  * Shift → axis lock
  * Arrow keys / X/Y/Z → axis constraint
  * Alt → tangent/perpendicular toggle

---





---





**🔧 UI/UX MICRO-BEHAVIORS — EXTENDED INSTRUCTION SET (AI-APPENDABLE ONLY)**





**Directive:** The following behaviors are to be **added to** the existing UI/UX specification files, **not overwritten** or altered. Any AI or contributor integrating these must ensure that no existing directives are deleted or diminished. Append only. Extend existing inference and tool behavior logic to include these micro-operations.





---





**1. Snapping Behavior Enhancements**

* Enable snapping to:


  * Circle, arc, ellipse, spline centers (shown as ring glyphs).


  * Endpoints and midpoints of all edges, curves, and arc segments.


  * Quadrant points on arcs/circles (0°, 90°, 180°, 270° from center).


  * Intersections—including real, virtual (implied), and projected.


  * Perpendicular and tangent positions from curves.


* Virtual intersections (projections across infinite extensions) must be inferred and offered as active snap glyphs.


* Tangent/perpendicular modes must be toggleable (e.g. with Alt/Shift).


* Snap-to behaviors must respect inference priority order: endpoints > intersections > midpoints > projections > axis/angle locks > free cursor.





---





**2. Visual Inference Guides**





* Show dynamic dotted guide lines between arc endpoints, potential closure points, axis-aligned and angle-aligned directions.


* When a loop is nearly closed, display a dashed preview of the face to be formed.


* When axis/angle lock is engaged, show faint guide lines labeled (e.g., “X”, “Y”) anchored from cursor.


* Add symmetry detection: if shapes reflect about an axis, show a faint mirror guide to assist mirroring.





---





**3. Manual Input Behavior**





* Always accept typed input (lengths, radius, angles, dimensions) during drawing operations without requiring field selection.


* Maintain input grammar:





  * For lines: `length`, or `length,angle`.


  * For rectangles: `width,height`, `w,h`, or `WxH`.


  * For circles/arcs: `r=<value>` or `d=<value>`.


  * For rotated shapes: `w,h,angle`.


  * For polygons: `sides,radius`.


* Typing overrides live cursor input until Enter or comma confirms.


* All units must be parsed (`mm`, `cm`, `m`, `in`, `ft`, `”`, etc.).


* Array modifiers (e.g. `*5`, `/2`) remain available for repeated actions.





---





**4. Face Creation & Closure**





* Detect closed loops in real-time as new segments are added.


* Auto-fill a planar face if a closed edge loop forms within merge/coplanar tolerances:





  * Merge tolerance: ~1e-6 m


  * Coplanarity tolerance: ~1e-5 m


* Face creation occurs when:





  * A final point snaps to the loop origin.


  * A rectangle or polygon is completed.


* Show visual face preview when closure is imminent.





---





**5. Edge Extension Behavior**





* When a new edge endpoint lands:





  * Directly on a vertex → merge.


  * On an existing edge → split.


  * Coincident with another edge → weld or collapse.


  * At intersection → create new vertex and join.


* Edge and face updates must occur live and respect clean manifold topology rules.


* Hover, pre-select, and select states must visually update:





  * Hover → soft glow/outline


  * Pre-select → highlight + glyph


  * Select → locked highlight


* Dashed previews are used for in-progress drawing.





---





**6. Axis Lock & Inference Priority**





* Inference priority must continue as:


  `snap > axis lock > angle lock > visual guide > free draw`


* Modifier keys:





  * Shift → axis lock


  * Arrow keys / X/Y/Z → axis constraint


  * Alt → tangent/perpendicular toggle



---

**⚠️ AI/CODE AGENT INTEGRATION NOTE:**


This block is to be appended verbatim to existing UI/UX specification files. Do not remove or modify previous rules. Integrate extended behaviors in a modular way, ideally via subclassed or modular override hooks to preserve forward compatibility with previous architectural decisions.

**⚠️ AI/CODE AGENT INTEGRATION NOTE:**
This block is to be appended verbatim to existing UI/UX specification files. Do not remove or modify previous rules. Integrate extended behaviors in a modular way, ideally via subclassed or modular override hooks to preserve forward compatibility with previous architectural decisions.

---
