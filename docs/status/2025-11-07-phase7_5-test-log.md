# Phase 7.5 Test Loop – 2025-11-07

## Environment
- Qt bootstrap via scripts/bootstrap.py (fails: ProxyError)
- System Qt packages detected automatically (6.4.x from Ubuntu repositories)

## Result
- Bootstrap download fails due to corporate proxy.
- Manual cmake configure succeeds using system Qt but build fails with extensive compile errors (SceneCommands, MainWindow, etc.).
- ctest cannot run (build artifacts missing).

## Transcript
```text
Script started on 2025-11-07 18:10:33+00:00 [TERM="xterm" COLUMNS="128" LINES="-1"]
# python3 scripts/bootstrap.py
Cache folder: /root/.local/share/aqt/cache
Temp folder: /root/.local/share/aqt/tmp
Requesting Qt modules: qtbase, qtimageformats, qttools, qtsvg, qttranslations
Running: /root/.pyenv/versions/3.12.10/bin/python3 -m aqtinstall qt linux desktop 6.5.3 gcc_64 -O /workspace/FreeCrafter/qt --modules qtbase qtimageformats qttools qtsvg qttranslations
INFO    : aqtinstall(aqt) v3.3.0 on Python 3.12.10 [CPython GCC 13.3.0]
WARNING : Failed to download checksum for the file 'online/qtsdkrepository/linux_x64/desktop/qt6_653/Updates.xml'. This may happen on unofficial mirrors.
WARNING : Connection to 'https://download.qt.io' failed. Retrying with fallback 'https://ftp.yz.yamagata-u.ac.jp/pub/qtproject/'.
WARNING : Failed to download checksum for the file 'online/qtsdkrepository/linux_x64/desktop/qt6_653/Updates.xml'. This may happen on unofficial mirrors.
ERROR   : Failure to connect to https://ftp.yz.yamagata-u.ac.jp/pub/qtproject/online/qtsdkrepository/linux_x64/desktop/qt6_653/Updates.xml: ProxyError
Qt installation failed
# cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
-- Could NOT find XKB (missing: XKB_LIBRARY XKB_INCLUDE_DIR) (Required is at least version "0.5.0")
-- Configuring done (0.2s)
-- Generating done (0.1s)
-- Build files have been written to: /workspace/FreeCrafter/build
# cmake --build build -j
[  0%] Built target freecrafter_lib_autogen_timestamp_deps
[  1%] [34m[1mAutomatic MOC and UIC for target freecrafter_lib[0m
[  1%] Built target freecrafter_lib_autogen
[  2%] [34m[1mAutomatic RCC for resources.qrc[0m
[  3%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/MainWindow.cpp.o[0m
[  4%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/freecrafter_lib_autogen/mocs_compilation.cpp.o[0m
[  4%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/GLViewport.cpp.o[0m
[  5%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/Renderer.cpp.o[0m
[  6%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/SunSettings.cpp.o[0m
[  7%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/SunModel.cpp.o[0m
[  8%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/PalettePreferences.cpp.o[0m
[  8%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/CameraController.cpp.o[0m
[  9%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/CameraNavigation.cpp.o[0m
[ 10%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/Core/Command.cpp.o[0m
[ 11%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/HotkeyManager.cpp.o[0m
[ 12%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/Navigation/ViewPresetManager.cpp.o[0m
[ 13%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/GeometryKernel/HalfEdgeMesh.cpp.o[0m
[ 14%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/Core/CommandStack.cpp.o[0m
[ 15%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/GeometryKernel/GeometryKernel.cpp.o[0m
[ 15%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/NavigationPreferences.cpp.o[0m
[ 15%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/app/AutosaveManager.cpp.o[0m
[ 17%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/GeometryKernel/Solid.cpp.o[0m
[ 18%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/GeometryKernel/Curve.cpp.o[0m
[ 17%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/GeometryKernel/Serialization.cpp.o[0m
[ 18%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/GeometryKernel/ShapeBuilder.cpp.o[0m
[ 19%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/Scene/Document.cpp.o[0m
[ 19%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/GeometryKernel/MeshUtils.cpp.o[0m
[ 20%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/GeometryKernel/TransformUtils.cpp.o[0m
[ 20%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/Scene/SceneSerializer.cpp.o[0m
[ 21%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/Scene/PrimitiveBuilder.cpp.o[0m
[ 22%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/Scene/SceneCommands.cpp.o[0m
[ 23%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/Scene/SceneSettings.cpp.o[0m
[ 25%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/FileIO/Importers/FileImporter.cpp.o[0m
[ 25%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/Tools/Tool.cpp.o[0m
[ 26%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/Tools/SmartSelectTool.cpp.o[0m
[ 27%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/Tools/ToolGeometryUtils.cpp.o[0m
[ 27%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/Scene/SectionPlane.cpp.o[0m
[ 27%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/Tools/LineTool.cpp.o[0m
[ 28%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/Tools/MoveTool.cpp.o[0m
[ 29%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/Tools/ScaleTool.cpp.o[0m
[ 30%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/Tools/ExtrudeTool.cpp.o[0m
[ 31%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/Tools/ChamferTool.cpp.o[0m
[ 31%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/Tools/RotateTool.cpp.o[0m
[ 31%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/Tools/LoftTool.cpp.o[0m
[ 32%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/Tools/GroundProjection.cpp.o[0m
[ 33%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/Tools/SectionTool.cpp.o[0m
[ 34%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/Tools/ToolCommands.cpp.o[0m
[ 34%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/Tools/ModificationTools.cpp.o[0m
[ 35%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/Tools/OrbitTool.cpp.o[0m
[ 36%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/Tools/DrawingTools.cpp.o[0m
[ 37%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/Tools/ToolManager.cpp.o[0m
[ 37%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/Tools/PanTool.cpp.o[0m
[ 37%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/Phase6/AdvancedModeling.cpp.o[0m
[ 38%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/FileIO/SceneIOFormat.cpp.o[0m
[ 39%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/Tools/ZoomTool.cpp.o[0m
[ 40%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/Interaction/InferenceEngine.cpp.o[0m
[ 41%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/FileIO/Exporters/SceneExporter.cpp.o[0m
[ 42%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/Tools/ToolRegistry.cpp.o[0m
[ 42%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/FileIO/Importers/SceneImporter.cpp.o[0m
[ 43%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/ui/CommandPaletteDialog.cpp.o[0m
[ 44%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/ui/MeasurementWidget.cpp.o[0m
[ 44%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/ui/InspectorPanel.cpp.o[0m
[ 45%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/ui/ChamferOptionsDialog.cpp.o[0m
[ 46%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/ui/ViewSettingsDialog.cpp.o[0m
[ 47%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/ui/LoftOptionsDialog.cpp.o[0m
[ 47%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/ui/EnvironmentPanel.cpp.o[0m
[ 48%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/ui/GuideManagerDialog.cpp.o[0m
[ 49%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/ui/PluginManagerDialog.cpp.o[0m
[ 50%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/ui/InsertShapeDialog.cpp.o[0m
[ 51%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/ui/LeftToolPalette.cpp.o[0m
[ 52%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/ui/ImageImportDialog.cpp.o[0m
[ 52%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/ui/ExternalReferenceDialog.cpp.o[0m
[ 52%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/ui/OutlinerPanel.cpp.o[0m
[ 53%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/ui/CollapsibleSection.cpp.o[0m
[ 54%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/ui/TerminalDock.cpp.o[0m
[ 55%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/ui/MaterialsPanel.cpp.o[0m
[ 56%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/ui/TagsPanel.cpp.o[0m
[ 56%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/ui/HistoryPanel.cpp.o[0m
[ 56%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/Scene/SceneGraphCommands.cpp.o[0m
[ 57%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/ui/RightTray.cpp.o[0m
[ 58%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/freecrafter_lib_autogen/EWIEGA46WW/qrc_resources.cpp.o[0m
[ 59%] [32mBuilding CXX object CMakeFiles/freecrafter_lib.dir/src/ui/ViewportOverlay.cpp.o[0m
[01m[K/workspace/FreeCrafter/src/GeometryKernel/Curve.cpp:[m[K In function ‘[01m[Kstd::unique_ptr<Curve> {anonymous}::[01;32m[KmakePolyline[m[K(std::vector<Vector3>&&, const std::vector<bool>&)[m[K’:
[01m[K/workspace/FreeCrafter/src/GeometryKernel/Curve.cpp:61:102:[m[K [01;31m[Kerror: [m[K‘[01m[KCurve::[01;32m[KCurve[m[K(std::vector<Vector3>, HalfEdgeMesh, std::vector<bool>)[m[K’ is private within this context
   61 |     return std::unique_ptr<Curve>(new Curve(std::move(boundary), std::move(mesh), std::move(hardness)[01;31m[K)[m[K);
      |                                                                                                      [01;31m[K^[m[K
In file included from [01m[K/workspace/FreeCrafter/src/GeometryKernel/Curve.cpp:1[m[K:
[01m[K/workspace/FreeCrafter/src/GeometryKernel/Curve.h:31:5:[m[K [01;36m[Knote: [m[Kdeclared private here
   31 |     [01;36m[KCurve[m[K(std::vector<Vector3> loop, HalfEdgeMesh mesh, std::vector<bool> hardness);
      |     [01;36m[K^~~~~[m[K
gmake[2]: *** [CMakeFiles/freecrafter_lib.dir/build.make:356: CMakeFiles/freecrafter_lib.dir/src/GeometryKernel/Curve.cpp.o] Error 1
gmake[2]: *** Waiting for unfinished jobs....
[01m[K/workspace/FreeCrafter/src/SunModel.cpp:54:42:[m[K [01;31m[Kerror: [m[K‘[01m[KhourAngleRad[m[K’ was not declared in this scope
   54 |     double azimuth = std::atan2(std::sin([01;31m[KhourAngleRad[m[K), std::cos(hourAngleRad) * std::sin(latitudeRad) - std::tan(sunDeclination) * std::cos(latitudeRad));
      |                                          [01;31m[K^~~~~~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/SunModel.cpp:54:66:[m[K [01;31m[Kerror: [m[K‘[01m[KhourAngleRad[m[K’ was not declared in this scope
   54 |     double azimuth = std::atan2(std::sin(hourAngleRad), std::cos([01;31m[KhourAngleRad[m[K) * std::sin(latitudeRad) - std::tan(sunDeclination) * std::cos(latitudeRad));
      |                                                                  [01;31m[K^~~~~~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/SunModel.cpp:54:91:[m[K [01;31m[Kerror: [m[K‘[01m[KlatitudeRad[m[K’ was not declared in this scope
   54 |     double azimuth = std::atan2(std::sin(hourAngleRad), std::cos(hourAngleRad) * std::sin([01;31m[KlatitudeRad[m[K) - std::tan(sunDeclination) * std::cos(latitudeRad));
      |                                                                                           [01;31m[K^~~~~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/SunModel.cpp:54:115:[m[K [01;31m[Kerror: [m[K‘[01m[KsunDeclination[m[K’ was not declared in this scope
   54 | uble azimuth = std::atan2(std::sin(hourAngleRad), std::cos(hourAngleRad) * std::sin(latitudeRad) - std::tan([01;31m[KsunDeclination[m[K) * std::cos(latitudeRad));
      |                                                                                                             [01;31m[K^~~~~~~~~~~~~~[m[K

[01m[K/workspace/FreeCrafter/src/SunModel.cpp:54:142:[m[K [01;31m[Kerror: [m[K‘[01m[KlatitudeRad[m[K’ was not declared in this scope
   54 | td::sin(hourAngleRad), std::cos(hourAngleRad) * std::sin(latitudeRad) - std::tan(sunDeclination) * std::cos([01;31m[KlatitudeRad[m[K));
      |                                                                                                             [01;31m[K^~~~~~~~~~~[m[K

[01m[K/workspace/FreeCrafter/src/SunModel.cpp:58:25:[m[K [01;31m[Kerror: [m[K‘[01m[KradToDeg[m[K’ was not declared in this scope
   58 |     double azimuthDeg = [01;31m[KradToDeg[m[K(azimuth) + 180.0;
      |                         [01;31m[K^~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/SunModel.cpp:62:5:[m[K [01;31m[Kerror: [m[Kexpected unqualified-id before ‘[01m[Kif[m[K’
   62 |     [01;31m[Kif[m[K (azimuthDeg < 0.0)
      |     [01;31m[K^~[m[K
[01m[K/workspace/FreeCrafter/src/SunModel.cpp:70:5:[m[K [01;31m[Kerror: [m[Kexpected unqualified-id before ‘[01m[Kif[m[K’
   70 |     [01;31m[Kif[m[K (azimuthDeg >= 360.0)
      |     [01;31m[K^~[m[K
[01m[K/workspace/FreeCrafter/src/SunModel.cpp:82:5:[m[K [01;31m[Kerror: [m[K‘[01m[Kresult[m[K’ does not name a type
   82 |     [01;31m[Kresult[m[K.altitudeDegrees = static_cast<float>(radToDeg(altitude));
      |     [01;31m[K^~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/SunModel.cpp:86:5:[m[K [01;31m[Kerror: [m[K‘[01m[Kresult[m[K’ does not name a type
   86 |     [01;31m[Kresult[m[K.azimuthDegrees = static_cast<float>(azimuthDeg);
      |     [01;31m[K^~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/SunModel.cpp:94:5:[m[K [01;31m[Kerror: [m[Kexpected unqualified-id before ‘[01m[Kif[m[K’
   94 |     [01;31m[Kif[m[K (result.altitudeDegrees <= -0.5f) {
      |     [01;31m[K^~[m[K
[01m[K/workspace/FreeCrafter/src/SunModel.cpp:114:41:[m[K [01;31m[Kerror: [m[K‘[01m[Kresult[m[K’ was not declared in this scope
  114 |     const double altitudeRad = degToRad([01;31m[Kresult[m[K.altitudeDegrees);
      |                                         [01;31m[K^~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/SunModel.cpp:114:32:[m[K [01;31m[Kerror: [m[K‘[01m[KdegToRad[m[K’ was not declared in this scope
  114 |     const double altitudeRad = [01;31m[KdegToRad[m[K(result.altitudeDegrees);
      |                                [01;31m[K^~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/SunModel.cpp:118:40:[m[K [01;31m[Kerror: [m[K‘[01m[Kresult[m[K’ was not declared in this scope
  118 |     const double azimuthRad = degToRad([01;31m[Kresult[m[K.azimuthDegrees);
      |                                        [01;31m[K^~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/SunModel.cpp:118:31:[m[K [01;31m[Kerror: [m[K‘[01m[KdegToRad[m[K’ was not declared in this scope
  118 |     const double azimuthRad = [01;31m[KdegToRad[m[K(result.azimuthDegrees);
      |                               [01;31m[K^~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/SunModel.cpp:146:5:[m[K [01;31m[Kerror: [m[K‘[01m[Kresult[m[K’ does not name a type
  146 |     [01;31m[Kresult[m[K.direction = QVector3D(static_cast<float>(east), static_cast<float>(up), static_cast<float>(north));
      |     [01;31m[K^~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/SunModel.cpp:150:5:[m[K [01;31m[Kerror: [m[Kexpected unqualified-id before ‘[01m[Kif[m[K’
  150 |     [01;31m[Kif[m[K (!result.direction.isNull()) {
      |     [01;31m[K^~[m[K
[01m[K/workspace/FreeCrafter/src/SunModel.cpp:170:5:[m[K [01;31m[Kerror: [m[Kexpected unqualified-id before ‘[01m[Kreturn[m[K’
  170 |     [01;31m[Kreturn[m[K result;
      |     [01;31m[K^~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/SunModel.cpp:174:1:[m[K [01;31m[Kerror: [m[Kexpected declaration before ‘[01m[K}[m[K’ token
  174 | [01;31m[K}[m[K
      | [01;31m[K^[m[K
In file included from [01m[K/workspace/FreeCrafter/src/Tools/MoveTool.cpp:1[m[K:
[01m[K/workspace/FreeCrafter/src/Tools/MoveTool.h:29:34:[m[K [01;31m[Kerror: [m[Kincomplete type ‘[01m[KScene::Document[m[K’ used in nested name specifier
   29 |     std::vector<Scene::Document::[01;31m[KObjectId[m[K> selectionIds() const;
      |                                  [01;31m[K^~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Tools/MoveTool.h:29:42:[m[K [01;31m[Kerror: [m[Ktemplate argument 1 is invalid
   29 |     std::vector<Scene::Document::ObjectId[01;31m[K>[m[K selectionIds() const;
      |                                          [01;31m[K^[m[K
[01m[K/workspace/FreeCrafter/src/Tools/MoveTool.h:29:42:[m[K [01;31m[Kerror: [m[Ktemplate argument 2 is invalid
gmake[2]: *** [CMakeFiles/freecrafter_lib.dir/build.make:202: CMakeFiles/freecrafter_lib.dir/src/SunModel.cpp.o] Error 1
In file included from [01m[K/workspace/FreeCrafter/src/Tools/RotateTool.cpp:1[m[K:
[01m[K/workspace/FreeCrafter/src/Tools/RotateTool.h:29:34:[m[K [01;31m[Kerror: [m[Kincomplete type ‘[01m[KScene::Document[m[K’ used in nested name specifier
   29 |     std::vector<Scene::Document::[01;31m[KObjectId[m[K> selectionIds() const;
      |                                  [01;31m[K^~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Tools/RotateTool.h:29:42:[m[K [01;31m[Kerror: [m[Ktemplate argument 1 is invalid
   29 |     std::vector<Scene::Document::ObjectId[01;31m[K>[m[K selectionIds() const;
      |                                          [01;31m[K^[m[K
[01m[K/workspace/FreeCrafter/src/Tools/RotateTool.h:29:42:[m[K [01;31m[Kerror: [m[Ktemplate argument 2 is invalid
In file included from [01m[K/workspace/FreeCrafter/src/Tools/ScaleTool.cpp:1[m[K:
[01m[K/workspace/FreeCrafter/src/Tools/ScaleTool.h:29:34:[m[K [01;31m[Kerror: [m[Kincomplete type ‘[01m[KScene::Document[m[K’ used in nested name specifier
   29 |     std::vector<Scene::Document::[01;31m[KObjectId[m[K> selectionIds() const;
      |                                  [01;31m[K^~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Tools/ScaleTool.h:29:42:[m[K [01;31m[Kerror: [m[Ktemplate argument 1 is invalid
   29 |     std::vector<Scene::Document::ObjectId[01;31m[K>[m[K selectionIds() const;
      |                                          [01;31m[K^[m[K
[01m[K/workspace/FreeCrafter/src/Tools/ScaleTool.h:29:42:[m[K [01;31m[Kerror: [m[Ktemplate argument 2 is invalid
[01m[K/workspace/FreeCrafter/src/Tools/ToolManager.cpp:[m[K In constructor ‘[01m[KToolManager::[01;32m[KToolManager[m[K(Scene::Document*, CameraController*, Core::CommandStack*)[m[K’:
[01m[K/workspace/FreeCrafter/src/Tools/ToolManager.cpp:37:26:[m[K [01;31m[Kerror: [m[Kinvalid use of incomplete type ‘[01m[Kclass Scene::Document[m[K’
   37 |     : geometry(doc ? &doc[01;31m[K->[m[Kgeometry() : nullptr)
      |                          [01;31m[K^~[m[K
In file included from [01m[K/workspace/FreeCrafter/src/Tools/ToolManager.h:8[m[K,
                 from [01m[K/workspace/FreeCrafter/src/Tools/ToolManager.cpp:1[m[K:
[01m[K/workspace/FreeCrafter/src/Tools/Tool.h:17:7:[m[K [01;36m[Knote: [m[Kforward declaration of ‘[01m[Kclass Scene::Document[m[K’
   17 | class [01;36m[KDocument[m[K;
      |       [01;36m[K^~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Tools/ToolManager.cpp:[m[K In member function ‘[01m[Kvoid ToolManager::[01;32m[KnotifyExternalGeometryChange[m[K()[m[K’:
[01m[K/workspace/FreeCrafter/src/Tools/ToolManager.cpp:417:17:[m[K [01;31m[Kerror: [m[Kinvalid use of incomplete type ‘[01m[Kclass Scene::Document[m[K’
  417 |         document[01;31m[K->[m[KsynchronizeWithGeometry();
      |                 [01;31m[K^~[m[K
[01m[K/workspace/FreeCrafter/src/Tools/Tool.h:17:7:[m[K [01;36m[Knote: [m[Kforward declaration of ‘[01m[Kclass Scene::Document[m[K’
   17 | class [01;36m[KDocument[m[K;
      |       [01;36m[K^~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Tools/ToolManager.cpp:[m[K In member function ‘[01m[Kvoid ToolManager::[01;32m[KhandleToolInteraction[m[K()[m[K’:
[01m[K/workspace/FreeCrafter/src/Tools/ToolManager.cpp:590:17:[m[K [01;31m[Kerror: [m[Kinvalid use of incomplete type ‘[01m[Kclass Scene::Document[m[K’
  590 |         document[01;31m[K->[m[KsynchronizeWithGeometry();
      |                 [01;31m[K^~[m[K
[01m[K/workspace/FreeCrafter/src/Tools/Tool.h:17:7:[m[K [01;36m[Knote: [m[Kforward declaration of ‘[01m[Kclass Scene::Document[m[K’
   17 | class [01;36m[KDocument[m[K;
      |       [01;36m[K^~~~~~~~[m[K
gmake[2]: *** [CMakeFiles/freecrafter_lib.dir/build.make:832: CMakeFiles/freecrafter_lib.dir/src/Tools/ToolManager.cpp.o] Error 1
[01m[K/workspace/FreeCrafter/src/Tools/MoveTool.cpp:[m[K In member function ‘[01m[Kvirtual void MoveTool::[01;32m[KonCommit[m[K()[m[K’:
[01m[K/workspace/FreeCrafter/src/Tools/MoveTool.cpp:93:18:[m[K [01;31m[Kerror: [m[Krequest for member ‘[01m[Kempty[m[K’ in ‘[01m[Kids[m[K’, which is of non-class type ‘[01m[Kint[m[K’
   93 |         if (!ids.[01;31m[Kempty[m[K()) {
      |                  [01;31m[K^~~~~[m[K
In file included from [01m[K/workspace/FreeCrafter/src/Scene/SceneCommands.cpp:1[m[K:
[01m[K/workspace/FreeCrafter/src/Scene/SceneCommands.h:17:5:[m[K [01;31m[Kerror: [m[K‘[01m[KObjectId[m[K’ does not name a type
   17 |     [01;31m[KObjectId[m[K objectId() const { return currentObjectId; }
      |     [01;31m[K^~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Scene/SceneCommands.h:26:5:[m[K [01;31m[Kerror: [m[K‘[01m[KObjectId[m[K’ does not name a type
   26 |     [01;31m[KObjectId[m[K currentObjectId = 0;
      |     [01;31m[K^~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Scene/SceneCommands.h:61:5:[m[K [01;31m[Kerror: [m[K‘[01m[KObjectId[m[K’ does not name a type
   61 |     [01;31m[KObjectId[m[K objectId() const { return currentObjectId; }
      |     [01;31m[K^~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Scene/SceneCommands.h:70:5:[m[K [01;31m[Kerror: [m[K‘[01m[KObjectId[m[K’ does not name a type
   70 |     [01;31m[KObjectId[m[K currentObjectId = 0;
      |     [01;31m[K^~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Tools/MoveTool.cpp:95:18:[m[K [01;31m[Kerror: [m[Kinvalid use of incomplete type ‘[01m[Kclass Core::CommandStack[m[K’
   95 |             stack[01;31m[K->[m[Kpush(std::move(command));
      |                  [01;31m[K^~[m[K
In file included from [01m[K/workspace/FreeCrafter/src/Tools/MoveTool.h:3[m[K:
[01m[K/workspace/FreeCrafter/src/Tools/Tool.h:13:7:[m[K [01;36m[Knote: [m[Kforward declaration of ‘[01m[Kclass Core::CommandStack[m[K’
   13 | class [01;36m[KCommandStack[m[K;
      |       [01;36m[K^~~~~~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Scene/SceneCommands.h:87:5:[m[K [01;31m[Kerror: [m[K‘[01m[KObjectId[m[K’ does not name a type
   87 |     [01;31m[KObjectId[m[K objectId() const { return currentObjectId; }
      |     [01;31m[K^~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Scene/SceneCommands.h:96:5:[m[K [01;31m[Kerror: [m[K‘[01m[KObjectId[m[K’ does not name a type
   96 |     [01;31m[KObjectId[m[K currentObjectId = 0;
      |     [01;31m[K^~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Tools/MoveTool.cpp:[m[K At global scope:
[01m[K/workspace/FreeCrafter/src/Tools/MoveTool.cpp:209:40:[m[K [01;31m[Kerror: [m[Kno declaration matches ‘[01m[Kstd::vector<long unsigned int> MoveTool::[01;32m[KselectionIds[m[K() const[m[K’
  209 | std::vector<Scene::Document::ObjectId> [01;31m[KMoveTool[m[K::selectionIds() const
      |                                        [01;31m[K^~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Tools/MoveTool.h:29:44:[m[K [01;36m[Knote: [m[Kcandidate is: ‘[01m[Kint MoveTool::[01;32m[KselectionIds[m[K() const[m[K’
   29 |     std::vector<Scene::Document::ObjectId> [01;36m[KselectionIds[m[K() const;
      |                                            [01;36m[K^~~~~~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Tools/MoveTool.h:7:7:[m[K [01;36m[Knote: [m[K‘[01m[Kclass MoveTool[m[K’ defined here
    7 | class [01;36m[KMoveTool[m[K : public Tool {
      |       [01;36m[K^~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Tools/SmartSelectTool.cpp:[m[K In member function ‘[01m[Kvirtual void SmartSelectTool::[01;32m[KonKeyDown[m[K(int)[m[K’:
[01m[K/workspace/FreeCrafter/src/Tools/SmartSelectTool.cpp:129:31:[m[K [01;31m[Kerror: [m[Kqualified-id in declaration before ‘[01m[K([m[K’ token
  129 | void SmartSelectTool::onCancel[01;31m[K([m[K)
      |                               [01;31m[K^[m[K
[01m[K/workspace/FreeCrafter/src/Tools/SmartSelectTool.cpp:137:37:[m[K [01;31m[Kerror: [m[Kqualified-id in declaration before ‘[01m[K([m[K’ token
  137 | void SmartSelectTool::onStateChanged[01;31m[K([m[KState previous, State next)
      |                                     [01;31m[K^[m[K
[01m[K/workspace/FreeCrafter/src/Tools/SmartSelectTool.cpp:146:49:[m[K [01;31m[Kerror: [m[Kqualified-id in declaration before ‘[01m[K([m[K’ token
  146 | Tool::PreviewState SmartSelectTool::buildPreview[01;31m[K([m[K) const
      |                                                 [01;31m[K^[m[K
[01m[K/workspace/FreeCrafter/src/Tools/SmartSelectTool.cpp:161:46:[m[K [01;31m[Kerror: [m[Kqualified-id in declaration before ‘[01m[K([m[K’ token
  161 | GeometryObject* SmartSelectTool::pickObjectAt[01;31m[K([m[Kconst Vector3& worldPoint)
      |                                              [01;31m[K^[m[K
[01m[K/workspace/FreeCrafter/src/Tools/SmartSelectTool.cpp:201:37:[m[K [01;31m[Kerror: [m[Kqualified-id in declaration before ‘[01m[K([m[K’ token
  201 | bool SmartSelectTool::pointerToWorld[01;31m[K([m[Kconst PointerInput& input, Vector3& out) const
      |                                     [01;31m[K^[m[K
[01m[K/workspace/FreeCrafter/src/Tools/SmartSelectTool.cpp:216:37:[m[K [01;31m[Kerror: [m[Kqualified-id in declaration before ‘[01m[K([m[K’ token
  216 | void SmartSelectTool::applySelection[01;31m[K([m[Kconst std::vector<GeometryObject*>& hits, bool additive, bool toggle)
      |                                     [01;31m[K^[m[K
[01m[K/workspace/FreeCrafter/src/Tools/SmartSelectTool.cpp:235:35:[m[K [01;31m[Kerror: [m[Kqualified-id in declaration before ‘[01m[K([m[K’ token
  235 | void SmartSelectTool::selectSingle[01;31m[K([m[Kconst PointerInput& input)
      |                                   [01;31m[K^[m[K
[01m[K/workspace/FreeCrafter/src/Tools/SmartSelectTool.cpp:252:40:[m[K [01;31m[Kerror: [m[Kqualified-id in declaration before ‘[01m[K([m[K’ token
  252 | void SmartSelectTool::selectByRectangle[01;31m[K([m[Kconst PointerInput& input)
      |                                        [01;31m[K^[m[K
[01m[K/workspace/FreeCrafter/src/Tools/SmartSelectTool.cpp:279:37:[m[K [01;31m[Kerror: [m[Kqualified-id in declaration before ‘[01m[K([m[K’ token
  279 | void SmartSelectTool::clearSelection[01;31m[K([m[K)
      |                                     [01;31m[K^[m[K
[01m[K/workspace/FreeCrafter/src/Tools/SmartSelectTool.cpp:309:12:[m[K [01;31m[Kerror: [m[Kreturn-statement with a value, in function returning ‘[01m[Kvoid[m[K’ [[01;31m[K-fpermissive[m[K]
  309 |     return [01;31m[Kstate[m[K;
      |            [01;31m[K^~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Tools/RotateTool.cpp:[m[K In member function ‘[01m[Kvirtual void RotateTool::[01;32m[KonCommit[m[K()[m[K’:
[01m[K/workspace/FreeCrafter/src/Tools/RotateTool.cpp:152:74:[m[K [01;31m[Kerror: [m[K‘[01m[Kids[m[K’ was not declared in this scope
  152 |             auto command = std::make_unique<Tools::RotateObjectsCommand>([01;31m[Kids[m[K, pivot, axis, currentAngle,
      |                                                                          [01;31m[K^~~[m[K
[01m[K/workspace/FreeCrafter/src/Tools/RotateTool.cpp:155:44:[m[K [01;31m[Kerror: [m[Kqualified-id in declaration before ‘[01m[K([m[K’ token
  155 | Tool::PreviewState RotateTool::buildPreview[01;31m[K([m[K) const
      |                                            [01;31m[K^[m[K
[01m[K/workspace/FreeCrafter/src/Tools/RotateTool.cpp:175:32:[m[K [01;31m[Kerror: [m[Kqualified-id in declaration before ‘[01m[K([m[K’ token
  175 | bool RotateTool::pointerToWorld[01;31m[K([m[Kconst PointerInput& input, Vector3& out) const
      |                                [01;31m[K^[m[K
[01m[K/workspace/FreeCrafter/src/Tools/RotateTool.cpp:187:57:[m[K [01;31m[Kerror: [m[Kqualified-id in declaration before ‘[01m[K([m[K’ token
  187 | std::vector<GeometryObject*> RotateTool::gatherSelection[01;31m[K([m[K) const
      |                                                         [01;31m[K^[m[K
[01m[K/workspace/FreeCrafter/src/Tools/RotateTool.cpp:190:16:[m[K [01;31m[Kerror: [m[K‘[01m[Kresult[m[K’ was not declared in this scope
  190 |         return [01;31m[Kresult[m[K;
      |                [01;31m[K^~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Tools/ScaleTool.cpp:[m[K In member function ‘[01m[Kvirtual void ScaleTool::[01;32m[KonCommit[m[K()[m[K’:
[01m[K/workspace/FreeCrafter/src/Tools/ScaleTool.cpp:180:43:[m[K [01;31m[Kerror: [m[Kqualified-id in declaration before ‘[01m[K([m[K’ token
  180 | Tool::PreviewState ScaleTool::buildPreview[01;31m[K([m[K) const
      |                                           [01;31m[K^[m[K
[01m[K/workspace/FreeCrafter/src/Tools/ScaleTool.cpp:199:31:[m[K [01;31m[Kerror: [m[Kqualified-id in declaration before ‘[01m[K([m[K’ token
  199 | bool ScaleTool::pointerToWorld[01;31m[K([m[Kconst PointerInput& input, Vector3& out) const
      |                               [01;31m[K^[m[K
[01m[K/workspace/FreeCrafter/src/Tools/ScaleTool.cpp:211:56:[m[K [01;31m[Kerror: [m[Kqualified-id in declaration before ‘[01m[K([m[K’ token
  211 | std::vector<GeometryObject*> ScaleTool::gatherSelection[01;31m[K([m[K) const
      |                                                        [01;31m[K^[m[K
[01m[K/workspace/FreeCrafter/src/Tools/ScaleTool.cpp:225:33:[m[K [01;31m[Kerror: [m[Kqualified-id in declaration before ‘[01m[K([m[K’ token
  225 | Vector3 ScaleTool::determineAxis[01;31m[K([m[K) const
      |                                 [01;31m[K^[m[K
[01m[K/workspace/FreeCrafter/src/Tools/ScaleTool.cpp:234:57:[m[K [01;31m[Kerror: [m[Kqualified-id in declaration before ‘[01m[K([m[K’ token
  234 | Tool::OverrideResult ScaleTool::applyMeasurementOverride[01;31m[K([m[Kdouble value)
      |                                                         [01;31m[K^[m[K
[01m[K/workspace/FreeCrafter/src/Tools/RotateTool.cpp:193:13:[m[K [01;31m[Kerror: [m[K‘[01m[Kresult[m[K’ was not declared in this scope
  193 |             [01;31m[Kresult[m[K.push_back(object.get());
      |             [01;31m[K^~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Tools/ScaleTool.cpp:260:13:[m[K [01;31m[Kerror: [m[K‘[01m[KmajorAxis[m[K’ was not declared in this scope
  260 |             [01;31m[KmajorAxis[m[K = 1;
      |             [01;31m[K^~~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Tools/ScaleTool.cpp:262:9:[m[K [01;31m[Kerror: [m[K‘[01m[Kelse[m[K’ without a previous ‘[01m[Kif[m[K’
  262 |         [01;31m[Kelse[m[K if (absAxis.z > absAxis.x && absAxis.z >= absAxis.y)
      |         [01;31m[K^~~~[m[K
[01m[K/workspace/FreeCrafter/src/Tools/RotateTool.cpp:196:12:[m[K [01;31m[Kerror: [m[K‘[01m[Kresult[m[K’ was not declared in this scope
  196 |     return [01;31m[Kresult[m[K;
      |            [01;31m[K^~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Tools/RotateTool.cpp:198:34:[m[K [01;31m[Kerror: [m[Kqualified-id in declaration before ‘[01m[K([m[K’ token
  198 | Vector3 RotateTool::determineAxis[01;31m[K([m[K) const
      |                                  [01;31m[K^[m[K
[01m[K/workspace/FreeCrafter/src/Tools/RotateTool.cpp:207:58:[m[K [01;31m[Kerror: [m[Kqualified-id in declaration before ‘[01m[K([m[K’ token
  207 | Tool::OverrideResult RotateTool::applyMeasurementOverride[01;31m[K([m[Kdouble value)
      |                                                          [01;31m[K^[m[K
[01m[K/workspace/FreeCrafter/src/Tools/RotateTool.cpp:216:31:[m[K [01;31m[Kerror: [m[Kqualified-id in declaration before ‘[01m[K([m[K’ token
  216 | void RotateTool::applyRotation[01;31m[K([m[Kfloat angleRadians)
      |                               [01;31m[K^[m[K
[01m[K/workspace/FreeCrafter/src/Tools/ScaleTool.cpp:262:18:[m[K [01;31m[Kerror: [m[K‘[01m[KabsAxis[m[K’ was not declared in this scope; did you mean ‘[01m[Kaxis[m[K’?
  262 |         else if ([01;31m[KabsAxis[m[K.z > absAxis.x && absAxis.z >= absAxis.y)
      |                  [01;31m[K^~~~~~~[m[K
      |                  [32m[Kaxis[m[K
[01m[K/workspace/FreeCrafter/src/Tools/ScaleTool.cpp:266:51:[m[K [01;31m[Kerror: [m[K‘[01m[Kratio[m[K’ was not declared in this scope; did you mean ‘[01m[Kstd::ratio[m[K’?
  266 |         scaleFactors = makeAxisFactors(majorAxis, [01;31m[Kratio[m[K);
      |                                                   [01;31m[K^~~~~[m[K
      |                                                   [32m[Kstd::ratio[m[K
In file included from [01m[K/usr/include/c++/13/bits/chrono.h:37[m[K,
                 from [01m[K/usr/include/c++/13/chrono:41[m[K,
                 from [01m[K/workspace/FreeCrafter/src/Scene/Document.h:16[m[K,
                 from [01m[K/workspace/FreeCrafter/src/Core/Command.h:3[m[K,
                 from [01m[K/workspace/FreeCrafter/src/Tools/ToolCommands.h:3[m[K,
                 from [01m[K/workspace/FreeCrafter/src/Tools/ScaleTool.cpp:9[m[K:
[01m[K/usr/include/c++/13/ratio:266:12:[m[K [01;36m[Knote: [m[K‘[01m[Kstd::ratio[m[K’ declared here
  266 |     struct [01;36m[Kratio[m[K
      |            [01;36m[K^~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Tools/ScaleTool.cpp:[m[K At global scope:
[01m[K/workspace/FreeCrafter/src/Tools/ScaleTool.cpp:268:7:[m[K [01;31m[Kerror: [m[Kexpected unqualified-id before ‘[01m[Kelse[m[K’
  268 |     } [01;31m[Kelse[m[K {
      |       [01;31m[K^~~~[m[K
[01m[K/workspace/FreeCrafter/src/Tools/ScaleTool.cpp:288:1:[m[K [01;31m[Kerror: [m[Kexpected declaration before ‘[01m[K}[m[K’ token
  288 | [01;31m[K}[m[K
      | [01;31m[K^[m[K
[01m[K/workspace/FreeCrafter/src/Tools/ScaleTool.cpp:292:6:[m[K [01;31m[Kerror: [m[Kredefinition of ‘[01m[Kvoid ScaleTool::[01;32m[KonPointerUp[m[K(const Tool::PointerInput&)[m[K’
  292 | void [01;31m[KScaleTool[m[K::onPointerUp(const PointerInput& input)
      |      [01;31m[K^~~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Tools/ScaleTool.cpp:139:6:[m[K [01;36m[Knote: [m[K‘[01m[Kvirtual void ScaleTool::[01;32m[KonPointerUp[m[K(const Tool::PointerInput&)[m[K’ previously defined here
  139 | void [01;36m[KScaleTool[m[K::onPointerUp(const PointerInput& input)
      |      [01;36m[K^~~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Tools/ScaleTool.cpp:308:6:[m[K [01;31m[Kerror: [m[Kredefinition of ‘[01m[Kvoid ScaleTool::[01;32m[KonCancel[m[K()[m[K’
  308 | void [01;31m[KScaleTool[m[K::onCancel()
      |      [01;31m[K^~~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Tools/ScaleTool.cpp:147:6:[m[K [01;36m[Knote: [m[K‘[01m[Kvirtual void ScaleTool::[01;32m[KonCancel[m[K()[m[K’ previously defined here
  147 | void [01;36m[KScaleTool[m[K::onCancel()
      |      [01;36m[K^~~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Tools/ScaleTool.cpp:324:6:[m[K [01;31m[Kerror: [m[Kredefinition of ‘[01m[Kvoid ScaleTool::[01;32m[KonStateChanged[m[K(Tool::State, Tool::State)[m[K’
  324 | void [01;31m[KScaleTool[m[K::onStateChanged(State previous, State next)
      |      [01;31m[K^~~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Tools/ScaleTool.cpp:155:6:[m[K [01;36m[Knote: [m[K‘[01m[Kvirtual void ScaleTool::[01;32m[KonStateChanged[m[K(Tool::State, Tool::State)[m[K’ previously defined here
  155 | void [01;36m[KScaleTool[m[K::onStateChanged(State previous, State next)
      |      [01;36m[K^~~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Tools/ScaleTool.cpp:342:6:[m[K [01;31m[Kerror: [m[Kredefinition of ‘[01m[Kvoid ScaleTool::[01;32m[KonCommit[m[K()[m[K’
  342 | void [01;31m[KScaleTool[m[K::onCommit()
      |      [01;31m[K^~~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Tools/ScaleTool.cpp:164:6:[m[K [01;36m[Knote: [m[K‘[01m[Kvirtual void ScaleTool::[01;32m[KonCommit[m[K()[m[K’ previously defined here
  164 | void [01;36m[KScaleTool[m[K::onCommit()
      |      [01;36m[K^~~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Tools/RotateTool.cpp:227:46:[m[K [01;31m[Kerror: [m[K‘[01m[Kworld[m[K’ was not declared in this scope
  227 |     Vector3 currentVector = projectOntoPlane([01;31m[Kworld[m[K - pivot, axis);
      |                                              [01;31m[K^~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Tools/RotateTool.cpp:[m[K At global scope:
[01m[K/workspace/FreeCrafter/src/Tools/RotateTool.cpp:255:6:[m[K [01;31m[Kerror: [m[Kredefinition of ‘[01m[Kvoid RotateTool::[01;32m[KonPointerUp[m[K(const Tool::PointerInput&)[m[K’
  255 | void [01;31m[KRotateTool[m[K::onPointerUp(const PointerInput& input)
      |      [01;31m[K^~~~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Tools/RotateTool.cpp:119:6:[m[K [01;36m[Knote: [m[K‘[01m[Kvirtual void RotateTool::[01;32m[KonPointerUp[m[K(const Tool::PointerInput&)[m[K’ previously defined here
  119 | void [01;36m[KRotateTool[m[K::onPointerUp(const PointerInput& input)
      |      [01;36m[K^~~~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Tools/RotateTool.cpp:271:6:[m[K [01;31m[Kerror: [m[Kredefinition of ‘[01m[Kvoid RotateTool::[01;32m[KonCancel[m[K()[m[K’
  271 | void [01;31m[KRotateTool[m[K::onCancel()
      |      [01;31m[K^~~~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Tools/RotateTool.cpp:127:6:[m[K [01;36m[Knote: [m[K‘[01m[Kvirtual void RotateTool::[01;32m[KonCancel[m[K()[m[K’ previously defined here
  127 | void [01;36m[KRotateTool[m[K::onCancel()
      |      [01;36m[K^~~~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Tools/RotateTool.cpp:287:6:[m[K [01;31m[Kerror: [m[Kredefinition of ‘[01m[Kvoid RotateTool::[01;32m[KonStateChanged[m[K(Tool::State, Tool::State)[m[K’
  287 | void [01;31m[KRotateTool[m[K::onStateChanged(State previous, State next)
      |      [01;31m[K^~~~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Tools/RotateTool.cpp:135:6:[m[K [01;36m[Knote: [m[K‘[01m[Kvirtual void RotateTool::[01;32m[KonStateChanged[m[K(Tool::State, Tool::State)[m[K’ previously defined here
  135 | void [01;36m[KRotateTool[m[K::onStateChanged(State previous, State next)
      |      [01;36m[K^~~~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Tools/RotateTool.cpp:305:6:[m[K [01;31m[Kerror: [m[Kredefinition of ‘[01m[Kvoid RotateTool::[01;32m[KonCommit[m[K()[m[K’
  305 | void [01;31m[KRotateTool[m[K::onCommit()
      |      [01;31m[K^~~~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Tools/RotateTool.cpp:144:6:[m[K [01;36m[Knote: [m[K‘[01m[Kvirtual void RotateTool::[01;32m[KonCommit[m[K()[m[K’ previously defined here
  144 | void [01;36m[KRotateTool[m[K::onCommit()
      |      [01;36m[K^~~~~~~~~~[m[K
In file included from [01m[K/workspace/FreeCrafter/src/ui/ExternalReferenceDialog.h:3[m[K,
                 from [01m[K/workspace/FreeCrafter/src/ui/ExternalReferenceDialog.cpp:1[m[K:
[01m[K/workspace/FreeCrafter/src/Scene/SceneCommands.h:17:5:[m[K [01;31m[Kerror: [m[K‘[01m[KObjectId[m[K’ does not name a type
   17 |     [01;31m[KObjectId[m[K objectId() const { return currentObjectId; }
      |     [01;31m[K^~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Scene/SceneCommands.h:26:5:[m[K [01;31m[Kerror: [m[K‘[01m[KObjectId[m[K’ does not name a type
   26 |     [01;31m[KObjectId[m[K currentObjectId = 0;
      |     [01;31m[K^~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Scene/SceneCommands.h:61:5:[m[K [01;31m[Kerror: [m[K‘[01m[KObjectId[m[K’ does not name a type
   61 |     [01;31m[KObjectId[m[K objectId() const { return currentObjectId; }
      |     [01;31m[K^~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Tools/ExtrudeTool.cpp:[m[K In member function ‘[01m[Kvirtual void ExtrudeTool::[01;32m[KonCommit[m[K()[m[K’:
[01m[K/workspace/FreeCrafter/src/Tools/ExtrudeTool.cpp:495:14:[m[K [01;31m[Kerror: [m[Kinvalid use of incomplete type ‘[01m[Kclass Core::CommandStack[m[K’
  495 |         stack[01;31m[K->[m[Kpush(std::move(command));
      |              [01;31m[K^~[m[K
[01m[K/workspace/FreeCrafter/src/Scene/SceneCommands.h:70:5:[m[K [01;31m[Kerror: [m[K‘[01m[KObjectId[m[K’ does not name a type
   70 |     [01;31m[KObjectId[m[K currentObjectId = 0;
      |     [01;31m[K^~~~~~~~[m[K
In file included from [01m[K/workspace/FreeCrafter/src/Tools/ExtrudeTool.h:2[m[K,
                 from [01m[K/workspace/FreeCrafter/src/Tools/ExtrudeTool.cpp:1[m[K:
[01m[K/workspace/FreeCrafter/src/Tools/Tool.h:13:7:[m[K [01;36m[Knote: [m[Kforward declaration of ‘[01m[Kclass Core::CommandStack[m[K’
   13 | class [01;36m[KCommandStack[m[K;
      |       [01;36m[K^~~~~~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Scene/SceneCommands.h:87:5:[m[K [01;31m[Kerror: [m[K‘[01m[KObjectId[m[K’ does not name a type
   87 |     [01;31m[KObjectId[m[K objectId() const { return currentObjectId; }
      |     [01;31m[K^~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Scene/SceneCommands.h:96:5:[m[K [01;31m[Kerror: [m[K‘[01m[KObjectId[m[K’ does not name a type
   96 |     [01;31m[KObjectId[m[K currentObjectId = 0;
      |     [01;31m[K^~~~~~~~[m[K
In file included from [01m[K/usr/include/c++/13/memory:78[m[K,
                 from [01m[K/workspace/FreeCrafter/src/Tools/../GeometryKernel/GeometryKernel.h:5[m[K,
                 from [01m[K/workspace/FreeCrafter/src/Tools/Tool.h:2[m[K:
/usr/include/c++/13/bits/unique_ptr.h: In instantiation of ‘[01m[Kstd::__detail::__unique_ptr_t<_Tp> std::[01;32m[Kmake_unique[m[K(_Args&& ...) [35m[K[with _Tp = Tools::TranslateObjectsCommand; _Args = {int&, Vector3&, QString}; __detail::__unique_ptr_t<_Tp> = __detail::__unique_ptr_t<Tools::TranslateObjectsCommand>][m[K[m[K’:
[01m[K/workspace/FreeCrafter/src/Tools/MoveTool.cpp:94:76:[m[K   required from here
[01m[K/usr/include/c++/13/bits/unique_ptr.h:1070:30:[m[K [01;31m[Kerror: [m[Kno matching function for call to ‘[01m[KTools::TranslateObjectsCommand::TranslateObjectsCommand(int&, Vector3&, QString)[m[K’
 1070 |     { return unique_ptr<_Tp>([01;31m[Knew _Tp(std::forward<_Args>(__args)...)[m[K); }
      |                              [01;31m[K^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~[m[K
In file included from [01m[K/workspace/FreeCrafter/src/Tools/MoveTool.cpp:7[m[K:
[01m[K/workspace/FreeCrafter/src/Tools/ToolCommands.h:57:5:[m[K [01;36m[Knote: [m[Kcandidate: ‘[01m[KTools::TranslateObjectsCommand::[01;32m[KTranslateObjectsCommand[m[K(std::vector<long unsigned int>, Vector3, const QString&)[m[K’
   57 |     [01;36m[KTranslateObjectsCommand[m[K(std::vector<Scene::Document::ObjectId> ids, Vector3 delta, const QString& description);
      |     [01;36m[K^~~~~~~~~~~~~~~~~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Tools/ToolCommands.h:57:68:[m[K [01;36m[Knote: [m[K  no known conversion for argument 1 from ‘[01m[Kint[m[K’ to ‘[01m[Kstd::vector<long unsigned int>[m[K’
   57 |     TranslateObjectsCommand([01;36m[Kstd::vector<Scene::Document::ObjectId> ids[m[K, Vector3 delta, const QString& description);
      |                             [01;36m[K~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~^~~[m[K
[01m[K/workspace/FreeCrafter/src/Tools/LineTool.cpp:12:5:[m[K [01;31m[Kerror: [m[Kexpected unqualified-id before ‘[01m[Kif[m[K’
   12 |     [01;31m[Kif[m[K (points.empty()) {
      |     [01;31m[K^~[m[K
[01m[K/workspace/FreeCrafter/src/Tools/LineTool.cpp:15:7:[m[K [01;31m[Kerror: [m[Kexpected unqualified-id before ‘[01m[Kelse[m[K’
   15 |     } [01;31m[Kelse[m[K if ((point - points.back()).lengthSquared() > 1e-8f) {
      |       [01;31m[K^~~~[m[K
[01m[K/workspace/FreeCrafter/src/Tools/LineTool.cpp:39:5:[m[K [01;31m[Kerror: [m[Kexpected unqualified-id before ‘[01m[Kif[m[K’
   39 |     [01;31m[Kif[m[K (!resolvePoint(input, point)) {
      |     [01;31m[K^~[m[K
[01m[K/workspace/FreeCrafter/src/Tools/LineTool.cpp:49:5:[m[K [01;31m[Kerror: [m[Kexpected unqualified-id before ‘[01m[Kif[m[K’
   49 |     [01;31m[Kif[m[K (points.empty()) {
      |     [01;31m[K^~[m[K
[01m[K/workspace/FreeCrafter/src/Tools/LineTool.cpp:55:7:[m[K [01;31m[Kerror: [m[Kexpected unqualified-id before ‘[01m[Kelse[m[K’
   55 |     } [01;31m[Kelse[m[K {
      |       [01;31m[K^~~~[m[K
[01m[K/workspace/FreeCrafter/src/Tools/LineTool.cpp:74:5:[m[K [01;31m[Kerror: [m[K‘[01m[KpreviewValid[m[K’ does not name a type
   74 |     [01;31m[KpreviewValid[m[K = true;
      |     [01;31m[K^~~~~~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Tools/LineTool.cpp:76:1:[m[K [01;31m[Kerror: [m[Kexpected declaration before ‘[01m[K}[m[K’ token
   76 | [01;31m[K}[m[K
      | [01;31m[K^[m[K
[01m[K/workspace/FreeCrafter/src/Tools/LineTool.cpp:[m[K In member function ‘[01m[Kvirtual void LineTool::[01;32m[KonPointerHover[m[K(const Tool::PointerInput&)[m[K’:
[01m[K/workspace/FreeCrafter/src/Tools/LineTool.cpp:150:26:[m[K [01;31m[Kerror: [m[Kqualified-id in declaration before ‘[01m[K([m[K’ token
  150 | void LineTool::resetChain[01;31m[K([m[K)
      |                          [01;31m[K^[m[K
[01m[K/workspace/FreeCrafter/src/Tools/LineTool.cpp:[m[K At global scope:
[01m[K/workspace/FreeCrafter/src/Tools/LineTool.cpp:196:6:[m[K [01;31m[Kerror: [m[Kredefinition of ‘[01m[Kvoid LineTool::[01;32m[KonStateChanged[m[K(Tool::State, Tool::State)[m[K’
  196 | void [01;31m[KLineTool[m[K::onStateChanged(State previous, State next)
      |      [01;31m[K^~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Tools/LineTool.cpp:31:6:[m[K [01;36m[Knote: [m[K‘[01m[Kvirtual void LineTool::[01;32m[KonStateChanged[m[K(Tool::State, Tool::State)[m[K’ previously defined here
   31 | void [01;36m[KLineTool[m[K::onStateChanged(State previous, State next)
      |      [01;36m[K^~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Tools/LoftTool.cpp:[m[K In member function ‘[01m[Kvirtual void LoftTool::[01;32m[KonPointerUp[m[K(const Tool::PointerInput&)[m[K’:
[01m[K/workspace/FreeCrafter/src/Tools/LoftTool.cpp:90:9:[m[K [01;31m[Kerror: [m[K‘[01m[KTool::State Tool::state[m[K’ is private within this context
   90 |     if ([01;31m[Kstate[m[K != State::Active)
      |         [01;31m[K^~~~~[m[K
In file included from [01m[K/workspace/FreeCrafter/src/Tools/LoftTool.h:3[m[K,
                 from [01m[K/workspace/FreeCrafter/src/Tools/LoftTool.cpp:1[m[K:
[01m[K/workspace/FreeCrafter/src/Tools/Tool.h:175:11:[m[K [01;36m[Knote: [m[Kdeclared private here
  175 |     State [01;36m[Kstate[m[K = State::Idle;
      |           [01;36m[K^~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Tools/LoftTool.cpp:[m[K In member function ‘[01m[Kvirtual void LoftTool::[01;32m[KonCommit[m[K()[m[K’:
[01m[K/workspace/FreeCrafter/src/Tools/LoftTool.cpp:105:14:[m[K [01;31m[Kerror: [m[Kinvalid use of incomplete type ‘[01m[Kclass Core::CommandStack[m[K’
  105 |         stack[01;31m[K->[m[Kpush(std::move(command));
      |              [01;31m[K^~[m[K
[01m[K/workspace/FreeCrafter/src/Tools/Tool.h:13:7:[m[K [01;36m[Knote: [m[Kforward declaration of ‘[01m[Kclass Core::CommandStack[m[K’
   13 | class [01;36m[KCommandStack[m[K;
      |       [01;36m[K^~~~~~~~~~~~[m[K
In file included from [01m[K/workspace/FreeCrafter/src/ui/ImageImportDialog.h:3[m[K,
                 from [01m[K/workspace/FreeCrafter/src/ui/ImageImportDialog.cpp:1[m[K:
[01m[K/workspace/FreeCrafter/src/Scene/SceneCommands.h:17:5:[m[K [01;31m[Kerror: [m[K‘[01m[KObjectId[m[K’ does not name a type
   17 |     [01;31m[KObjectId[m[K objectId() const { return currentObjectId; }
      |     [01;31m[K^~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Scene/SceneCommands.h:26:5:[m[K [01;31m[Kerror: [m[K‘[01m[KObjectId[m[K’ does not name a type
   26 |     [01;31m[KObjectId[m[K currentObjectId = 0;
      |     [01;31m[K^~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Scene/SceneCommands.h:61:5:[m[K [01;31m[Kerror: [m[K‘[01m[KObjectId[m[K’ does not name a type
   61 |     [01;31m[KObjectId[m[K objectId() const { return currentObjectId; }
      |     [01;31m[K^~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Tools/DrawingTools.cpp:[m[K In member function ‘[01m[Kvoid RectangleTool::[01;32m[KfinalizeRectangle[m[K(const Vector3&)[m[K’:
[01m[K/workspace/FreeCrafter/src/Tools/DrawingTools.cpp:921:14:[m[K [01;31m[Kerror: [m[Kinvalid use of incomplete type ‘[01m[Kclass Core::CommandStack[m[K’
  921 |         stack[01;31m[K->[m[Kpush(std::move(command));
      |              [01;31m[K^~[m[K
In file included from [01m[K/workspace/FreeCrafter/src/Tools/DrawingTools.h:3[m[K,
                 from [01m[K/workspace/FreeCrafter/src/Tools/DrawingTools.cpp:1[m[K:
[01m[K/workspace/FreeCrafter/src/Tools/Tool.h:13:7:[m[K [01;36m[Knote: [m[Kforward declaration of ‘[01m[Kclass Core::CommandStack[m[K’
   13 | class [01;36m[KCommandStack[m[K;
      |       [01;36m[K^~~~~~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Scene/SceneCommands.h:70:5:[m[K [01;31m[Kerror: [m[K‘[01m[KObjectId[m[K’ does not name a type
   70 |     [01;31m[KObjectId[m[K currentObjectId = 0;
      |     [01;31m[K^~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Scene/SceneCommands.h:87:5:[m[K [01;31m[Kerror: [m[K‘[01m[KObjectId[m[K’ does not name a type
   87 |     [01;31m[KObjectId[m[K objectId() const { return currentObjectId; }
      |     [01;31m[K^~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Scene/SceneCommands.h:96:5:[m[K [01;31m[Kerror: [m[K‘[01m[KObjectId[m[K’ does not name a type
   96 |     [01;31m[KObjectId[m[K currentObjectId = 0;
      |     [01;31m[K^~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Tools/ChamferTool.cpp:[m[K In member function ‘[01m[Kvirtual void ChamferTool::[01;32m[KonPointerUp[m[K(const Tool::PointerInput&)[m[K’:
[01m[K/workspace/FreeCrafter/src/Tools/ChamferTool.cpp:75:9:[m[K [01;31m[Kerror: [m[K‘[01m[KTool::State Tool::state[m[K’ is private within this context
   75 |     if ([01;31m[Kstate[m[K != State::Active)
      |         [01;31m[K^~~~~[m[K
In file included from [01m[K/workspace/FreeCrafter/src/Tools/ChamferTool.h:3[m[K,
                 from [01m[K/workspace/FreeCrafter/src/Tools/ChamferTool.cpp:1[m[K:
[01m[K/workspace/FreeCrafter/src/Tools/Tool.h:175:11:[m[K [01;36m[Knote: [m[Kdeclared private here
  175 |     State [01;36m[Kstate[m[K = State::Idle;
      |           [01;36m[K^~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Tools/ChamferTool.cpp:[m[K In member function ‘[01m[Kvirtual void ChamferTool::[01;32m[KonCommit[m[K()[m[K’:
[01m[K/workspace/FreeCrafter/src/Tools/ChamferTool.cpp:90:14:[m[K [01;31m[Kerror: [m[Kinvalid use of incomplete type ‘[01m[Kclass Core::CommandStack[m[K’
   90 |         stack[01;31m[K->[m[Kpush(std::move(command));
      |              [01;31m[K^~[m[K
[01m[K/workspace/FreeCrafter/src/Tools/Tool.h:13:7:[m[K [01;36m[Knote: [m[Kforward declaration of ‘[01m[Kclass Core::CommandStack[m[K’
   13 | class [01;36m[KCommandStack[m[K;
      |       [01;36m[K^~~~~~~~~~~~[m[K
gmake[2]: *** [CMakeFiles/freecrafter_lib.dir/build.make:622: CMakeFiles/freecrafter_lib.dir/src/Tools/MoveTool.cpp.o] Error 1
gmake[2]: *** [CMakeFiles/freecrafter_lib.dir/build.make:608: CMakeFiles/freecrafter_lib.dir/src/Tools/SmartSelectTool.cpp.o] Error 1
gmake[2]: *** [CMakeFiles/freecrafter_lib.dir/build.make:650: CMakeFiles/freecrafter_lib.dir/src/Tools/ScaleTool.cpp.o] Error 1
gmake[2]: *** [CMakeFiles/freecrafter_lib.dir/build.make:636: CMakeFiles/freecrafter_lib.dir/src/Tools/RotateTool.cpp.o] Error 1
gmake[2]: *** [CMakeFiles/freecrafter_lib.dir/build.make:664: CMakeFiles/freecrafter_lib.dir/src/Tools/ExtrudeTool.cpp.o] Error 1
gmake[2]: *** [CMakeFiles/freecrafter_lib.dir/build.make:594: CMakeFiles/freecrafter_lib.dir/src/Tools/LineTool.cpp.o] Error 1
gmake[2]: *** [CMakeFiles/freecrafter_lib.dir/build.make:720: CMakeFiles/freecrafter_lib.dir/src/Tools/DrawingTools.cpp.o] Error 1
gmake[2]: *** [CMakeFiles/freecrafter_lib.dir/build.make:692: CMakeFiles/freecrafter_lib.dir/src/Tools/LoftTool.cpp.o] Error 1
gmake[2]: *** [CMakeFiles/freecrafter_lib.dir/build.make:678: CMakeFiles/freecrafter_lib.dir/src/Tools/ChamferTool.cpp.o] Error 1
[01m[K/workspace/FreeCrafter/src/Scene/SceneSerializer.cpp:[m[K In function ‘[01m[Kstd::array<float, 16> Scene::{anonymous}::[01;32m[KtransformFromJson[m[K(const QJsonArray&)[m[K’:
[01m[K/workspace/FreeCrafter/src/Scene/SceneSerializer.cpp:193:31:[m[K [01;31m[Kerror: [m[Kno matching function for call to ‘[01m[Kmin(qsizetype, int)[m[K’
  193 |     const int count = [01;31m[Kstd::min(arr.size(), 16)[m[K;
      |                       [01;31m[K~~~~~~~~^~~~~~~~~~~~~~~~[m[K
In file included from [01m[K/usr/include/c++/13/string:51[m[K,
                 from [01m[K/workspace/FreeCrafter/src/Scene/SceneSerializer.h:5[m[K,
                 from [01m[K/workspace/FreeCrafter/src/Scene/SceneSerializer.cpp:1[m[K:
[01m[K/usr/include/c++/13/bits/stl_algobase.h:233:5:[m[K [01;36m[Knote: [m[Kcandidate: ‘[01m[Ktemplate<class _Tp> constexpr const _Tp& std::[01;32m[Kmin[m[K(const _Tp&, const _Tp&)[m[K’
  233 |     [01;36m[Kmin[m[K(const _Tp& __a, const _Tp& __b)
      |     [01;36m[K^~~[m[K
[01m[K/usr/include/c++/13/bits/stl_algobase.h:233:5:[m[K [01;36m[Knote: [m[K  template argument deduction/substitution failed:
[01m[K/workspace/FreeCrafter/src/Scene/SceneSerializer.cpp:193:31:[m[K [01;36m[Knote: [m[K  deduced conflicting types for parameter ‘[01m[Kconst _Tp[m[K’ (‘[01m[Klong long int[m[K’ and ‘[01m[Kint[m[K’)
  193 |     const int count = [01;36m[Kstd::min(arr.size(), 16)[m[K;
      |                       [01;36m[K~~~~~~~~^~~~~~~~~~~~~~~~[m[K
[01m[K/usr/include/c++/13/bits/stl_algobase.h:281:5:[m[K [01;36m[Knote: [m[Kcandidate: ‘[01m[Ktemplate<class _Tp, class _Compare> constexpr const _Tp& std::[01;32m[Kmin[m[K(const _Tp&, const _Tp&, _Compare)[m[K’
  281 |     [01;36m[Kmin[m[K(const _Tp& __a, const _Tp& __b, _Compare __comp)
      |     [01;36m[K^~~[m[K
[01m[K/usr/include/c++/13/bits/stl_algobase.h:281:5:[m[K [01;36m[Knote: [m[K  template argument deduction/substitution failed:
[01m[K/workspace/FreeCrafter/src/Scene/SceneSerializer.cpp:193:31:[m[K [01;36m[Knote: [m[K  deduced conflicting types for parameter ‘[01m[Kconst _Tp[m[K’ (‘[01m[Klong long int[m[K’ and ‘[01m[Kint[m[K’)
  193 |     const int count = [01;36m[Kstd::min(arr.size(), 16)[m[K;
      |                       [01;36m[K~~~~~~~~^~~~~~~~~~~~~~~~[m[K
In file included from [01m[K/usr/include/c++/13/functional:67[m[K,
                 from [01m[K/workspace/FreeCrafter/src/Scene/../GeometryKernel/Curve.h:2[m[K,
                 from [01m[K/workspace/FreeCrafter/src/Scene/../GeometryKernel/GeometryKernel.h:11[m[K,
                 from [01m[K/workspace/FreeCrafter/src/Scene/Document.h:6[m[K,
                 from [01m[K/workspace/FreeCrafter/src/Scene/SceneSerializer.cpp:3[m[K:
[01m[K/usr/include/c++/13/bits/stl_algo.h:5775:5:[m[K [01;36m[Knote: [m[Kcandidate: ‘[01m[Ktemplate<class _Tp> constexpr _Tp std::[01;32m[Kmin[m[K(initializer_list<_Tp>)[m[K’
 5775 |     [01;36m[Kmin[m[K(initializer_list<_Tp> __l)
      |     [01;36m[K^~~[m[K
[01m[K/usr/include/c++/13/bits/stl_algo.h:5775:5:[m[K [01;36m[Knote: [m[K  template argument deduction/substitution failed:
[01m[K/workspace/FreeCrafter/src/Scene/SceneSerializer.cpp:193:31:[m[K [01;36m[Knote: [m[K  mismatched types ‘[01m[Kstd::initializer_list<_Tp>[m[K’ and ‘[01m[Klong long int[m[K’
  193 |     const int count = [01;36m[Kstd::min(arr.size(), 16)[m[K;
      |                       [01;36m[K~~~~~~~~^~~~~~~~~~~~~~~~[m[K
[01m[K/usr/include/c++/13/bits/stl_algo.h:5785:5:[m[K [01;36m[Knote: [m[Kcandidate: ‘[01m[Ktemplate<class _Tp, class _Compare> constexpr _Tp std::[01;32m[Kmin[m[K(initializer_list<_Tp>, _Compare)[m[K’
 5785 |     [01;36m[Kmin[m[K(initializer_list<_Tp> __l, _Compare __comp)
      |     [01;36m[K^~~[m[K
[01m[K/usr/include/c++/13/bits/stl_algo.h:5785:5:[m[K [01;36m[Knote: [m[K  template argument deduction/substitution failed:
[01m[K/workspace/FreeCrafter/src/Scene/SceneSerializer.cpp:193:31:[m[K [01;36m[Knote: [m[K  mismatched types ‘[01m[Kstd::initializer_list<_Tp>[m[K’ and ‘[01m[Klong long int[m[K’
  193 |     const int count = [01;36m[Kstd::min(arr.size(), 16)[m[K;
      |                       [01;36m[K~~~~~~~~^~~~~~~~~~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Scene/SceneCommands.cpp:[m[K In member function ‘[01m[Kvirtual void Scene::AddPrimitiveCommand::[01;32m[Kredo[m[K()[m[K’:
[01m[K/workspace/FreeCrafter/src/Scene/SceneCommands.cpp:80:5:[m[K [01;31m[Kerror: [m[K‘[01m[KcurrentObjectId[m[K’ was not declared in this scope
   80 |     [01;31m[KcurrentObjectId[m[K = document.ensureObjectForGeometry(added, fallbackName(options));
      |     [01;31m[K^~~~~~~~~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Scene/SceneSerializer.cpp:[m[K In function ‘[01m[KQJsonObject Scene::{anonymous}::[01;32m[KprototypeToJson[m[K(const Scene::Document::PrototypeNode&, const std::unordered_map<const GeometryObject*, long unsigned int>&)[m[K’:
[01m[K/workspace/FreeCrafter/src/Scene/SceneSerializer.cpp:345:45:[m[K [01;31m[Kerror: [m[K‘[01m[Kstruct Scene::Document::PrototypeNode[m[K’ is private within this context
  345 | QJsonObject prototypeToJson(const Document::[01;31m[KPrototypeNode[m[K& proto,
      |                                             [01;31m[K^~~~~~~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Scene/Document.h:187:12:[m[K [01;36m[Knote: [m[Kdeclared private here
  187 |     struct [01;36m[KPrototypeNode[m[K {
      |            [01;36m[K^~~~~~~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Scene/SceneCommands.cpp:[m[K In member function ‘[01m[Kvirtual void Scene::AddPrimitiveCommand::[01;32m[Kundo[m[K()[m[K’:
[01m[K/workspace/FreeCrafter/src/Scene/SceneCommands.cpp:86:9:[m[K [01;31m[Kerror: [m[K‘[01m[KcurrentObjectId[m[K’ was not declared in this scope
   86 |     if ([01;31m[KcurrentObjectId[m[K != 0) {
      |         [01;31m[K^~~~~~~~~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Scene/SceneCommands.cpp:[m[K In member function ‘[01m[Kvirtual void Scene::AddImagePlaneCommand::[01;32m[Kredo[m[K()[m[K’:
[01m[K/workspace/FreeCrafter/src/Scene/SceneCommands.cpp:145:5:[m[K [01;31m[Kerror: [m[K‘[01m[KcurrentObjectId[m[K’ was not declared in this scope
  145 |     [01;31m[KcurrentObjectId[m[K = document.ensureObjectForGeometry(added, naming.name);
      |     [01;31m[K^~~~~~~~~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Scene/SceneSerializer.cpp:[m[K In function ‘[01m[Kstd::unique_ptr<Scene::Document::PrototypeNode> Scene::{anonymous}::[01;32m[KprototypeFromJson[m[K(const QJsonObject&, const std::vector<GeometryObject*>&)[m[K’:
[01m[K/workspace/FreeCrafter/src/Scene/SceneSerializer.cpp:368:6:[m[K [01;31m[Kerror: [m[K‘[01m[Kstruct Scene::Document::PrototypeNode[m[K’ is private within this context
  368 | std::[01;31m[Kunique_ptr<Document::PrototypeNode>[m[K prototypeFromJson(
      |      [01;31m[K^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Scene/Document.h:187:12:[m[K [01;36m[Knote: [m[Kdeclared private here
  187 |     struct [01;36m[KPrototypeNode[m[K {
      |            [01;36m[K^~~~~~~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Scene/SceneSerializer.cpp:372:23:[m[K [01;31m[Kerror: [m[K‘[01m[Kstruct Scene::Document::PrototypeNode[m[K’ is private within this context
  372 |     auto proto = std::[01;31m[Kmake_unique<Document::PrototypeNode>[m[K();
      |                       [01;31m[K^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Scene/Document.h:187:12:[m[K [01;36m[Knote: [m[Kdeclared private here
  187 |     struct [01;36m[KPrototypeNode[m[K {
      |            [01;36m[K^~~~~~~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Scene/SceneSerializer.cpp:[m[K In function ‘[01m[KQJsonObject Scene::{anonymous}::[01;32m[KobjectNodeToJson[m[K(const Scene::Document::ObjectNode&, const std::unordered_map<const GeometryObject*, long unsigned int>&)[m[K’:
[01m[K/workspace/FreeCrafter/src/Scene/SceneSerializer.cpp:421:14:[m[K [01;31m[Kerror: [m[K‘[01m[Kclass QJsonArray[m[K’ has no member named ‘[01m[Kreserve[m[K’
  421 |     children.[01;31m[Kreserve[m[K(static_cast<int>(node.children.size()));
      |              [01;31m[K^~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Scene/SceneCommands.cpp:[m[K In member function ‘[01m[Kvirtual void Scene::AddImagePlaneCommand::[01;32m[Kundo[m[K()[m[K’:
[01m[K/workspace/FreeCrafter/src/Scene/SceneCommands.cpp:157:9:[m[K [01;31m[Kerror: [m[K‘[01m[KcurrentObjectId[m[K’ was not declared in this scope
  157 |     if ([01;31m[KcurrentObjectId[m[K != 0) {
      |         [01;31m[K^~~~~~~~~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Scene/SceneSerializer.cpp:[m[K In function ‘[01m[KQJsonObject Scene::{anonymous}::[01;32m[KimportMetadataToJson[m[K(Scene::Document::ObjectId, const Scene::Document::ImportMetadata&)[m[K’:
[01m[K/workspace/FreeCrafter/src/Scene/SceneSerializer.cpp:475:5:[m[K [01;31m[Kerror: [m[Kdeclaration does not declare anything [[01;31m[K-fpermissive[m[K]
  475 |     [01;31m[KQJsonArray[m[K slots;
      |     [01;31m[K^~~~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Scene/SceneSerializer.cpp:477:14:[m[K [01;31m[Kerror: [m[Kexpected primary-expression before ‘[01m[K.[m[K’ token
  477 |         slots[01;31m[K.[m[Kappend(QString::fromStdString(slot));
      |              [01;31m[K^[m[K
[01m[K/workspace/FreeCrafter/src/Scene/SceneSerializer.cpp:478:54:[m[K [01;31m[Kerror: [m[Kexpected primary-expression before ‘[01m[K)[m[K’ token
  478 |     obj.insert(QStringLiteral("materialSlots"), slots[01;31m[K)[m[K;
      |                                                      [01;31m[K^[m[K
[01m[K/workspace/FreeCrafter/src/Scene/SceneSerializer.cpp:[m[K In function ‘[01m[KScene::Document::ImportMetadata Scene::{anonymous}::[01;32m[KimportMetadataFromJson[m[K(const QJsonObject&)[m[K’:
[01m[K/workspace/FreeCrafter/src/Scene/SceneSerializer.cpp:492:28:[m[K [01;31m[Kerror: [m[Kexpected unqualified-id before ‘[01m[K=[m[K’ token
  492 |     const QJsonArray slots [01;31m[K=[m[K obj.value(QStringLiteral("materialSlots")).toArray();
      |                            [01;31m[K^[m[K
[01m[K/workspace/FreeCrafter/src/Scene/SceneSerializer.cpp:493:37:[m[K [01;31m[Kerror: [m[Kexpected primary-expression before ‘[01m[K.[m[K’ token
  493 |     meta.materialSlots.reserve(slots[01;31m[K.[m[Ksize());
      |                                     [01;31m[K^[m[K
[01m[K/workspace/FreeCrafter/src/Scene/SceneSerializer.cpp:494:35:[m[K [01;31m[Kerror: [m[Kexpected primary-expression before ‘[01m[K)[m[K’ token
  494 |     for (const auto& entry : slots[01;31m[K)[m[K
      |                                   [01;31m[K^[m[K
[01m[K/workspace/FreeCrafter/src/Scene/SceneCommands.cpp:[m[K In member function ‘[01m[Kvirtual void Scene::LinkExternalReferenceCommand::[01;32m[Kredo[m[K()[m[K’:
[01m[K/workspace/FreeCrafter/src/Scene/SceneCommands.cpp:196:5:[m[K [01;31m[Kerror: [m[K‘[01m[KcurrentObjectId[m[K’ was not declared in this scope
  196 |     [01;31m[KcurrentObjectId[m[K = document.ensureObjectForGeometry(added, primitive.name);
      |     [01;31m[K^~~~~~~~~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Scene/SceneCommands.cpp:[m[K In member function ‘[01m[Kvirtual void Scene::LinkExternalReferenceCommand::[01;32m[Kundo[m[K()[m[K’:
[01m[K/workspace/FreeCrafter/src/Scene/SceneCommands.cpp:207:9:[m[K [01;31m[Kerror: [m[K‘[01m[KcurrentObjectId[m[K’ was not declared in this scope
  207 |     if ([01;31m[KcurrentObjectId[m[K != 0) {
      |         [01;31m[K^~~~~~~~~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Scene/SceneSerializer.cpp:[m[K In static member function ‘[01m[Kstatic Scene::SceneSerializer::Result Scene::SceneSerializer::[01;32m[KsaveToStream[m[K(const Scene::Document&, std::ostream&)[m[K’:
[01m[K/workspace/FreeCrafter/src/Scene/SceneSerializer.cpp:640:39:[m[K [01;31m[Kerror: [m[Kinvalid conversion from ‘[01m[Kstd::tuple_element<0, const std::pair<const long unsigned int, std::__cxx11::basic_string<char> > >::type[m[K’ {aka ‘[01m[Klong unsigned int[m[K’} to ‘[01m[Kstd::unordered_map<const GeometryObject*, long unsigned int>::key_type[m[K’ {aka ‘[01m[Kconst GeometryObject*[m[K’} [[01;31m[K-fpermissive[m[K]
  640 |         auto it = geometryLookup.find([01;31m[Kgeom[m[K);
      |                                       [01;31m[K^~~~[m[K
      |                                       [01;31m[K|[m[K
      |                                       [01;31m[Kstd::tuple_element<0, const std::pair<const long unsigned int, std::__cxx11::basic_string<char> > >::type {aka long unsigned int}[m[K
In file included from [01m[K/usr/include/c++/13/unordered_map:41[m[K,
                 from [01m[K/workspace/FreeCrafter/src/Scene/../GeometryKernel/GeometryKernel.h:8[m[K:
[01m[K/usr/include/c++/13/bits/unordered_map.h:875:28:[m[K [01;36m[Knote: [m[K  initializing argument 1 of ‘[01m[Kstd::unordered_map<_Key, _Tp, _Hash, _Pred, _Alloc>::iterator std::unordered_map<_Key, _Tp, _Hash, _Pred, _Alloc>::[01;32m[Kfind[m[K(const key_type&) [35m[K[with _Key = const GeometryObject*; _Tp = long unsigned int; _Hash = std::hash<const GeometryObject*>; _Pred = std::equal_to<const GeometryObject*>; _Alloc = std::allocator<std::pair<const GeometryObject* const, long unsigned int> >; iterator = std::__detail::_Insert_base<const GeometryObject*, std::pair<const GeometryObject* const, long unsigned int>, std::allocator<std::pair<const GeometryObject* const, long unsigned int> >, std::__detail::_Select1st, std::equal_to<const GeometryObject*>, std::hash<const GeometryObject*>, std::__detail::_Mod_range_hashing, std::__detail::_Default_ranged_hash, std::__detail::_Prime_rehash_policy, std::__detail::_Hashtable_traits<false, false, true> >::iterator; key_type = const GeometryObject*][m[K[m[K’
  875 |       find([01;36m[Kconst key_type& __x[m[K)
      |            [01;36m[K~~~~~~~~~~~~~~~~^~~[m[K
[01m[K/workspace/FreeCrafter/src/ui/RightTray.cpp:[m[K In member function ‘[01m[Kvoid RightTray::[01;32m[KhandleRenameObject[m[K(Scene::Document::ObjectId, const QString&)[m[K’:
[01m[K/workspace/FreeCrafter/src/ui/RightTray.cpp:91:18:[m[K [01;31m[Kerror: [m[Kinvalid use of incomplete type ‘[01m[Kclass Core::CommandStack[m[K’
   91 |     commandStack_[01;31m[K->[m[Kpush(std::move(command));
      |                  [01;31m[K^~[m[K
In file included from [01m[K/workspace/FreeCrafter/src/ui/RightTray.cpp:1[m[K:
[01m[K/workspace/FreeCrafter/src/ui/RightTray.h:17:7:[m[K [01;36m[Knote: [m[Kforward declaration of ‘[01m[Kclass Core::CommandStack[m[K’
   17 | class [01;36m[KCommandStack[m[K;
      |       [01;36m[K^~~~~~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/ui/RightTray.cpp:[m[K In member function ‘[01m[Kvoid RightTray::[01;32m[KhandleVisibilityChange[m[K(Scene::Document::ObjectId, bool)[m[K’:
[01m[K/workspace/FreeCrafter/src/ui/RightTray.cpp:100:18:[m[K [01;31m[Kerror: [m[Kinvalid use of incomplete type ‘[01m[Kclass Core::CommandStack[m[K’
  100 |     commandStack_[01;31m[K->[m[Kpush(std::move(command));
      |                  [01;31m[K^~[m[K
[01m[K/workspace/FreeCrafter/src/ui/RightTray.h:17:7:[m[K [01;36m[Knote: [m[Kforward declaration of ‘[01m[Kclass Core::CommandStack[m[K’
   17 | class [01;36m[KCommandStack[m[K;
      |       [01;36m[K^~~~~~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/ui/RightTray.cpp:[m[K In member function ‘[01m[Kvoid RightTray::[01;32m[KhandleAssignMaterial[m[K(const QString&)[m[K’:
[01m[K/workspace/FreeCrafter/src/ui/RightTray.cpp:128:18:[m[K [01;31m[Kerror: [m[Kinvalid use of incomplete type ‘[01m[Kclass Core::CommandStack[m[K’
  128 |     commandStack_[01;31m[K->[m[Kpush(std::move(command));
      |                  [01;31m[K^~[m[K
[01m[K/workspace/FreeCrafter/src/ui/RightTray.h:17:7:[m[K [01;36m[Knote: [m[Kforward declaration of ‘[01m[Kclass Core::CommandStack[m[K’
   17 | class [01;36m[KCommandStack[m[K;
      |       [01;36m[K^~~~~~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/ui/RightTray.cpp:[m[K In member function ‘[01m[Kvoid RightTray::[01;32m[KhandleCreateTag[m[K(const QString&, const QColor&)[m[K’:
[01m[K/workspace/FreeCrafter/src/ui/RightTray.cpp:138:18:[m[K [01;31m[Kerror: [m[Kinvalid use of incomplete type ‘[01m[Kclass Core::CommandStack[m[K’
  138 |     commandStack_[01;31m[K->[m[Kpush(std::move(command));
      |                  [01;31m[K^~[m[K
[01m[K/workspace/FreeCrafter/src/ui/RightTray.h:17:7:[m[K [01;36m[Knote: [m[Kforward declaration of ‘[01m[Kclass Core::CommandStack[m[K’
   17 | class [01;36m[KCommandStack[m[K;
      |       [01;36m[K^~~~~~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/ui/RightTray.cpp:[m[K In member function ‘[01m[Kvoid RightTray::[01;32m[KhandleRenameTag[m[K(Scene::Document::TagId, const QString&)[m[K’:
[01m[K/workspace/FreeCrafter/src/ui/RightTray.cpp:147:18:[m[K [01;31m[Kerror: [m[Kinvalid use of incomplete type ‘[01m[Kclass Core::CommandStack[m[K’
  147 |     commandStack_[01;31m[K->[m[Kpush(std::move(command));
      |                  [01;31m[K^~[m[K
[01m[K/workspace/FreeCrafter/src/ui/RightTray.h:17:7:[m[K [01;36m[Knote: [m[Kforward declaration of ‘[01m[Kclass Core::CommandStack[m[K’
   17 | class [01;36m[KCommandStack[m[K;
      |       [01;36m[K^~~~~~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/ui/RightTray.cpp:[m[K In member function ‘[01m[Kvoid RightTray::[01;32m[KhandleSetTagVisibility[m[K(Scene::Document::TagId, bool)[m[K’:
[01m[K/workspace/FreeCrafter/src/ui/RightTray.cpp:156:18:[m[K [01;31m[Kerror: [m[Kinvalid use of incomplete type ‘[01m[Kclass Core::CommandStack[m[K’
  156 |     commandStack_[01;31m[K->[m[Kpush(std::move(command));
      |                  [01;31m[K^~[m[K
[01m[K/workspace/FreeCrafter/src/ui/RightTray.h:17:7:[m[K [01;36m[Knote: [m[Kforward declaration of ‘[01m[Kclass Core::CommandStack[m[K’
   17 | class [01;36m[KCommandStack[m[K;
      |       [01;36m[K^~~~~~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/ui/RightTray.cpp:[m[K In member function ‘[01m[Kvoid RightTray::[01;32m[KhandleSetTagColor[m[K(Scene::Document::TagId, const QColor&)[m[K’:
[01m[K/workspace/FreeCrafter/src/ui/RightTray.cpp:166:18:[m[K [01;31m[Kerror: [m[Kinvalid use of incomplete type ‘[01m[Kclass Core::CommandStack[m[K’
  166 |     commandStack_[01;31m[K->[m[Kpush(std::move(command));
      |                  [01;31m[K^~[m[K
[01m[K/workspace/FreeCrafter/src/ui/RightTray.h:17:7:[m[K [01;36m[Knote: [m[Kforward declaration of ‘[01m[Kclass Core::CommandStack[m[K’
   17 | class [01;36m[KCommandStack[m[K;
      |       [01;36m[K^~~~~~~~~~~~[m[K
In file included from [01m[K/workspace/FreeCrafter/build/freecrafter_lib_autogen/UVLADIE3JM/moc_MainWindow.cpp:10[m[K,
                 from [01m[K/workspace/FreeCrafter/build/freecrafter_lib_autogen/mocs_compilation.cpp:4[m[K:
[01m[K/workspace/FreeCrafter/build/freecrafter_lib_autogen/UVLADIE3JM/../../../src/MainWindow.h:323:32:[m[K [01;31m[Kerror: [m[Kfield ‘[01m[KchamferDefaults_[m[K’ has incomplete type ‘[01m[KPhase6::RoundCornerOptions[m[K’
  323 |     Phase6::RoundCornerOptions [01;31m[KchamferDefaults_[m[K;
      |                                [01;31m[K^~~~~~~~~~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/build/freecrafter_lib_autogen/UVLADIE3JM/../../../src/MainWindow.h:35:8:[m[K [01;36m[Knote: [m[Kforward declaration of ‘[01m[Kstruct Phase6::RoundCornerOptions[m[K’
   35 | struct [01;36m[KRoundCornerOptions[m[K;
      |        [01;36m[K^~~~~~~~~~~~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/build/freecrafter_lib_autogen/UVLADIE3JM/../../../src/MainWindow.h:324:25:[m[K [01;31m[Kerror: [m[Kfield ‘[01m[KloftDefaults_[m[K’ has incomplete type ‘[01m[KPhase6::LoftOptions[m[K’
  324 |     Phase6::LoftOptions [01;31m[KloftDefaults_[m[K;
      |                         [01;31m[K^~~~~~~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/build/freecrafter_lib_autogen/UVLADIE3JM/../../../src/MainWindow.h:36:8:[m[K [01;36m[Knote: [m[Kforward declaration of ‘[01m[Kstruct Phase6::LoftOptions[m[K’
   36 | struct [01;36m[KLoftOptions[m[K;
      |        [01;36m[K^~~~~~~~~~~[m[K
In file included from [01m[K/workspace/FreeCrafter/src/MainWindow.cpp:1[m[K:
[01m[K/workspace/FreeCrafter/src/MainWindow.h:323:32:[m[K [01;31m[Kerror: [m[Kfield ‘[01m[KchamferDefaults_[m[K’ has incomplete type ‘[01m[KPhase6::RoundCornerOptions[m[K’
  323 |     Phase6::RoundCornerOptions [01;31m[KchamferDefaults_[m[K;
      |                                [01;31m[K^~~~~~~~~~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/MainWindow.h:35:8:[m[K [01;36m[Knote: [m[Kforward declaration of ‘[01m[Kstruct Phase6::RoundCornerOptions[m[K’
   35 | struct [01;36m[KRoundCornerOptions[m[K;
      |        [01;36m[K^~~~~~~~~~~~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/MainWindow.h:324:25:[m[K [01;31m[Kerror: [m[Kfield ‘[01m[KloftDefaults_[m[K’ has incomplete type ‘[01m[KPhase6::LoftOptions[m[K’
  324 |     Phase6::LoftOptions [01;31m[KloftDefaults_[m[K;
      |                         [01;31m[K^~~~~~~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/MainWindow.h:36:8:[m[K [01;36m[Knote: [m[Kforward declaration of ‘[01m[Kstruct Phase6::LoftOptions[m[K’
   36 | struct [01;36m[KLoftOptions[m[K;
      |        [01;36m[K^~~~~~~~~~~[m[K
In file included from [01m[K/workspace/FreeCrafter/src/MainWindow.cpp:14[m[K:
[01m[K/workspace/FreeCrafter/src/Scene/SceneCommands.h:17:5:[m[K [01;31m[Kerror: [m[K‘[01m[KObjectId[m[K’ does not name a type
   17 |     [01;31m[KObjectId[m[K objectId() const { return currentObjectId; }
      |     [01;31m[K^~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Scene/SceneCommands.h:26:5:[m[K [01;31m[Kerror: [m[K‘[01m[KObjectId[m[K’ does not name a type
   26 |     [01;31m[KObjectId[m[K currentObjectId = 0;
      |     [01;31m[K^~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Scene/SceneCommands.h:61:5:[m[K [01;31m[Kerror: [m[K‘[01m[KObjectId[m[K’ does not name a type
   61 |     [01;31m[KObjectId[m[K objectId() const { return currentObjectId; }
      |     [01;31m[K^~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Scene/SceneCommands.h:70:5:[m[K [01;31m[Kerror: [m[K‘[01m[KObjectId[m[K’ does not name a type
   70 |     [01;31m[KObjectId[m[K currentObjectId = 0;
      |     [01;31m[K^~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Scene/SceneCommands.h:87:5:[m[K [01;31m[Kerror: [m[K‘[01m[KObjectId[m[K’ does not name a type
   87 |     [01;31m[KObjectId[m[K objectId() const { return currentObjectId; }
      |     [01;31m[K^~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Scene/SceneCommands.h:96:5:[m[K [01;31m[Kerror: [m[K‘[01m[KObjectId[m[K’ does not name a type
   96 |     [01;31m[KObjectId[m[K currentObjectId = 0;
      |     [01;31m[K^~~~~~~~[m[K
gmake[2]: *** [CMakeFiles/freecrafter_lib.dir/build.make:510: CMakeFiles/freecrafter_lib.dir/src/Scene/SceneCommands.cpp.o] Error 1
[01m[K/workspace/FreeCrafter/src/GLViewport.cpp:[m[K In member function ‘[01m[Kvoid GLViewport::[01;32m[KdrawCursorOverlay[m[K(QPainter&)[m[K’:
[01m[K/workspace/FreeCrafter/src/GLViewport.cpp:1683:41:[m[K [01;31m[Kerror: [m[Kno matching function for call to ‘[01m[Kmax(float, qreal)[m[K’
 1683 |         badgeFont.setPointSizeF([01;31m[Kstd::max(9.5f, badgeFont.pointSizeF() - 1.0f)[m[K);
      |                                 [01;31m[K~~~~~~~~^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~[m[K
In file included from [01m[K/usr/include/c++/13/bits/specfun.h:43[m[K,
                 from [01m[K/usr/include/c++/13/cmath:3699[m[K,
                 from [01m[K/usr/include/x86_64-linux-gnu/qt6/QtCore/qnumeric.h:8[m[K,
                 from [01m[K/usr/include/x86_64-linux-gnu/qt6/QtCore/qglobal.h:1405[m[K,
                 from [01m[K/usr/include/x86_64-linux-gnu/qt6/QtOpenGLWidgets/qtopenglwidgetsglobal.h:7[m[K,
                 from [01m[K/usr/include/x86_64-linux-gnu/qt6/QtOpenGLWidgets/qopenglwidget.h:7[m[K,
                 from [01m[K/usr/include/x86_64-linux-gnu/qt6/QtOpenGLWidgets/QOpenGLWidget:1[m[K,
                 from [01m[K/workspace/FreeCrafter/src/GLViewport.h:3[m[K,
                 from [01m[K/workspace/FreeCrafter/src/GLViewport.cpp:1[m[K:
[01m[K/usr/include/c++/13/bits/stl_algobase.h:257:5:[m[K [01;36m[Knote: [m[Kcandidate: ‘[01m[Ktemplate<class _Tp> constexpr const _Tp& std::[01;32m[Kmax[m[K(const _Tp&, const _Tp&)[m[K’
  257 |     [01;36m[Kmax[m[K(const _Tp& __a, const _Tp& __b)
      |     [01;36m[K^~~[m[K
[01m[K/usr/include/c++/13/bits/stl_algobase.h:257:5:[m[K [01;36m[Knote: [m[K  template argument deduction/substitution failed:
[01m[K/workspace/FreeCrafter/src/GLViewport.cpp:1683:41:[m[K [01;36m[Knote: [m[K  deduced conflicting types for parameter ‘[01m[Kconst _Tp[m[K’ (‘[01m[Kfloat[m[K’ and ‘[01m[Kqreal[m[K’ {aka ‘[01m[Kdouble[m[K’})
 1683 |         badgeFont.setPointSizeF([01;36m[Kstd::max(9.5f, badgeFont.pointSizeF() - 1.0f)[m[K);
      |                                 [01;36m[K~~~~~~~~^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~[m[K
[01m[K/usr/include/c++/13/bits/stl_algobase.h:303:5:[m[K [01;36m[Knote: [m[Kcandidate: ‘[01m[Ktemplate<class _Tp, class _Compare> constexpr const _Tp& std::[01;32m[Kmax[m[K(const _Tp&, const _Tp&, _Compare)[m[K’
  303 |     [01;36m[Kmax[m[K(const _Tp& __a, const _Tp& __b, _Compare __comp)
      |     [01;36m[K^~~[m[K
[01m[K/usr/include/c++/13/bits/stl_algobase.h:303:5:[m[K [01;36m[Knote: [m[K  template argument deduction/substitution failed:
[01m[K/workspace/FreeCrafter/src/GLViewport.cpp:1683:41:[m[K [01;36m[Knote: [m[K  deduced conflicting types for parameter ‘[01m[Kconst _Tp[m[K’ (‘[01m[Kfloat[m[K’ and ‘[01m[Kqreal[m[K’ {aka ‘[01m[Kdouble[m[K’})
 1683 |         badgeFont.setPointSizeF([01;36m[Kstd::max(9.5f, badgeFont.pointSizeF() - 1.0f)[m[K);
      |                                 [01;36m[K~~~~~~~~^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~[m[K
In file included from [01m[K/usr/include/c++/13/functional:67[m[K,
                 from [01m[K/usr/include/x86_64-linux-gnu/qt6/QtCore/qchar.h:9[m[K,
                 from [01m[K/usr/include/x86_64-linux-gnu/qt6/QtCore/qstring.h:14[m[K,
                 from [01m[K/usr/include/x86_64-linux-gnu/qt6/QtCore/qobject.h:11[m[K,
                 from [01m[K/usr/include/x86_64-linux-gnu/qt6/QtWidgets/qwidget.h:9[m[K,
                 from [01m[K/usr/include/x86_64-linux-gnu/qt6/QtWidgets/QWidget:1[m[K,
                 from [01m[K/usr/include/x86_64-linux-gnu/qt6/QtOpenGLWidgets/qopenglwidget.h:9[m[K:
[01m[K/usr/include/c++/13/bits/stl_algo.h:5795:5:[m[K [01;36m[Knote: [m[Kcandidate: ‘[01m[Ktemplate<class _Tp> constexpr _Tp std::[01;32m[Kmax[m[K(initializer_list<_Tp>)[m[K’
 5795 |     [01;36m[Kmax[m[K(initializer_list<_Tp> __l)
      |     [01;36m[K^~~[m[K
[01m[K/usr/include/c++/13/bits/stl_algo.h:5795:5:[m[K [01;36m[Knote: [m[K  template argument deduction/substitution failed:
[01m[K/workspace/FreeCrafter/src/GLViewport.cpp:1683:41:[m[K [01;36m[Knote: [m[K  mismatched types ‘[01m[Kstd::initializer_list<_Tp>[m[K’ and ‘[01m[Kfloat[m[K’
 1683 |         badgeFont.setPointSizeF([01;36m[Kstd::max(9.5f, badgeFont.pointSizeF() - 1.0f)[m[K);
      |                                 [01;36m[K~~~~~~~~^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~[m[K
[01m[K/usr/include/c++/13/bits/stl_algo.h:5805:5:[m[K [01;36m[Knote: [m[Kcandidate: ‘[01m[Ktemplate<class _Tp, class _Compare> constexpr _Tp std::[01;32m[Kmax[m[K(initializer_list<_Tp>, _Compare)[m[K’
 5805 |     [01;36m[Kmax[m[K(initializer_list<_Tp> __l, _Compare __comp)
      |     [01;36m[K^~~[m[K
[01m[K/usr/include/c++/13/bits/stl_algo.h:5805:5:[m[K [01;36m[Knote: [m[K  template argument deduction/substitution failed:
[01m[K/workspace/FreeCrafter/src/GLViewport.cpp:1683:41:[m[K [01;36m[Knote: [m[K  mismatched types ‘[01m[Kstd::initializer_list<_Tp>[m[K’ and ‘[01m[Kfloat[m[K’
 1683 |         badgeFont.setPointSizeF([01;36m[Kstd::max(9.5f, badgeFont.pointSizeF() - 1.0f)[m[K);
      |                                 [01;36m[K~~~~~~~~^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/GLViewport.cpp:1702:45:[m[K [01;31m[Kerror: [m[Kno matching function for call to ‘[01m[Kmax(float, qreal)[m[K’
 1702 |         inferenceFont.setPointSizeF([01;31m[Kstd::max(11.0f, baseFont.pointSizeF() + 1.0f)[m[K);
      |                                     [01;31m[K~~~~~~~~^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~[m[K
[01m[K/usr/include/c++/13/bits/stl_algobase.h:257:5:[m[K [01;36m[Knote: [m[Kcandidate: ‘[01m[Ktemplate<class _Tp> constexpr const _Tp& std::[01;32m[Kmax[m[K(const _Tp&, const _Tp&)[m[K’
  257 |     [01;36m[Kmax[m[K(const _Tp& __a, const _Tp& __b)
      |     [01;36m[K^~~[m[K
[01m[K/usr/include/c++/13/bits/stl_algobase.h:257:5:[m[K [01;36m[Knote: [m[K  template argument deduction/substitution failed:
[01m[K/workspace/FreeCrafter/src/GLViewport.cpp:1702:45:[m[K [01;36m[Knote: [m[K  deduced conflicting types for parameter ‘[01m[Kconst _Tp[m[K’ (‘[01m[Kfloat[m[K’ and ‘[01m[Kqreal[m[K’ {aka ‘[01m[Kdouble[m[K’})
 1702 |         inferenceFont.setPointSizeF([01;36m[Kstd::max(11.0f, baseFont.pointSizeF() + 1.0f)[m[K);
      |                                     [01;36m[K~~~~~~~~^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~[m[K
[01m[K/usr/include/c++/13/bits/stl_algobase.h:303:5:[m[K [01;36m[Knote: [m[Kcandidate: ‘[01m[Ktemplate<class _Tp, class _Compare> constexpr const _Tp& std::[01;32m[Kmax[m[K(const _Tp&, const _Tp&, _Compare)[m[K’
  303 |     [01;36m[Kmax[m[K(const _Tp& __a, const _Tp& __b, _Compare __comp)
      |     [01;36m[K^~~[m[K
[01m[K/usr/include/c++/13/bits/stl_algobase.h:303:5:[m[K [01;36m[Knote: [m[K  template argument deduction/substitution failed:
[01m[K/workspace/FreeCrafter/src/GLViewport.cpp:1702:45:[m[K [01;36m[Knote: [m[K  deduced conflicting types for parameter ‘[01m[Kconst _Tp[m[K’ (‘[01m[Kfloat[m[K’ and ‘[01m[Kqreal[m[K’ {aka ‘[01m[Kdouble[m[K’})
 1702 |         inferenceFont.setPointSizeF([01;36m[Kstd::max(11.0f, baseFont.pointSizeF() + 1.0f)[m[K);
      |                                     [01;36m[K~~~~~~~~^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~[m[K
[01m[K/usr/include/c++/13/bits/stl_algo.h:5795:5:[m[K [01;36m[Knote: [m[Kcandidate: ‘[01m[Ktemplate<class _Tp> constexpr _Tp std::[01;32m[Kmax[m[K(initializer_list<_Tp>)[m[K’
 5795 |     [01;36m[Kmax[m[K(initializer_list<_Tp> __l)
      |     [01;36m[K^~~[m[K
[01m[K/usr/include/c++/13/bits/stl_algo.h:5795:5:[m[K [01;36m[Knote: [m[K  template argument deduction/substitution failed:
[01m[K/workspace/FreeCrafter/src/GLViewport.cpp:1702:45:[m[K [01;36m[Knote: [m[K  mismatched types ‘[01m[Kstd::initializer_list<_Tp>[m[K’ and ‘[01m[Kfloat[m[K’
 1702 |         inferenceFont.setPointSizeF([01;36m[Kstd::max(11.0f, baseFont.pointSizeF() + 1.0f)[m[K);
      |                                     [01;36m[K~~~~~~~~^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~[m[K
[01m[K/usr/include/c++/13/bits/stl_algo.h:5805:5:[m[K [01;36m[Knote: [m[Kcandidate: ‘[01m[Ktemplate<class _Tp, class _Compare> constexpr _Tp std::[01;32m[Kmax[m[K(initializer_list<_Tp>, _Compare)[m[K’
 5805 |     [01;36m[Kmax[m[K(initializer_list<_Tp> __l, _Compare __comp)
      |     [01;36m[K^~~[m[K
[01m[K/usr/include/c++/13/bits/stl_algo.h:5805:5:[m[K [01;36m[Knote: [m[K  template argument deduction/substitution failed:
[01m[K/workspace/FreeCrafter/src/GLViewport.cpp:1702:45:[m[K [01;36m[Knote: [m[K  mismatched types ‘[01m[Kstd::initializer_list<_Tp>[m[K’ and ‘[01m[Kfloat[m[K’
 1702 |         inferenceFont.setPointSizeF([01;36m[Kstd::max(11.0f, baseFont.pointSizeF() + 1.0f)[m[K);
      |                                     [01;36m[K~~~~~~~~^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/GLViewport.cpp:1722:40:[m[K [01;31m[Kerror: [m[Kno matching function for call to ‘[01m[Kmax(float, qreal)[m[K’
 1722 |         hintFont.setPointSizeF([01;31m[Kstd::max(9.0f, baseFont.pointSizeF() - 0.5f)[m[K);
      |                                [01;31m[K~~~~~~~~^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~[m[K
[01m[K/usr/include/c++/13/bits/stl_algobase.h:257:5:[m[K [01;36m[Knote: [m[Kcandidate: ‘[01m[Ktemplate<class _Tp> constexpr const _Tp& std::[01;32m[Kmax[m[K(const _Tp&, const _Tp&)[m[K’
  257 |     [01;36m[Kmax[m[K(const _Tp& __a, const _Tp& __b)
      |     [01;36m[K^~~[m[K
[01m[K/usr/include/c++/13/bits/stl_algobase.h:257:5:[m[K [01;36m[Knote: [m[K  template argument deduction/substitution failed:
[01m[K/workspace/FreeCrafter/src/GLViewport.cpp:1722:40:[m[K [01;36m[Knote: [m[K  deduced conflicting types for parameter ‘[01m[Kconst _Tp[m[K’ (‘[01m[Kfloat[m[K’ and ‘[01m[Kqreal[m[K’ {aka ‘[01m[Kdouble[m[K’})
 1722 |         hintFont.setPointSizeF([01;36m[Kstd::max(9.0f, baseFont.pointSizeF() - 0.5f)[m[K);
      |                                [01;36m[K~~~~~~~~^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~[m[K
[01m[K/usr/include/c++/13/bits/stl_algobase.h:303:5:[m[K [01;36m[Knote: [m[Kcandidate: ‘[01m[Ktemplate<class _Tp, class _Compare> constexpr const _Tp& std::[01;32m[Kmax[m[K(const _Tp&, const _Tp&, _Compare)[m[K’
  303 |     [01;36m[Kmax[m[K(const _Tp& __a, const _Tp& __b, _Compare __comp)
      |     [01;36m[K^~~[m[K
[01m[K/usr/include/c++/13/bits/stl_algobase.h:303:5:[m[K [01;36m[Knote: [m[K  template argument deduction/substitution failed:
[01m[K/workspace/FreeCrafter/src/GLViewport.cpp:1722:40:[m[K [01;36m[Knote: [m[K  deduced conflicting types for parameter ‘[01m[Kconst _Tp[m[K’ (‘[01m[Kfloat[m[K’ and ‘[01m[Kqreal[m[K’ {aka ‘[01m[Kdouble[m[K’})
 1722 |         hintFont.setPointSizeF([01;36m[Kstd::max(9.0f, baseFont.pointSizeF() - 0.5f)[m[K);
      |                                [01;36m[K~~~~~~~~^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~[m[K
[01m[K/usr/include/c++/13/bits/stl_algo.h:5795:5:[m[K [01;36m[Knote: [m[Kcandidate: ‘[01m[Ktemplate<class _Tp> constexpr _Tp std::[01;32m[Kmax[m[K(initializer_list<_Tp>)[m[K’
 5795 |     [01;36m[Kmax[m[K(initializer_list<_Tp> __l)
      |     [01;36m[K^~~[m[K
[01m[K/usr/include/c++/13/bits/stl_algo.h:5795:5:[m[K [01;36m[Knote: [m[K  template argument deduction/substitution failed:
[01m[K/workspace/FreeCrafter/src/GLViewport.cpp:1722:40:[m[K [01;36m[Knote: [m[K  mismatched types ‘[01m[Kstd::initializer_list<_Tp>[m[K’ and ‘[01m[Kfloat[m[K’
 1722 |         hintFont.setPointSizeF([01;36m[Kstd::max(9.0f, baseFont.pointSizeF() - 0.5f)[m[K);
      |                                [01;36m[K~~~~~~~~^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~[m[K
[01m[K/usr/include/c++/13/bits/stl_algo.h:5805:5:[m[K [01;36m[Knote: [m[Kcandidate: ‘[01m[Ktemplate<class _Tp, class _Compare> constexpr _Tp std::[01;32m[Kmax[m[K(initializer_list<_Tp>, _Compare)[m[K’
 5805 |     [01;36m[Kmax[m[K(initializer_list<_Tp> __l, _Compare __comp)
      |     [01;36m[K^~~[m[K
[01m[K/usr/include/c++/13/bits/stl_algo.h:5805:5:[m[K [01;36m[Knote: [m[K  template argument deduction/substitution failed:
[01m[K/workspace/FreeCrafter/src/GLViewport.cpp:1722:40:[m[K [01;36m[Knote: [m[K  mismatched types ‘[01m[Kstd::initializer_list<_Tp>[m[K’ and ‘[01m[Kfloat[m[K’
 1722 |         hintFont.setPointSizeF([01;36m[Kstd::max(9.0f, baseFont.pointSizeF() - 0.5f)[m[K);
      |                                [01;36m[K~~~~~~~~^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~[m[K
gmake[2]: *** [CMakeFiles/freecrafter_lib.dir/build.make:1168: CMakeFiles/freecrafter_lib.dir/src/ui/RightTray.cpp.o] Error 1
gmake[2]: *** [CMakeFiles/freecrafter_lib.dir/build.make:1042: CMakeFiles/freecrafter_lib.dir/src/ui/ImageImportDialog.cpp.o] Error 1
gmake[2]: *** [CMakeFiles/freecrafter_lib.dir/build.make:1056: CMakeFiles/freecrafter_lib.dir/src/ui/ExternalReferenceDialog.cpp.o] Error 1
In file included from [01m[K/workspace/FreeCrafter/build/freecrafter_lib_autogen/YPKJ5OE7LN/../../../src/ui/ExternalReferenceDialog.h:3[m[K,
                 from [01m[K/workspace/FreeCrafter/build/freecrafter_lib_autogen/YPKJ5OE7LN/moc_ExternalReferenceDialog.cpp:10[m[K,
                 from [01m[K/workspace/FreeCrafter/build/freecrafter_lib_autogen/mocs_compilation.cpp:12[m[K:
[01m[K/workspace/FreeCrafter/src/Scene/SceneCommands.h:17:5:[m[K [01;31m[Kerror: [m[K‘[01m[KObjectId[m[K’ does not name a type
   17 |     [01;31m[KObjectId[m[K objectId() const { return currentObjectId; }
      |     [01;31m[K^~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Scene/SceneCommands.h:26:5:[m[K [01;31m[Kerror: [m[K‘[01m[KObjectId[m[K’ does not name a type
   26 |     [01;31m[KObjectId[m[K currentObjectId = 0;
      |     [01;31m[K^~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Scene/SceneCommands.h:61:5:[m[K [01;31m[Kerror: [m[K‘[01m[KObjectId[m[K’ does not name a type
   61 |     [01;31m[KObjectId[m[K objectId() const { return currentObjectId; }
      |     [01;31m[K^~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Scene/SceneCommands.h:70:5:[m[K [01;31m[Kerror: [m[K‘[01m[KObjectId[m[K’ does not name a type
   70 |     [01;31m[KObjectId[m[K currentObjectId = 0;
      |     [01;31m[K^~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Scene/SceneCommands.h:87:5:[m[K [01;31m[Kerror: [m[K‘[01m[KObjectId[m[K’ does not name a type
   87 |     [01;31m[KObjectId[m[K objectId() const { return currentObjectId; }
      |     [01;31m[K^~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/Scene/SceneCommands.h:96:5:[m[K [01;31m[Kerror: [m[K‘[01m[KObjectId[m[K’ does not name a type
   96 |     [01;31m[KObjectId[m[K currentObjectId = 0;
      |     [01;31m[K^~~~~~~~[m[K
gmake[2]: *** [CMakeFiles/freecrafter_lib.dir/build.make:482: CMakeFiles/freecrafter_lib.dir/src/Scene/SceneSerializer.cpp.o] Error 1
gmake[2]: *** [CMakeFiles/freecrafter_lib.dir/build.make:174: CMakeFiles/freecrafter_lib.dir/src/GLViewport.cpp.o] Error 1
[01m[K/workspace/FreeCrafter/src/MainWindow.cpp:[m[K In member function ‘[01m[Kvoid MainWindow::[01;32m[KmaybeRestoreAutosave[m[K()[m[K’:
[01m[K/workspace/FreeCrafter/src/MainWindow.cpp:1191:76:[m[K [01;31m[Kerror: [m[K‘[01m[KDefaultLocaleLongDate[m[K’ is not a member of ‘[01m[KQt[m[K’
 1191 |     const QString timestamp = latest->timestamp.toLocalTime().toString(Qt::[01;31m[KDefaultLocaleLongDate[m[K);
      |                                                                            [01;31m[K^~~~~~~~~~~~~~~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/MainWindow.cpp:[m[K In member function ‘[01m[Kvoid MainWindow::[01;32m[KcreateMenus[m[K()[m[K’:
[01m[K/workspace/FreeCrafter/src/MainWindow.cpp:1674:102:[m[K [01;31m[Kerror: [m[K‘[01m[KshowChamferOptionsDialog[m[K’ is not a member of ‘[01m[KMainWindow[m[K’
 1674 |     chamferOptionsAction = advancedToolsMenu->addAction(tr("Chamfer Options..."), this, &MainWindow::[01;31m[KshowChamferOptionsDialog[m[K);
      |                                                                                                      [01;31m[K^~~~~~~~~~~~~~~~~~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/MainWindow.cpp:1679:96:[m[K [01;31m[Kerror: [m[K‘[01m[KshowLoftOptionsDialog[m[K’ is not a member of ‘[01m[KMainWindow[m[K’
 1679 |     loftOptionsAction = advancedToolsMenu->addAction(tr("Loft Options..."), this, &MainWindow::[01;31m[KshowLoftOptionsDialog[m[K);
      |                                                                                                [01;31m[K^~~~~~~~~~~~~~~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/MainWindow.cpp:[m[K In member function ‘[01m[Kvoid MainWindow::[01;32m[KcreateToolbars[m[K()[m[K’:
[01m[K/workspace/FreeCrafter/src/MainWindow.cpp:2440:80:[m[K [01;31m[Kerror: [m[K‘[01m[KshowChamferOptionsDialog[m[K’ is not a member of ‘[01m[KMainWindow[m[K’
 2440 |         connect(chamferDialogButton, &QToolButton::clicked, this, &MainWindow::[01;31m[KshowChamferOptionsDialog[m[K);
      |                                                                                [01;31m[K^~~~~~~~~~~~~~~~~~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/MainWindow.cpp:2492:77:[m[K [01;31m[Kerror: [m[K‘[01m[KshowLoftOptionsDialog[m[K’ is not a member of ‘[01m[KMainWindow[m[K’
 2492 |         connect(loftDialogButton, &QToolButton::clicked, this, &MainWindow::[01;31m[KshowLoftOptionsDialog[m[K);
      |                                                                             [01;31m[K^~~~~~~~~~~~~~~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/MainWindow.cpp:[m[K At global scope:
[01m[K/workspace/FreeCrafter/src/MainWindow.cpp:4621:6:[m[K [01;31m[Kerror: [m[Kno declaration matches ‘[01m[Kvoid MainWindow::[01;32m[KshowChamferOptionsDialog[m[K()[m[K’
 4621 | void [01;31m[KMainWindow[m[K::showChamferOptionsDialog()
      |      [01;31m[K^~~~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/MainWindow.cpp:4621:6:[m[K [01;36m[Knote: [m[Kno functions named ‘[01m[Kvoid MainWindow::[01;32m[KshowChamferOptionsDialog[m[K()[m[K’
[01m[K/workspace/FreeCrafter/src/MainWindow.h:55:7:[m[K [01;36m[Knote: [m[K‘[01m[Kclass MainWindow[m[K’ defined here
   55 | class [01;36m[KMainWindow[m[K : public QMainWindow {
      |       [01;36m[K^~~~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/MainWindow.cpp:4641:6:[m[K [01;31m[Kerror: [m[Kno declaration matches ‘[01m[Kvoid MainWindow::[01;32m[KshowLoftOptionsDialog[m[K()[m[K’
 4641 | void [01;31m[KMainWindow[m[K::showLoftOptionsDialog()
      |      [01;31m[K^~~~~~~~~~[m[K
[01m[K/workspace/FreeCrafter/src/MainWindow.cpp:4641:6:[m[K [01;36m[Knote: [m[Kno functions named ‘[01m[Kvoid MainWindow::[01;32m[KshowLoftOptionsDialog[m[K()[m[K’
[01m[K/workspace/FreeCrafter/src/MainWindow.h:55:7:[m[K [01;36m[Knote: [m[K‘[01m[Kclass MainWindow[m[K’ defined here
   55 | class [01;36m[KMainWindow[m[K : public QMainWindow {
      |       [01;36m[K^~~~~~~~~~[m[K
gmake[2]: *** [CMakeFiles/freecrafter_lib.dir/build.make:146: CMakeFiles/freecrafter_lib.dir/freecrafter_lib_autogen/mocs_compilation.cpp.o] Error 1
gmake[2]: *** [CMakeFiles/freecrafter_lib.dir/build.make:160: CMakeFiles/freecrafter_lib.dir/src/MainWindow.cpp.o] Error 1
gmake[1]: *** [CMakeFiles/Makefile2:143: CMakeFiles/freecrafter_lib.dir/all] Error 2
gmake: *** [Makefile:166: all] Error 2
# ctest --test-dir build --output-on-failure
Internal ctest changing into directory: /workspace/FreeCrafter/build
Test project /workspace/FreeCrafter/build
      Start  1: render_regression
Could not find executable /workspace/FreeCrafter/build/test_render
Looked in the following places:
/workspace/FreeCrafter/build/test_render
/workspace/FreeCrafter/build/test_render
/workspace/FreeCrafter/build/Release/test_render
/workspace/FreeCrafter/build/Release/test_render
/workspace/FreeCrafter/build/Debug/test_render
/workspace/FreeCrafter/build/Debug/test_render
/workspace/FreeCrafter/build/MinSizeRel/test_render
/workspace/FreeCrafter/build/MinSizeRel/test_render
/workspace/FreeCrafter/build/RelWithDebInfo/test_render
/workspace/FreeCrafter/build/RelWithDebInfo/test_render
/workspace/FreeCrafter/build/Deployment/test_render
/workspace/FreeCrafter/build/Deployment/test_render
/workspace/FreeCrafter/build/Development/test_render
/workspace/FreeCrafter/build/Development/test_render
workspace/FreeCrafter/build/test_render
workspace/FreeCrafter/build/test_render
workspace/FreeCrafter/build/Release/test_render
workspace/FreeCrafter/build/Release/test_render
workspace/FreeCrafter/build/Debug/test_render
workspace/FreeCrafter/build/Debug/test_render
workspace/FreeCrafter/build/MinSizeRel/test_render
workspace/FreeCrafter/build/MinSizeRel/test_render
workspace/FreeCrafter/build/RelWithDebInfo/test_render
workspace/FreeCrafter/build/RelWithDebInfo/test_render
workspace/FreeCrafter/build/Deployment/test_render
workspace/FreeCrafter/build/Deployment/test_render
workspace/FreeCrafter/build/Development/test_render
workspace/FreeCrafter/build/Development/test_render
Unable to find executable: /workspace/FreeCrafter/build/test_render
 1/13 Test  #1: render_regression ................***Not Run   0.00 sec
      Start  2: viewport_depth_range
Could not find executable /workspace/FreeCrafter/build/test_viewport_depth
Looked in the following places:
/workspace/FreeCrafter/build/test_viewport_depth
/workspace/FreeCrafter/build/test_viewport_depth
/workspace/FreeCrafter/build/Release/test_viewport_depth
/workspace/FreeCrafter/build/Release/test_viewport_depth
/workspace/FreeCrafter/build/Debug/test_viewport_depth
/workspace/FreeCrafter/build/Debug/test_viewport_depth
/workspace/FreeCrafter/build/MinSizeRel/test_viewport_depth
/workspace/FreeCrafter/build/MinSizeRel/test_viewport_depth
/workspace/FreeCrafter/build/RelWithDebInfo/test_viewport_depth
/workspace/FreeCrafter/build/RelWithDebInfo/test_viewport_depth
/workspace/FreeCrafter/build/Deployment/test_viewport_depth
/workspace/FreeCrafter/build/Deployment/test_viewport_depth
/workspace/FreeCrafter/build/Development/test_viewport_depth
/workspace/FreeCrafter/build/Development/test_viewport_depth
workspace/FreeCrafter/build/test_viewport_depth
workspace/FreeCrafter/build/test_viewport_depth
workspace/FreeCrafter/build/Release/test_viewport_depth
workspace/FreeCrafter/build/Release/test_viewport_depth
workspace/FreeCrafter/build/Debug/test_viewport_depth
workspace/FreeCrafter/build/Debug/test_viewport_depth
workspace/FreeCrafter/build/MinSizeRel/test_viewport_depth
workspace/FreeCrafter/build/MinSizeRel/test_viewport_depth
workspace/FreeCrafter/build/RelWithDebInfo/test_viewport_depth
workspace/FreeCrafter/build/RelWithDebInfo/test_viewport_depth
workspace/FreeCrafter/build/Deployment/test_viewport_depth
workspace/FreeCrafter/build/Deployment/test_viewport_depth
workspace/FreeCrafter/build/Development/test_viewport_depth
workspace/FreeCrafter/build/Development/test_viewport_depth
Unable to find executable: /workspace/FreeCrafter/build/test_viewport_depth
 2/13 Test  #2: viewport_depth_range .............***Not Run   0.00 sec
      Start  3: scene_settings
Could not find executable /workspace/FreeCrafter/build/test_scene_settings
Looked in the following places:
/workspace/FreeCrafter/build/test_scene_settings
/workspace/FreeCrafter/build/test_scene_settings
/workspace/FreeCrafter/build/Release/test_scene_settings
/workspace/FreeCrafter/build/Release/test_scene_settings
/workspace/FreeCrafter/build/Debug/test_scene_settings
/workspace/FreeCrafter/build/Debug/test_scene_settings
/workspace/FreeCrafter/build/MinSizeRel/test_scene_settings
/workspace/FreeCrafter/build/MinSizeRel/test_scene_settings
/workspace/FreeCrafter/build/RelWithDebInfo/test_scene_settings
/workspace/FreeCrafter/build/RelWithDebInfo/test_scene_settings
/workspace/FreeCrafter/build/Deployment/test_scene_settings
/workspace/FreeCrafter/build/Deployment/test_scene_settings
/workspace/FreeCrafter/build/Development/test_scene_settings
/workspace/FreeCrafter/build/Development/test_scene_settings
workspace/FreeCrafter/build/test_scene_settings
workspace/FreeCrafter/build/test_scene_settings
workspace/FreeCrafter/build/Release/test_scene_settings
workspace/FreeCrafter/build/Release/test_scene_settings
workspace/FreeCrafter/build/Debug/test_scene_settings
workspace/FreeCrafter/build/Debug/test_scene_settings
workspace/FreeCrafter/build/MinSizeRel/test_scene_settings
workspace/FreeCrafter/build/MinSizeRel/test_scene_settings
workspace/FreeCrafter/build/RelWithDebInfo/test_scene_settings
workspace/FreeCrafter/build/RelWithDebInfo/test_scene_settings
workspace/FreeCrafter/build/Deployment/test_scene_settings
workspace/FreeCrafter/build/Deployment/test_scene_settings
workspace/FreeCrafter/build/Development/test_scene_settings
workspace/FreeCrafter/build/Development/test_scene_settings
Unable to find executable: /workspace/FreeCrafter/build/test_scene_settings
 3/13 Test  #3: scene_settings ...................***Not Run   0.00 sec
      Start  4: scene_commands
Could not find executable /workspace/FreeCrafter/build/test_scene_commands
Looked in the following places:
/workspace/FreeCrafter/build/test_scene_commands
/workspace/FreeCrafter/build/test_scene_commands
/workspace/FreeCrafter/build/Release/test_scene_commands
/workspace/FreeCrafter/build/Release/test_scene_commands
/workspace/FreeCrafter/build/Debug/test_scene_commands
/workspace/FreeCrafter/build/Debug/test_scene_commands
/workspace/FreeCrafter/build/MinSizeRel/test_scene_commands
/workspace/FreeCrafter/build/MinSizeRel/test_scene_commands
/workspace/FreeCrafter/build/RelWithDebInfo/test_scene_commands
/workspace/FreeCrafter/build/RelWithDebInfo/test_scene_commands
/workspace/FreeCrafter/build/Deployment/test_scene_commands
/workspace/FreeCrafter/build/Deployment/test_scene_commands
/workspace/FreeCrafter/build/Development/test_scene_commands
/workspace/FreeCrafter/build/Development/test_scene_commands
workspace/FreeCrafter/build/test_scene_commands
workspace/FreeCrafter/build/test_scene_commands
workspace/FreeCrafter/build/Release/test_scene_commands
workspace/FreeCrafter/build/Release/test_scene_commands
workspace/FreeCrafter/build/Debug/test_scene_commands
workspace/FreeCrafter/build/Debug/test_scene_commands
workspace/FreeCrafter/build/MinSizeRel/test_scene_commands
workspace/FreeCrafter/build/MinSizeRel/test_scene_commands
workspace/FreeCrafter/build/RelWithDebInfo/test_scene_commands
workspace/FreeCrafter/build/RelWithDebInfo/test_scene_commands
workspace/FreeCrafter/build/Deployment/test_scene_commands
workspace/FreeCrafter/build/Deployment/test_scene_commands
workspace/FreeCrafter/build/Development/test_scene_commands
workspace/FreeCrafter/build/Development/test_scene_commands
Unable to find executable: /workspace/FreeCrafter/build/test_scene_commands
 4/13 Test  #4: scene_commands ...................***Not Run   0.00 sec
      Start  5: command_stack
Could not find executable /workspace/FreeCrafter/build/test_command_stack
Looked in the following places:
/workspace/FreeCrafter/build/test_command_stack
/workspace/FreeCrafter/build/test_command_stack
/workspace/FreeCrafter/build/Release/test_command_stack
/workspace/FreeCrafter/build/Release/test_command_stack
/workspace/FreeCrafter/build/Debug/test_command_stack
/workspace/FreeCrafter/build/Debug/test_command_stack
/workspace/FreeCrafter/build/MinSizeRel/test_command_stack
/workspace/FreeCrafter/build/MinSizeRel/test_command_stack
/workspace/FreeCrafter/build/RelWithDebInfo/test_command_stack
/workspace/FreeCrafter/build/RelWithDebInfo/test_command_stack
/workspace/FreeCrafter/build/Deployment/test_command_stack
/workspace/FreeCrafter/build/Deployment/test_command_stack
/workspace/FreeCrafter/build/Development/test_command_stack
/workspace/FreeCrafter/build/Development/test_command_stack
workspace/FreeCrafter/build/test_command_stack
workspace/FreeCrafter/build/test_command_stack
workspace/FreeCrafter/build/Release/test_command_stack
workspace/FreeCrafter/build/Release/test_command_stack
workspace/FreeCrafter/build/Debug/test_command_stack
workspace/FreeCrafter/build/Debug/test_command_stack
workspace/FreeCrafter/build/MinSizeRel/test_command_stack
workspace/FreeCrafter/build/MinSizeRel/test_command_stack
workspace/FreeCrafter/build/RelWithDebInfo/test_command_stack
workspace/FreeCrafter/build/RelWithDebInfo/test_command_stack
workspace/FreeCrafter/build/Deployment/test_command_stack
workspace/FreeCrafter/build/Deployment/test_command_stack
workspace/FreeCrafter/build/Development/test_command_stack
workspace/FreeCrafter/build/Development/test_command_stack
Unable to find executable: /workspace/FreeCrafter/build/test_command_stack
 5/13 Test  #5: command_stack ....................***Not Run   0.00 sec
      Start  6: phase4_tools
Could not find executable /workspace/FreeCrafter/build/test_phase4
Looked in the following places:
/workspace/FreeCrafter/build/test_phase4
/workspace/FreeCrafter/build/test_phase4
/workspace/FreeCrafter/build/Release/test_phase4
/workspace/FreeCrafter/build/Release/test_phase4
/workspace/FreeCrafter/build/Debug/test_phase4
/workspace/FreeCrafter/build/Debug/test_phase4
/workspace/FreeCrafter/build/MinSizeRel/test_phase4
/workspace/FreeCrafter/build/MinSizeRel/test_phase4
/workspace/FreeCrafter/build/RelWithDebInfo/test_phase4
/workspace/FreeCrafter/build/RelWithDebInfo/test_phase4
/workspace/FreeCrafter/build/Deployment/test_phase4
/workspace/FreeCrafter/build/Deployment/test_phase4
/workspace/FreeCrafter/build/Development/test_phase4
/workspace/FreeCrafter/build/Development/test_phase4
workspace/FreeCrafter/build/test_phase4
workspace/FreeCrafter/build/test_phase4
workspace/FreeCrafter/build/Release/test_phase4
workspace/FreeCrafter/build/Release/test_phase4
workspace/FreeCrafter/build/Debug/test_phase4
workspace/FreeCrafter/build/Debug/test_phase4
workspace/FreeCrafter/build/MinSizeRel/test_phase4
workspace/FreeCrafter/build/MinSizeRel/test_phase4
workspace/FreeCrafter/build/RelWithDebInfo/test_phase4
workspace/FreeCrafter/build/RelWithDebInfo/test_phase4
workspace/FreeCrafter/build/Deployment/test_phase4
workspace/FreeCrafter/build/Deployment/test_phase4
workspace/FreeCrafter/build/Development/test_phase4
workspace/FreeCrafter/build/Development/test_phase4
Unable to find executable: /workspace/FreeCrafter/build/test_phase4
 6/13 Test  #6: phase4_tools .....................***Not Run   0.00 sec
      Start  7: phase5_object_management
Could not find executable /workspace/FreeCrafter/build/test_phase5
Looked in the following places:
/workspace/FreeCrafter/build/test_phase5
/workspace/FreeCrafter/build/test_phase5
/workspace/FreeCrafter/build/Release/test_phase5
/workspace/FreeCrafter/build/Release/test_phase5
/workspace/FreeCrafter/build/Debug/test_phase5
/workspace/FreeCrafter/build/Debug/test_phase5
/workspace/FreeCrafter/build/MinSizeRel/test_phase5
/workspace/FreeCrafter/build/MinSizeRel/test_phase5
/workspace/FreeCrafter/build/RelWithDebInfo/test_phase5
/workspace/FreeCrafter/build/RelWithDebInfo/test_phase5
/workspace/FreeCrafter/build/Deployment/test_phase5
/workspace/FreeCrafter/build/Deployment/test_phase5
/workspace/FreeCrafter/build/Development/test_phase5
/workspace/FreeCrafter/build/Development/test_phase5
workspace/FreeCrafter/build/test_phase5
workspace/FreeCrafter/build/test_phase5
workspace/FreeCrafter/build/Release/test_phase5
workspace/FreeCrafter/build/Release/test_phase5
workspace/FreeCrafter/build/Debug/test_phase5
workspace/FreeCrafter/build/Debug/test_phase5
workspace/FreeCrafter/build/MinSizeRel/test_phase5
workspace/FreeCrafter/build/MinSizeRel/test_phase5
workspace/FreeCrafter/build/RelWithDebInfo/test_phase5
workspace/FreeCrafter/build/RelWithDebInfo/test_phase5
workspace/FreeCrafter/build/Deployment/test_phase5
workspace/FreeCrafter/build/Deployment/test_phase5
workspace/FreeCrafter/build/Development/test_phase5
workspace/FreeCrafter/build/Development/test_phase5
Unable to find executable: /workspace/FreeCrafter/build/test_phase5
 7/13 Test  #7: phase5_object_management .........***Not Run   0.00 sec
      Start  8: phase6_advanced_tools
Could not find executable /workspace/FreeCrafter/build/test_phase6
Looked in the following places:
/workspace/FreeCrafter/build/test_phase6
/workspace/FreeCrafter/build/test_phase6
/workspace/FreeCrafter/build/Release/test_phase6
/workspace/FreeCrafter/build/Release/test_phase6
/workspace/FreeCrafter/build/Debug/test_phase6
/workspace/FreeCrafter/build/Debug/test_phase6
/workspace/FreeCrafter/build/MinSizeRel/test_phase6
/workspace/FreeCrafter/build/MinSizeRel/test_phase6
/workspace/FreeCrafter/build/RelWithDebInfo/test_phase6
/workspace/FreeCrafter/build/RelWithDebInfo/test_phase6
/workspace/FreeCrafter/build/Deployment/test_phase6
/workspace/FreeCrafter/build/Deployment/test_phase6
/workspace/FreeCrafter/build/Development/test_phase6
/workspace/FreeCrafter/build/Development/test_phase6
workspace/FreeCrafter/build/test_phase6
workspace/FreeCrafter/build/test_phase6
workspace/FreeCrafter/build/Release/test_phase6
workspace/FreeCrafter/build/Release/test_phase6
workspace/FreeCrafter/build/Debug/test_phase6
workspace/FreeCrafter/build/Debug/test_phase6
workspace/FreeCrafter/build/MinSizeRel/test_phase6
workspace/FreeCrafter/build/MinSizeRel/test_phase6
workspace/FreeCrafter/build/RelWithDebInfo/test_phase6
workspace/FreeCrafter/build/RelWithDebInfo/test_phase6
workspace/FreeCrafter/build/Deployment/test_phase6
workspace/FreeCrafter/build/Deployment/test_phase6
workspace/FreeCrafter/build/Development/test_phase6
workspace/FreeCrafter/build/Development/test_phase6
Unable to find executable: /workspace/FreeCrafter/build/test_phase6
 8/13 Test  #8: phase6_advanced_tools ............***Not Run   0.00 sec
      Start  9: tool_activation
Could not find executable /workspace/FreeCrafter/build/test_tool_activation
Looked in the following places:
/workspace/FreeCrafter/build/test_tool_activation
/workspace/FreeCrafter/build/test_tool_activation
/workspace/FreeCrafter/build/Release/test_tool_activation
/workspace/FreeCrafter/build/Release/test_tool_activation
/workspace/FreeCrafter/build/Debug/test_tool_activation
/workspace/FreeCrafter/build/Debug/test_tool_activation
/workspace/FreeCrafter/build/MinSizeRel/test_tool_activation
/workspace/FreeCrafter/build/MinSizeRel/test_tool_activation
/workspace/FreeCrafter/build/RelWithDebInfo/test_tool_activation
/workspace/FreeCrafter/build/RelWithDebInfo/test_tool_activation
/workspace/FreeCrafter/build/Deployment/test_tool_activation
/workspace/FreeCrafter/build/Deployment/test_tool_activation
/workspace/FreeCrafter/build/Development/test_tool_activation
/workspace/FreeCrafter/build/Development/test_tool_activation
workspace/FreeCrafter/build/test_tool_activation
workspace/FreeCrafter/build/test_tool_activation
workspace/FreeCrafter/build/Release/test_tool_activation
workspace/FreeCrafter/build/Release/test_tool_activation
workspace/FreeCrafter/build/Debug/test_tool_activation
workspace/FreeCrafter/build/Debug/test_tool_activation
workspace/FreeCrafter/build/MinSizeRel/test_tool_activation
workspace/FreeCrafter/build/MinSizeRel/test_tool_activation
workspace/FreeCrafter/build/RelWithDebInfo/test_tool_activation
workspace/FreeCrafter/build/RelWithDebInfo/test_tool_activation
workspace/FreeCrafter/build/Deployment/test_tool_activation
workspace/FreeCrafter/build/Deployment/test_tool_activation
workspace/FreeCrafter/build/Development/test_tool_activation
workspace/FreeCrafter/build/Development/test_tool_activation
Unable to find executable: /workspace/FreeCrafter/build/test_tool_activation
 9/13 Test  #9: tool_activation ..................***Not Run   0.00 sec
      Start 10: cursor_overlay
Could not find executable /workspace/FreeCrafter/build/test_cursor_overlay
Looked in the following places:
/workspace/FreeCrafter/build/test_cursor_overlay
/workspace/FreeCrafter/build/test_cursor_overlay
/workspace/FreeCrafter/build/Release/test_cursor_overlay
/workspace/FreeCrafter/build/Release/test_cursor_overlay
/workspace/FreeCrafter/build/Debug/test_cursor_overlay
/workspace/FreeCrafter/build/Debug/test_cursor_overlay
/workspace/FreeCrafter/build/MinSizeRel/test_cursor_overlay
/workspace/FreeCrafter/build/MinSizeRel/test_cursor_overlay
/workspace/FreeCrafter/build/RelWithDebInfo/test_cursor_overlay
/workspace/FreeCrafter/build/RelWithDebInfo/test_cursor_overlay
/workspace/FreeCrafter/build/Deployment/test_cursor_overlay
/workspace/FreeCrafter/build/Deployment/test_cursor_overlay
/workspace/FreeCrafter/build/Development/test_cursor_overlay
/workspace/FreeCrafter/build/Development/test_cursor_overlay
workspace/FreeCrafter/build/test_cursor_overlay
workspace/FreeCrafter/build/test_cursor_overlay
workspace/FreeCrafter/build/Release/test_cursor_overlay
workspace/FreeCrafter/build/Release/test_cursor_overlay
workspace/FreeCrafter/build/Debug/test_cursor_overlay
workspace/FreeCrafter/build/Debug/test_cursor_overlay
workspace/FreeCrafter/build/MinSizeRel/test_cursor_overlay
workspace/FreeCrafter/build/MinSizeRel/test_cursor_overlay
workspace/FreeCrafter/build/RelWithDebInfo/test_cursor_overlay
workspace/FreeCrafter/build/RelWithDebInfo/test_cursor_overlay
workspace/FreeCrafter/build/Deployment/test_cursor_overlay
workspace/FreeCrafter/build/Deployment/test_cursor_overlay
workspace/FreeCrafter/build/Development/test_cursor_overlay
workspace/FreeCrafter/build/Development/test_cursor_overlay
Unable to find executable: /workspace/FreeCrafter/build/test_cursor_overlay
10/13 Test #10: cursor_overlay ...................***Not Run   0.00 sec
      Start 11: advanced_tools
Could not find executable /workspace/FreeCrafter/build/test_advanced_tools
Looked in the following places:
/workspace/FreeCrafter/build/test_advanced_tools
/workspace/FreeCrafter/build/test_advanced_tools
/workspace/FreeCrafter/build/Release/test_advanced_tools
/workspace/FreeCrafter/build/Release/test_advanced_tools
/workspace/FreeCrafter/build/Debug/test_advanced_tools
/workspace/FreeCrafter/build/Debug/test_advanced_tools
/workspace/FreeCrafter/build/MinSizeRel/test_advanced_tools
/workspace/FreeCrafter/build/MinSizeRel/test_advanced_tools
/workspace/FreeCrafter/build/RelWithDebInfo/test_advanced_tools
/workspace/FreeCrafter/build/RelWithDebInfo/test_advanced_tools
/workspace/FreeCrafter/build/Deployment/test_advanced_tools
/workspace/FreeCrafter/build/Deployment/test_advanced_tools
/workspace/FreeCrafter/build/Development/test_advanced_tools
/workspace/FreeCrafter/build/Development/test_advanced_tools
workspace/FreeCrafter/build/test_advanced_tools
workspace/FreeCrafter/build/test_advanced_tools
workspace/FreeCrafter/build/Release/test_advanced_tools
workspace/FreeCrafter/build/Release/test_advanced_tools
workspace/FreeCrafter/build/Debug/test_advanced_tools
workspace/FreeCrafter/build/Debug/test_advanced_tools
workspace/FreeCrafter/build/MinSizeRel/test_advanced_tools
workspace/FreeCrafter/build/MinSizeRel/test_advanced_tools
workspace/FreeCrafter/build/RelWithDebInfo/test_advanced_tools
workspace/FreeCrafter/build/RelWithDebInfo/test_advanced_tools
workspace/FreeCrafter/build/Deployment/test_advanced_tools
workspace/FreeCrafter/build/Deployment/test_advanced_tools
workspace/FreeCrafter/build/Development/test_advanced_tools
workspace/FreeCrafter/build/Development/test_advanced_tools
Unable to find executable: /workspace/FreeCrafter/build/test_advanced_tools
11/13 Test #11: advanced_tools ...................***Not Run   0.00 sec
      Start 12: autosave_manager
Could not find executable /workspace/FreeCrafter/build/test_autosave
Looked in the following places:
/workspace/FreeCrafter/build/test_autosave
/workspace/FreeCrafter/build/test_autosave
/workspace/FreeCrafter/build/Release/test_autosave
/workspace/FreeCrafter/build/Release/test_autosave
/workspace/FreeCrafter/build/Debug/test_autosave
/workspace/FreeCrafter/build/Debug/test_autosave
/workspace/FreeCrafter/build/MinSizeRel/test_autosave
/workspace/FreeCrafter/build/MinSizeRel/test_autosave
/workspace/FreeCrafter/build/RelWithDebInfo/test_autosave
/workspace/FreeCrafter/build/RelWithDebInfo/test_autosave
/workspace/FreeCrafter/build/Deployment/test_autosave
/workspace/FreeCrafter/build/Deployment/test_autosave
/workspace/FreeCrafter/build/Development/test_autosave
/workspace/FreeCrafter/build/Development/test_autosave
workspace/FreeCrafter/build/test_autosave
workspace/FreeCrafter/build/test_autosave
workspace/FreeCrafter/build/Release/test_autosave
workspace/FreeCrafter/build/Release/test_autosave
workspace/FreeCrafter/build/Debug/test_autosave
workspace/FreeCrafter/build/Debug/test_autosave
workspace/FreeCrafter/build/MinSizeRel/test_autosave
workspace/FreeCrafter/build/MinSizeRel/test_autosave
workspace/FreeCrafter/build/RelWithDebInfo/test_autosave
workspace/FreeCrafter/build/RelWithDebInfo/test_autosave
workspace/FreeCrafter/build/Deployment/test_autosave
workspace/FreeCrafter/build/Deployment/test_autosave
workspace/FreeCrafter/build/Development/test_autosave
workspace/FreeCrafter/build/Development/test_autosave
Unable to find executable: /workspace/FreeCrafter/build/test_autosave
12/13 Test #12: autosave_manager .................***Not Run   0.00 sec
      Start 13: file_io_exporters
Could not find executable /workspace/FreeCrafter/build/test_exporters
Looked in the following places:
/workspace/FreeCrafter/build/test_exporters
/workspace/FreeCrafter/build/test_exporters
/workspace/FreeCrafter/build/Release/test_exporters
/workspace/FreeCrafter/build/Release/test_exporters
/workspace/FreeCrafter/build/Debug/test_exporters
/workspace/FreeCrafter/build/Debug/test_exporters
/workspace/FreeCrafter/build/MinSizeRel/test_exporters
/workspace/FreeCrafter/build/MinSizeRel/test_exporters
/workspace/FreeCrafter/build/RelWithDebInfo/test_exporters
/workspace/FreeCrafter/build/RelWithDebInfo/test_exporters
/workspace/FreeCrafter/build/Deployment/test_exporters
/workspace/FreeCrafter/build/Deployment/test_exporters
/workspace/FreeCrafter/build/Development/test_exporters
/workspace/FreeCrafter/build/Development/test_exporters
workspace/FreeCrafter/build/test_exporters
workspace/FreeCrafter/build/test_exporters
workspace/FreeCrafter/build/Release/test_exporters
workspace/FreeCrafter/build/Release/test_exporters
workspace/FreeCrafter/build/Debug/test_exporters
workspace/FreeCrafter/build/Debug/test_exporters
workspace/FreeCrafter/build/MinSizeRel/test_exporters
workspace/FreeCrafter/build/MinSizeRel/test_exporters
workspace/FreeCrafter/build/RelWithDebInfo/test_exporters
workspace/FreeCrafter/build/RelWithDebInfo/test_exporters
workspace/FreeCrafter/build/Deployment/test_exporters
workspace/FreeCrafter/build/Deployment/test_exporters
workspace/FreeCrafter/build/Development/test_exporters
workspace/FreeCrafter/build/Development/test_exporters
Unable to find executable: /workspace/FreeCrafter/build/test_exporters
13/13 Test #13: file_io_exporters ................***Not Run   0.00 sec

0% tests passed[0;0m, [0;31m13 tests failed[0;0m out of 13

Total Test time (real) =   0.01 sec

The following tests FAILED:
	[0;33m  1 - render_regression (Not Run)[0;0m
	[0;33m  2 - viewport_depth_range (Not Run)[0;0m
	[0;33m  3 - scene_settings (Not Run)[0;0m
	[0;33m  4 - scene_commands (Not Run)[0;0m
	[0;33m  5 - command_stack (Not Run)[0;0m
	[0;33m  6 - phase4_tools (Not Run)[0;0m
	[0;33m  7 - phase5_object_management (Not Run)[0;0m
	[0;33m  8 - phase6_advanced_tools (Not Run)[0;0m
	[0;33m  9 - tool_activation (Not Run)[0;0m
	[0;33m 10 - cursor_overlay (Not Run)[0;0m
	[0;33m 11 - advanced_tools (Not Run)[0;0m
	[0;33m 12 - autosave_manager (Not Run)[0;0m
	[0;33m 13 - file_io_exporters (Not Run)[0;0m
Errors while running CTest
# exit

Script done on 2025-11-07 18:13:41+00:00 [COMMAND_EXIT_CODE="8"]
```
