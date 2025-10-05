# Getting Started & Troubleshooting

This guide is intended for contributors who want to build FreeCrafter from source, generate a distributable "one-click" bootstrapper, and diagnose the most common errors encountered along the way. Follow the sections in order the first time you work with the repository, then use the troubleshooting references and validation checklist to keep subsequent builds healthy.

## 1. Prepare Your Environment

1. Install **Python 3.10+** with `pip` on your development machine.
2. Install the **Visual Studio 2022 Build Tools** (or the full IDE) with the *Desktop development with C++* workload. This provides the MSVC compiler, Windows SDK, and `ctest`.
3. Ensure Git is on `PATH` and you can clone repositories.
4. Optionally create and activate a Python virtual environment before running any of the scripts.

> ℹ️ The bootstrap script automatically installs Python dependencies listed in [`scripts/requirements.txt`](../scripts/requirements.txt) on-demand. You only need manual `pip install -r scripts/requirements.txt` runs if you prefer to pre-populate a virtual environment.

## 2. Run the Bootstrap Build (CLI)

1. Open a **Developer Command Prompt for VS 2022** so the MSVC compiler and CMake tools are available on `PATH`.
2. Clone the repository and change into it.
3. Execute the bootstrap script:

   ```powershell
   python scripts/bootstrap.py
   ```

4. The script will:
   - Download the pinned Qt runtime if it is not already cached under `qt/6.5.3/`.
   - Configure CMake with the located Qt.
   - Build the application and stage a runnable binary under `build/`.
   - Deploy Qt runtime artifacts into the build output so the program can launch on machines without Qt installed.

5. On success you can run `build/FreeCrafter.exe` directly. Keep the `build/` directory around for testing and packaging in later steps.

### GUI Bootstrapper (Optional)

For a friendlier experience, launch the Qt-based bootstrapper that wraps the same workflow:

```powershell
python scripts/gui_bootstrap.py
```

The GUI streams the output of `bootstrap.py` and exposes an **Install** button. Leave the window open until it reports completion, then verify the same `build/` directory described above.

## 3. Generate the One-Click Installer

The repository provides [`scripts/package_gui_bootstrap.py`](../scripts/package_gui_bootstrap.py) to turn the GUI bootstrapper into a single-file executable via PyInstaller.

1. From the repository root, ensure PyInstaller is available (either through `pip install pyinstaller` or the bootstrap script installing it).
2. Run the packaging script:

   ```powershell
   python scripts/package_gui_bootstrap.py
   ```

3. The script emits `dist/FreeCrafterInstaller.exe`. This file contains Python, Qt, and the bootstrapper UI, allowing teammates to kick off the build with one double-click.
4. Archive the `dist/` output when preparing releases, and keep the build logs produced in the console for later validation.

## 4. Verify the Packaged Build

After packaging, always exercise the installer in an environment that mimics an end-user machine:

1. Copy `dist/FreeCrafterInstaller.exe` to a **clean system** (VM, container, or freshly provisioned machine) without pre-installed Qt or build tools.
2. Launch the executable. The GUI should start without missing DLL errors and run through the bootstrap workflow.
3. Confirm the script downloads Qt, configures CMake, and builds FreeCrafter successfully on the clean system.
4. Locate the generated application (`build/FreeCrafter.exe`) and run it once to ensure Qt plugins load correctly.
5. Optionally install the build with `cmake --install build --prefix dist` to inspect the deployable artifacts.

## 5. Troubleshooting Common Failures

### Missing Qt DLLs or Plugins

**Symptoms:** When running the bootstrapper or the built application you see errors such as `Could not find the Qt platform plugin "windows"` or the process exits immediately.

**Detection:**
- The bootstrap scripts warn when Qt folders under `qt/6.5.3/` are missing.
- [`scripts/run_tests_with_qt_env.ps1`](../scripts/run_tests_with_qt_env.ps1) explicitly checks for `bin`, `plugins`, and `plugins/platforms` directories and aborts if they are missing.

**Fix:**
1. Rerun `python scripts/bootstrap.py` to reinstall Qt.
2. Ensure antivirus or disk cleaning tools are not deleting the cached Qt directory.
3. When distributing builds, keep the `platforms/` and other plugin folders alongside the executable or rely on the bootstrapper to populate them.

### `ctest` Not Found

**Symptoms:** Running tests or `cmake --build` fails with `ctest is not recognized` or similar messages.

**Detection:**
- Launching [`scripts/run_tests_with_qt_env.ps1`](../scripts/run_tests_with_qt_env.ps1) with `-UseCTest` performs a `Get-Command ctest` check and stops with a descriptive error if CTest is absent.
- Manual invocations of `ctest` from a non-developer shell will also fail.

**Fix:**
1. Use the **Developer Command Prompt for VS 2022** where CMake and CTest are preconfigured on `PATH`.
2. Alternatively, add the CMake `bin/` directory (e.g., `C:\Program Files\CMake\bin`) to your system `PATH`.
3. Re-run the tests via:

   ```powershell
   pwsh scripts/run_tests_with_qt_env.ps1 -UseCTest
   ```

### Visual C++ Redistributables Missing on Target Machines

**Symptoms:** Launching `FreeCrafter.exe` on a clean system results in runtime errors such as `VCRUNTIME140.dll was not found`.

**Detection:**
- Windows Event Viewer logs and the pop-up dialog will reference the missing MSVC runtime DLLs.
- Running the executable from `cmd.exe` will print the missing DLL name.

**Fix:**
1. Install the [Microsoft Visual C++ Redistributable for VS 2022](https://aka.ms/vs/17/release/vc_redist.x64.exe) on the target machine.
2. For automated validation, include the redistributable installer in your VM snapshot or provisioning script before testing user-facing builds.
3. Future releases can bundle the redistributable as part of the official installer—track this in the release checklist.

### Other Dependency Issues

- **Python package import errors (`ImportError: No module named aqtinstall`):** Run `pip install -r scripts/requirements.txt` in your active environment.
- **`mt.exe` not located:** Ensure the Windows SDK is installed and that `C:\Program Files (x86)\Windows Kits\10\bin\<version>\x64` is on `PATH`, or run `run_bootstrap.bat`, which sets `CMAKE_MT` explicitly.

## 6. Validation Checklist

Use this checklist before publishing an internal build or release candidate:

- [ ] **Bootstrap build passes locally** by running `python scripts/bootstrap.py` (or via the GUI wrapper).
- [ ] **Installer executable generated** using `python scripts/package_gui_bootstrap.py`, with logs captured for traceability.
- [ ] **Clean-system installation validated:** copy `dist/FreeCrafterInstaller.exe` to a fresh VM/host, run it end-to-end, and confirm the application launches.
- [ ] **Smoke test on first launch:** open FreeCrafter, load the default scene, interact with basic tools (orbit, pan, simple sketch) to ensure no immediate crashes.
- [ ] **Optional automated tests** executed with `pwsh scripts/run_tests_with_qt_env.ps1 -UseCTest` (requires CTest on `PATH`).
- [ ] **Visual C++ Redistributable** presence verified on the target machine (install if absent).
- [ ] **Artifacts archived:** store `dist/FreeCrafterInstaller.exe`, bootstrap logs, and smoke-test notes in your release folder.

Keeping this checklist in your release documentation ensures the "one-click installer" promise is verifiable and repeatable.
