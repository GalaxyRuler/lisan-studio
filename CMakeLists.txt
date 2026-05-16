cmake_minimum_required(VERSION 3.24)

project(LisanStudio
    VERSION 0.1.0
    DESCRIPTION "Arabic-first native IDE for .apy files"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Test Concurrent)

qt_standard_project_setup()

add_library(acs_core
    src/CommandRegistry.h
    src/CommandRegistry.cpp
    src/DocumentFileIO.h
    src/DocumentFileIO.cpp
    src/DocumentRegistry.h
    src/DocumentRegistry.cpp
    src/EditorFindService.h
    src/EditorFindService.cpp
    src/OutputTranscript.h
    src/OutputTranscript.cpp
    src/ApySnippetService.h
    src/ApySnippetService.cpp
    src/UnsavedChangesGuard.h
    src/UnsavedChangesGuard.cpp
    src/WorkbenchState.h
    src/WorkbenchState.cpp
    src/ApyHighlighter.h
    src/ApyHighlighter.cpp
    src/EditorSurface.h
    src/EditorSurface.cpp
    src/ProjectModel.h
    src/ProjectModel.cpp
    src/ProjectFileOperations.h
    src/ProjectFileOperations.cpp
    src/ProjectReplaceService.h
    src/ProjectReplaceService.cpp
    src/RuntimeRunner.h
    src/RuntimeRunner.cpp
    src/RuntimeRunConfiguration.h
    src/RuntimeRunConfiguration.cpp
    src/RuntimeRunConfigurationStore.h
    src/RuntimeRunConfigurationStore.cpp
    src/RuntimeHistory.h
    src/RuntimeHistory.cpp
    src/RuntimeProblemParser.h
    src/RuntimeProblemParser.cpp
    src/TerminalProfileModel.h
    src/TerminalProfileModel.cpp
    src/SearchService.h
    src/SearchService.cpp
    src/ShortcutSettingsModel.h
    src/ShortcutSettingsModel.cpp
    src/SettingsDialogModel.h
    src/SettingsDialogModel.cpp
    src/SettingsStore.h
    src/SettingsStore.cpp
    src/WorkspaceSettingsStore.h
    src/WorkspaceSettingsStore.cpp
)

target_include_directories(acs_core PUBLIC src)
target_link_libraries(acs_core PUBLIC Qt6::Core Qt6::Gui Qt6::Widgets)

qt_add_executable(LisanStudio WIN32
    src/main.cpp
    src/MainWindow.h
    src/MainWindow.cpp
    resources/app.rc
    resources/resources.qrc
)

target_link_libraries(LisanStudio PRIVATE acs_core Qt6::Concurrent)

install(TARGETS LisanStudio
    RUNTIME DESTINATION .)

enable_testing()

qt_add_executable(acs_editor_tests
    tests/TestEditorSurface.cpp
)

target_link_libraries(acs_editor_tests PRIVATE acs_core Qt6::Test)

qt_add_executable(acs_command_registry_tests
    tests/TestCommandRegistry.cpp
)

target_link_libraries(acs_command_registry_tests PRIVATE acs_core Qt6::Test)

qt_add_executable(acs_workbench_state_tests
    tests/TestWorkbenchState.cpp
)

target_link_libraries(acs_workbench_state_tests PRIVATE acs_core Qt6::Test)

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

target_link_libraries(acs_main_window_tests PRIVATE acs_core Qt6::Test Qt6::Concurrent)

qt_add_executable(acs_editor_torture_tests
    tests/TestEditorTorture.cpp
)

target_link_libraries(acs_editor_torture_tests PRIVATE acs_core Qt6::Test)

add_test(NAME acs_editor_tests COMMAND acs_editor_tests)
add_test(NAME acs_command_registry_tests COMMAND acs_command_registry_tests)
add_test(NAME acs_workbench_state_tests COMMAND acs_workbench_state_tests)
add_test(NAME acs_project_runtime_tests COMMAND acs_project_runtime_tests)
add_test(NAME acs_main_window_tests COMMAND acs_main_window_tests)
add_test(NAME acs_editor_torture_tests COMMAND acs_editor_torture_tests)
