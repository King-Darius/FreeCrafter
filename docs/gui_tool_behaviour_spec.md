# FreeCrafter - GUI & Tool Behaviour Specifications

*(Ultra-Detailed, Repo-Aligned V2 - implementation-grade)*

**Purpose.** Single source of truth for FreeCrafter's interface, tools, input grammar, snapping, camera, rendering, panels, assets, and code wiring. Aligned to the Figma concept (React app shell) and the current repository structure. Use this to drive development, QA, docs, and parity between web demo and native builds.

**Scope.** Ideal interface; all baseline tools; roadmap/advanced tools (pre-spec); selection/snap/inference; measurement grammar; camera and viewport; rendering; persistence; performance and tolerances; accessibility/internationalization; theming/assets; QA matrices; code cross-references.

---

## 0. Conventions & Glossary

- **Axes and colors:** X = Red, Y = Green, Z = Blue.
- **Screen units:** px. **Model units:** mm (default). Parser accepts `mm|cm|m|in|ft` plus `"` and `'`.
- **Inference glyphs:** Endpoint (filled circle), Midpoint (diamond), Center (ring), Intersection (plus), On-Edge (bar), On-Face (checker), Parallel (double bar), Perpendicular (right angle), Tangent (tangent arc).
- **Selection marquee:** L->R = Inside; R->L = Crossing.
- **Default tolerances:** Merge 1e-6 m; Coplanarity 1e-5; Angle snap 15 deg (preferences adjustable).
- **File paths reference:** `src/*`, `resources/*`.

---

## 1. Interaction Model & Event System

### 1.1 Tool State Machine (all tools implement)

**States:** `Idle -> Armed -> [Step1..N with Preview] -> (Commit | Cancel)`

- **Cancel:** `Esc` cancels current step; second `Esc` aborts tool (falls back to Select).
- **Commit:** `Enter` or final click; typed measurement also finalizes (see section 4.4).
- **Context menu:** `RMB` shows options; never commits.

### 1.2 Event Routing

- **Pointer:** OS -> `GLViewport` -> `ToolManager` -> `ActiveTool.onMouse*` (unless a temporary navigation tool is active).
- **Keyboard:** OS -> `ToolManager` -> (`ActiveTool.onKey*` for measurement) -> `HotkeyManager` (unhandled).
- **Temporary tools:** Pushed on button-down (MMB/RMB depending on scheme) and popped on button-up; prior tool restored (see section 7).

### 1.3 Focus & Measurement

- Measurement box is ghost-focused: typing routes to `ActiveTool.applyMeasurementOverride()` without clicking the field. Switching tools flushes pending numeric input.

---

## 2. Ideal Application Shell (Figma Concept Parity)

### 2.1 Menus

- **File:** New, Open, Save, Save As, Import (OBJ/STL), Export (OBJ/GLTF), Revert, Recent, Exit.
- **Edit:** Undo, Redo, Cut, Copy, Paste, Delete, Select All, Invert Selection, Preferences.
- **View:** Styles (Wireframe/Hidden line/Shaded/Shaded+Edges), Projection (Perspective/Parallel), Presets (Front/Back/Left/Right/Top/Bottom/Iso), Grid (Show/Size/Div), Guides, Section Cuts, Shadows, Axes, Face/Edge styles.
- **Tools:** Groups: Selection, Drawing, Modification, Annotation, Section, Navigation.
- **Window:** Panels toggle plus tray presets.
- **Help:** Docs, Shortcuts, About.

### 2.2 Toolbars

- **Primary bar:** New/Open/Save; Undo/Redo; View Style; Projection; Shadows/Sun; Presets; Theme.
- **Context bar (tool-aware):** Appears when a tool is active; hosts options (for example, Polygon sides, Rotate snap step, Offset keep-holes).
- **Left tool strip (exclusive groups):**
  - **Selection and Erase**
  - **Drawing:** Line, Rectangle, RotatedRect, Circle, Polygon, Arc (3-pt), Center Arc, Tangent Arc, Bezier, Freehand
  - **Modify:** Move, Rotate, Scale, Offset, Push/Pull, Extrude, Follow Me
  - **Annotate:** Paint Bucket, Text, Dimension, Tape Measure, Axes
  - **Section:** Section Plane
  - **Navigate:** Orbit, Pan, Zoom (also as temporary tools via mouse)

### 2.3 Right Tray (Dock Stack)

- Pinned by default; unpinned auto-hide; resizable 260-560 px; accordion sections; double-click header collapses/expands when pinned.
- **Keyboard:** `Ctrl/Cmd+Shift+P` pin/unpin; `Ctrl/Cmd+]` collapse/expand.
- Panels: Outliner, Entity Info, Materials, Tags/Layers, Environment, Shortcuts and Navigation Preferences.

### 2.4 Status Bar (Footer)

- Left: Active tool and step hint.
- Center: Inference/lock badges.
- Right: Selection count, cursor world coordinates, measurement echo.

---

## 3. Panels & Data Flow

### 3.1 Entity Info (Inspector)

- Fields: Name, Tag/Layer, Hidden/Locked, Smoothing/Soft edges, Materials (Front/Back), Component definition.
- Transform: Position (X,Y,Z), Rotation (H,P,B), Scale (Sx,Sy,Sz). Numeric edits emit `TransformCommand` with pivot equal to selection center or last pick. Multi-select shows "--" for mixed values; typing applies to all.
- **Files:** `src/ui/InspectorPanel.*`

### 3.2 Outliner

- Tree of Groups/Components/Sections. Drag to reparent (preserve world transform). Eye/Lock toggles; Alt-click isolate. Context actions: Make Group/Component, Explode, Rename.
- **Files:** `src/ui/OutlinerPanel.*`
- Edits (rename, visibility) must emit `Scene::RenameObjectCommand`/`Scene::SetObjectVisibilityCommand` through `Core::CommandStack` so undo/redo stays consistent.

### 3.3 Materials

- Swatch grid (search/import texture). Eyedropper routed from Paint tool (Alt). Apply by click/drag; respects component-vs-face override rules.
- **Files:** `src/ui/MaterialsPanel.*`
- Applying a swatch triggers `Scene::AssignMaterialCommand` so material overrides participate in the undo stack.

### 3.4 Tags/Layers

- Create/rename/delete; color chip; visibility and lock. Assign via Inspector or context bar selector.
- **Files:** `src/ui/TagsPanel.*`
- Tag creation/rename/color/visibility flow through `Scene::CreateTagCommand` et al. to keep history and shared state in sync.

### 3.5 Environment & View Settings

- Sun (date/time/azimuth/altitude), sky model, background gradient, grid (toggle/size/divisions), section fill color.
- **Files:** `src/ui/EnvironmentPanel.*`, `src/ui/ViewSettingsDialog.*`

### 3.6 Shortcuts & Navigation Prefs

- Editors over `resources/config/hotkeys_default.json` and `resources/config/navigation_default.json`. Import/Export/Reset.
- **Files:** `src/ui/HotkeyDialog.*`, `src/ui/NavigationPreferences.*`

**Data flow:** Panels mutate `Scene::Document` -> signal `GLViewport` -> redraw. All panel edits are routed through `CommandStack` for undo.

---

## 4. Selection, Snapping/Inference, Measurement HUD

### 4.1 Selection Model

- **Click:** pick nearest hit (depth-sorted raycast).
- **Double-click:** enter group/component (or select face plus edges on raw mesh).
- **Triple-click:** connected surface.
- **Marquee:** L->R = inside; R->L = crossing.
- **Modifiers:** Shift = Add; Ctrl/Cmd = Toggle; Alt = Subtract.
- **Esc chain:** first Esc clears marquee; second Esc clears selection or exits edit context.

### 4.2 Inference & Snap Ranking

1. Explicit glyph snaps (Endpoint > Midpoint > Center > Intersection)
2. On-Edge and On-Face projections
3. Axis locks (parallel to X/Y/Z) plus view orthogonals
4. Derived constraints (perpendicular, parallel, tangent) when certain
5. Free cursor

- **Sticky axis/locks:** Tap `X/Y/Z` (or Arrow keys) to lock; hold `Shift` to make current inference sticky.
- **Search radius:** 8 px (grow to 12 px if no hit). Hover hysteresis reduces flicker.

### 4.3 Guides

- Tape Measure can place guide edges/points; visibility toggled in View menu. Guides are non-exporting.

### 4.4 Measurement HUD & Grammar (EBNF)

```ebnf
input    := number[unit] | pair | triplet | arrayop | labeled
pair     := number[unit] sep number[unit]
triplet  := number[unit] sep number[unit] sep number[unit]
sep      := "," | "x"
arrayop  := ('*'|'/') integer
labeled  := ('r=' num[unit] | 'd=' num[unit] | 'a=' num[unit] | 's=' integer)
unit     := mm|cm|m|in|ft|"|'
```

- Interpretation is tool-step specific (for example, Rectangle consumes `pair`, Rotate consumes angle). Bare numbers use model unit; mixed units allowed (for example, `3', 6"`).

---

## 5. Camera, Viewport, Rendering

### 5.1 Camera Controller

- **Projection:** Perspective (FOV 25-100 deg) or Parallel (ortho height).
- **Presets:** Front/Back/Left/Right/Top/Bottom/Iso with 200-300 ms easing and pivot preservation.
- **Orbit:** Trackball about pivot (last valid pick; else model origin).
- **Pan:** Screen-space; scales with distance.
- **Zoom:** Dolly (perspective) or scale ortho height; optional zoom-to-cursor recenters pivot.

### 5.2 Rendering

- **Pass 1 (Scene):** Faces (front/back materials), edges (profiles/extensions), hidden edges optional.
- **Pass 2 (Overlay):** Selection outlines, inference glyphs, guides, section planes, HUD, marquee.
- **AA:** MSAA; fallback FXAA.
- **Depth bias:** Polygon offset for edges avoids z-fighting.
- **Section cuts:** Plane clipping in shader; optional fill.

### 5.3 Picking

- BVH/Octree acceleration; screen-space pixel tolerance; last-hit hover stabilization.

### 5.4 Cursor Overlay (tool-driven)

- OS cursor collapses to a blank sprite while the viewport renders the active tool cursor.
- Default pick radius = **6 px** (selection 5.5 px, paint 7 px). Crosshair outer radius = **18 px** with a **4 px** gap at the center.
- Sticky/axis locks surface as compact badges next to the crosshair ("X", "-Z", etc.). Locked inferences reuse the inference label plus a lock suffix.
- Modifier hints (for example, `Shift: Stick inference • X/Y/Z: Axis lock`) live in a dark pill adjacent to the crosshair so they stay readable in light/dark themes.

The overlay deliberately relies on FreeCrafter’s bundled tool artwork instead of whatever cursor a platform ships. Windows, macOS, and Linux all expose different cursor packs, sizes, and DPI scaling rules; leaning on them would shatter the Figma framing and leave nowhere to stage inference locks or modifier hints. By hiding the OS cursor we can render the pick radius, crosshair, badges, and glyphs at spec-accurate dimensions, tint them for either theme, and animate sticky locks in real time.

Each cursor mode resolves to an icon under `:/icons` (for example, Line → `:/icons/line.png`, Move → `:/icons/move.png`, Rotate → `:/icons/rotate.png`, Paint Bucket → `:/icons/paintbucket.png`). The viewport paints that glyph just left of the crosshair with a soft shadow so the active tool remains obvious even when multiple modes share the same pick circle. For regression captures run `tests/test_cursor_overlay.cpp`; set `FREECRAFTER_CAPTURE_CURSOR=/tmp/cursor.png` (or `.base64`) before launching the test to dump the overlay without checking in binary artefacts.

<!-- Cursor overlay modes illustrated with existing tool icons -->
<p align="center">
  <img src="../resources/icons/line.png" alt="Line tool icon" width="96" />
  <img src="../resources/icons/move.png" alt="Move tool icon" width="96" />
  <img src="../resources/icons/rotate.png" alt="Rotate tool icon" width="96" />
  <img src="../resources/icons/paintbucket.png" alt="Paint bucket tool icon" width="96" />
</p>
<sub>Existing tool icons reinforce the cursor overlay modes: drawing, moving, rotating, and annotating. Observe the live overlay in the viewport to see pick radii, crosshair spacing, and modifier hints in context.</sub>

---

## 6. Tools - Exhaustive Behaviour & UX

For each tool: Activation, Icon/Asset, Cursor, Hotkeys, Snaps/Locks, States/Steps, Measurement, Preview, Edge Cases, Commit/Undo, Context Bar Options, Performance Notes, Integration (files, IDs), Telemetry/Accessibility, QA Checks.

### 6.1 Select - `tools.select`

- **Activation:** Space or `S`
- **Icon:** `resources/icons/select.svg` (mono) -> future colored swap.
- **Cursor:** Arrow; on-drag marquee. Tooltip shows entity kind/name.
- **Snaps:** none (picking only).
- **States:** Idle -> Hover -> Click/Marquee -> Update selection.
- **Measurement:** not applicable.
- **Preview:** Hover outline plus slight tint; marquee dashed with 25 percent fill; live count badge near cursor.
- **Edge cases:** Hidden/locked/tag-off not selectable unless view override; double-click enters group/component; `Esc` exits edit context.
- **Commit/Undo:** Selection change; preference toggles whether it is undoable.
- **Context bar:** Filters such as "Select only faces/edges/components"; "Ignore backfaces" toggle; "Through selection" toggle.
- **Performance:** Hover cache; coalesce hit-tests per frame.
- **Integration:** `src/Tools/SmartSelectTool.*`, `src/Interaction/SelectionSet.*`
- **Telemetry/Accessibility:** Count selections, marquee direction; announce selection count with accessibility hooks.
- **QA:** Inside vs Crossing; locked/hidden respect; enter/exit groups with `Esc`.

### 6.2 Eraser - `tools.eraser`

- **Activation:** `E`
- **Icon:** `resources/icons/eraser.png`
- **Cursor:** Eraser pencil.
- **Snaps:** Edge hits; `Shift` soften/smooth; `Ctrl` hide.
- **Steps:** Click edge = delete; drag = paint delete.
- **Edge cases:** Prevent non-manifold if "Strict Manifold" preference on; smoothing toggles for coplanar edges.
- **Commit:** `DeleteEdgesCommand` or property flip; drag merges into single command.
- **QA:** Delete vs Smooth vs Hide; undo once for drag batch.

### 6.3 Line - `tools.line`

- **Activation:** `L`
- **Cursor:** Crosshair with glyphs.
- **Snaps/Locks:** Endpoint/Mid/Intersect; On-Edge/On-Face; axis locks `X/Y/Z`; `Shift` sticky.
- **States:** Place P0 -> Rubberband -> Place P1 (chains) -> `Esc` ends chain; `Backspace` removes last segment.
- **Measurement:** Segment length; repeat overrides last segment.
- **Preview:** Dashed preview; endpoint glyphs; HUD length.
- **Edge cases:** Close loop heals face (coplanarity epsilon 1e-5); auto-split intersected edges; coincident merge epsilon 1e-6.
- **Commit:** Create edges; face auto-heal.
- **Context:** "Continue on Enter" toggle; "Infinite guides from lines" off by default.
- **QA:** Axis lock priority vs Endpoint; typed length correctness; face splitting.

### 6.4 Rectangle - `tools.rectangle`

- **Activation:** `R`
- **Cursor:** Crosshair.
- **Snaps:** On-Face projection; axis alignment.
- **Steps:** Corner1 -> Corner2.
- **Measurement:** `w,h` or `w x h`.
- **Preview:** Rubber rectangle with live W x H; shows on-face plane badge if projected.
- **Edge cases:** Zero-area ignored; plane projection resolved from hover.
- **Commit:** 4 edges plus face.
- **QA:** Numeric W x H; projection correctness.

### 6.5 Rotated Rectangle - `tools.rotated_rectangle`

- **Steps:** Corner1 -> Direction (sets theta) -> Height point.
- **Measurement:** `w,h,theta`.
- **Preview:** Protractor badge during direction pick.
- **QA:** Angle snapping; typed theta overrides.

### 6.6 Circle - `tools.circle`

- **Activation:** `C`
- **Steps:** Center -> Radius.
- **Measurement:** `r=<n>` or `d=<n>`; context: sides spinner (24 default).
- **QA:** Sides affect tessellation; diameter vs radius input.

### 6.7 Polygon - `tools.polygon`

- **Activation:** `P`
- **Steps:** Center -> Radius.
- **Measurement:** `s=<n>, r=<n>` or `s=<n>, d=<n>`.
- **QA:** Odd/even vertex orientation; inscribed radius correctness.

### 6.8 Arc (3-pt) - `tools.arc`

- **Steps:** Start -> End -> Bulge.
- **Measurement:** Accepts radius/angle/chord length (labeled or inferred).
- **Preview:** Arc sweep; chord badge.
- **Edge cases:** Auto-split faces it crosses.

### 6.9 Center Arc - `tools.center_arc`

- **Steps:** Center -> Start -> End.
- **Measurement:** Angle or radius.

### 6.10 Tangent Arc - `tools.tangent_arc`

- **Steps:** Pick tangent edge -> End.
- **Edge cases:** If multiple candidates, require explicit confirmation on edge (hover glow).

### 6.11 Bezier - `tools.bezier`

- **Steps:** Place control points; `Ctrl` toggles corner/smooth; `Enter` commits.
- **Context:** Show handles; snap handles to axes if `Shift` held.
- **Edge cases:** Simplify if handles collapse.

### 6.12 Freehand - `tools.freehand`

- **Steps:** Press-drag to sample; on release, simplify (Douglas-Peucker).
- **Context:** Tolerance slider (default 0.25 mm).

### 6.13 Move - `tools.move`

- **Activation:** `M`
- **Cursor:** Four arrows.
- **Snaps/Locks:** Full inference; `X/Y/Z` axis; `Ctrl` toggles Copy.
- **Steps:** Pick grab point -> Drag/type distance -> Place.
- **Measurement:** Distance; arrays via `*n` and `/n`.
- **Edge cases:** Weld on merge; honor tag locks/visibility; moving planar faces keeps coplanarity.
- **Commit:** `TransformCommand` (translate) or Duplicate plus Transform (copy/array) collapsed to one undo.

### 6.14 Rotate - `tools.rotate`

- **Cursor:** Rotate.
- **Steps:** Resolve plane (hover face/axis) -> Center -> First arm -> Second arm/angle.
- **Measurement:** Angle; arrays `*n` `/n`.
- **Preview:** Protractor plus arc sweep; angle tick every 15 deg (preference).

### 6.15 Scale - `tools.scale`

- **States:** Pick origin -> Drag handle(s).
- **Modes:** Uniform (`Shift`) or axis-constrained (`X/Y/Z` key or direct handle).
- **Measurement:** Factor (for example, `2`, `0.5`) or triplet `sx,sy,sz`.
- **Edge cases:** Negative factors flip normals; warn if inverted.

### 6.16 Offset - `tools.offset`

- **Steps:** Hover face -> Click to arm boundary -> Drag/type distance.
- **Options:** `Alt` toggles keep-holes vs cap.
- **Edge cases:** Self-intersection resolves by split/merge; tiny islands below threshold discarded (preference).

### 6.17 Push/Pull - `tools.pushpull`

- **Steps:** Click planar face -> Drag along normal -> Click/Enter to commit.
- **Options:** `Ctrl` = create new (do not merge); double-click repeats last distance on similar faces.
- **Edge cases:** Self-intersections split topology; zero thickness ignored; back-face push allowed if "Allow reverse" preference on.

### 6.18 Extrude - `tools.extrude`

- **Modes:** Linear (profile -> height) and Path (profile -> pick path).
- **Measurement:** Height; path uses parallel transport (no twist) with option for Frenet.
- **Edge cases:** Close path caps; open path optional caps.

### 6.19 Follow Me - `tools.followme`

- **Steps:** Select profile -> Activate -> Click path -> Preview -> Commit.
- **Options:** Miter/round; cap ends; "Keep normals" toggle.

### 6.20 Paint Bucket - `tools.paint`

- **Steps:** Hover (preview swatch) -> Click apply; `Alt` eyedropper.
- **Rules:** Component paint overrides internals unless face has explicit material.
- **Context:** Sample all instances toggle.

### 6.21 Text - `tools.text`

- **Modes:** Screen-aligned label vs face-attached text.
- **Steps:** Click to place -> Inline edit -> `Enter` to commit.
- **Options:** Leader on/off; size in model units.

### 6.22 Dimension - `tools.dimension`

- **Auto-detect:** Linear (two points), aligned (edge), angular (two edges), radial (circle/arc).
- **Steps:** Pick references -> Drag leader -> Place.
- **Options:** Precision, unit style, suppression of trailing zeros.

### 6.23 Tape Measure - `tools.tape`

- **Steps:** P1 -> P2 shows length; `Ctrl` leaves guide.
- **Options:** Toggle guides visibility from View; delete all guides via context.

### 6.24 Axes - `tools.axes`

- **Steps:** Set origin -> Red direction -> Green direction (Blue inferred).
- **Options:** Reset to world; save as scene axes.

### 6.25 Section Plane - `tools.section`

- **Steps:** Place on face/axis -> Flip via arrows -> Drag gizmo along normal.
- **Options:** Display cuts, Fill, Symbols; name/activate; scenes remember states.

### 6.26 Navigation - `tools.orbit`, `tools.pan`, `tools.zoom`

- **SketchUp scheme:** MMB = Orbit (temporary), `Shift+MMB` = Pan, Wheel = Zoom-to-cursor, RMB = Context.
- **DSM scheme:** RMB = Orbit (temporary), MMB = Pan, Wheel = Zoom-to-cursor.
- **Zoom tool:** Drag vertical to dolly; double-click focuses pivot; `Alt` adjusts speed.

---

## 7. Navigation Schemes & Temporary Tools

- Configured in `resources/config/navigation_default.json`.
- `ToolManager` maintains a temporary tool stack; on mouse-up, prior tool and measurement focus restored.

---

## 8. File I/O, Scene State, Persistence

- **Import:** OBJ/STL triangulated as Components named `Imported_*`; detect units if available, else prompt.
- **Export:** OBJ (now), GLTF (roadmap). Options: triangulate, export materials/textures, Y-up/Z-up.
- **Scene state:** Camera, projection, style, tag visibility, grid/guides, section planes, sun, tray layout, recent files.
- **Files:** `src/FileIO/*`, `src/Scene/SceneSerializer.*`

---

## 9. Undo/Redo Command Architecture

- `Command` base (`apply/undo`); array operations collapsed; panel edits routed through commands; optional "selection undoable" preference.
- **Files:** `src/Core/Command.*`, `src/Core/CommandStack.*`

---

## 10. Performance, Tolerances, Robustness

- **Epsilons:** Merge 1e-6 m; Coplanarity 1e-5; Angle snap 15 deg; Freehand simplify 0.25 mm (default).
- **Budgets:** >= 60 FPS with 1M edges on mid-tier GPU; overlays <= 2 ms/frame.
- **Picking:** BVH incremental updates; hover hysteresis; snap ranking prevents flicker.
- **Stability:** Cursor badges mirror lock states; render passes decoupled.

---

## 11. Accessibility & Internationalization

- All toolbar items focusable; tool hints via ARIA live region; color cues always paired with glyph references (parallel, perpendicular, tangent).
- Strings under `resources/strings/*.json`; decimal comma supported; measurement parser accepts localized separators.

---

## 12. Assets, Iconography, Cursors & Theming

- **Mono icon set (now):** `resources/icons/*` - filenames map 1:1 to tool IDs.
- **Colored icon set (future):** `resources/icons_colored/*` using identical filenames for hot-swap (theme-based).
- **Cursors:** `GLViewport::cursorForToolName()` maps tool to cursor; lock badges (X/Y/Z, tangent, center) overlay near cursor.
- **Right-column colored groups (future):** Cluster colored assets by tool group; tray exposes palette switcher.

---

## 13. Roadmap / Advanced Tools (Pre-spec, Implementation-ready)

- Each inherits common tool contract. Stubs live in `src/AdvancedTools/*`. Context bar holds options; telemetry records usage and cancels.

### 13.1 Round Corner

- Select edges/faces -> set Radius -> choose Miter type (Round/Bevel/Chamfer) -> Preview -> Commit.
- **Options:** Merge UVs, Segment count, Corner continuity.
- **Edge cases:** Small features below threshold culled; preserves hard/soft edges as per option.

### 13.2 CurveIt

- Convert polylines to splines.
- **Options:** Continuity G0/G1/G2, resample density, preserve endpoints, "Keep on face."

### 13.3 PushANDPull (Thicken)

- Offset a surface to a solid.
- **Options:** Shell thickness, Cap boundaries, Both sides.
- **Edge cases:** Non-manifold handling; collision testing.

### 13.4 Surface

- Loft between profiles; skin over rails; patch holes.
- **Options:** Rebuild count, Smoothness, Rail weights, Boundary constraints.

### 13.5 BezierKnife

- Project Bezier onto faces; cut topology.
- **Options:** Projection axis, Tolerance, Simplify.

### 13.6 QuadTools

- Quadize triangulated meshes; stitch/flip edges; preserve flow.
- **Options:** Angle tolerance, Preserve borders, Manual bridge.

### 13.7 SubD

- Catmull-Clark preview with cage overlay; per-edge crease weights.
- **Options:** Level, Crease weight, Limit surface display.

### 13.8 Weld

- Merge collinear edges into polylines.
- **Options:** Angle tolerance, Keep endpoints.

### 13.9 Vertex Tools

- Soft selection (Gaussian falloff); move/slide/relax/smooth; magnet to surface.
- **Options:** Radius, Falloff, Normal vs Tangent move; Symmetry.

### 13.10 Clean

- Remove duplicates, stray edges, tiny faces; unify normals.
- **Options:** Thresholds per class; Report only mode.

### 13.11 ClothEngine

- Pins, wind/gravity, collisions; bake to mesh.
- **Options:** Substeps, Damping, Self-collision, Pin groups.

---

## 14. QA Acceptance Matrix (Excerpt)

- **Select:** Inside vs Crossing; hidden/locked respect; group enter/exit; Esc sequencing; hover stability.
- **Line:** Axis lock plus typed length; face splitting; undo merges.
- **Rectangle/RotRect:** `w,h[,theta]` grammar; on-face projection; zero-area reject.
- **Move/Rotate/Scale:** Arrays `*n` `/n`; weld correctness; single undo step.
- **Offset:** Keep-holes toggle; self-intersection handling.
- **Push/Pull:** Repeat by double-click; create-new vs merge; reverse push preference.
- **Paint:** Component vs face precedence.
- **Section:** Clip and fill; gizmo drag/flip; persistence.
- **Navigation:** Temporary tool restore; zoom-to-cursor; pivot focus on double-click.

---

## 15. Implementation Checklist & Open Questions

### 15.1 Checklist

1. Map every tool to `ToolManager` IDs, icons, cursors (including Annotation/Section).
2. Route measurement input to all tools via `applyMeasurementOverride`.
3. Implement inference glyph rendering and ranking with sticky locks.
4. Status HUD (hint plus value plus lock badges) wired to active tool.
5. Two-way panel binding (Inspector/Outliner/Materials/Tags) with undo.
6. Complete geometry kernels (Push/Pull, Offset, Follow Me) with robust merges.
7. Section planes: clip plus arrows plus fill plus persistence.
8. Camera presets plus animation plus persistence.
9. Preferences UI for units, epsilons, navigation scheme, hotkeys.
10. Import/Export wired; progress feedback.
11. QA smoke tests plus regression scenes.

### 15.2 Open Questions

- Default model unit on first run (mm vs cm)?
- Selection undo behavior default?
- Zoom-to-cursor default on?
- GLTF exporter scope (PBR, tangents, instances)?

---

## 16. React Concept Parity (Web Demo)

- Use the provided `SketchUpLikeAppShell` layout: pinned/unpinned tray, resize, accordion, keyboard toggles.
- Map left toolbar buttons to tool IDs (`tools.line`, `tools.move`, etc.) and forward events to a JS/WASM adapter mirroring `ToolManager`.
- Implement WebGL viewport with identical cursor badges and inference glyph indicators.
- Keep Entity Info/Outliner/Materials/Tags in sync via shared state (for example, store) and route edits through undoable commands for parity with native.

---

## 17. Appendices

### Appendix A. Hotkeys (defaults)

- Select `S/Space`, Line `L`, Rectangle `R`, Circle `C`, Polygon `P`, Arc `A`, Move `M`, Rotate `Q`, Scale `Shift+S` (or `S` secondary), Offset `F`, Push/Pull `Pp`, Extrude `X`, Follow Me `Fm`, Text `T`, Dimension `D`, Tape `Alt+T`, Axes `Ax`, Section `Sc`, Orbit (temporary via mouse), Pan `H`, Zoom `Z`. Final bindings live in `resources/config/hotkeys_default.json`.

### Appendix B. Icon Map (mono vs colored)

- `resources/icons/*.svg|png` <-> `resources/icons_colored/*` (1:1 filename parity).

### Appendix C. Default Preferences

- Units mm; Angle degrees; decimal separator locale; snaps on; inference step 15 deg; merge epsilon 1e-6 m; navigation scheme SketchUp; zoom-to-cursor on; View Shaded+Edges, Sky+Ground on, Grid off, Profiles 1 px.

### Appendix D. Measurement Examples

- Line `2400` -> 2400 mm; `96"` -> 8 ft; `3' 6"` -> 1067 mm.
- Rectangle `1200,800`.
- Rotate `45`.
- Scale `2,1,1`.
- Polygon `s=12, r=500`.

- End of Ultra-Detailed V2 -

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

---

