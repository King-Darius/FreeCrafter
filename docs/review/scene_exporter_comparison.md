# Scene Export/Import PR Comparison

## Reference commits
- **Baseline (before PR):** `f849d17` – Merge pull request #63 from King-Darius/codex/locate-phase-6-details`
- **Current branch head:** `c4a449e` – Add scene export/import pipeline and regression coverage

## High-level deltas
- Introduces a new FileIO exporter/importer subsystem (`src/FileIO/`) with 1,257 lines of implementation covering OBJ/STL/glTF plus stubs for optional FBX/DAE/SKP support.
- Extends `GeometryKernel` helpers and public API to expose indexed triangle buffers, material lookup tables, and transform extraction utilities needed by the exporters.
- Expands `MainWindow::exportFile` flow with a multi-format save dialog and status reporting, wiring it into the new exporter layer.
- Adds regression tests under `tests/file_io/` that attempt to round-trip representative scenes through the exporters/importers.
- Updates build configuration (`CMakeLists.txt`) and roadmap/legal docs to account for the new file I/O capabilities and third-party licensing notes.

## File-level summary
- `CMakeLists.txt`: Adds include directories and source lists for the new FileIO components.
- `docs/legal/notices.md`: Documents optional SDK obligations for SketchUp/ODA integrations.
- `src/FileIO/SceneIOFormat.*`: Defines enumerations, descriptors, and feature-detection helpers for supported export formats.
- `src/FileIO/Exporters/SceneExporter.*`: Implements the exporter facade that walks `Scene::Document`, tessellates geometry, and emits meshes via Assimp or tinygltf.
- `src/FileIO/Importers/SceneImporter.*`: Adds importer paths to reload exported assets back into the scene graph for regression testing.
- `src/GeometryKernel/GeometryKernel.*`: Provides new mesh/transform utilities leveraged by the exporter/importer flows while retaining existing `.fcm` serialization.
- `src/MainWindow.cpp`: Hooks the UI export command to the new exporter layer and surfaces results in the status bar.
- `tests/file_io/test_exporters.cpp`: Adds automated round-trip regression coverage for OBJ/STL/glTF exports.

## Potential review considerations
- The new exporter/importer implementations contribute ~1.1k lines each; reviewers may wish to validate their alignment with existing architecture and performance expectations.
- Export regression tests depend on the importer functionality introduced here; double-check coverage scope and test data readiness.
- Build/test execution still requires Qt6; CI/packaging updates may be necessary if the environment lacks the Qt toolchain.

## Conclusion
The current branch carries substantial new functionality relative to the baseline. If these capabilities already landed elsewhere or the feature set is redundant, closing the PR would revert to commit `f849d17` without the exporter/importer subsystem or associated documentation/tests.
