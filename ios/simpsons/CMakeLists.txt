# The Simpsons Game - ReXGlue Recompiled Project, iOS build.
#
# Game hooks come from YesterMester/TheSimpsonsGameRecomp (simpsons/src); the
# runtime is the iOS port of the ReXGlue SDK, found through rexglue_DIR (see
# CMakePresets.json). SDK boilerplate lives in generated/rexglue.cmake.

cmake_minimum_required(VERSION 3.25)
project(simpsons VERSION 0.2.4 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Keep build-machine paths (they contain the user name) out of the binary:
# source file names end up in log and assert messages.
add_compile_options("-ffile-prefix-map=${CMAKE_CURRENT_SOURCE_DIR}=simpsons")

include(generated/rexglue.cmake)

set(SIMPSONS_SOURCES
    src/main.cpp
    src/fps_unlock.cpp
    src/frame_pacing.cpp
    src/ring_wait.cpp
    src/subtitles.cpp
)

add_executable(simpsons ${SIMPSONS_SOURCES})

# simpsons_register.cpp is one flat function of ~76k function-pointer
# registrations, run once at startup, in a single basic block. LLVM is
# quadratic on it: -O1 takes over 20 minutes on an M1, and so does -O0 on
# arm64, where GlobalISel's Localizer pass walks the whole block for every
# constant. -O0 with the classic instruction selector compiles it quickly. The
# -O3 precompiled header cannot be used at -O0, so it is skipped here.
set_source_files_properties(
    ${CMAKE_CURRENT_SOURCE_DIR}/generated/default/simpsons_register.cpp
    PROPERTIES COMPILE_OPTIONS "-O0;-mllvm;-global-isel=0" SKIP_PRECOMPILE_HEADERS ON)

if(APPLE)
    # CFLocale, for the game language default (src/simpsons_app.h).
    target_link_libraries(simpsons PRIVATE "-framework CoreFoundation")
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    enable_language(OBJCXX)
    target_sources(simpsons PRIVATE src/iso_import.mm)
    set_source_files_properties(src/iso_import.mm PROPERTIES COMPILE_OPTIONS "-fobjc-arc")
    target_link_libraries(simpsons PRIVATE "-framework UIKit" "-framework UniformTypeIdentifiers")
    # The GPU plugin is embedded in Frameworks/ below and loaded from there; a
    # loose copy next to the executable would not be covered by the signature.
    rexglue_setup_target(simpsons)
else()
    rexglue_setup_target(simpsons GPU_PLUGINS xenos)
endif()

# =============================================================================
#  iOS: a .app bundle that embeds the SDK's runtime dylibs. Game data is not
#  bundled; it goes in the app's Documents/game folder (Files app).
# =============================================================================
if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(SIMPSONS_IOS_BUNDLE_ID "com.devz.simpsons" CACHE STRING "iOS bundle identifier")
    set_target_properties(simpsons PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/ios/Info.plist.in"
        MACOSX_BUNDLE_GUI_IDENTIFIER "${SIMPSONS_IOS_BUNDLE_ID}"
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${SIMPSONS_IOS_BUNDLE_ID}"
        # The SDK helper sets @executable_path, right for a flat macOS layout;
        # inside a bundle the dylibs live in Frameworks/.
        INSTALL_RPATH "@executable_path/Frameworks"
        BUILD_WITH_INSTALL_RPATH ON
    )
    # iOS only loads dylibs from inside the signed bundle.
    get_filename_component(SIMPSONS_IOS_SDK_LIB "${rexglue_DIR}/../.." ABSOLUTE)
    set(SIMPSONS_IOS_EMBED
        "${SIMPSONS_IOS_SDK_LIB}/librexruntime.dylib"
        "${SIMPSONS_IOS_SDK_LIB}/librexgpu-xenos.dylib"
        "${SIMPSONS_IOS_SDK_LIB}/libMoltenVK.dylib")
    add_custom_target(simpsons_ios_bundle ALL
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_BUNDLE_CONTENT_DIR:simpsons>/Frameworks"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SIMPSONS_IOS_EMBED}
                "$<TARGET_BUNDLE_CONTENT_DIR:simpsons>/Frameworks"
        DEPENDS simpsons ${SIMPSONS_IOS_EMBED}
        COMMENT "Refreshing runtime dylibs in simpsons.app/Frameworks"
        VERBATIM)

    # App icons. Without an asset catalog iOS resolves icons by file name from
    # the bundle root (CFBundleIcons in Info.plist), so the PNGs go in flat.
    file(GLOB SIMPSONS_IOS_ICONS "${CMAKE_CURRENT_SOURCE_DIR}/ios/icons/*.png")
    if(SIMPSONS_IOS_ICONS)
        add_custom_command(TARGET simpsons_ios_bundle POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SIMPSONS_IOS_ICONS}
                    "$<TARGET_BUNDLE_CONTENT_DIR:simpsons>"
            COMMENT "Copying app icons into simpsons.app"
            VERBATIM)
    endif()
endif()
