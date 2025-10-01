# Milestone: Tools

Tracks [roadmap item B](../../ROADMAP.md#high-level-milestones).

## Description
Provide core modeling tools and interaction framework.

## Acceptance Criteria
- [x] State-machine based tool framework with cancel (Esc) and commit (Enter).
- [x] Implement line, move, rotate, scale, and select tools.
- [x] Inference engine supports endpoint, midpoint, and axis locking.
- [x] Visual feedback for hover and selection highlighting.

## Verification Notes
- The shared tool base runs a state machine that arms on pointer down, cancels back to idle, and routes Enter/Esc through
  `Tool::commit()`/`Tool::cancel()`, which `GLViewport` triggers from key events. 【F:src/Tools/Tool.cpp†L5-L159】【F:src/GLViewport.cpp†L1247-L1275】
- The milestone’s required tools are implemented as concrete classes that manage selection, previews, and measurement overrides: `LineTool`,
  `SmartSelectTool`, `MoveTool`, `RotateTool`, and `ScaleTool`. 【F:src/Tools/LineTool.cpp†L55-L199】【F:src/Tools/SmartSelectTool.cpp†L65-L240】【F:src/Tools/MoveTool.cpp†L13-L194】【F:src/Tools/RotateTool.cpp†L55-L216】【F:src/Tools/ScaleTool.cpp†L40-L233】
- Snapping enumerates endpoints, midpoints, intersections, and face centers, while `ToolManager` layers sticky locks (`Shift`) and axis locks
  mapped to X/Y/Z directions so manipulation tools receive `InferenceSnapType::Axis` results. 【F:src/Interaction/InferenceEngine.cpp†L450-L519】【F:src/Tools/ToolManager.cpp†L182-L466】
- Hover and selection cues render through the viewport: geometry colors shift when selected, and the inference overlay draws glyphs, dashed guides,
  and axis anchors for locked snaps. 【F:src/GLViewport.cpp†L352-L420】【F:src/GLViewport.cpp†L935-L1004】

## Test Plan
### Automated Tests
- Unit tests for geometry operations used by tools.
- Regression tests for selection and transformation accuracy.

### Manual Tests
- Users can draw connected lines with snapping.
- Move/rotate/scale operations respect axis constraints.
- Selection highlights respond to hover and click.
