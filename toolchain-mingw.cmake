# CMake toolchain file for cross-compiling to Windows using MinGW

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Try to find the MinGW compiler (works on both macOS and Linux)
find_program(MINGW_C_COMPILER NAMES x86_64-w64-mingw32-gcc)
find_program(MINGW_CXX_COMPILER NAMES x86_64-w64-mingw32-g++)
find_program(MINGW_RC_COMPILER NAMES x86_64-w64-mingw32-windres)

if(MINGW_C_COMPILER)
    set(CMAKE_C_COMPILER ${MINGW_C_COMPILER})
else()
    message(FATAL_ERROR "MinGW C compiler not found")
endif()

if(MINGW_CXX_COMPILER)
    set(CMAKE_CXX_COMPILER ${MINGW_CXX_COMPILER})
endif()

if(MINGW_RC_COMPILER)
    set(CMAKE_RC_COMPILER ${MINGW_RC_COMPILER})
endif()

# Search for programs in the build host directories
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# Search for libraries and headers in the target directories
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Ensure we generate Windows executables
set(CMAKE_EXECUTABLE_SUFFIX ".exe")
