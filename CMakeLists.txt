cmake_minimum_required(VERSION 3.21)
project(FreeCrafter LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGL OpenGLWidgets Svg)

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
    src/Tools/Tool.cpp
    src/Tools/ToolGeometryUtils.cpp
    src/Tools/LineTool.cpp
    src/Tools/SmartSelectTool.cpp
    src/Tools/MoveTool.cpp
    src/Tools/RotateTool.cpp
    src/Tools/ScaleTool.cpp
    src/Tools/ExtrudeTool.cpp
    src/Tools/SectionTool.cpp
    src/Tools/OrbitTool.cpp
    src/Tools/PanTool.cpp
    src/Tools/ZoomTool.cpp
    src/Tools/ToolManager.cpp
    src/Interaction/InferenceEngine.cpp
    src/ui/MeasurementWidget.cpp
    src/ui/ViewSettingsDialog.cpp
    src/ui/EnvironmentPanel.cpp
    resources.qrc
)

target_include_directories(freecrafter_lib PRIVATE src src/GeometryKernel src/Tools src/Interaction src/ui src/Scene)
target_link_libraries(freecrafter_lib PUBLIC Qt6::Widgets Qt6::OpenGL Qt6::OpenGLWidgets Qt6::Svg)

add_executable(${PROJECT_NAME}
    src/main.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE freecrafter_lib)
# Installation
install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION bin
        BUNDLE DESTINATION .)

# Tests
enable_testing()
add_executable(test_render tests/test_render.cpp)

target_include_directories(test_render PRIVATE src)

target_link_libraries(test_render
    PRIVATE freecrafter_lib
            Qt6::Widgets Qt6::OpenGL Qt6::OpenGLWidgets Qt6::Svg)

set(_qt_bin_dir "${CMAKE_SOURCE_DIR}/qt/6.5.3/msvc2019_64/bin")
add_test(NAME render_regression COMMAND $<TARGET_FILE:test_render>)
if(EXISTS "${_qt_bin_dir}")
  string(REPLACE ";" "\;" _system_path "$ENV{PATH}")
  set_property(TEST render_regression PROPERTY ENVIRONMENT "PATH=${_qt_bin_dir}\;${_system_path}")
endif()

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






