# Milestone: UI Polish

Tracks [roadmap item F](../../ROADMAP.md#high-level-milestones).

## Description
Refine the user interface, themes, and onboarding experience.

## Acceptance Criteria
- [ ] Consistent SVG icon set with high-DPI support.
- [ ] Light and dark themes selectable at runtime.
- [ ] Localization framework with at least one additional language.
- [ ] Instructor panel providing contextual tool tips.

## Test Plan
### Automated Tests
- UI regression tests verifying layout and theme switching.
- Linting for translation files ensuring required keys.

### Manual Tests
- Icons render crisply across supported DPI settings.
- Switching themes updates the entire interface without restart.
- Language selection changes all visible strings.
- Instructor panel shows correct tip for each active tool.
