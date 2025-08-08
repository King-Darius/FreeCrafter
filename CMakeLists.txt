cmake_minimum_required(VERSION 3.21)
project(FreeSketch LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGL OpenGLWidgets)

add_executable(FreeSketch
    src/main.cpp
    src/MainWindow.cpp
    src/GLViewport.cpp
    src/CameraController.cpp
    src/GeometryKernel/Curve.cpp
    src/GeometryKernel/Solid.cpp
    src/GeometryKernel/GeometryKernel.cpp
    src/Tools/SelectionTool.cpp
    src/Tools/SketchTool.cpp
    src/Tools/ExtrudeTool.cpp
    src/Tools/ToolManager.cpp
    resources.qrc
)

target_include_directories(FreeSketch PRIVATE src src/GeometryKernel src/Tools)

target_link_libraries(FreeSketch PRIVATE Qt6::Widgets Qt6::OpenGL Qt6::OpenGLWidgets)

# On Windows, deploy Qt runtime dependencies next to the built executable
if(WIN32)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt)
    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET FreeSketch POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}" "$<TARGET_FILE:FreeSketch>"
            COMMENT "Running windeployqt to copy Qt DLLs" VERBATIM)
    endif()
endif()
