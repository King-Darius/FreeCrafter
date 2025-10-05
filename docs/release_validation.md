# Release Packaging Validation Checklist

This checklist captures the manual verification steps required before publishing
Windows builds of FreeCrafter. The flow focuses on validating that the packaged
Qt runtime runs on stock Windows systems with no developer toolchains
installed.

## 1. Build and Package on Windows

1. Launch an elevated **x64 Native Tools Command Prompt for VS 2019** (or newer).
2. Ensure the repository has been bootstrapped so `qt/6.5.3/msvc2019_64` exists.
3. Run the packaging helper to build, install, and produce both the NSIS
   installer and `.tgz` portable archive:

   ```powershell
   powershell -ExecutionPolicy Bypass -File scripts/package_build.ps1
   ```
4. Confirm the following artifacts are produced inside the `dist` directory:
   - `FreeCrafter-*-win64.exe` (NSIS installer)
   - `FreeCrafter-*-win64.tgz` (portable archive)

## 2. Validate on Clean Virtual Machines

Perform these checks on **two** pristine Windows VMs that do **not** contain
Visual Studio, Qt, or other developer runtimes:

1. Transfer the NSIS installer to VM A, run it, and launch FreeCrafter.
   - Verify that the application starts without missing DLL errors.
   - Exercise common UI workflows (open a project, place a block, toggle render
     modes) to ensure Qt plugins load correctly.
2. Transfer the `.tgz` archive to VM B, extract it (e.g., with 7-Zip), and run
   the unpacked `FreeCrafter.exe`.
   - Confirm the bundled Qt runtime works standalone without additional system
     dependencies.
3. Record any missing dependencies or runtime errors encountered. If issues
   appear, update the packaging script (`scripts/package_build.ps1`) or the
   CMake install rules to bundle the required assets, then rebuild the
   artifacts and repeat the validation on fresh VMs.

## 3. Document the Results

After successful validation, capture the following information in the release
notes or checklist for the upcoming tag:

- Date of validation and Windows build number of each VM.
- Hashes of the verified installer and archive.
- Summary of verification steps completed and any adjustments made to the
  packaging process.
- A note confirming that FreeCrafter launches without external Qt/Visual Studio
  dependencies on clean systems.

> **Note:** These instructions were authored from a non-Windows environment. If
> packaging cannot be performed locally, ensure a team member with Windows
> access executes the checklist prior to publishing releases.
