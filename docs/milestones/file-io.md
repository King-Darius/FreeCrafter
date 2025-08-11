# Milestone: File I/O

Tracks [roadmap item E](../../ROADMAP.md#high-level-milestones).

## Description
Implement import, export, and native project persistence.

## Acceptance Criteria
- [ ] New/Open/Save for native project format.
- [ ] Import and export of OBJ and STL models.
- [ ] File operations handle invalid paths and missing files gracefully.
- [ ] Round-trip tests preserve geometry and materials.

## Test Plan
### Automated Tests
- Unit tests for serialization/deserialization of project files.
- Integration tests for OBJ/STL import/export round-trip.

### Manual Tests
- Users can create a model, save it, reopen it, and continue editing.
- Importing sample OBJ/STL files reproduces geometry accurately.
- Exported files open correctly in external viewers.
