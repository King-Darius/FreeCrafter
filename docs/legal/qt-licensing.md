# Qt Licensing Obligations

FreeCrafter's source code is published under the MIT License. When the application is built it links against the Qt 6 runtime, which is provided under the GNU Lesser General Public License version 3 (LGPLv3) unless a commercial Qt license is purchased. This document summarises the obligations that apply when redistributing FreeCrafter together with Qt under the LGPLv3 option.

## Key requirements when shipping the one-click installer

When the Windows installer produced by `scripts/package_build.ps1` or `scripts/package_gui_bootstrap.py` bundles Qt libraries, the distribution must meet the LGPLv3 terms:

- **Provide the Qt license text.** Include the full LGPLv3 licence and the Qt third-party notices in the installer payload and ensure they are displayed or readily accessible during/after installation.
- **Credit Qt.** Document that FreeCrafter uses the Qt framework (for example in the README, installer UI, or "About" dialog) and refer users to [https://www.qt.io](https://www.qt.io).
- **Allow relinking with modified Qt builds.** Ship the Qt dynamic libraries unmodified (DLLs), or if static linking is ever introduced ensure object files or another relinking mechanism is provided so users can replace Qt with their own build.
- **Offer source access.** Either provide a copy of the Qt source code that matches the bundled version or include a clear, written offer giving users instructions to obtain it (for example, a link to Qt's official download mirrors and the exact version used).
- **Track licence compatibility.** Ensure any additional Qt modules you enable are covered by the LGPL or another compatible licence. Avoid shipping GPL-only Qt add-ons unless FreeCrafter itself adopts a GPL-compatible licence.

## Repository guidance

- Keep the MIT `LICENSE` file to cover FreeCrafter's original code. No changes are required to the MIT text itself.
- Add the Qt licence text and third-party notices to the repository (for example under `docs/legal/`) so packaging scripts can copy them into the installer. This also gives developers a reference when auditing releases.
- Update release checklists to confirm the installer includes the licence documents and satisfies the obligations above.

For additional details, review the official [Qt Licensing FAQ](https://www.qt.io/faq/qt-licensing) and the [LGPLv3 text](https://www.gnu.org/licenses/lgpl-3.0.html). If FreeCrafter eventually adopts a commercial Qt licence, replace the steps above with the terms of that agreement.
