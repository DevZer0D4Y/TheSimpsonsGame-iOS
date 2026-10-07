#pragma once
/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2013 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 *
 * @modified    Tom Clay, 2026 - Adapted for ReXGlue runtime
 */

#include <rex/cvar.h>

// Input/HID configuration flags
REXCVAR_DECLARE(bool, guide_button);
REXCVAR_DECLARE(std::string, hid_mappings_file);
REXCVAR_DECLARE(std::string, input_backend);

// On-screen gamepad (touch screens)
REXCVAR_DECLARE(std::string, touch_controls);
REXCVAR_DECLARE(bool, touch_haptics);
REXCVAR_DECLARE(double, touch_controls_opacity);
REXCVAR_DECLARE(double, touch_look_points_per_full_scale);
REXCVAR_DECLARE(double, touch_look_vertical_scale);
REXCVAR_DECLARE(double, touch_look_hold_seconds);
REXCVAR_DECLARE(double, touch_button_tap_hold_seconds);
