# AGENTS.md — FreeCrafter

> Minimal, strict guidance for coding agents (and humans) contributing to **FreeCrafter**.  
> Objectives: restore **day‑to‑day usability**, keep the **GUI top‑down look**, and enforce **consistent naming** across tools/actions.

---

## 1) Project overview
**FreeCrafter** is a cross‑platform 3D modeling sandbox built with **C++17** and **Qt 6**. The app uses a tool‑driven interaction model (Select, Line, Rectangle, Circle, Move, Rotate, Scale, Push/Pull, etc.), a document model for scene graph, and a QWidget/QOpenGLWidget viewport.

---

## 2) UI alignment with the Figma concept (top‑down contract)
Adhere to the concept’s **top‑down** frame so the app looks and behaves as designed:

1. **MenuBar** — File, Edit, View, Tools, Window, Help  
2. **Top Toolbar** — context/tool actions (short tooltips; no text by default)  
3. **Left Toolbar** — primary tools (Select, Line, Arc, Rectangle, Push/Pull, Move, Rotate, Scale, Paint, …)  
4. **Viewport (central)** — sacred; overlays (grid, nav ball, horizon) are opt‑in and **must not occlude** interaction by default  
5. **Right Dock Stack** — Inspector/Properties, Layers/Outliner, Materials  
6. **Status Bar** — dynamic hints, coordinates, selection badges

**Naming & wiring:** UI actions must activate tools via the **exact** tool ID string (see §5). Buttons are checkable and grouped exclusively.

**Theme toggle (Light/Dark) — overview:** The GUI exposes **View ▸ Toggle Dark Mode**. Theme choice applies immediately to all widgets (docks/tabs/inputs/trees/status bar) and **persists**. Details and code in **§3.7 Styling (QSS themes)**.

---

## 3) Implementation guidance (minimum viable creation & edit)
### 3.1 Tool activation (single source of truth)
Create a map `{Action → ToolID}`. Every toolbar/menu slot calls `toolManager->activateTool(id)` and sets a clear status hint. Disable non‑implemented tools and show a small toast (“Coming soon”).

### 3.2 Event pipeline (strict order)
Forward events from the viewport in this order: **active tool → selection → camera** (unless a navigation tool is active). Consider a tiny **cursor HUD** showing current tool + mode.

### 3.3 Commit to the document
On creation finish: build mesh/face → `doc->addNode(...)` → push **Undo** → emit change → **select** the new node. Recompute scene bbox and `maybeFrameNew(bbox)` on the first entity.

### 3.4 Primitives
Provide **Box, Plane, Cylinder, Sphere** (Insert menu + toolbar). Support numeric dialog + drag variant. Assign default material/tag. Place at origin or under cursor with grid snap.

### 3.5 Sketch→Face bridge
Detect closed edge loops → create planar face (triangulate n‑gons). Highlight face; allow **Push/Pull** immediately.

### 3.6 Selection & transforms
After creation, auto‑select new node; gizmo appears; axis locks respond to keys.

### 3.7 Styling (QSS themes) — **Dark Mode Addendum**
**Goal:** user‑selectable Light/Dark mode consistent with the concept. The action lives under **View ▸ Toggle Dark Mode**, is checkable, applies immediately, and **persists**.

**Startup load (persisted):**
```cpp
// On app startup
QSettings settings("FreeCrafter", "FreeCrafter");
bool dark = settings.value("ui/darkMode", true).toBool();
auto loadTheme = [&](bool dark){
    QFile f(dark ? ":/styles/dark.qss" : ":/styles/light.qss");
    if (f.open(QIODevice::ReadOnly))
        qApp->setStyleSheet(QString::fromUtf8(f.readAll()));
};
loadTheme(dark);
```

**View menu action:**
```cpp
QAction* actDark = viewMenu->addAction(tr("Toggle Dark Mode"));
actDark->setCheckable(true);
actDark->setChecked(dark);
connect(actDark, &QAction::toggled, this, [=](bool on){
    settings.setValue("ui/darkMode", on);
    loadTheme(on);
    // Optional: swap theme-specific toolbar icons (sun/moon) here
});
```

**Notes:**
- Keep **parity** between `dark.qss` and `light.qss` (same metrics/widgets; colors differ).
- Ensure tooltip contrast, focus rings, and selection highlights meet accessibility contrast in both themes.
- If you add a toolbar Theme button, bind it to the same action so menu/toolbar stay in sync.

### 3.8 Render & visibility defaults
Beginner‑safe defaults: **Shaded + Edges**, grid ON, axes ON; default tag visible; perspective FOV sensible. Add **View ▸ Reset Camera & Style** to recover from bad states.

### 3.9 Error handling
If a tool can’t commit (invalid loop/zero area), show a clear inline error and keep the tool active.

---

## 4) Build & test (canonical)
Prereqs: Qt 6.6+, CMake 3.24+, Python 3.10+, a C++17 compiler.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
ctest --test-dir build --output-on-failure
```

If Qt is not found, set one of:
```bash
# Linux/macOS
export CMAKE_PREFIX_PATH=/path/to/Qt/6.x/gcc_64
# or
export Qt6_DIR=/path/to/Qt/6.x/gcc_64/lib/cmake/Qt6

# Windows (PowerShell)
$env:CMAKE_PREFIX_PATH="C:/Qt/6.x/msvc2019_64"
```

---

## 5) Code style & naming (follow what exists)
- **Language:** C++17, Qt 6. Prefer RAII, smart pointers; QObject parentage for ownership.
- **Classes:** `PascalCase` (e.g., `MoveTool`, `RectangleTool`).  
- **Methods/vars:** `camelCase()`, members lower camelCase. Constants often `kLikeThis`.
- **UI text:** wrap user‑visible strings in `tr("…")`.
- **Tool IDs (critical):** Use the **exact** `getName()` string for activation. Historical mix exists:
  - Some tools: `"MoveTool"`, `"RotateTool"`, `"SmartSelectTool"` (include `Tool`).
  - Others: `"Rectangle"`, `"Circle"`, `"Arc"`, `"Offset"` (no suffix).
  Always match the existing tool’s `getName()` across code, actions, and bindings.
- **Actions & icons:** Title‑case action text (e.g., *Move*, *Push/Pull*). Prefer existing SVG icons; add new ones with logical names (e.g., `:/icons/offset.svg`) and register in `resources.qrc`.

---

## 6) Usability triage (quick reference)
**Symptoms:** tools don’t create geometry; edits can’t select/transform; Zoom Extents frames nothing.  
**Likely causes:** activation not wired; event routing gap; no document commit/signal; visibility filter; missing sketch→face; projection/clipping.  
**Diagnostics:** log tool activation; tap viewport events to the active tool; print `nodeCount()`/bbox after create; enforce **Shaded + Edges**; auto‑face closed loops; frame new bbox.  
**Smoke tests:** Box; Circle→Face→Push/Pull; Rect array with copy `x5`; Measure; Hide/Unhide; Tags & Color‑by‑Tag; Undo/Redo; Zoom Extents; Reset Camera & Style; Dark/Light toggle.

---

## 7) Task scope & PR checklist
Keep diffs small and local to target modules. Avoid sweeping refactors.

**PR must include:**
- Repro steps (bug) or acceptance criteria (feature).
- Build + test transcript (or CI green).
- Before/after screenshots for UI changes.
- Do **not** include links to agent conversations or chat logs in the PR message summary or description.

**Definition of Done:**
- Clean build; tests pass for non‑trivial changes.
- Tool activation and UI naming remain consistent.
- GUI respects top‑down contract; viewport not occluded by default.

---

## 8) Modules & boundaries
```
src/
  app/        # QApplication, MainWindow wiring
  ui/         # Widgets, menus, toolbars, docks, overlays (no heavy logic)
  core/       # Geometry/data models/utilities (no UI deps)
  graphics/   # Rendering glue; minimal surface
  io/         # Import/export, serialization
  ops/        # Commands/tools/undo‑redo; thin adapters over core
resources/    # Icons, images, .qrc
tests/        # Unit/integration tests
docs/         # Architecture, conventions, UI design
```
**Rules:** `core` must not depend on `ui`. Rendering state is isolated in `graphics`. `ops` remains testable.

---

## 9) Common pitfalls
- Mismatching tool IDs (e.g., using `"ArcTool"` when `getName()` is `"Arc"`).  
- Swallowing viewport events in overlays; ensure the **active tool** receives input first.  
- Creating geometry without committing/selecting/emitting change.  
- Hardcoded colors/margins; prefer existing styles/patterns.  
- Blocking work on the UI thread; offload heavy ops.

---

## 10) Ownership
- **Owner:** @King-Darius  
- When in doubt: open a small draft PR with questions at the top.
