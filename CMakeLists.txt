cmake_minimum_required(VERSION 3.24)

project(TaskManagerOG
    VERSION 0.1.0
    DESCRIPTION "Task Manager OG by Don PELY"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)
qt_standard_project_setup()

add_library(tmog_core STATIC
    src/core/settings.cpp
    src/core/settings.h
    src/core/version.h
)
target_include_directories(tmog_core PUBLIC src)
target_link_libraries(tmog_core PUBLIC Qt6::Core)
target_compile_definitions(tmog_core PUBLIC
    TMOG_VERSION="${PROJECT_VERSION}"
    TMOG_PRODUCT_NAME="Task Manager OG by Don PELY"
)

if(MSVC)
    target_compile_options(tmog_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(tmog_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

qt_add_executable(TaskManagerOG
    src/app/main.cpp
    src/ui/main_window.cpp
    src/ui/main_window.h
    src/ui/command_palette.cpp
    src/ui/command_palette.h
    src/ui/destination_page.cpp
    src/ui/destination_page.h
    src/ui/settings_dialog.cpp
    src/ui/settings_dialog.h
    src/ui/theme.cpp
    src/ui/theme.h
)
target_link_libraries(TaskManagerOG PRIVATE tmog_core Qt6::Widgets)

if(MSVC)
    target_compile_options(TaskManagerOG PRIVATE /W4 /permissive-)
else()
    target_compile_options(TaskManagerOG PRIVATE -Wall -Wextra -Wpedantic)
endif()

set_target_properties(TaskManagerOG PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
    OUTPUT_NAME "TaskManagerOG"
    MACOSX_BUNDLE_BUNDLE_NAME "Task Manager OG"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.donpely.taskmanagerog"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
)

add_executable(tmog_cli
    src/app/cli_main.cpp
)
target_link_libraries(tmog_cli PRIVATE tmog_core)
set_target_properties(tmog_cli PROPERTIES OUTPUT_NAME "tmog-cli")

if(MSVC)
    target_compile_options(tmog_cli PRIVATE /W4 /permissive-)
else()
    target_compile_options(tmog_cli PRIVATE -Wall -Wextra -Wpedantic)
endif()

install(TARGETS TaskManagerOG tmog_cli
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)

enable_testing()

add_executable(tmog_tests
    tests/test_main.cpp
    src/ui/theme.cpp
    src/ui/theme.h
)
target_link_libraries(tmog_tests PRIVATE tmog_core Qt6::Widgets)
target_include_directories(tmog_tests PRIVATE src)

if(MSVC)
    target_compile_options(tmog_tests PRIVATE /W4 /permissive-)
else()
    target_compile_options(tmog_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME tmog_tests COMMAND tmog_tests)
