cmake_minimum_required(VERSION 3.16)
project(imggetpixels VERSION 1.1.0 LANGUAGES CXX)

# Standard C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files
set(SOURCES
    src/main.cpp
    src/include/ArgumentParser.cpp
)

# Configure version header
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/include/version.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/version.h"
)

# Get current year for copyright
string(TIMESTAMP COPYRIGHT_YEAR "%Y")

# Windows Resource file
if(WIN32)
    set(PROJECT_ICON_PATH "${CMAKE_CURRENT_SOURCE_DIR}/docs/media/icons/imggetpixels.ico")
    file(TO_NATIVE_PATH "${PROJECT_ICON_PATH}" PROJECT_ICON_PATH)
    string(REPLACE "\\" "\\\\" PROJECT_ICON_PATH "${PROJECT_ICON_PATH}")

    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/src/include/resource.rc.in"
        "${CMAKE_CURRENT_BINARY_DIR}/resource.rc"
    )
    list(APPEND SOURCES "${CMAKE_CURRENT_BINARY_DIR}/resource.rc")
endif()

# Header include paths
include_directories(src/include)
include_directories("${CMAKE_CURRENT_BINARY_DIR}") # For generated version.h

# Find Threads (required for std::async/std::thread on Linux/Unix)
find_package(Threads REQUIRED)

# Executable
add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)

# Precompiled Headers (Native CMake support 3.16+)
# This will handle pch.h correctly across GCC, Clang, and MSVC.
target_precompile_headers(${PROJECT_NAME} PRIVATE src/include/pch.h)

# Compilation Options
if(MSVC)
    # /utf-8 ensures source is read correctly; /W4 for high-quality warnings
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /utf-8)
    target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
endif()

# Option for static linking (useful for the "single file executable" goal)
option(BUILD_STATIC "Build as a static executable" OFF)
if(BUILD_STATIC)
    if(MSVC)
        # For MSVC, use /MT to link the runtime statically
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    else()
        # For GCC/Clang, use -static
        target_link_options(${PROJECT_NAME} PRIVATE -static)
    endif()
endif()

# Install rules
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)
