# simpsons - ReXGlue Recompiled Project
#
# SDK boilerplate lives in generated/rexglue.cmake.

cmake_minimum_required(VERSION 3.25)
project(simpsons LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(generated/rexglue.cmake)

# Sources
set(SIMPSONS_SOURCES
    src/main.cpp
)

if(WIN32)
    add_executable(simpsons WIN32 ${SIMPSONS_SOURCES})
else()
    add_executable(simpsons ${SIMPSONS_SOURCES})
endif()

rexglue_setup_target(simpsons)

# Windows POSIX shims for the few generated TUs that reference <sys/mman.h>
# (mprotect, neutered/no-op) and <unistd.h> (usleep). Scoped to this target.
if(WIN32)
    target_include_directories(simpsons PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/compat_win)
endif()
