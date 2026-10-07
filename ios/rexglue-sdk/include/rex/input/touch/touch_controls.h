#pragma once
/**
 * @file        rex/input/touch/touch_controls.h
 * @brief       On-screen gamepad for touch screens.
 *
 * @license     BSD 3-Clause License
 *              See LICENSE file in the project root for full license text.
 *
 * @remarks     The overlay is a port of XeniOS's touch controls
 *              (github.com/xenios-jp/XeniOS, BSD license).
 */

#include <functional>

namespace rex::ui {
class Window;
}

namespace rex::input::touch {

struct TouchControlsHooks {
  // True while a system dialog (message box, keyboard, achievements) is up.
  // The full-screen look area then steps aside so taps reach the dialog; the
  // buttons stay and drive the dialog's controller navigation.
  std::function<bool()> is_system_ui_active;
};

// Puts the on-screen gamepad over `window` where the platform has one (iOS);
// elsewhere both are no-ops. The `touch_controls` cvar decides when it shows.
// UI thread only.
void InstallTouchControls(rex::ui::Window* window, TouchControlsHooks hooks = {});
void RemoveTouchControls();

}  // namespace rex::input::touch
