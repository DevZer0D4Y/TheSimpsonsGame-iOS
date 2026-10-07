#pragma once
/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 *
 * @modified    2026 - Ported from XeniOS (github.com/xenios-jp/XeniOS) to the
 *              ReXGlue runtime.
 */

#include <cstdint>
#include <deque>
#include <mutex>
#include <vector>

#include <rex/input/input_driver.h>

namespace rex::input::touch {

// Presents the on-screen gamepad to the guest as a synthetic pad on user 0,
// merged with any hardware pad there. It reports a device only while a
// platform overlay is attached, and state only while that overlay is on
// screen, so a hidden overlay never holds the guest's input neutral.
class TouchInputDriver final : public InputDriver {
 public:
  explicit TouchInputDriver(rex::ui::Window* window, size_t window_z_order);
  ~TouchInputDriver() override;

  X_STATUS Setup() override;

  void EnumerateDevices(std::vector<DeviceInfo>& out) override;
  X_RESULT GetDeviceState(DeviceId id, X_INPUT_STATE* out_state) override;
  X_RESULT GetDeviceCapabilities(DeviceId id, uint32_t flags,
                                 X_INPUT_CAPABILITIES* out_caps) override;
  X_RESULT SetDeviceVibration(DeviceId id, X_INPUT_VIBRATION* vibration) override;
  X_RESULT GetDeviceKeystroke(DeviceId id, uint32_t flags,
                              X_INPUT_KEYSTROKE* out_keystroke) override;

 private:
  std::mutex keystroke_mutex_;
  bool has_keystroke_state_ = false;
  uint64_t last_keystroke_field_ = 0;
  std::deque<X_INPUT_KEYSTROKE> pending_keystrokes_;
};

}  // namespace rex::input::touch
