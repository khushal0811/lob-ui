cmake_minimum_required(VERSION 3.20)
project(lob_qt CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_AUTOMOC ON)

add_compile_options(-Wall -Wextra -Wpedantic -Werror)

# Qt 6
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Charts)
qt_standard_project_setup()

# Import lob_core from sibling project (pre-built release library)
set(LOB_ENGINE_DIR "${CMAKE_SOURCE_DIR}/../lob-engine")
add_library(lob_core STATIC IMPORTED)
set_target_properties(lob_core PROPERTIES
    IMPORTED_LOCATION "${LOB_ENGINE_DIR}/build_release/src/liblob_core.a"
    INTERFACE_INCLUDE_DIRECTORIES "${LOB_ENGINE_DIR}/src"
)

# Collect all .cpp sources under src/
file(GLOB_RECURSE LOB_QT_SOURCES src/*.cpp)

qt_add_executable(lob_qt ${LOB_QT_SOURCES})

qt_add_resources(lob_qt "resources"
    PREFIX "/"
    FILES src/style.qss
)

target_include_directories(lob_qt PRIVATE src/)
target_link_libraries(lob_qt PRIVATE
    lob_core
    Qt6::Core
    Qt6::Widgets
    Qt6::Charts
)
