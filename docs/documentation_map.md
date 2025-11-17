# FreeCrafter Documentation Map & Agent Reference

**Purpose.** Give coding agents and contributors a single index of authoritative documents and where to place new guidance so specs stay discoverable and non-duplicative.

## Canonical references by topic
- **Product, roadmap, and current state**
  - `README.md` — high-level orientation, features, and limitations.
  - `ROADMAP.md` — phase goals; align work and tests with the active milestone.
  - `docs/status/` — progress snapshots; use when validating completeness vs roadmap.

- **UI, tools, and interaction contracts (binding)**
  - `docs/gui_tool_behaviour_spec.md` — native/Qt implementation-grade behaviour, layouts, and input grammar.
  - `docs/codex_master_ui_tools_spec.md` — **canonical cross-stack contract** mirroring the GUI spec; update in lockstep so tool IDs, timings, and interaction grammar never diverge between Qt and other runtimes.
  - `docs/testing.md` §Renderer notes — how to run Qt/renderer-sensitive tests (headless or CI) without breaking contexts.

- **Architecture gaps, risk, and technical debt**
  - `docs/geometry_system_gaps.md` — missing kernel/topology capabilities and recompute needs.
  - `docs/technical_debt_tasks.md` — refactors and follow-ups that should not regress across tools.
  - `AGENT_CODE_HAZARD_CHECKLIST.md` — where to annotate hot spots (viewport, layout/theming, tool activation, autosave/undo, testing gaps, stubs).

- **Process, release, and validation**
  - `docs/getting_started_and_troubleshooting.md` — environment/bootstrap, packaging entry points, and sanity checks for new setups.
  - `docs/process/bug_triage_workflow.md` & `docs/process/ci_cd_release_actions.md` — issue handling and CI/release expectations.
  - `docs/release_validation.md` — packaging/VM validation checklist for installers and portable archives.
  - `docs/reviews/` (`freecrafter-reality-check.md`, `phase6-implementation-assessment.md`) — sanity checks on what is implemented vs. promised.

- **Planning and milestones**
  - `docs/milestones/` — phase-specific goals and missing pieces that map to the roadmap.
  - `docs/planning/` — legal/tech review notes and forward-looking design decisions.

## Placement rules for new or updated guidance
1. **Update the closest existing source** instead of creating a parallel document. Example: UI/interaction changes belong in `docs/gui_tool_behaviour_spec.md` and, if cross-stack or platform-neutral, in `docs/codex_master_ui_tools_spec.md` **with matching IDs and sequencing**.
2. **Cross-link related guidance** in this map when adding a substantive doc so agents know where to look next.
3. **Flag hazards in-place** using the severities from `AGENT_CODE_HAZARD_CHECKLIST.md` and keep that checklist in sync when hotspots move.
4. **Roadmap alignment first:** ensure new docs cite the relevant `ROADMAP.md` phase or milestone folder so priorities stay coherent.
5. **Testing expectations live in one place:** extend `docs/testing.md` (renderer, headless Qt, CI flags) instead of sprinkling ad-hoc notes.

Maintaining this map keeps future agents aware of the canonical specs and prevents divergent conventions.
