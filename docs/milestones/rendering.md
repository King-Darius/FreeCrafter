# Milestone: Rendering

Tracks [roadmap item A](../../ROADMAP.md#high-level-milestones).

## Description
Implement the rendering infrastructure needed for FreeCrafter's real-time viewport.

## Acceptance Criteria
- [ ] Basic OpenGL pipeline with shader management.
- [ ] Support for wireframe, shaded, and shaded with edges styles.
- [ ] Frame rate HUD displays FPS and draw calls.
- [ ] Rendering performance meets target (60 FPS on reference hardware).

## Test Plan
### Automated Tests
- Unit tests for shader compilation and resource loading.
- Integration tests verifying render output for basic primitives.

### Manual Tests
- User can switch between rendering styles.
- HUD shows accurate frame statistics during navigation.
- Stress test scenes maintain ≥60 FPS on reference hardware.
