# Extrude Tool QA Scenarios

These checks cover the redesigned Extrude tool (linear and path modes) per
`docs/gui_tool_behaviour_spec.md` §6.18. Perform each scenario in a debug build.

## Linear Extrusion
1. Start with a closed face profile (e.g., draw a rectangle) and leave no path selected.
2. Activate **Extrude**.
3. Click the face to set it as the active profile (status hint should remain armed).
4. Click-drag along the face normal; confirm live preview follows the drag and the measurement box echoes the distance.
5. Type a numeric value and press **Enter** to commit; confirm the resulting solid height matches the override and undo restores the pre-extrude state.
6. Redo and verify the preview distance is respected and caps remain on both ends.

## Path Extrusion
1. Create a closed face profile and a separate open guiding path curve.
2. Activate **Extrude** and click the face to select it as the profile.
3. Click the guide curve to set path mode; ensure a default preview appears using the path length.
4. Drag to adjust the extrusion distance; confirm the preview updates and clamps near four times the path length.
5. Commit the extrusion and verify caps are omitted for open paths while normals remain outward.
6. Undo and Redo the command; the geometry and selection should follow the command stack without leftover preview artifacts.

Document observed issues (if any) before marking QA complete.
