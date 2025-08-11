# Milestone: DSM

Tracks [roadmap item C](../../ROADMAP.md#high-level-milestones).

## Description
Introduce document and scene management features.

## Acceptance Criteria
- [ ] Groups and components can be created, edited, and made unique.
- [ ] Tag system for visibility control with color-by-tag.
- [ ] Outliner displays hierarchy with drag-and-drop support.
- [ ] Scenes capture camera, style, and tag states.

## Test Plan
### Automated Tests
- Unit tests for serialization of groups/components and tag visibility.
- Integration tests for scene save/load cycle.

### Manual Tests
- Creating and editing groups/components updates outliner.
- Tags can hide/show objects and reflect colors.
- Switching scenes restores saved camera and visibility settings.
