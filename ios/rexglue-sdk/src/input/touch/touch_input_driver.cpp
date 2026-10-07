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

#include <rex/input/touch/touch_input_driver.h>

#include <algorithm>
#include <array>
#include <cstring>

#include <rex/input/touch/touch_layout.h>
#include <rex/ui/virtual_key.h>

namespace rex::input::touch {

namespace {

// A single device, so its handle is a constant.
constexpr DeviceId kTouchDevice = static_cast<DeviceId>(0x544F5543);  // 'TOUC'

constexpr uint8_t kTouchTriggerCapability = 0xFF;
constexpr int16_t kTouchThumbCapability = static_cast<int16_t>(0xFFFFu);
constexpr uint8_t kTouchTriggerThreshold = 0x1F;
constexpr int16_t kTouchThumbThreshold = 0x4E00;

// Bit n of the keystroke field -> the virtual key it reports. Bits 0-15 are
// the XInput button mask; the triggers and stick directions follow.
constexpr std::array<ui::VirtualKey, 34> kTouchVirtualKeys = {
    ui::VirtualKey::kXInputPadDpadUp,          ui::VirtualKey::kXInputPadDpadDown,
    ui::VirtualKey::kXInputPadDpadLeft,        ui::VirtualKey::kXInputPadDpadRight,
    ui::VirtualKey::kXInputPadStart,           ui::VirtualKey::kXInputPadBack,
    ui::VirtualKey::kXInputPadLThumbPress,     ui::VirtualKey::kXInputPadRThumbPress,
    ui::VirtualKey::kXInputPadLShoulder,       ui::VirtualKey::kXInputPadRShoulder,
    ui::VirtualKey::kNone /* guide */,         ui::VirtualKey::kNone,
    ui::VirtualKey::kXInputPadA,               ui::VirtualKey::kXInputPadB,
    ui::VirtualKey::kXInputPadX,               ui::VirtualKey::kXInputPadY,
    ui::VirtualKey::kXInputPadLTrigger,        ui::VirtualKey::kXInputPadRTrigger,
    ui::VirtualKey::kXInputPadLThumbUp,        ui::VirtualKey::kXInputPadLThumbDown,
    ui::VirtualKey::kXInputPadLThumbRight,     ui::VirtualKey::kXInputPadLThumbLeft,
    ui::VirtualKey::kXInputPadLThumbUpLeft,    ui::VirtualKey::kXInputPadLThumbUpRight,
    ui::VirtualKey::kXInputPadLThumbDownRight, ui::VirtualKey::kXInputPadLThumbDownLeft,
    ui::VirtualKey::kXInputPadRThumbUp,        ui::VirtualKey::kXInputPadRThumbDown,
    ui::VirtualKey::kXInputPadRThumbRight,     ui::VirtualKey::kXInputPadRThumbLeft,
    ui::VirtualKey::kXInputPadRThumbUpLeft,    ui::VirtualKey::kXInputPadRThumbUpRight,
    ui::VirtualKey::kXInputPadRThumbDownRight, ui::VirtualKey::kXInputPadRThumbDownLeft,
};

uint64_t TouchAnalogToKeyfield(const TouchResolvedState& state) {
  uint64_t keyfield = 0;
  keyfield |= uint64_t(state.left_trigger > kTouchTriggerThreshold) << 16;
  keyfield |= uint64_t(state.right_trigger > kTouchTriggerThreshold) << 17;

  auto append_thumb = [&keyfield](int16_t thumb_x, int16_t thumb_y, size_t bit_base) {
    uint64_t up = thumb_y > kTouchThumbThreshold;
    uint64_t down = thumb_y < -kTouchThumbThreshold;
    uint64_t right = thumb_x > kTouchThumbThreshold;
    uint64_t left = thumb_x < -kTouchThumbThreshold;
    if (up && left) {
      up = 0;
      left = 0;
      keyfield |= uint64_t(1) << (bit_base + 4);
    }
    if (up && right) {
      up = 0;
      right = 0;
      keyfield |= uint64_t(1) << (bit_base + 5);
    }
    if (down && right) {
      down = 0;
      right = 0;
      keyfield |= uint64_t(1) << (bit_base + 6);
    }
    if (down && left) {
      down = 0;
      left = 0;
      keyfield |= uint64_t(1) << (bit_base + 7);
    }
    keyfield |= up << bit_base;
    keyfield |= down << (bit_base + 1);
    keyfield |= right << (bit_base + 2);
    keyfield |= left << (bit_base + 3);
  };

  append_thumb(state.thumb_lx, state.thumb_ly, 18);
  append_thumb(state.thumb_rx, state.thumb_ry, 26);
  return keyfield;
}

uint64_t TouchKeystrokeFieldFromState(const TouchResolvedState& state) {
  return uint64_t(state.buttons) | TouchAnalogToKeyfield(state);
}

void ApplyMappedCapabilities(const TouchControlDefinition& control,
                             X_INPUT_CAPABILITIES* out_caps) {
  out_caps->gamepad.buttons = uint16_t(out_caps->gamepad.buttons) | control.mapped_buttons;
  if (control.mapped_left_trigger) {
    out_caps->gamepad.left_trigger = kTouchTriggerCapability;
  }
  if (control.mapped_right_trigger) {
    out_caps->gamepad.right_trigger = kTouchTriggerCapability;
  }
}

void ApplyAnalogOutputCapabilities(TouchAnalogOutput output, X_INPUT_CAPABILITIES* out_caps) {
  switch (output) {
    case TouchAnalogOutput::kLook:
      out_caps->gamepad.thumb_rx = kTouchThumbCapability;
      out_caps->gamepad.thumb_ry = kTouchThumbCapability;
      break;
    case TouchAnalogOutput::kMove:
      out_caps->gamepad.thumb_lx = kTouchThumbCapability;
      out_caps->gamepad.thumb_ly = kTouchThumbCapability;
      break;
    case TouchAnalogOutput::kNone:
    default:
      break;
  }
}

TouchAnalogOutput EffectiveAnalogOutput(TouchAnalogOutput output, bool relative_look) {
  if (output != TouchAnalogOutput::kNone) {
    return output;
  }
  return relative_look ? TouchAnalogOutput::kLook : TouchAnalogOutput::kNone;
}

void ApplyControlCapabilities(const TouchControlDefinition& control,
                              X_INPUT_CAPABILITIES* out_caps) {
  switch (control.type) {
    case TouchControlType::kMoveStick:
      ApplyAnalogOutputCapabilities(control.action == TouchAction::kLook ? TouchAnalogOutput::kLook
                                                                         : TouchAnalogOutput::kMove,
                                    out_caps);
      if (control.move_with_dpad_ring) {
        out_caps->gamepad.buttons =
            uint16_t(out_caps->gamepad.buttons) | X_INPUT_GAMEPAD_DPAD_UP |
            X_INPUT_GAMEPAD_DPAD_DOWN | X_INPUT_GAMEPAD_DPAD_LEFT | X_INPUT_GAMEPAD_DPAD_RIGHT;
      }
      break;
    case TouchControlType::kLookSwipeZone:
      ApplyAnalogOutputCapabilities(control.action == TouchAction::kMove ? TouchAnalogOutput::kMove
                                                                         : TouchAnalogOutput::kLook,
                                    out_caps);
      break;
    case TouchControlType::kActionButton:
      ApplyMappedCapabilities(control, out_caps);
      ApplyAnalogOutputCapabilities(
          EffectiveAnalogOutput(control.drag_output, control.enables_relative_look), out_caps);
      break;
  }

  if (control.secondary_behavior.trigger != TouchInteractionTrigger::kNone) {
    TouchControlDefinition secondary;
    if (ConfigureTouchControlAction(control.secondary_behavior.action, &secondary)) {
      ApplyMappedCapabilities(secondary, out_caps);
    }
    ApplyAnalogOutputCapabilities(
        EffectiveAnalogOutput(control.secondary_behavior.analog_output,
                              control.secondary_behavior.enables_relative_look),
        out_caps);
  }
}

}  // namespace

TouchInputDriver::TouchInputDriver(rex::ui::Window* window, size_t window_z_order)
    : InputDriver(window, window_z_order) {}

TouchInputDriver::~TouchInputDriver() = default;

X_STATUS TouchInputDriver::Setup() {
  return X_STATUS_SUCCESS;
}

void TouchInputDriver::EnumerateDevices(std::vector<DeviceInfo>& out) {
  // No overlay, no pad: otherwise it would claim user 0 on a device that
  // cannot show the controls.
  if (!GetTouchRuntimeModel().attached()) {
    return;
  }
  DeviceInfo info;
  info.id = kTouchDevice;
  info.name = "Touch Controls";
  info.synthetic = true;
  out.push_back(info);
}

X_RESULT TouchInputDriver::GetDeviceCapabilities(DeviceId id, uint32_t flags,
                                                 X_INPUT_CAPABILITIES* out_caps) {
  (void)flags;
  const TouchRuntimeModel& model = GetTouchRuntimeModel();
  if (id != kTouchDevice || !model.attached()) {
    return X_ERROR_DEVICE_NOT_CONNECTED;
  }
  if (!out_caps) {
    return X_ERROR_BAD_ARGUMENTS;
  }
  // Reported whether or not the overlay is currently shown: titles query
  // capabilities up front and size their UI from them.
  std::memset(out_caps, 0, sizeof(*out_caps));
  out_caps->type = XINPUT_DEVTYPE_GAMEPAD;
  out_caps->sub_type = 0x01;  // XINPUT_DEVSUBTYPE_GAMEPAD
  out_caps->flags = 0;
  for (const auto& control : model.layout().controls) {
    ApplyControlCapabilities(control, out_caps);
  }
  return X_ERROR_SUCCESS;
}

X_RESULT TouchInputDriver::GetDeviceState(DeviceId id, X_INPUT_STATE* out_state) {
  const TouchRuntimeModel& model = GetTouchRuntimeModel();
  if (id != kTouchDevice || !model.attached()) {
    return X_ERROR_DEVICE_NOT_CONNECTED;
  }
  const TouchResolvedState state = model.LoadResolvedState();
  if (!state.gameplay_enabled) {
    return X_ERROR_DEVICE_NOT_CONNECTED;
  }
  if (out_state) {
    std::memset(out_state, 0, sizeof(*out_state));
    out_state->packet_number = state.packet_number;
    out_state->gamepad.buttons = state.buttons;
    out_state->gamepad.left_trigger = state.left_trigger;
    out_state->gamepad.right_trigger = state.right_trigger;
    out_state->gamepad.thumb_lx = state.thumb_lx;
    out_state->gamepad.thumb_ly = state.thumb_ly;
    out_state->gamepad.thumb_rx = state.thumb_rx;
    out_state->gamepad.thumb_ry = state.thumb_ry;
  }
  return X_ERROR_SUCCESS;
}

X_RESULT TouchInputDriver::SetDeviceVibration(DeviceId id, X_INPUT_VIBRATION* vibration) {
  (void)vibration;
  const TouchRuntimeModel& model = GetTouchRuntimeModel();
  if (id != kTouchDevice || !model.attached() || !model.LoadResolvedState().gameplay_enabled) {
    return X_ERROR_DEVICE_NOT_CONNECTED;
  }
  return X_ERROR_SUCCESS;
}

X_RESULT TouchInputDriver::GetDeviceKeystroke(DeviceId id, uint32_t flags,
                                              X_INPUT_KEYSTROKE* out_keystroke) {
  (void)flags;
  const TouchRuntimeModel& model = GetTouchRuntimeModel();
  if (id != kTouchDevice || !model.attached()) {
    return X_ERROR_DEVICE_NOT_CONNECTED;
  }
  if (!out_keystroke) {
    return X_ERROR_BAD_ARGUMENTS;
  }

  const TouchResolvedState state = model.LoadResolvedState();
  std::lock_guard<std::mutex> lock(keystroke_mutex_);
  if (!state.gameplay_enabled) {
    pending_keystrokes_.clear();
    has_keystroke_state_ = false;
    last_keystroke_field_ = 0;
    return X_ERROR_DEVICE_NOT_CONNECTED;
  }

  // Keystrokes are the edges between successive states.
  const uint64_t current_field = TouchKeystrokeFieldFromState(state);
  if (!has_keystroke_state_) {
    has_keystroke_state_ = true;
    last_keystroke_field_ = current_field;
  } else if (current_field != last_keystroke_field_) {
    const uint64_t changed_bits = current_field ^ last_keystroke_field_;
    for (size_t bit_index = 0; bit_index < kTouchVirtualKeys.size(); ++bit_index) {
      const uint64_t bit_mask = uint64_t(1) << bit_index;
      if (!(changed_bits & bit_mask) || kTouchVirtualKeys[bit_index] == ui::VirtualKey::kNone) {
        continue;
      }
      X_INPUT_KEYSTROKE keystroke = {};
      keystroke.virtual_key = uint16_t(kTouchVirtualKeys[bit_index]);
      keystroke.flags =
          (current_field & bit_mask) ? X_INPUT_KEYSTROKE_KEYDOWN : X_INPUT_KEYSTROKE_KEYUP;
      pending_keystrokes_.push_back(keystroke);
    }
    last_keystroke_field_ = current_field;
  }

  if (pending_keystrokes_.empty()) {
    return X_ERROR_EMPTY;
  }
  std::memset(out_keystroke, 0, sizeof(*out_keystroke));
  *out_keystroke = pending_keystrokes_.front();
  pending_keystrokes_.pop_front();
  return X_ERROR_SUCCESS;
}

}  // namespace rex::input::touch
