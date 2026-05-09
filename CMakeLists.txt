cmake_minimum_required(VERSION 3.24)

project(ArabicCodeStudioQt
    VERSION 0.1.0
    DESCRIPTION "Arabic-first native IDE for .apy files"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Test)

qt_standard_project_setup()

add_library(acs_core
    src/ApyHighlighter.h
    src/ApyHighlighter.cpp
    src/EditorSurface.h
    src/EditorSurface.cpp
    src/ProjectModel.h
    src/ProjectModel.cpp
    src/RuntimeRunner.h
    src/RuntimeRunner.cpp
    src/SearchService.h
    src/SearchService.cpp
    src/SettingsStore.h
    src/SettingsStore.cpp
)

target_include_directories(acs_core PUBLIC src)
target_link_libraries(acs_core PUBLIC Qt6::Core Qt6::Gui Qt6::Widgets)

qt_add_executable(ArabicCodeStudioQt
    src/main.cpp
    src/MainWindow.h
    src/MainWindow.cpp
    resources/app.rc
    resources/resources.qrc
)

target_link_libraries(ArabicCodeStudioQt PRIVATE acs_core)

install(TARGETS ArabicCodeStudioQt
    RUNTIME DESTINATION .)

enable_testing()

qt_add_executable(acs_editor_tests
    tests/TestEditorSurface.cpp
)

target_link_libraries(acs_editor_tests PRIVATE acs_core Qt6::Test)

qt_add_executable(acs_project_runtime_tests
    tests/TestProjectSearchRuntime.cpp
)

target_link_libraries(acs_project_runtime_tests PRIVATE acs_core Qt6::Test)

qt_add_executable(acs_main_window_tests
    src/MainWindow.h
    src/MainWindow.cpp
    resources/resources.qrc
    tests/TestMainWindow.cpp
)

target_link_libraries(acs_main_window_tests PRIVATE acs_core Qt6::Test)

add_test(NAME acs_editor_tests COMMAND acs_editor_tests)
add_test(NAME acs_project_runtime_tests COMMAND acs_project_runtime_tests)
add_test(NAME acs_main_window_tests COMMAND acs_main_window_tests)
