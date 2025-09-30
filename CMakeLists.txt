cmake_minimum_required(VERSION 3.21)
project(FreeCrafter LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGL OpenGLWidgets Svg)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/MainWindow.cpp
    src/GLViewport.cpp
    src/CameraController.cpp
    src/HotkeyManager.cpp
    src/GeometryKernel/Curve.cpp
    src/GeometryKernel/HalfEdgeMesh.cpp
    src/GeometryKernel/MeshUtils.cpp
    src/GeometryKernel/Solid.cpp
    src/GeometryKernel/GeometryKernel.cpp
    src/GeometryKernel/Serialization.cpp
    src/Tools/SelectionTool.cpp
    src/Tools/SketchTool.cpp
    src/Tools/ExtrudeTool.cpp
    src/Tools/ToolManager.cpp
    src/Interaction/InferenceEngine.cpp
    src/ui/MeasurementWidget.cpp
    resources.qrc
)

target_include_directories(${PROJECT_NAME} PRIVATE src src/GeometryKernel src/Tools src/Interaction src/ui)
target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Widgets Qt6::OpenGL Qt6::OpenGLWidgets Qt6::Svg)
# Installation
install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION bin
        BUNDLE DESTINATION .)

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
