cmake_minimum_required(VERSION 3.21)
project(FreeCrafter LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

option(FREECRAFTER_ENABLE_ASSIMP "Enable Assimp importer integration" ON)
option(FREECRAFTER_ENABLE_ODA "Enable ODA/Teigha DXF/DWG importer integration" ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGL OpenGLWidgets Svg)

set(FREECRAFTER_HAS_ASSIMP FALSE)
if(FREECRAFTER_ENABLE_ASSIMP)
  find_package(assimp CONFIG QUIET)
  if(NOT assimp_FOUND)
    find_package(Assimp QUIET)
  endif()
  if(TARGET assimp::assimp OR ASSIMP_LIBRARIES)
    set(FREECRAFTER_HAS_ASSIMP TRUE)
  endif()
endif()

set(FREECRAFTER_HAS_ODA FALSE)
if(FREECRAFTER_ENABLE_ODA)
  find_package(ODA QUIET)
  find_package(Teigha QUIET)
  if(ODA_FOUND OR Teigha_FOUND)
    set(FREECRAFTER_HAS_ODA TRUE)
  endif()
endif()

add_library(freecrafter_lib
    src/MainWindow.cpp
    src/GLViewport.cpp
    src/Renderer.cpp
    src/SunModel.cpp
    src/SunSettings.cpp
    src/CameraController.cpp
    src/CameraNavigation.cpp
    src/HotkeyManager.cpp
    src/NavigationPreferences.cpp
    src/PalettePreferences.cpp
    src/Navigation/ViewPresetManager.cpp
    src/GeometryKernel/Curve.cpp
    src/GeometryKernel/HalfEdgeMesh.cpp
    src/GeometryKernel/MeshUtils.cpp
    src/GeometryKernel/Solid.cpp
    src/GeometryKernel/TransformUtils.cpp
    src/GeometryKernel/GeometryKernel.cpp
    src/GeometryKernel/Serialization.cpp
    src/Scene/Document.cpp
    src/Scene/SceneSettings.cpp
    src/Scene/SectionPlane.cpp
    src/FileIO/Importers/FileImporter.cpp
    src/Tools/Tool.cpp
    src/Tools/ToolGeometryUtils.cpp
    src/Tools/LineTool.cpp
    src/Tools/SmartSelectTool.cpp
    src/Tools/MoveTool.cpp
    src/Tools/RotateTool.cpp
    src/Tools/ScaleTool.cpp
    src/Tools/ExtrudeTool.cpp
    src/Tools/GroundProjection.cpp
    src/Tools/DrawingTools.cpp
    src/Tools/ModificationTools.cpp
    src/Tools/SectionTool.cpp
    src/Tools/OrbitTool.cpp
    src/Tools/PanTool.cpp
    src/Tools/ZoomTool.cpp
    src/Tools/ToolManager.cpp
    src/Phase6/AdvancedModeling.cpp
    src/Interaction/InferenceEngine.cpp
    src/FileIO/SceneIOFormat.cpp
    src/FileIO/Exporters/SceneExporter.cpp
    src/FileIO/Importers/SceneImporter.cpp
    src/ui/MeasurementWidget.cpp
    src/ui/ViewSettingsDialog.cpp
    src/ui/EnvironmentPanel.cpp
    resources.qrc
)

target_include_directories(freecrafter_lib PRIVATE src src/GeometryKernel src/Tools src/Interaction src/ui src/Scene src/Phase6 src/FileIO)
target_link_libraries(freecrafter_lib PUBLIC Qt6::Widgets Qt6::OpenGL Qt6::OpenGLWidgets Qt6::Svg)

if(FREECRAFTER_HAS_ASSIMP)
  if(TARGET assimp::assimp)
    target_link_libraries(freecrafter_lib PRIVATE assimp::assimp)
  elseif(ASSIMP_LIBRARIES)
    target_link_libraries(freecrafter_lib PRIVATE ${ASSIMP_LIBRARIES})
    if(ASSIMP_INCLUDE_DIRS)
      target_include_directories(freecrafter_lib PRIVATE ${ASSIMP_INCLUDE_DIRS})
    endif()
  endif()
  target_compile_definitions(freecrafter_lib PRIVATE FREECRAFTER_USE_ASSIMP)
endif()

if(FREECRAFTER_HAS_ODA)
  target_compile_definitions(freecrafter_lib PRIVATE FREECRAFTER_USE_ODA)
endif()

add_executable(${PROJECT_NAME}
    src/main.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE freecrafter_lib)
# Installation
install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION bin
        BUNDLE DESTINATION .)

if(COMMAND qt_generate_deploy_app_script)
  qt_generate_deploy_app_script(
    TARGET ${PROJECT_NAME}
    OUTPUT_SCRIPT qt_deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
  )
  install(SCRIPT ${qt_deploy_script})
endif()

# Tests
enable_testing()
add_executable(test_render tests/test_render.cpp)

target_include_directories(test_render PRIVATE src)

target_link_libraries(test_render
    PRIVATE freecrafter_lib
            Qt6::Widgets Qt6::OpenGL Qt6::OpenGLWidgets Qt6::Svg)

add_executable(test_scene_settings tests/test_scene_settings.cpp)
target_include_directories(test_scene_settings PRIVATE src)
target_link_libraries(test_scene_settings PRIVATE freecrafter_lib)

set(_qt_bin_dir "${CMAKE_SOURCE_DIR}/qt/6.5.3/msvc2019_64/bin")
add_test(NAME render_regression COMMAND $<TARGET_FILE:test_render>)
if(EXISTS "${_qt_bin_dir}")
  string(REPLACE ";" "\;" _system_path "$ENV{PATH}")
  set_property(TEST render_regression PROPERTY ENVIRONMENT "PATH=${_qt_bin_dir}\;${_system_path}")
endif()

add_test(NAME scene_settings COMMAND $<TARGET_FILE:test_scene_settings>)

add_executable(test_phase4 tests/test_phase4.cpp)
target_include_directories(test_phase4 PRIVATE src)
target_link_libraries(test_phase4 PRIVATE freecrafter_lib Qt6::Widgets Qt6::OpenGL Qt6::OpenGLWidgets Qt6::Svg)
add_test(NAME phase4_tools COMMAND $<TARGET_FILE:test_phase4>)

add_executable(test_phase5 tests/test_phase5.cpp)
target_include_directories(test_phase5 PRIVATE src)
target_link_libraries(test_phase5 PRIVATE freecrafter_lib Qt6::Widgets Qt6::OpenGL Qt6::OpenGLWidgets Qt6::Svg)
add_test(NAME phase5_object_management COMMAND $<TARGET_FILE:test_phase5>)

add_executable(test_phase6 tests/test_phase6.cpp)
target_include_directories(test_phase6 PRIVATE src)
target_link_libraries(test_phase6 PRIVATE freecrafter_lib Qt6::Widgets Qt6::OpenGL Qt6::OpenGLWidgets Qt6::Svg)
add_test(NAME phase6_advanced_tools COMMAND $<TARGET_FILE:test_phase6>)

add_executable(test_file_import tests/file_io/test_file_import.cpp)
target_include_directories(test_file_import PRIVATE src)
target_link_libraries(test_file_import PRIVATE freecrafter_lib Qt6::Widgets Qt6::OpenGL Qt6::OpenGLWidgets Qt6::Svg)
add_test(NAME file_import COMMAND $<TARGET_FILE:test_file_import>)

# Include Windows redistributable if present

# Uninstall target
configure_file(cmake/cmake_uninstall.cmake.in
               ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
               IMMEDIATE @ONLY)
add_custom_target(uninstall
  COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)

# Packaging with CPack
set(CPACK_PACKAGE_NAME "FreeCrafter")
set(CPACK_PACKAGE_VENDOR "FreeCrafter Project")
set(CPACK_PACKAGE_VERSION "0.1.0")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "FreeCrafter")

set(CPACK_GENERATOR "NSIS;DragNDrop;TGZ")
set(CPACK_NSIS_DISPLAY_NAME "FreeCrafter")
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
set(CPACK_NSIS_CREATE_ICONS_EXTRA
  "CreateShortCut '$SMPROGRAMS\\FreeCrafter\\FreeCrafter.lnk' '$INSTDIR\\bin\\FreeCrafter.exe'")
set(CPACK_NSIS_DELETE_ICONS_EXTRA
  "Delete '$SMPROGRAMS\\FreeCrafter\\FreeCrafter.lnk'"
  "RMDir '$SMPROGRAMS\\FreeCrafter'")

if(WIN32)
  set(CPACK_NSIS_INCLUDE_SCRIPT "${CMAKE_SOURCE_DIR}/installer/windows/nsis/prereqs.nsi")
  set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS " !insertmacro InstallPrereqs ")
endif()

include(CPack)






