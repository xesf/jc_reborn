cmake_minimum_required(VERSION 3.10)
project(jc_reborn C)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Compiler flags from original Makefile
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wpedantic")

# Source files (common to all platforms)
set(COMMON_SOURCES
    jc_reborn.c
    utils.c
    uncompress.c
    resource.c
    dump.c
    story.c
    walk.c
    calcpath.c
    ads.c
    ttm.c
    island.c
    bench.c
    graphics.c
    sound.c
    events.c
    config.c
)

# Platform detection and specific sources
if(EMSCRIPTEN)
    message(STATUS "Building for Web (Emscripten)")
    set(PLATFORM_SOURCES platform_web.c)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Os")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -sASYNCIFY -sALLOW_MEMORY_GROWTH -sSTANDALONE_WASM=0 -sEXIT_RUNTIME")
    set(CMAKE_EXECUTABLE_SUFFIX ".js")
    add_definitions(-DPLATFORM_WEB)
elseif(WIN32)
    message(STATUS "Building for Windows")
    set(PLATFORM_SOURCES platform_windows.c)
    add_definitions(-DPLATFORM_WINDOWS)
    # Static linking for MinGW cross-compilation
    if(CMAKE_CROSSCOMPILING)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc")
    endif()
    # Windows-specific libraries will be added below
elseif(APPLE)
    message(STATUS "Building for macOS")
    set(PLATFORM_SOURCES platform_macos.m)
    add_definitions(-DPLATFORM_MACOS)
    # macOS-specific frameworks will be added below
elseif(UNIX)
    message(STATUS "Building for Linux")
    set(PLATFORM_SOURCES platform_linux.c)
    add_definitions(-DPLATFORM_LINUX)
    # Linux-specific libraries will be added below
else()
    message(FATAL_ERROR "Unsupported platform")
endif()

# Create executable
add_executable(jc_reborn ${COMMON_SOURCES} ${PLATFORM_SOURCES})

# Set Objective-C compilation for macOS platform file
if(APPLE)
    set_source_files_properties(platform_macos.m PROPERTIES COMPILE_FLAGS "-x objective-c")
endif()

# Platform-specific linking
if(EMSCRIPTEN)
    # No additional libraries needed for Emscripten
    # Everything is handled through JavaScript
elseif(WIN32)
    # Windows libraries
    target_link_libraries(jc_reborn
        gdi32
        user32
        winmm
    )
elseif(APPLE)
    # macOS frameworks
    find_library(COCOA_FRAMEWORK Cocoa REQUIRED)
    find_library(AUDIO_TOOLBOX AudioToolbox REQUIRED)
    target_link_libraries(jc_reborn
        ${COCOA_FRAMEWORK}
        ${AUDIO_TOOLBOX}
    )
elseif(UNIX)
    # Linux libraries
    find_package(X11 REQUIRED)
    find_package(ALSA REQUIRED)
    find_package(Threads REQUIRED)
    
    target_include_directories(jc_reborn PRIVATE
        ${X11_INCLUDE_DIR}
        ${ALSA_INCLUDE_DIRS}
    )
    
    target_link_libraries(jc_reborn
        ${X11_LIBRARIES}
        ${ALSA_LIBRARIES}
        ${CMAKE_THREAD_LIBS_INIT}
        m
    )
endif()

# Include directories
target_include_directories(jc_reborn PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Installation
install(TARGETS jc_reborn DESTINATION bin)

# Print build configuration
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "C Flags: ${CMAKE_C_FLAGS}")
message(STATUS "Platform sources: ${PLATFORM_SOURCES}")
