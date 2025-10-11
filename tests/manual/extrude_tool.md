# Extrude Tool QA

Manual regression checks for the redesigned Extrude tool. These scenarios mirror the
expectations described in `docs/gui_tool_behaviour_spec.md` §6.18.

## Linear Mode — Profile ➝ Height

1. Start from a blank document and draw a closed rectangle on the ground plane.
2. Switch to Select and click the rectangle loop so the curve highlights (profile selected).
3. Activate the Extrude tool.
4. Click and drag upward in the viewport. Verify a preview outline appears above the
   source profile while dragging.
5. Release the mouse to create the solid. The resulting prism should be added to the scene
   and selected. Run Undo/Redo to confirm the command stack records the operation.
6. Repeat with a drag that moves below the plane; the preview should flip and the extrusion
   should extend below the original profile.
7. With the tool still active, type `1500` in the measurement input and press Enter. The
   extrusion should snap to exactly 1500 mm height and commit immediately.

## Path Mode — Profile ➝ Path

1. Draw a closed rectangle (profile) and, separately, a polyline path that begins near the
   rectangle. The path can be two or more segments.
2. Select both the rectangle loop and the path (one closed, one open).
3. Activate the Extrude tool. The status bar should hint that the path is active.
4. Click and drag to preview the sweep. The ghost outline should extend along the path
   direction and respect the path length by default.
5. Type `1000` and press Enter. The extrusion should commit using the entered length along
   the path direction, and Undo/Redo should remove and restore the solid.
6. Delete the generated solid and ensure the original profile and path remain unchanged.
