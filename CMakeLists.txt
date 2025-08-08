cmake_minimum_required(VERSION 3.21)
project(FreeCrafter LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGL OpenGLWidgets)

add_executable(FreeCrafter
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

target_include_directories(FreeCrafter PRIVATE src src/GeometryKernel src/Tools)

target_link_libraries(FreeCrafter PRIVATE Qt6::Widgets Qt6::OpenGL Qt6::OpenGLWidgets)

# After building, attempt to bundle Qt runtime dependencies so the executable
# can run on machines without a Qt installation.  Each platform uses its
# corresponding deployment tool if it is available in PATH.
if(WIN32)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt)
    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET FreeCrafter POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}" "$<TARGET_FILE:FreeCrafter>"
            COMMENT "Running windeployqt to copy Qt DLLs" VERBATIM)
    endif()
elseif(APPLE)
    find_program(MACDEPLOYQT_EXECUTABLE macdeployqt)
    if(MACDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET FreeCrafter POST_BUILD
            COMMAND "${MACDEPLOYQT_EXECUTABLE}" "$<TARGET_FILE:FreeCrafter>"
            COMMENT "Running macdeployqt to bundle Qt frameworks" VERBATIM)
    endif()
else()
    find_program(LINUXDEPLOYQT_EXECUTABLE linuxdeployqt)
    if(LINUXDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET FreeCrafter POST_BUILD
            COMMAND "${LINUXDEPLOYQT_EXECUTABLE}" "$<TARGET_FILE:FreeCrafter>"
                    -qmlimport="$<TARGET_FILE_DIR:FreeCrafter>"
            COMMENT "Running linuxdeployqt to bundle Qt libraries" VERBATIM)
    endif()
endif()
