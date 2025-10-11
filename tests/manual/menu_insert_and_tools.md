# Insert, Tools, and Window Menu Smoke Test

1. Launch FreeCrafter in the desktop environment.
2. Open **Insert ▸ Shapes** and confirm the primitive dialog appears with Box, Plane, Cylinder, and Sphere tabs. Accept defaults and verify a box is added to the scene.
3. Open **Insert ▸ Guides**, enable the world X/Y/Z guides, press **OK**, and ensure the guides are visible in the viewport. Re-open the dialog and choose **Clear All Guides** to remove them.
4. Open **Insert ▸ Images**, choose any local PNG/JPG, accept with the default size, and confirm an image plane proxy appears in the scene.
5. Open **Insert ▸ External Reference**, pick any mesh file (OBJ/STL/etc.), set a display name, press **OK**, and ensure a proxy box with the given name is created.
6. Open **Tools ▸ Plugins…** and verify the plugin manager lists the scanned directories and allows refreshing.
7. Open **Window ▸ New Window** and confirm a second FreeCrafter window is spawned.
8. Undo the previous insert actions and verify geometry and metadata are removed each time.

Document the outcome of each step along with any unexpected UI or console messages.

