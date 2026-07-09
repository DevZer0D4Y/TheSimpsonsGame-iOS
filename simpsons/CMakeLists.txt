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
    # Zero-behavior-change instrumentation of the two guest D3D draw entry
    # points (the only PM4 DRAW_INDX writers in the image) -- measures how
    # batchable the draw stream is before any draw-merging work is attempted.
    src/draw_telemetry.cpp
)

if(WIN32)
    add_executable(simpsons WIN32 ${SIMPSONS_SOURCES})
else()
    add_executable(simpsons ${SIMPSONS_SOURCES})
endif()

rexglue_setup_target(simpsons)

# The generated translation units are where nearly all guest CPU time goes,
# but CMake's stock RelWithDebInfo only compiles them at -O2, and local builds
# never got the -march=x86-64-v3 that CI already ships in every release
# (build.yml/release.yml pass it on the command line). Apply both directly on
# the target so every build gets them: target_compile_options land after the
# per-config flags on the compile line, so -O3 wins over the default -O2.
# x86-64-v3 (AVX2/BMI2/FMA, Haswell/Zen1+) matches what releases already
# require, so this adds no new hardware exposure.
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU"
   AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    target_compile_options(simpsons PRIVATE
        $<$<CONFIG:RelWithDebInfo,Release>:-O3>
        -march=x86-64-v3
    )
endif()

# Windows POSIX shims for the few generated TUs that reference <sys/mman.h>
# (mprotect, neutered/no-op) and <unistd.h> (usleep). Scoped to this target.
if(WIN32)
    target_include_directories(simpsons PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/compat_win)
endif()
