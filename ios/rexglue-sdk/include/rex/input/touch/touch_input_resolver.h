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

#include <rex/input/touch/touch_layout.h>

namespace rex::input::touch {

// Turns captured touches into stick, button and trigger values. Platform
// independent: the platform overlay tracks the fingers and calls in here.

enum class TouchComboSubzone : uint8_t {
  kNone = 0,
  kStick,
  kDpadUp,
  kDpadDown,
  kDpadLeft,
  kDpadRight,
};

// One finger's capture of a control.
struct TouchInputCapture {
  TouchPoint anchor_point;
  TouchPoint current_point;
  double began_time = 0.0;
  bool secondary_behavior_triggered = false;
  TouchComboSubzone combo_subzone = TouchComboSubzone::kNone;
};

struct TouchInteractionBehaviorState {
  bool active = false;
  TouchAnalogOutput analog_output = TouchAnalogOutput::kNone;
  TouchAnalogTuning analog_tuning;
  bool enables_relative_look = false;
  float relative_look_scale = 1.0f;
};

TouchPoint ClampTouchLookVector(TouchPoint value);
TouchPoint ApplyTouchAnalogTuning(TouchPoint value, const TouchAnalogTuning& tuning);
TouchPoint ApplyTouchAnalogTuningWithVelocity(TouchPoint value, const TouchAnalogTuning& tuning,
                                              float velocity_full_scales_per_second);
TouchPoint TouchSwipeLookVectorForDelta(TouchPoint delta, float look_scale,
                                        float points_per_full_scale, float vertical_scale);
TouchComboSubzone ResolveTouchComboSubzone(const TouchControlDefinition& control,
                                           const TouchRect& resolved_frame, TouchPoint point);
bool TouchControlContainsPoint(const TouchControlDefinition& control,
                               const TouchRect& resolved_frame, TouchPoint point);
int16_t TouchAxisFromUnit(float unit_value);
bool TouchStatesEqualIgnoringPacket(const TouchResolvedState& left,
                                    const TouchResolvedState& right);
void ApplyTouchActionMapping(const TouchControlDefinition& control, TouchResolvedState* state);
void ApplyTouchActionMappingForAction(TouchAction action, TouchResolvedState* state);
bool TouchInteractionBehaviorConfigured(const TouchInteractionBehavior& behavior);
TouchInteractionBehaviorState ResolveTouchInteractionBehaviorState(
    const TouchInteractionBehavior& behavior, const TouchInputCapture& capture,
    double current_time);
// A button whose press is only reported on release, because holding it
// triggers something else instead.
bool TouchControlUsesDeferredPrimaryTap(const TouchControlDefinition& control);
TouchPoint MoveStickUnitVectorForCapture(const TouchControlDefinition& control,
                                         const TouchRect& frame, const TouchInputCapture& capture);
bool MoveStickCaptureQualifiesForDoubleTapForward(const TouchControlDefinition& control,
                                                  const TouchRect& frame,
                                                  const TouchInputCapture& capture,
                                                  double current_time);

}  // namespace rex::input::touch
