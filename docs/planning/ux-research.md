# UX Research — Micro-behavior Inventory

## Reference Products
- **SketchUp Pro 2023** – inference engine, sticky shift axis locks, VCB input behavior.
- **Rhino 8 WIP** – command history, tabbed inspectors, customizable tool ribbons.
- **Fusion 360** – contextual toolbars, task progress pills, timeline-based history.
- **Blender 3.x** – keymap customization, command search palette.

## Micro-behaviors Catalogued
| Behavior | Source | Notes |
| --- | --- | --- |
| Sticky inference with Shift | SketchUp | Shift locks the current axis/guide until released. Implemented via ToolManager modifiers. |
| Axis lock via arrow keys | SketchUp | Arrow keys map to RGB axes (Right = Red/X, Left = Green/Y, Up = Blue/Z). |
| Dynamic VCB prompts | SketchUp | The status bar measurement box updates with context-specific hints and accepts typed overrides. |
| Command palette | Blender | Quick search (Ctrl+P) lists actions and hotkeys with fuzzy matching. |
| Docked inspectors with tabs | Rhino | Inspector/Explorer/History live within a single dock widget, persist width per project. |
| Background task pills | Fusion 360 | Long-running operations show progress and open a detail drawer on click. |
| Tooltips with hotkeys | Fusion 360 | Ribbon buttons display tooltip text plus the active shortcut. |
| Orbit/pan modifiers | SketchUp/Blender | Hold middle mouse to orbit, Shift+MMB to pan; double-tap middle to zoom extents. |
| Reduced-motion respect | macOS HIG | Animations disabled when user opts out. |

## Research Insights
- Users expect consistent iconography and 32px hit targets even for dense toolbars.
- Right-hand inspector must persist collapsed/expanded state between sessions.
- Command palette doubles as shortcut discovery; we will index menu and toolbar actions automatically.
- Measurements must support both architectural (ft/in) and metric formats, with auto unit conversion.
- Task feedback needs to surface the last three operations and allow cancellation when possible.

## Next Steps
1. Map inference behaviors into ToolManager state machine design (Phase 2).
2. Prototype measurement parser supporting feet/inch and metric overrides.
3. Design palette filtering and ranking algorithm (Phase 1.5 follow-up).
4. Validate focus order and keyboard navigation with accessibility audit.
