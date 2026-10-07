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

#include <rex/input/touch/touch_layout.h>

#include <utility>

#include <rex/cvar.h>
#include <rex/input/input.h>

REXCVAR_DEFINE_STRING(touch_controls, "auto", "Input/Touch",
                      "On-screen gamepad on touch screens: auto = shown while no controller is "
                      "connected, on = always shown, off = never shown.")
    .allowed({"auto", "on", "off"});

REXCVAR_DEFINE_BOOL(touch_haptics, true, "Input/Touch",
                    "Play a haptic tap when an on-screen button or the D-pad is pressed.");

REXCVAR_DEFINE_DOUBLE(touch_controls_opacity, 1.0, "Input/Touch",
                      "Opacity multiplier for the on-screen gamepad (0.1 to 1.0).");

REXCVAR_DEFINE_DOUBLE(touch_look_points_per_full_scale, 4.0, "Input/Touch",
                      "Swipe distance, in points per touch update, that drives the look stick "
                      "to full deflection. Lower is more sensitive.");

REXCVAR_DEFINE_DOUBLE(touch_look_vertical_scale, 1.20, "Input/Touch",
                      "Extra multiplier for vertical swipe-look, since titles usually turn "
                      "slower on pitch than on yaw.");

REXCVAR_DEFINE_DOUBLE(touch_look_hold_seconds, 0.10, "Input/Touch",
                      "How long a swipe keeps the look stick deflected, so a title polling "
                      "input at a low frame rate still sees it.");

REXCVAR_DEFINE_DOUBLE(touch_button_tap_hold_seconds, 0.10, "Input/Touch",
                      "Shortest time a tapped on-screen button stays pressed, so a title polling "
                      "input at a low frame rate still sees quick taps.");

namespace rex::input::touch {

namespace {

constexpr float kHoldSeconds = 0.30f;
constexpr float kHoldDragSeconds = 0.16f;
constexpr float kDoubleTapSeconds = 0.24f;

// XeniOS "FPS Compact" positions, normalized to the overlay.
constexpr TouchRect kMoveStickFrame = {0.055f, 0.56f, 0.190f, 0.315f};
constexpr TouchRect kLookZoneFrame = {0.0f, 0.0f, 1.0f, 1.0f};
constexpr TouchRect kBackFrame = {0.390f, 0.045f, 0.080f, 0.112f};
constexpr TouchRect kStartFrame = {0.495f, 0.045f, 0.085f, 0.112f};
constexpr TouchRect kLeftBumperFrame = {0.660f, 0.050f, 0.085f, 0.112f};
constexpr TouchRect kRightBumperFrame = {0.765f, 0.050f, 0.085f, 0.112f};
constexpr TouchRect kLeftTriggerFrame = {0.095f, 0.405f, 0.120f, 0.110f};
constexpr TouchRect kButtonYFrame = {0.760f, 0.455f, 0.065f, 0.115f};
constexpr TouchRect kButtonXFrame = {0.700f, 0.585f, 0.065f, 0.115f};
constexpr TouchRect kButtonBFrame = {0.820f, 0.585f, 0.065f, 0.115f};
constexpr TouchRect kButtonAFrame = {0.760f, 0.715f, 0.065f, 0.115f};
constexpr TouchRect kRightTriggerFrame = {0.860f, 0.405f, 0.120f, 0.110f};

TouchControlDefinition MakeActionButton(const char* identifier, TouchAction action,
                                        const TouchRect& normalized_frame,
                                        uint8_t capture_priority) {
  TouchControlDefinition control;
  control.identifier = identifier;
  control.type = TouchControlType::kActionButton;
  control.shape = TouchControlShape::kCircle;
  control.normalized_frame = normalized_frame;
  control.activation_radius = 0.5f;
  control.analog_tuning.activation_radius = control.activation_radius;
  control.visual_opacity = 0.92f;
  control.capture_priority = capture_priority;
  ConfigureTouchControlAction(action, &control);
  return control;
}

TouchControlDefinition MakeMoveStick() {
  TouchControlDefinition control;
  control.identifier = "move_stick";
  control.type = TouchControlType::kMoveStick;
  control.action = TouchAction::kMove;
  control.shape = TouchControlShape::kCircle;
  control.normalized_frame = kMoveStickFrame;
  control.deadzone = 0.14f;
  control.activation_radius = 0.48f;
  control.analog_tuning.deadzone = control.deadzone;
  control.analog_tuning.activation_radius = control.activation_radius;
  control.visual_opacity = 0.80f;
  control.move_with_dpad_ring = true;
  control.secondary_behavior.trigger = TouchInteractionTrigger::kDoubleTapForward;
  control.secondary_behavior.action = TouchAction::kLeftThumb;
  control.secondary_behavior.hold_seconds =
      DefaultTouchHoldSecondsForTrigger(control.secondary_behavior.trigger);
  control.capture_priority = 220;
  return control;
}

TouchControlDefinition MakeLookZone() {
  TouchControlDefinition control;
  control.identifier = "look_zone";
  control.type = TouchControlType::kLookSwipeZone;
  control.action = TouchAction::kLook;
  control.shape = TouchControlShape::kRoundedRect;
  control.normalized_frame = kLookZoneFrame;
  control.drag_output = TouchAnalogOutput::kLook;
  control.visual_opacity = 0.0f;
  control.label_hidden = true;
  // Lowest priority: it is under every other control.
  control.capture_priority = 8;
  return control;
}

}  // namespace

void TouchResolvedStateBuffer::Store(const TouchResolvedState& state) {
  std::lock_guard<std::mutex> lock(mutex_);
  state_ = state;
}

TouchResolvedState TouchResolvedStateBuffer::Load() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return state_;
}

TouchRuntimeModel::TouchRuntimeModel() : layout_(CreateDefaultTouchLayout()) {}

void TouchRuntimeModel::SetLayout(TouchLayoutModel layout) {
  layout_ = std::move(layout);
}

TouchRuntimeModel& GetTouchRuntimeModel() {
  static TouchRuntimeModel model;
  return model;
}

TouchLayoutModel CreateDefaultTouchLayout() {
  TouchLayoutModel layout;
  layout.layout_id = "fps_compact";
  layout.display_name = "FPS Compact";

  layout.controls.push_back(MakeMoveStick());
  layout.controls.push_back(MakeLookZone());
  layout.controls.push_back(MakeActionButton("back_button", TouchAction::kBack, kBackFrame, 250));
  layout.controls.push_back(
      MakeActionButton("start_button", TouchAction::kStart, kStartFrame, 250));
  layout.controls.push_back(MakeActionButton("left_bumper_button", TouchAction::kLeftBumper,
                                             kLeftBumperFrame, 242));
  layout.controls.push_back(MakeActionButton("right_bumper_button", TouchAction::kRightBumper,
                                             kRightBumperFrame, 243));
  layout.controls.push_back(MakeActionButton("left_trigger_button", TouchAction::kLeftTrigger,
                                             kLeftTriggerFrame, 240));
  layout.controls.push_back(
      MakeActionButton("y_button", TouchAction::kButtonY, kButtonYFrame, 236));
  layout.controls.push_back(
      MakeActionButton("x_button", TouchAction::kButtonX, kButtonXFrame, 236));
  TouchControlDefinition b_button =
      MakeActionButton("b_button", TouchAction::kButtonB, kButtonBFrame, 237);
  // B is used for sustained gameplay actions. Keep its original press until
  // the finger lifts, even if it drifts outside the button. A long press must
  // not defer B or replace it with the FPS layout's right-stick click.
  b_button.hold_while_captured = true;
  layout.controls.push_back(std::move(b_button));
  layout.controls.push_back(
      MakeActionButton("a_button", TouchAction::kButtonA, kButtonAFrame, 238));
  layout.controls.push_back(MakeActionButton("right_trigger_button", TouchAction::kRightTrigger,
                                             kRightTriggerFrame, 242));
  return layout;
}

const char* TouchActionDisplayName(TouchAction action) {
  switch (action) {
    case TouchAction::kNone:
      return "Unused";
    case TouchAction::kMove:
      return "Move";
    case TouchAction::kLook:
      return "Look";
    case TouchAction::kButtonA:
      return "A";
    case TouchAction::kButtonB:
      return "B";
    case TouchAction::kButtonX:
      return "X";
    case TouchAction::kButtonY:
      return "Y";
    case TouchAction::kLeftBumper:
      return "LB";
    case TouchAction::kRightBumper:
      return "RB";
    case TouchAction::kLeftTrigger:
      return "LT";
    case TouchAction::kRightTrigger:
      return "RT";
    case TouchAction::kBack:
      return "Back";
    case TouchAction::kStart:
      return "Start";
    case TouchAction::kLeftThumb:
      return "LS";
    case TouchAction::kRightThumb:
      return "RS";
    case TouchAction::kDpadUp:
      return "D-Pad Up";
    case TouchAction::kDpadDown:
      return "D-Pad Down";
    case TouchAction::kDpadLeft:
      return "D-Pad Left";
    case TouchAction::kDpadRight:
      return "D-Pad Right";
  }
  return "Control";
}

std::string TouchControlVisibleLabel(const TouchControlDefinition& control) {
  if (control.label_hidden) {
    return {};
  }
  if (!control.label.empty()) {
    return control.label;
  }
  return TouchActionDisplayName(control.action);
}

bool ConfigureTouchControlAction(TouchAction action, TouchControlDefinition* control) {
  if (!control) {
    return false;
  }

  TouchControlDefinition updated = *control;
  updated.action = action;
  updated.mapped_buttons = 0;
  updated.mapped_left_trigger = 0;
  updated.mapped_right_trigger = 0;
  updated.hold_while_captured = false;
  updated.enables_relative_look = false;
  updated.drag_output = TouchAnalogOutput::kNone;
  updated.analog_tuning.horizontal_scale = 1.0f;
  updated.analog_tuning.vertical_scale = 1.0f;
  updated.relative_look_scale = 1.0f;
  updated.held_look_scale = 1.0f;
  updated.held_move_scale = 1.0f;

  switch (action) {
    case TouchAction::kMove:
    case TouchAction::kLook:
    case TouchAction::kNone:
      break;
    case TouchAction::kButtonA:
      updated.mapped_buttons = X_INPUT_GAMEPAD_A;
      break;
    case TouchAction::kButtonB:
      updated.mapped_buttons = X_INPUT_GAMEPAD_B;
      break;
    case TouchAction::kButtonX:
      updated.mapped_buttons = X_INPUT_GAMEPAD_X;
      break;
    case TouchAction::kButtonY:
      updated.mapped_buttons = X_INPUT_GAMEPAD_Y;
      break;
    case TouchAction::kLeftBumper:
      updated.mapped_buttons = X_INPUT_GAMEPAD_LEFT_SHOULDER;
      break;
    case TouchAction::kRightBumper:
      updated.mapped_buttons = X_INPUT_GAMEPAD_RIGHT_SHOULDER;
      break;
    // Triggers stay pulled while the finger is down and turn its drag into
    // look, so one thumb can hold fire and aim at the same time.
    case TouchAction::kLeftTrigger:
      updated.mapped_left_trigger = 255;
      updated.hold_while_captured = true;
      updated.enables_relative_look = true;
      updated.drag_output = TouchAnalogOutput::kLook;
      updated.relative_look_scale = 0.80f;
      updated.analog_tuning.horizontal_scale = updated.relative_look_scale;
      updated.analog_tuning.vertical_scale = updated.relative_look_scale;
      break;
    case TouchAction::kRightTrigger:
      updated.mapped_right_trigger = 255;
      updated.hold_while_captured = true;
      updated.enables_relative_look = true;
      updated.drag_output = TouchAnalogOutput::kLook;
      updated.relative_look_scale = 0.92f;
      updated.analog_tuning.horizontal_scale = updated.relative_look_scale;
      updated.analog_tuning.vertical_scale = updated.relative_look_scale;
      break;
    case TouchAction::kBack:
      updated.mapped_buttons = X_INPUT_GAMEPAD_BACK;
      break;
    case TouchAction::kStart:
      updated.mapped_buttons = X_INPUT_GAMEPAD_START;
      break;
    case TouchAction::kLeftThumb:
      updated.mapped_buttons = X_INPUT_GAMEPAD_LEFT_THUMB;
      break;
    case TouchAction::kRightThumb:
      updated.mapped_buttons = X_INPUT_GAMEPAD_RIGHT_THUMB;
      break;
    case TouchAction::kDpadUp:
      updated.mapped_buttons = X_INPUT_GAMEPAD_DPAD_UP;
      break;
    case TouchAction::kDpadDown:
      updated.mapped_buttons = X_INPUT_GAMEPAD_DPAD_DOWN;
      break;
    case TouchAction::kDpadLeft:
      updated.mapped_buttons = X_INPUT_GAMEPAD_DPAD_LEFT;
      break;
    case TouchAction::kDpadRight:
      updated.mapped_buttons = X_INPUT_GAMEPAD_DPAD_RIGHT;
      break;
    default:
      return false;
  }

  *control = std::move(updated);
  return true;
}

float DefaultTouchHoldSecondsForTrigger(TouchInteractionTrigger trigger) {
  switch (trigger) {
    case TouchInteractionTrigger::kHoldDrag:
      return kHoldDragSeconds;
    case TouchInteractionTrigger::kDoubleTap:
    case TouchInteractionTrigger::kDoubleTapForward:
      return kDoubleTapSeconds;
    case TouchInteractionTrigger::kHold:
    case TouchInteractionTrigger::kNone:
    default:
      return kHoldSeconds;
  }
}

TouchRect ResolveTouchRect(const TouchRect& normalized_rect, const TouchLayoutSpace& space) {
  if (space.IsEmpty()) {
    return TouchRect{};
  }
  return TouchRect{
      space.origin_x + normalized_rect.x * space.width,
      space.origin_y + normalized_rect.y * space.height,
      normalized_rect.width * space.width,
      normalized_rect.height * space.height,
  };
}

bool TouchRectContainsPoint(const TouchRect& rect, const TouchPoint& point) {
  return point.x >= rect.x && point.x <= rect.x + rect.width && point.y >= rect.y &&
         point.y <= rect.y + rect.height;
}

}  // namespace rex::input::touch
