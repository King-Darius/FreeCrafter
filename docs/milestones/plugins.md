# Milestone: Plugins

Tracks [roadmap item D](../../ROADMAP.md#high-level-milestones).

## Description
Provide a plugin architecture allowing third parties to extend FreeCrafter.

## Acceptance Criteria
- [ ] Plugin loader discovers and loads plugins from a user directory.
- [ ] Plugins can register new tools and menu items.
- [ ] Sandbox ensures plugins cannot crash the host application.
- [ ] Sample plugin demonstrates extending the toolset.

## Test Plan
### Automated Tests
- Unit tests for plugin API version checks and error handling.
- Integration tests loading and unloading the sample plugin.

### Manual Tests
- Adding a plugin to the directory registers a new tool.
- Disabling the plugin removes its UI elements.
- Malformed plugins fail gracefully with logged errors.
