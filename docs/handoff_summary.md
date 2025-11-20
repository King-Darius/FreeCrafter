> Note: this might be slightly out of date as I have already started working on fixes.
# FreeCrafter Handoff Summary

## Current state
- User reports tools (lines, rectangles, polygons, circles) still fail to render/appear in the viewport even though geometry creation paths have been tweaked repeatedly.
- Axes now render from the world origin and remain visible per the latest user feedback, but tool geometry visibility is still broken.
- Dark mode and nav-orb aesthetics were considered acceptable; remaining priority is sketch visibility and rendering reliability.
- Painter fallback has been used as a stopgap in prior iterations to ensure visibility when GL rendering fails (especially on ANGLE/ES contexts), but this is not a final fix.
- Loop closure was tightened so closed tools force the last vertex to equal the first before sending data to the kernel.

## Suspected root causes
- GPU path drops draw calls on some systems (ANGLE/ES). Need to confirm whether Renderer::flush is emitting geometry batches and whether shaders compile/link.
- Planarity/tolerance issues may stop faces from forming, which leaves loops rendered as open or not drawn in GL.
- Potential disconnect between document geometry objects and renderer traversal (visibility masks or missing synchronize calls).

## Diagnostics to run
- Enable `FREECRAFTER_RENDER_TRACE=1` and log batch counts inside `Renderer::addLineStrip`/`addTriangle`/`flush` to see if geometry reaches GL.
- Print `document.geometry().getObjects().size()` and per-object visibility during `drawSceneGeometry()` when tools commit.
- Inspect shader compile/link logs on ANGLE/ES; verify attribute bindings and program readiness before draw calls.
- Compare renderer/viewport code against the last known good revision where sketch tools rendered correctly.

## Fix plan
1) Restore reliable GL rendering
   - If shaders fail, emit clear logs and trigger painter fallback only when the geometry pass outputs zero draws.
   - Validate vertex buffers and attribute bindings for curves/meshes, especially under ANGLE/ES contexts.
2) Stabilize sketch→face creation
   - Keep loops in a workplane coordinate system, enforce closure/tolerance there, then project to 3D before calling the kernel.
   - Highlight or log loops that fail planarity so users know why faces are missing.
3) Re-enable painter fallback as a true fallback
   - Use QPainter only when the GL scene pass fails, not every frame.
4) Regression checks
   - Run manual smoke: draw rectangle/polygon/circle, ensure faces appear and Push/Pull works; verify axes stay visible; confirm nav-orb still matches spec.

## Additional notes
- Light mode is expected as the default; theme toggle persists via QSettings.
- Rebuild the executable before manual testing (`cmake --build build --target FreeCrafter -j8` then launch from `build/Debug`).
- Avoid incorporating GPL/LGPL code directly; permissive references like Dust3D/JSketcher are acceptable for patterns.
