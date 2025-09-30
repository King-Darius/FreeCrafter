# Rendering Smoke Test

1. Launch **FreeCrafter** and load the default empty document.
2. Confirm that the toolbar now shows a **Style** drop-down. Switch between **Wireframe**, **Shaded**, and **Shaded+Edges** and verify that the viewport updates instantly.
3. Orbit the camera around the default axes and ensure that wireframe mode hides face shading, shaded mode hides edge overlays, and shaded+edges renders both fills and silhouettes.
4. Close the application while a non-default style is active, relaunch, and verify that the selected style is restored automatically.
5. Extrude a simple rectangle and confirm that ghost previews and inference overlays respect the active render style (e.g., shaded mode still shows preview curves with transparency).
