# Getting Started & Troubleshooting

This guide walks through the complete FreeCrafter build lifecycle: preparing a
workstation, running the Qt bootstrap (CLI and GUI), producing the PyInstaller
"one-click" bootstrapper, and validating that the packaged build works on a
clean machine. It also catalogs the most common runtime issues and the helper
scripts that detect or fix them.

Follow the sections in order the first time you build the project. Once you
have a working setup, jump straight to the troubleshooting matrix and
validation checklist when you cut new artifacts.

## 1. Prepare Your Environment

> The bootstrap flow is primarily exercised on Windows today. Linux and macOS
> paths are included where they differ, but the packaging scripts currently
> target Windows binaries.

1. Install **Python 3.10+** and verify that `python --version` resolves to the
   interpreter you intend to use.
2. Install the **Visual Studio 2022 Build Tools** (or the full IDE) with the
   *Desktop development with C++* workload. This provides MSVC, Ninja/CMake,
   and `ctest`.
3. Ensure **Git** is on your `PATH`. If you clone into a long path on Windows,
   enable long-file-path support (`git config --system core.longpaths true`).
4. Optional but recommended: create and activate a virtual environment before
   running any Python scripts:

   ```powershell
   python -m venv .venv
   .\.venv\Scripts\Activate.ps1
   pip install --upgrade pip
   ```

   On Unix shells use `source .venv/bin/activate`.
5. If you expect to work offline, pre-download the wheels listed in
   [`scripts/requirements.txt`](../scripts/requirements.txt) and place them in
   a cache that you can point to with
   `python scripts/bootstrap.py --wheel-cache <path>`.
6. Optional utilities:
   - `pwsh` for running the PowerShell helper scripts on non-Windows systems.
   - A clean Windows VM snapshot dedicated to installer verification.

## 2. Run the Bootstrap Build (CLI & GUI)

The bootstrap entry points live in `scripts/`:

- `bootstrap.py` performs the end-to-end build from the command line.
- `gui_bootstrap.py` wraps the same logic in a Qt interface for a one-click
  experience.

Both scripts detect an existing Qt install via environment variables
(`CMAKE_PREFIX_PATH`, `Qt6_DIR`, etc.), reuse caches under `qt/6.5.3/<arch>/`,
fall back to [`aqtinstall`](https://pypi.org/project/aqtinstall/) when Qt is
missing, configure CMake in `build/`, compile the application, and deploy the
Qt runtime beside `build/FreeCrafter.exe`.

### CLI bootstrap

1. Open a **Developer Command Prompt for VS 2022** (or any shell where
   `cl.exe`, `cmake`, and `ctest` resolve).
2. Clone the repository and change into it.
3. Run:

   ```powershell
   python scripts/bootstrap.py
   ```

   On Unix hosts, run the same command in a shell with your compiler toolchain
   on `PATH`.

Use `--offline` when you want to prevent new downloads, `--wheel-cache` to
point at a directory of pre-downloaded packages, and `--verbose` to echo the
full CMake trace.

For a curated environment that sets `vcvarsall`, `CMAKE_PREFIX_PATH`, and
`CMAKE_MT`, you can also run the checked-in [`run_bootstrap.bat`](../run_bootstrap.bat).

### GUI bootstrap

Launch the GUI wrapper for the same workflow:

```powershell
python scripts/gui_bootstrap.py
```

Keep the window open until it reports **Finished**. The GUI streams the same
log output you see in the CLI and writes the binaries to the identical
`build/` folder. Close the window once the run succeeds, then proceed to the
packaging step.

## 3. Package the GUI Bootstrapper

[`scripts/package_gui_bootstrap.py`](../scripts/package_gui_bootstrap.py) turns
`gui_bootstrap.py` into a standalone executable using PyInstaller.

1. Ensure the bootstrap step above completed successfully so that all
   dependencies (including PyInstaller) are installed.
2. From the repository root, execute:

   ```powershell
   python scripts/package_gui_bootstrap.py
   ```

3. PyInstaller writes the bundle to `dist/FreeCrafterInstaller.exe` on Windows
   (and `dist/FreeCrafterInstaller` on Unix hosts). Subsequent runs overwrite
   the file, so archive it elsewhere when cutting a release.
4. Capture the console transcript for the release notes—it documents which Qt
   build and Python environment produced the installer.

If you need to rebuild the project with CMake or stage installers with CPack,
[`scripts/package_build.ps1`](../scripts/package_build.ps1) layers installation
(`cmake --install`) and packaging (`cmake --build . --target package`) on top
of the Qt runtime configured by the bootstrap.

## 4. Verify the Packaged Build

Always test the generated installer on a fresh machine (or virtual machine)
that mimics an end-user environment:

1. Copy `dist/FreeCrafterInstaller.exe` plus the bootstrap logs to the clean
   system. Do **not** preinstall Visual Studio or Qt on this host.
2. Launch the executable and confirm that:
   - The GUI starts without missing-DLL dialogs.
   - Qt downloads (when not cached) and installs into `%LOCALAPPDATA%` or the
     repository checkout.
   - CMake configures and builds FreeCrafter without human intervention.
3. Locate `build/FreeCrafter.exe` on the clean host and run it once. Ensure the
   Qt window opens, the viewport renders, and basic camera controls (orbit,
   pan, zoom) work.
4. Optional: run the staging install to inspect deployable artifacts:

   ```powershell
   cmake --install build --prefix dist
   ```

   Confirm that `dist/FreeCrafter.exe` (or the platform equivalent) launches.

Document your observations so that release artifacts remain auditable.

## 5. Troubleshooting Reference

| Symptom | Detection | Resolution |
| --- | --- | --- |
| **Missing Qt DLLs or plugins** | The application prints `Could not find the Qt platform plugin "windows"` or exits immediately. [`scripts/run_tests_with_qt_env.ps1`](../scripts/run_tests_with_qt_env.ps1) checks for the required `bin`, `plugins`, and `plugins/platforms` directories and aborts if they are missing. | Rerun `python scripts/bootstrap.py` so Qt is redeployed. Verify antivirus or cleanup tools are not deleting the `qt/6.5.3/<arch>/` cache. When distributing builds, ensure the `platforms/` and `styles/` folders travel with the executable. |
| **`ctest` not found** | Manual `ctest` runs fail, or `run_tests_with_qt_env.ps1 -UseCTest` halts with a descriptive message. | Launch a Developer Command Prompt where CMake/CTest are on `PATH`, or add `C:\Program Files\CMake\bin` manually. Re-run the helper script: `pwsh scripts/run_tests_with_qt_env.ps1 -UseCTest`. |
| **Visual C++ redistributable missing** | Running `FreeCrafter.exe` on a clean machine surfaces `VCRUNTIME140.dll was not found`. | Install the [VS 2022 redistributable](https://aka.ms/vs/17/release/vc_redist.x64.exe) on the target host. Add this step to VM provisioning scripts. |
| **Python import errors (`ImportError: No module named aqtinstall`)** | Bootstrap or packaging scripts fail while importing helper modules. | Activate your virtual environment (if any) and execute `pip install -r scripts/requirements.txt`. When offline, point `bootstrap.py` at a pre-populated wheel cache with `--wheel-cache`. |
| **`mt.exe` or Windows SDK tools missing** | Resource compilation fails during the build. | Ensure the Windows 10/11 SDK is installed alongside the build tools. Re-run [`run_bootstrap.bat`](../run_bootstrap.bat), which sets `CMAKE_MT` explicitly before calling CMake. |
| **Network or SSL errors during Qt download** | `aqtinstall` exits with TLS or network timeouts. | Retry on a stable connection, pre-download Qt archives, or invoke `bootstrap.py --offline --qt-dir <path>` to reuse a known-good Qt tree. |

## 6. Validation Checklist

Run this checklist before promoting any build to teammates or users:

- [ ] **Bootstrap build (CLI or GUI) succeeds** on the development machine using
      `python scripts/bootstrap.py` or the GUI wrapper.
- [ ] **PyInstaller package generated** via `python scripts/package_gui_bootstrap.py`,
      with the artifact stored under version control or release storage.
- [ ] **Clean-system verification completed:** the packaged installer was
      executed on a fresh VM/host with no pre-existing Qt or MSVC runtimes, and
      the bootstrap flow completed end-to-end.
- [ ] **First-launch smoke test passed:** `build/FreeCrafter.exe` (or the staged
      `dist/` copy) launches, renders the default scene, and basic camera
      interactions succeed without crashes.
- [ ] **Optional regression tests executed** with
      `pwsh scripts/run_tests_with_qt_env.ps1 -UseCTest` when `ctest` is
      available.
- [ ] **Visual C++ redistributable presence confirmed** on any target machines
      used for validation or distribution.
- [ ] **Artifacts archived:** keep the PyInstaller output, bootstrap logs,
      smoke-test notes, and any regression logs with the release hand-off.

## 7. Viewport bring-up checklist

Use this checklist when the 3D area looks empty or unresponsive after a build.
Each item points to the existing wiring so you can spot anything that drifted
while integrating new features:

- **Confirm the viewport is actually ticking.** `GLViewport` starts a 16 ms
  timer that calls `update()` and keeps `paintGL()` running; if the widget is
  not the central window content or the timer is stopped, nothing will refresh.
  Check the constructor for the `QTimer` setup and `update()` connection before
  debugging deeper.
- **Verify the default draw path.** On every frame the viewport clears the
  framebuffer, renders the sky band, then draws the grid and axes before any
  scene geometry. If you see only a clear color, the draw routine is still
  alive—look at the grid toggle or camera state next.
- **Ensure input reaches the camera and tools.** Mouse and keyboard handlers in
  the viewport hand events to the `ToolManager`, which in turn drives camera
  navigation bindings. If `setToolManager()` was never called, orbit/pan/zoom
  will not move even though the math is implemented.
- **Propagate document changes back to the viewport.** The main window registers
  callbacks that call `viewport->update()` whenever geometry or selection
  changes. When adding new tool flows, use the same pattern so the renderer
  knows to upload new meshes.
- **Double-check visibility flags.** The renderer skips hidden geometry unless
  the “show hidden” flag is enabled, and it exits early if the grid visibility
  toggle is off. If you are creating objects via the geometry kernel, make sure
  they are marked visible and registered with the document so the draw loop can
  find them.

Keep this document handy during release cycles so the "one-click installer"
promise remains measurable and trustworthy.
