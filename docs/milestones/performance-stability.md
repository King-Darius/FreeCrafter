# Milestone: Performance & Stability

Tracks [roadmap item G](../../ROADMAP.md#high-level-milestones).

## Description
Optimize the application to remain responsive and reliable under heavy workloads.

## Acceptance Criteria
- [ ] BVH accelerates selection and raycast queries.
- [ ] Frustum and occlusion culling reduce render workload.
- [ ] Level-of-detail and instancing support large scenes.
- [ ] Multithreaded booleans execute without blocking the UI.
- [ ] Background tasks display progress indicators.
- [ ] Memory pools and geometry compression lower memory footprint.
- [ ] Autosave and crash recovery restore work after failure.

## Test Plan
### Automated Tests
- Benchmarks measure selection and culling performance improvements.
- Unit tests cover memory pool allocations and autosave recovery.

### Manual Tests
- Stress-test scenes with many objects to verify LOD and instancing.
- Simulate long-running operations and confirm progress UI.
- Force-quit during edits and ensure recovery succeeds.
