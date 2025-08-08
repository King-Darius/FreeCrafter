# FreeCrafter  <img width="30" height="30" alt="FreeCrafter Logo" src="https://github.com/user-attachments/assets/4fb35500-bc0c-4275-97a9-96ad20268567" />


FreeCrafter is an experimental 3D modeling sandbox built with Qt and OpenGL. It currently provides a minimal interface and a handful of prototype tools for sketching and extruding simple geometry.   The long-term goal is to build an CAD/architectural design style application to compliment Blender.  It would fit a gap for game developers and for CAD prototyping where tools like Blender are not ideal.  However, the plan would be to work with Blender to texture or render scenes if required.

## Features
- Qt-based desktop interface with a custom OpenGL viewport
- Camera controller supporting orbit, pan, and zoom
- Geometry kernel that stores curves and extruded solids
- Tools for selection, freehand sketching on the ground plane, and basic extrusion

## Building

The repository ships with a helper script that fetches a minimal Qt runtime and
builds the project for you. After cloning, simply run:

```bash
python scripts/bootstrap.py
```

The script downloads the necessary Qt libraries using the open‑source
`aqtinstall` tool, configures CMake with that Qt, compiles FreeCrafter, and runs
the appropriate deployment utility so the resulting executable already includes
the Qt runtime.

The built program can be found in the `build` directory and should run on a
machine without any additional setup.

Though we hope to skip even this step in future.

## Resources
Icon assets are stored under `resources/icons` and bundled using Qt's resource system.  They are currently only dummy assets.

## License
This project is released under the terms of the MIT License. See [LICENSE](LICENSE) for details.
