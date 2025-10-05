# Getting Started & Troubleshooting

This guide walks you through building FreeCrafter from source, producing the
PyInstaller-based “one-click” bootstrapper, and verifying that the resulting
package works on a clean machine. It also catalogs the most common runtime
errors and the helper scripts that detect or resolve them.

If you are onboarding for the first time, follow the sections in order. Once you
have a working setup, jump straight to the troubleshooting matrix and validation
checklist when preparing releases.

## 1. Prepare Your Environment

> The bootstrap scripts expect a Windows host today. Linux and macOS builds are
> still experimental.

1. Install **Python 3.10 or newer**. `python --version` should resolve to the
   interpreter you intend to use.
2. Install the **Visual Studio 2022 Build Tools** (or the full IDE) with the
   *Desktop development with C++* workload. This installs MSVC, Ninja/CMake, and
   `ctest`.
3. Ensure Git is on your `PATH` and that long file paths are enabled if your
   checkout is under a deep directory hierarchy.
4. Optional but recommended: create a virtual environment and activate it
   before running any Python scripts:

   ```powershell
   python -m venv .venv
   .\.venv\Scripts\Activate.ps1
   pip install --upgrade pip
   ```

5. If you plan to work offline, download the wheels listed in
   [`scripts/requirements.txt`](../scripts/requirements.txt) ahead of time and
   place them into a cache directory that you can reference with
   `python scripts/bootstrap.py --wheel-cache <path>`.

## 2. Run the Bootstrap Build (CLI & GUI)

Both bootstrap entry points live in the `scripts/` directory:

- `bootstrap.py` performs the entire build from the command line.
- `gui_bootstrap.py` wraps the same logic in a Qt interface for a friendlier
  “one-click” experience.

### CLI bootstrap

1. Open a **Developer Command Prompt for VS 2022** (or any shell where `cl.exe`
   and `cmake` resolve).
2. Clone the repository and change into it.
3. Run:

   ```powershell
   python scripts/bootstrap.py
   ```

The script will:

- Reuse an existing Qt install when available (it searches `CMAKE_PREFIX_PATH`,
  `Qt6_DIR`, and caches under `qt/6.5.3/<arch>/`).
- If Qt is missing, call `aqtinstall` to download the pinned `6.5.3`
  distribution into `qt/`.
- Configure CMake under `build/`.
- Compile the application and deploy the Qt runtime beside
  `build/FreeCrafter.exe` so the binary can launch on machines without Qt.

Captured build output stays in the console. When diagnosing failures, rerun the
command with `--verbose` to echo the full CMake trace.

### GUI bootstrap

Launch the GUI wrapper for the same workflow:

```powershell
python scripts/gui_bootstrap.py
```

Keep the window open until it reports **Finished**. The GUI streams the same log
output you see in the CLI and writes the binaries to the identical `build/`
folder. When you are satisfied with the run, close the window and proceed to the
packaging step.

## 3. Package the GUI Bootstrapper

The repository includes [`scripts/package_gui_bootstrap.py`](../scripts/package_gui_bootstrap.py)
to turn `gui_bootstrap.py` into a single executable using PyInstaller.

1. Make sure the bootstrap step above completed successfully so that all
   dependencies (including `PyInstaller`) are installed.
2. From the repository root, execute:

   ```powershell
   python scripts/package_gui_bootstrap.py
   ```

3. PyInstaller writes the bundle to `dist/FreeCrafterInstaller.exe` on Windows
   (and `dist/FreeCrafterInstaller` on Unix hosts). Subsequent runs overwrite the
   file, so archive it elsewhere when cutting a release.
4. Collect the console transcript for the release notes—it documents which Qt
   build and Python environment the installer was produced with.

If you hit import errors here, consult the troubleshooting section on Python
package issues.

## 4. Verify the Packaged Build

Always test the generated installer on a fresh machine (or virtual machine) that
mimics an end-user environment:

1. Copy `dist/FreeCrafterInstaller.exe` and the bootstrap logs to the clean
   system. Do **not** install Visual Studio or Qt on this host.
2. Launch the executable and confirm that:
   - The GUI starts without missing-DLL dialogs.
   - Qt downloads (when not cached) and installs into `%LOCALAPPDATA%` or the
     repository checkout.
   - CMake configures and builds FreeCrafter without human intervention.
3. Locate `build/FreeCrafter.exe` on the clean host and run it once. Ensure the
   Qt window opens, the viewport renders, and basic camera controls (orbit, pan,
   zoom) work.
4. Optional: run the staging install to inspect deployable artifacts:

   ```powershell
   cmake --install build --prefix dist
   ```

   Confirm that `dist/FreeCrafter.exe` (or the platform equivalent) launches.

Document your observations so the release artifacts remain auditable.

## 5. Troubleshooting Reference

| Symptom | Detection | Resolution |
| --- | --- | --- |
| **Missing Qt DLLs or plugins** | The application prints `Could not find the Qt platform plugin "windows"` or exits immediately. [`scripts/run_tests_with_qt_env.ps1`](../scripts/run_tests_with_qt_env.ps1) checks for the required `bin`, `plugins`, and `plugins/platforms` directories and aborts if they are missing. | Rerun `python scripts/bootstrap.py` so Qt is redeployed. Verify antivirus or cleanup tools are not deleting the `qt/6.5.3/<arch>/` cache. When distributing builds, ensure the `platforms/` and `styles/` folders travel with the executable. |
| **`ctest` not found** | Manual `ctest` runs fail, or `run_tests_with_qt_env.ps1 -UseCTest` halts with a descriptive message. | Launch a Developer Command Prompt where CMake/CTest are on `PATH`, or add `C:\Program Files\CMake\bin` manually. Re-run the helper script: `pwsh scripts/run_tests_with_qt_env.ps1 -UseCTest`. |
| **Visual C++ redistributable missing** | Running `FreeCrafter.exe` on a clean machine pops up `VCRUNTIME140.dll was not found`. | Install the [VS 2022 redistributable](https://aka.ms/vs/17/release/vc_redist.x64.exe) on the target host. Add this step to your VM provisioning scripts. |
| **Python import errors (`ImportError: No module named aqtinstall`)** | Bootstrap or packaging scripts fail while importing helper modules. | Activate your virtual environment (if any) and execute `pip install -r scripts/requirements.txt`. When offline, point `bootstrap.py` at a pre-populated wheel cache with `--wheel-cache`. |
| **`mt.exe` or Windows SDK tools missing** | Resource compilation fails during the build. | Ensure the Windows 10/11 SDK is installed alongside the build tools. Re-run `run_bootstrap.bat`, which sets `CMAKE_MT` explicitly before calling CMake. |

## 6. Validation Checklist

Run this checklist before promoting any build to teammates or users:

- [ ] **Bootstrap build (CLI or GUI) succeeds** on the development machine using
      `python scripts/bootstrap.py` or the GUI wrapper.
- [ ] **PyInstaller package generated** via `python scripts/package_gui_bootstrap.py`,
      with the artifact stored under version control or release storage.
- [ ] **Clean-system verification completed:** the packaged installer was executed
      on a fresh VM/host with no pre-existing Qt or MSVC runtimes, and the
      bootstrap flow completed end-to-end.
- [ ] **First-launch smoke test passed:** `build/FreeCrafter.exe` (or the staged
      `dist/` copy) launches, renders the default scene, and basic camera
      interactions succeed without crashes.
- [ ] **Optional regression tests executed** with `pwsh scripts/run_tests_with_qt_env.ps1 -UseCTest`
      when `ctest` is available.
- [ ] **Visual C++ redistributable presence confirmed** on any target machines
      used for validation or distribution.
- [ ] **Artifacts archived:** keep the PyInstaller output, bootstrap logs, smoke
      test notes, and any regression logs with the release hand-off.

Keep this document handy during release cycles so the “one-click installer”
promise remains measurable and trustworthy.
