# Manual Test: Inference shortcuts and overlays

## Purpose
Verify that the inference engine surfaces snap suggestions, sticky locking, and axis locking. The test also confirms that overlay glyphs render while hovering geometry within 12 ms.

## Preconditions
* Build and launch FreeCrafter with the updated inference system.
* Load or sketch a curve/solid so that vertices, edges, and faces are present.

## Steps
1. **Hover snapping**
   1. Move the cursor over curve vertices, edge midpoints, and face centers.
   2. Expect the inference glyph to appear in less than 12 ms with an identifying label (e.g., “Endpoint”, “Midpoint”).
   3. Confirm that the dashed guide aligns with the inferred feature.
2. **Sticky lock (Shift)**
   1. Hover a desired snap point and press and hold **Shift**.
   2. Move the cursor away; the glyph should remain “locked” and its label shows “(locked)”.
   3. Release **Shift** and confirm the lock clears.
3. **Axis lock (arrow keys)**
   1. With a snap highlighted, press one of the arrow keys:
      * **Left/Right** lock to the ±X axis.
      * **Up/Down** lock to the ±Y axis.
      * **PageUp/PageDown** lock to the ±Z axis.
   2. Verify that the inference type switches to “Axis”, a dashed axis guide renders, and the snap point slides along that axis as the cursor moves.
   3. Release the arrow key to clear the lock.
4. **Tool integration**
   1. With the Sketch tool active, click while a snap is shown and confirm the new vertex uses the snapped position.
   2. With the Selection tool, click a snapped location and ensure the corresponding object becomes selected.

## Expected Result
* Inference glyphs update within a frame (<12 ms), including type-specific colors.
* Sticky locks hold the snap until Shift is released.
* Arrow keys toggle axis locks with dashed guides, and tools honor the locked snap positions.
