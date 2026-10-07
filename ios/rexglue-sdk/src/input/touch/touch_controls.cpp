/**
 * @file        input/touch/touch_controls.cpp
 * @brief       No-op touch controls for platforms without an overlay.
 *
 * @license     BSD 3-Clause License
 *              See LICENSE file in the project root for full license text.
 */

#include <rex/input/touch/touch_controls.h>
#include <rex/platform.h>

// iOS builds touch_controls_ios.mm instead.
#if !REX_PLATFORM_IOS

namespace rex::input::touch {

void InstallTouchControls(rex::ui::Window* window, TouchControlsHooks hooks) {
  (void)window;
  (void)hooks;
}

void RemoveTouchControls() {}

}  // namespace rex::input::touch

#endif  // !REX_PLATFORM_IOS
