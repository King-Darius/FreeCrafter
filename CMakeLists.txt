cmake_minimum_required(VERSION 3.21)
project(FreeCrafter LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGL OpenGLWidgets)

add_executable(${PROJECT_NAME} WIN32 MACOSX_BUNDLE
    src/main.cpp
    src/MainWindow.cpp
    src/GLViewport.cpp
    src/CameraController.cpp
    src/GeometryKernel/Curve.cpp
    src/GeometryKernel/Solid.cpp
    src/GeometryKernel/Serialization.cpp
    src/GeometryKernel/GeometryKernel.cpp
    src/Tools/SelectionTool.cpp
    src/Tools/SketchTool.cpp
    src/Tools/ExtrudeTool.cpp
    src/Tools/ToolManager.cpp
    resources.qrc
)

target_include_directories(${PROJECT_NAME} PRIVATE src src/GeometryKernel src/Tools)

target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Widgets Qt6::OpenGL Qt6::OpenGLWidgets)

install(TARGETS ${PROJECT_NAME} BUNDLE DESTINATION .)

# After building, attempt to bundle Qt runtime dependencies so the executable
# can run on machines without a Qt installation.  Each platform uses its
# corresponding deployment tool if it is available in PATH.
if(WIN32)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt)
    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}" "$<TARGET_FILE:${PROJECT_NAME}>"
            COMMENT "Running windeployqt to copy Qt DLLs" VERBATIM)
    endif()
elseif(APPLE)
    find_program(MACDEPLOYQT_EXECUTABLE macdeployqt)
    if(MACDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND "${MACDEPLOYQT_EXECUTABLE}" "$<TARGET_FILE:${PROJECT_NAME}>"
            COMMENT "Running macdeployqt to bundle Qt frameworks" VERBATIM)
    endif()
else()
    find_program(LINUXDEPLOYQT_EXECUTABLE linuxdeployqt)
    if(LINUXDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND "${LINUXDEPLOYQT_EXECUTABLE}" "$<TARGET_FILE:${PROJECT_NAME}>"
                    -qmlimport="$<TARGET_FILE_DIR:${PROJECT_NAME}>"
            COMMENT "Running linuxdeployqt to bundle Qt libraries" VERBATIM)
    endif()
endif()

# Testing setup
include(CTest)
enable_testing()

add_executable(test_geometry
    tests/test_geometry.cpp
    src/GeometryKernel/GeometryKernel.cpp
    src/GeometryKernel/Curve.cpp
    src/GeometryKernel/Serialization.cpp
    src/GeometryKernel/Solid.cpp
)
target_include_directories(test_geometry PRIVATE src/GeometryKernel)
add_test(NAME test_geometry COMMAND test_geometry)

add_executable(test_ui
    tests/test_ui.cpp
    src/MainWindow.cpp
    src/GLViewport.cpp
    src/CameraController.cpp
    src/GeometryKernel/GeometryKernel.cpp
    src/GeometryKernel/Curve.cpp
    src/GeometryKernel/Serialization.cpp
    src/GeometryKernel/Solid.cpp
    src/Tools/SelectionTool.cpp
    src/Tools/SketchTool.cpp
    src/Tools/ExtrudeTool.cpp
    src/Tools/ToolManager.cpp
    resources.qrc
)
target_include_directories(test_ui PRIVATE src src/GeometryKernel src/Tools)
target_link_libraries(test_ui PRIVATE Qt6::Widgets Qt6::OpenGL Qt6::OpenGLWidgets)
add_test(NAME test_ui COMMAND test_ui -platform offscreen)

set(CPACK_GENERATOR "NSIS;DragNDrop")
include(CPack)
