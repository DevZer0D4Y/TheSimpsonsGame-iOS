# rexinput - Input system library
# Input system and drivers for Xbox 360 controller emulation

add_library(rexinput OBJECT
    input_system.cpp
    device_assignment.cpp
    state_merge.cpp
    mnk/mnk_input_driver.cpp
    nop/nop_input_driver.cpp
)
add_library(rex::input ALIAS rexinput)

rex_target_pch(rexinput
    <mutex>
    <rex/math.h>
    <rex/string.h>
    <rex/memory.h>
    <rex/filesystem.h>
    <rex/thread.h>
    <rex/thread/mutex.h>
    <rex/ppc/context.h>
    <rex/system/kernel_state.h>
    <rex/system/xthread.h>
    <rex/ui/window.h>
    <rex/ui/presenter.h>
)

target_include_directories(rexinput PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
if(WIN32)
    target_sources(rexinput PRIVATE sdl/sdl_input_driver.cpp xinput/xinput_input_driver.cpp)
else()
    target_sources(rexinput PRIVATE sdl/sdl_input_driver.cpp)
endif()

target_link_libraries(rexinput PUBLIC rexcore rexui SDL3::SDL3)

# On-screen gamepad. The model, resolver and driver are portable; only iOS has
# an overlay to drive them (touch_controls.cpp is the no-op elsewhere).
target_sources(rexinput PRIVATE
    touch/touch_controls.cpp
    touch/touch_input_driver.cpp
    touch/touch_input_resolver.cpp
    touch/touch_layout.cpp
)
if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    target_sources(rexinput PRIVATE touch/touch_controls_ios.mm)
    set_source_files_properties(touch/touch_controls_ios.mm PROPERTIES
        COMPILE_OPTIONS "-fobjc-arc"
        SKIP_PRECOMPILE_HEADERS ON)
    target_link_libraries(rexinput PRIVATE
        "-framework UIKit"
        "-framework GameController"
        "-framework QuartzCore")
endif()
