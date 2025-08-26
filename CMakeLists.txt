cmake_minimum_required(VERSION 3.21)
project(FreeCrafter LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGL OpenGLWidgets)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/MainWindow.cpp
    src/GLViewport.cpp
    src/CameraController.cpp
    src/GeometryKernel/Curve.cpp
    src/GeometryKernel/Solid.cpp
    src/GeometryKernel/GeometryKernel.cpp
    src/GeometryKernel/Serialization.cpp
    src/Tools/SelectionTool.cpp
    src/Tools/SketchTool.cpp
    src/Tools/ExtrudeTool.cpp
    src/Tools/ToolManager.cpp
    resources.qrc
)

target_include_directories(${PROJECT_NAME} PRIVATE src src/GeometryKernel src/Tools)
target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Widgets Qt6::OpenGL Qt6::OpenGLWidgets)
