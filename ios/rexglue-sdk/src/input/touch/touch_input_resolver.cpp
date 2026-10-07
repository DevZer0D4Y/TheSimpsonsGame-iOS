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

#include <rex/input/touch/touch_input_resolver.h>

#include <algorithm>
#include <cmath>

namespace rex::input::touch {

namespace {

constexpr float kTouchAxisMax = 32767.0f;
constexpr float kTouchComboStickRadiusFraction = 0.32f;
constexpr float kMoveStickDoubleTapMaxSeconds = 0.34f;
constexpr float kMoveStickDoubleTapForwardThreshold = 0.58f;
constexpr float kMoveStickDoubleTapLateralThreshold = 0.58f;

TouchPoint ApplyTouchAnalogTuningImpl(TouchPoint value, const TouchAnalogTuning& tuning,
                                      bool apply_deadzone,
                                      float velocity_full_scales_per_second) {
  if (tuning.invert_x) {
    value.x = -value.x;
  }
  if (tuning.invert_y) {
    value.y = -value.y;
  }

  const float magnitude = std::hypot(value.x, value.y);
  if (apply_deadzone) {
    const float deadzone = std::clamp(tuning.deadzone, 0.0f, 0.95f);
    if (magnitude <= deadzone || magnitude <= 0.0f) {
      return TouchPoint{};
    }
    const float rescaled_magnitude =
        std::clamp((magnitude - deadzone) / std::max(1.0f - deadzone, 0.001f), 0.0f, 1.0f);
    value.x = value.x / magnitude * rescaled_magnitude;
    value.y = value.y / magnitude * rescaled_magnitude;
  }

  value.x *= std::clamp(tuning.horizontal_scale, 0.1f, 4.0f);
  value.y *= std::clamp(tuning.vertical_scale, 0.1f, 4.0f);

  const float abs_x = std::abs(value.x);
  const float abs_y = std::abs(value.y);
  const float dominant_axis = std::max(abs_x, abs_y);
  if (dominant_axis > 0.0f) {
    const float diagonal_mix = std::min(abs_x, abs_y) / dominant_axis;
    const float diagonal_target = std::clamp(tuning.diagonal_scale, 0.1f, 4.0f);
    const float diagonal_scale = 1.0f + (diagonal_target - 1.0f) * diagonal_mix;
    value.x *= diagonal_scale;
    value.y *= diagonal_scale;
  }

  const float tuned_magnitude = std::hypot(value.x, value.y);
  if (tuned_magnitude > 0.0f) {
    const float curve = std::clamp(tuning.response_curve, 0.25f, 4.0f);
    const float curved_magnitude = std::pow(std::min(tuned_magnitude, 1.0f), curve);
    const float curve_scale = curved_magnitude / tuned_magnitude;
    value.x *= curve_scale;
    value.y *= curve_scale;
  }

  const float acceleration = std::clamp(tuning.acceleration_scale, 0.0f, 2.0f);
  if (acceleration > 0.0f && velocity_full_scales_per_second > 0.0f) {
    const float velocity_mix = std::clamp(velocity_full_scales_per_second / 80.0f, 0.0f, 1.0f);
    const float acceleration_boost = 1.0f + acceleration * velocity_mix;
    value.x *= acceleration_boost;
    value.y *= acceleration_boost;
  }

  const float max_output = std::clamp(tuning.max_output, 0.1f, 1.0f);
  const float output_magnitude = std::hypot(value.x, value.y);
  if (output_magnitude > max_output && output_magnitude > 0.0f) {
    const float scale = max_output / output_magnitude;
    value.x *= scale;
    value.y *= scale;
  }

  return ClampTouchLookVector(value);
}

}  // namespace

TouchPoint ClampTouchLookVector(TouchPoint value) {
  return TouchPoint{std::clamp(value.x, -1.0f, 1.0f), std::clamp(value.y, -1.0f, 1.0f)};
}

TouchPoint ApplyTouchAnalogTuning(TouchPoint value, const TouchAnalogTuning& tuning) {
  return ApplyTouchAnalogTuningImpl(value, tuning, true, 0.0f);
}

TouchPoint ApplyTouchAnalogTuningWithVelocity(TouchPoint value, const TouchAnalogTuning& tuning,
                                              float velocity_full_scales_per_second) {
  return ApplyTouchAnalogTuningImpl(value, tuning, true,
                                    std::max(0.0f, velocity_full_scales_per_second));
}

TouchPoint TouchSwipeLookVectorForDelta(TouchPoint delta, float look_scale,
                                        float points_per_full_scale, float vertical_scale) {
  const float safe_points_per_full_scale = std::clamp(points_per_full_scale, 1.0f, 64.0f);
  const float clamped_look_scale = std::clamp(look_scale, 0.25f, 4.0f);
  const float clamped_vertical_scale = std::clamp(vertical_scale, 0.25f, 4.0f);
  return ClampTouchLookVector(
      TouchPoint{delta.x / safe_points_per_full_scale * clamped_look_scale,
                 -delta.y / safe_points_per_full_scale * clamped_look_scale *
                     clamped_vertical_scale});
}

TouchComboSubzone ResolveTouchComboSubzone(const TouchControlDefinition& control,
                                           const TouchRect& resolved_frame, TouchPoint point) {
  if (!control.move_with_dpad_ring || control.type != TouchControlType::kMoveStick) {
    return TouchComboSubzone::kStick;
  }
  if (resolved_frame.width <= 0.0f || resolved_frame.height <= 0.0f) {
    return TouchComboSubzone::kStick;
  }
  const float centre_x = resolved_frame.x + resolved_frame.width * 0.5f;
  const float centre_y = resolved_frame.y + resolved_frame.height * 0.5f;
  const float short_side = std::min(resolved_frame.width, resolved_frame.height);
  const float stick_radius = short_side * kTouchComboStickRadiusFraction;
  const float dx = point.x - centre_x;
  const float dy = point.y - centre_y;
  if (dx * dx + dy * dy <= stick_radius * stick_radius) {
    return TouchComboSubzone::kStick;
  }
  if (std::abs(dx) > std::abs(dy)) {
    return dx > 0.0f ? TouchComboSubzone::kDpadRight : TouchComboSubzone::kDpadLeft;
  }
  return dy > 0.0f ? TouchComboSubzone::kDpadDown : TouchComboSubzone::kDpadUp;
}

bool TouchControlContainsPoint(const TouchControlDefinition& control,
                               const TouchRect& resolved_frame, TouchPoint point) {
  if (!TouchRectContainsPoint(resolved_frame, point)) {
    return false;
  }
  if (control.move_with_dpad_ring && control.type == TouchControlType::kMoveStick) {
    return true;
  }
  if (control.shape != TouchControlShape::kCircle) {
    return true;
  }

  // Circles wider or taller than they are round are capsules: a rectangle with
  // a half-circle at each end.
  const float width = resolved_frame.width;
  const float height = resolved_frame.height;
  if (width <= 0.0f || height <= 0.0f) {
    return false;
  }

  const float radius = std::min(width, height) * 0.5f;
  if (width >= height) {
    if (point.x >= resolved_frame.x + radius && point.x <= resolved_frame.x + width - radius) {
      return true;
    }
    const float center_y = resolved_frame.y + height * 0.5f;
    const float left_dx = point.x - (resolved_frame.x + radius);
    const float right_dx = point.x - (resolved_frame.x + width - radius);
    const float dy = point.y - center_y;
    return left_dx * left_dx + dy * dy <= radius * radius ||
           right_dx * right_dx + dy * dy <= radius * radius;
  }

  if (point.y >= resolved_frame.y + radius && point.y <= resolved_frame.y + height - radius) {
    return true;
  }
  const float center_x = resolved_frame.x + width * 0.5f;
  const float dx = point.x - center_x;
  const float top_dy = point.y - (resolved_frame.y + radius);
  const float bottom_dy = point.y - (resolved_frame.y + height - radius);
  return dx * dx + top_dy * top_dy <= radius * radius ||
         dx * dx + bottom_dy * bottom_dy <= radius * radius;
}

int16_t TouchAxisFromUnit(float unit_value) {
  const float clamped_value = std::clamp(unit_value, -1.0f, 1.0f);
  return static_cast<int16_t>(std::lround(clamped_value * kTouchAxisMax));
}

bool TouchStatesEqualIgnoringPacket(const TouchResolvedState& left,
                                    const TouchResolvedState& right) {
  return left.buttons == right.buttons && left.left_trigger == right.left_trigger &&
         left.right_trigger == right.right_trigger && left.thumb_lx == right.thumb_lx &&
         left.thumb_ly == right.thumb_ly && left.thumb_rx == right.thumb_rx &&
         left.thumb_ry == right.thumb_ry && left.gameplay_enabled == right.gameplay_enabled;
}

void ApplyTouchActionMapping(const TouchControlDefinition& control, TouchResolvedState* state) {
  if (!state) {
    return;
  }
  state->buttons |= control.mapped_buttons;
  state->left_trigger = std::max(state->left_trigger, control.mapped_left_trigger);
  state->right_trigger = std::max(state->right_trigger, control.mapped_right_trigger);
}

void ApplyTouchActionMappingForAction(TouchAction action, TouchResolvedState* state) {
  TouchControlDefinition control;
  ConfigureTouchControlAction(action, &control);
  ApplyTouchActionMapping(control, state);
}

bool TouchInteractionBehaviorConfigured(const TouchInteractionBehavior& behavior) {
  return behavior.trigger != TouchInteractionTrigger::kNone &&
         (behavior.action != TouchAction::kNone ||
          behavior.analog_output != TouchAnalogOutput::kNone || behavior.enables_relative_look);
}

TouchInteractionBehaviorState ResolveTouchInteractionBehaviorState(
    const TouchInteractionBehavior& behavior, const TouchInputCapture& capture,
    double current_time) {
  TouchInteractionBehaviorState state;
  if (!TouchInteractionBehaviorConfigured(behavior) || capture.began_time <= 0.0) {
    return state;
  }

  const float elapsed_seconds = static_cast<float>(current_time - capture.began_time);
  switch (behavior.trigger) {
    case TouchInteractionTrigger::kHold: {
      const float hold_seconds = std::clamp(behavior.hold_seconds, 0.05f, 1.0f);
      if (elapsed_seconds < hold_seconds) {
        return state;
      }
    } break;
    case TouchInteractionTrigger::kHoldDrag: {
      const float hold_seconds = std::clamp(behavior.hold_seconds, 0.05f, 1.0f);
      if (elapsed_seconds < hold_seconds) {
        return state;
      }
      const float drag_distance = std::hypot(capture.current_point.x - capture.anchor_point.x,
                                             capture.current_point.y - capture.anchor_point.y);
      if (drag_distance < std::clamp(behavior.drag_threshold_points, 2.0f, 96.0f)) {
        return state;
      }
    } break;
    case TouchInteractionTrigger::kDoubleTap:
    case TouchInteractionTrigger::kDoubleTapForward:
    case TouchInteractionTrigger::kNone:
    default:
      return state;
  }

  state.active = true;
  state.analog_output = behavior.analog_output;
  state.analog_tuning = behavior.analog_tuning;
  state.enables_relative_look = behavior.enables_relative_look;
  state.relative_look_scale = std::clamp(behavior.relative_look_scale, 0.1f, 2.0f);
  if (state.analog_output == TouchAnalogOutput::kNone && behavior.enables_relative_look) {
    state.analog_output = TouchAnalogOutput::kLook;
    state.analog_tuning.horizontal_scale = state.relative_look_scale;
    state.analog_tuning.vertical_scale = state.relative_look_scale;
  }
  if (state.analog_output == TouchAnalogOutput::kLook) {
    state.enables_relative_look = true;
    state.analog_tuning.horizontal_scale =
        std::clamp(state.analog_tuning.horizontal_scale, 0.1f, 4.0f);
    state.analog_tuning.vertical_scale = std::clamp(state.analog_tuning.vertical_scale, 0.1f, 4.0f);
  }
  return state;
}

bool TouchControlUsesDeferredPrimaryTap(const TouchControlDefinition& control) {
  return control.type == TouchControlType::kActionButton && !control.hold_while_captured &&
         (control.secondary_behavior.trigger == TouchInteractionTrigger::kHold ||
          control.secondary_behavior.trigger == TouchInteractionTrigger::kHoldDrag) &&
         TouchInteractionBehaviorConfigured(control.secondary_behavior);
}

TouchPoint MoveStickUnitVectorForCapture(const TouchControlDefinition& control,
                                         const TouchRect& frame, const TouchInputCapture& capture) {
  const float outer_radius =
      std::min(frame.width, frame.height) * std::max(control.activation_radius, 0.24f);
  TouchPoint delta{capture.current_point.x - capture.anchor_point.x,
                   capture.current_point.y - capture.anchor_point.y};
  const float distance = std::hypot(delta.x, delta.y);
  if (distance > outer_radius && distance > 0.0f) {
    const float scale = outer_radius / distance;
    delta.x *= scale;
    delta.y *= scale;
  }

  float normalized_x = outer_radius > 0.0f ? delta.x / outer_radius : 0.0f;
  float normalized_y = outer_radius > 0.0f ? delta.y / outer_radius : 0.0f;
  const float magnitude = std::sqrt(normalized_x * normalized_x + normalized_y * normalized_y);
  if (magnitude < control.deadzone || magnitude <= 0.0f) {
    return TouchPoint{};
  }
  const float rescaled_magnitude = std::clamp(
      (magnitude - control.deadzone) / std::max(1.0f - control.deadzone, 0.001f), 0.0f, 1.0f);
  normalized_x = normalized_x / magnitude * rescaled_magnitude;
  normalized_y = normalized_y / magnitude * rescaled_magnitude;
  return ApplyTouchAnalogTuningImpl(TouchPoint{normalized_x, normalized_y}, control.analog_tuning,
                                    false, 0.0f);
}

bool MoveStickCaptureQualifiesForDoubleTapForward(const TouchControlDefinition& control,
                                                  const TouchRect& frame,
                                                  const TouchInputCapture& capture,
                                                  double current_time) {
  if (capture.began_time <= 0.0 ||
      (current_time - capture.began_time) > kMoveStickDoubleTapMaxSeconds) {
    return false;
  }
  const TouchPoint unit = MoveStickUnitVectorForCapture(control, frame, capture);
  return unit.y <= -kMoveStickDoubleTapForwardThreshold &&
         std::abs(unit.x) <= kMoveStickDoubleTapLateralThreshold;
}

}  // namespace rex::input::touch
