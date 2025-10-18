# Qt LGPL Compliance Notes

FreeCrafter is distributed under the MIT License, but it dynamically links against Qt 6 libraries that are provided under the GNU Lesser General Public License version 3 (LGPLv3) or later. This document summarizes the obligations that apply when shipping FreeCrafter builds that bundle Qt.

## Key obligations when redistributing Qt

- **Preserve the Qt license texts.** Every binary distribution must include the full LGPLv3 text and the Qt-specific notices that accompany the modules you ship. If you are deploying Qt with the bootstrap scripts, retain the `LICENSE.LGPLv3`, `LICENSE.GPL3-EXCEPT`, and `LGPL_EXCEPTION.txt` files that ship in Qt's `Licenses/` directory.
- **Provide access to the Qt sources.** Either ship the unmodified Qt source code alongside your binaries, or provide a clear, written offer that explains how recipients can obtain the sources (for example, a URL pointing to Qt's official downloads).
- **Allow reverse engineering for debugging.** You must not forbid recipients from reverse engineering the Qt libraries for the purposes permitted by the LGPL.
- **Relinkability.** Because FreeCrafter uses Qt via dynamic libraries, users already have the ability to replace Qt with their own builds. Do not statically link Qt unless you are prepared to comply with the additional requirements (e.g., object file distribution).
- **Document your Qt version and modifications.** Note the exact Qt version and any patches you apply so downstream users know which sources they need.

## Recommended repository practices

- Keep the project-level MIT license in `LICENSE` and point to this file from documentation.
- Reference this compliance note (or a similar third-party notices file) from the README so distributors know about the Qt obligations.
- Track additional third-party libraries in `docs/legal/third_party_notices.md` as they are added to the project.
- Keep `qt/manifest.json` up to date when the Qt version or module list changes so downstream users know precisely which runtime components were shipped.

These steps ensure FreeCrafter's MIT-licensed code and the LGPL-licensed Qt runtime remain compliant when shipped together.

## Repository policy on Qt binaries

To keep the repository lean and avoid accidentally versioning personal build artifacts, we do **not** check Qt DLLs or other redistributable binaries into `main`.  Instead, automated packaging (see `docs/process/ci_cd_release_actions.md`) runs `windeployqt` during `cmake --install` so release artifacts include every required runtime along with the Qt license texts copied from the official distribution.  Developers who need a runnable tree locally should invoke `python scripts/bootstrap.py`, which stages the same DLL set under `dist/` without committing it to source control.
