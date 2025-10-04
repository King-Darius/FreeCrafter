# Renderer Test Notes

## Running the Windows regression test

```powershell
powershell -ExecutionPolicy Bypass -File scripts/run_tests_with_qt_env.ps1 -UseCTest
```

The helper script extends `PATH` so the bundled Qt runtime is found, enables
`QT_DEBUG_PLUGINS=1` so plugin discovery is echoed to the console, and forces
`QT_QPA_PLATFORM=windows` with `QT_OPENGL=angle`. The stock `qoffscreen`
backend bundled with Qt 6.5 cannot create `QOpenGLWidget` contexts on many
headless VMs, so the script switches to the Windows platform plugin and ANGLE
for reliable context creation. When the run falls back to the software WARP
renderer the scene heuristics become noisy, so the script also exports
`FREECRAFTER_RENDER_SKIP_COVERAGE=1`; this short-circuits the image coverage
assertions while still verifying that Qt can create a context, render a frame,
and exercise hidden-geometry toggles.

Plugin debug output prints to stderr. To capture it for later inspection run:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/run_tests_with_qt_env.ps1 -UseCTest 2> build/qt_plugins.log
```

The `ctest` transcript is always written to
`build/Testing/Temporary/LastTest.log`.
