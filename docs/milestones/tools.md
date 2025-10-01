# Milestone: Tools

Tracks [roadmap item B](../../ROADMAP.md#high-level-milestones).

## Description
Provide core modeling tools and interaction framework.

## Acceptance Criteria
- [x] State-machine based tool framework with cancel (Esc) and commit (Enter).
- [x] Implement line, move, rotate, scale, and select tools.
- [x] Inference engine supports endpoint, midpoint, and axis locking.
- [x] Visual feedback for hover and selection highlighting.

## Test Plan
### Automated Tests
- Unit tests for geometry operations used by tools.
- Regression tests for selection and transformation accuracy.

### Manual Tests
- Users can draw connected lines with snapping.
- Move/rotate/scale operations respect axis constraints.
- Selection highlights respond to hover and click.
