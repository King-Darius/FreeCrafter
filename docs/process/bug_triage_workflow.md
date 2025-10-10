# Bug Triage Workflow

This workflow establishes a consistent path from the initial bug report through verification of the fix. Apply it to every reported defect so that ownership and expectations stay clear across FreeCrafter teams.

## 1. Capture
- Log the issue in the tracker immediately.
- Record reporter, affected build/commit, operating system, GPU/driver (if graphics), and any attachments (screenshots, logs, sample files).
- Assign a quick impact label: `P0-blocker`, `P1-critical`, `P2-major`, `P3-minor`, `P4-nice-to-have`.
- Add one or more area labels (`area:core`, `area:graphics`, `area:ui`, `area:tools`, `area:file-io`, `area:ops`, `area:docs`, `area:build`).

## 2. Reproduce
- Attempt to reproduce with the exact steps provided.
- If reproduction fails, request additional detail from the reporter and capture the attempt in the issue.
- Once a reliable repro exists, distill it to the shortest deterministic sequence and add it to the issue description.
- Attach supporting artifacts (screen recordings, traces, failing files). Store any large sample assets under `tests/data/` and reference them by relative path.

## 3. Isolate
- Identify the failing subsystem and owning maintainer.
- Capture logs, assertions, and stack traces. Use `--enable-logging` or the relevant debug flag where applicable.
- Determine the first known good and bad commits when regression is suspected.
- If the bug stems from bad data, add temporary instrumentation or guard rails to detect the condition earlier in the pipeline.

## 4. Fix
- Create a short-lived branch for the fix and link its PR back to the issue.
- Ensure the fix includes:
  * Code changes scoped to the minimal surface area.
  * Regression tests (unit/integration) that fail before the fix and pass after.
  * Documentation updates when behaviour or workflows change.
- Note any follow-up work (tech debt, refactors) that should land separately.

## 5. Verify
- Re-run the documented repro steps on the patched build.
- Execute the automated tests added in the fix, plus any impacted suites (e.g., `ctest`, `tests/test_render`, targeted Python harnesses).
- Update the issue with the verification results, including build number and platform.
- Close the issue only after confirmation from QA or the original reporter when the bug was externally sourced.

## 6. Categorise Open Issues
- Review the issue backlog weekly.
- Ensure each open bug has:
  * A current priority aligned with the latest assessment.
  * A clear owner or explicit `status:unassigned`.
  * A phase/milestone tag (`phase:core-shell`, `phase:phase7`, `phase:phase7.5`, etc.).
- Move stale issues (no updates in 30 days) into `status:needs-triage` and schedule them for review during the next bug council.

## 7. Reporting
- Summarise weekly triage metrics in `docs/status/qa_status.md`:
  * New bugs opened vs. resolved.
  * Outstanding P0/P1 issues.
  * Top problem areas by label.
- Escalate blocking defects to `#release-triage` Slack along with repro steps and owner.

Following this workflow keeps the roadmap trustworthy and ensures regressions are caught and verified before they reach users.
