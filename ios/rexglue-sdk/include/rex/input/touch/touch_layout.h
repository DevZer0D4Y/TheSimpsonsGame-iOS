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
 *              ReXGlue runtime. Gameplay model only; the layout editor, layout
 *              library and TOML layout files are not part of the port.
 */

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <type_traits>
#include <vector>

namespace rex::input::touch {

// The on-screen gamepad's data model: which controls exist, where they sit, and
// what they emit. Platform independent; each platform supplies a view that
// hit-tests touches against it and publishes a TouchResolvedState, which
// TouchInputDriver hands to the guest as an XInput pad.

enum class TouchControlType : uint8_t {
  kMoveStick = 0,
  kLookSwipeZone,
  kActionButton,
};

enum class TouchControlShape : uint8_t {
  kCircle = 0,
  kRoundedRect,
};

enum class TouchAction : uint8_t {
  kNone = 0,
  kMove,
  kLook,
  kButtonA,
  kButtonB,
  kButtonX,
  kButtonY,
  kLeftBumper,
  kRightBumper,
  kLeftTrigger,
  kRightTrigger,
  kBack,
  kStart,
  kLeftThumb,
  kRightThumb,
  kDpadUp,
  kDpadDown,
  kDpadLeft,
  kDpadRight,
};

enum class TouchInteractionTrigger : uint8_t {
  kNone = 0,
  kHold,
  kHoldDrag,
  kDoubleTap,
  kDoubleTapForward,
};

enum class TouchAnalogOutput : uint8_t {
  kNone = 0,
  kLook,
  kMove,
};

struct TouchPoint {
  float x = 0.0f;
  float y = 0.0f;
};

struct TouchRect {
  float x = 0.0f;
  float y = 0.0f;
  float width = 0.0f;
  float height = 0.0f;
};

struct TouchAnalogTuning {
  float deadzone = 0.0f;
  float activation_radius = 1.0f;
  float horizontal_scale = 1.0f;
  float vertical_scale = 1.0f;
  float diagonal_scale = 1.0f;
  float response_curve = 1.0f;
  float acceleration_scale = 0.0f;
  float smoothing = 0.0f;
  float max_output = 1.0f;
  bool invert_x = false;
  bool invert_y = false;
};

struct TouchLayoutSpace {
  float origin_x = 0.0f;
  float origin_y = 0.0f;
  float width = 0.0f;
  float height = 0.0f;

  bool IsEmpty() const { return width <= 0.0f || height <= 0.0f; }
};

// A second behavior layered on a control: a hold, a hold-and-drag, a double
// tap, or (on the move stick) a double tap forward.
struct TouchInteractionBehavior {
  TouchInteractionTrigger trigger = TouchInteractionTrigger::kNone;
  TouchAction action = TouchAction::kNone;
  TouchAnalogOutput analog_output = TouchAnalogOutput::kNone;
  TouchAnalogTuning analog_tuning;
  bool enables_relative_look = false;
  float relative_look_scale = 1.0f;
  float hold_seconds = 0.30f;
  float drag_threshold_points = 14.0f;
};

struct TouchControlDefinition {
  std::string identifier;
  std::string label;
  bool label_hidden = false;
  TouchControlType type = TouchControlType::kActionButton;
  TouchAction action = TouchAction::kNone;
  TouchControlShape shape = TouchControlShape::kCircle;
  // Normalized to the overlay's bounds: 0..1 on both axes.
  TouchRect normalized_frame;
  float deadzone = 0.0f;
  float activation_radius = 0.0f;
  float visual_opacity = 1.0f;
  // Keep the control pressed for as long as the finger that started on it is
  // down, even after it slides off (triggers held while aiming).
  bool hold_while_captured = false;
  bool enables_relative_look = false;
  TouchAnalogOutput drag_output = TouchAnalogOutput::kNone;
  TouchAnalogTuning analog_tuning;
  // Look swipe zones: scales swipe distance per full stick deflection. Action
  // buttons with enables_relative_look: scales the look emitted while held.
  float relative_look_scale = 1.0f;
  // Scale all look / move output while this control is held.
  float held_look_scale = 1.0f;
  float held_move_scale = 1.0f;
  // Move stick only: four D-pad arrows around the stick base. A touch that
  // lands on an arrow presses that D-pad direction instead of moving.
  bool move_with_dpad_ring = false;
  TouchInteractionBehavior secondary_behavior;
  // Overlapping controls: the highest priority takes the touch.
  uint8_t capture_priority = 0;
  uint16_t mapped_buttons = 0;
  uint8_t mapped_left_trigger = 0;
  uint8_t mapped_right_trigger = 0;
};

struct TouchLayoutModel {
  std::string layout_id;
  std::string display_name;
  std::vector<TouchControlDefinition> controls;
};

struct TouchResolvedState {
  uint32_t packet_number = 0;
  uint16_t buttons = 0;
  uint8_t left_trigger = 0;
  uint8_t right_trigger = 0;
  int16_t thumb_lx = 0;
  int16_t thumb_ly = 0;
  int16_t thumb_rx = 0;
  int16_t thumb_ry = 0;
  // The overlay is on screen and owns the player's input.
  bool gameplay_enabled = false;
};

static_assert(std::is_trivially_copyable<TouchResolvedState>::value,
              "TouchResolvedState crosses threads by copy");

class TouchResolvedStateBuffer {
 public:
  void Store(const TouchResolvedState& state);
  TouchResolvedState Load() const;

 private:
  mutable std::mutex mutex_;
  TouchResolvedState state_{};
};

// Shared between the platform overlay (UI thread) and the input driver (guest
// threads). The layout is written only before the overlay attaches and is
// read-only afterwards; the resolved state is the one cross-thread channel.
class TouchRuntimeModel {
 public:
  TouchRuntimeModel();

  const TouchLayoutModel& layout() const { return layout_; }
  void SetLayout(TouchLayoutModel layout);

  void StoreResolvedState(const TouchResolvedState& state) { resolved_state_.Store(state); }
  TouchResolvedState LoadResolvedState() const { return resolved_state_.Load(); }

  // Whether a platform overlay is driving this model. Without one there is no
  // touch pad to report.
  bool attached() const { return attached_.load(std::memory_order_acquire); }
  void set_attached(bool attached) { attached_.store(attached, std::memory_order_release); }

 private:
  TouchLayoutModel layout_;
  TouchResolvedStateBuffer resolved_state_;
  std::atomic<bool> attached_{false};
};

TouchRuntimeModel& GetTouchRuntimeModel();

// XeniOS's "FPS Compact" layout, without its pause button: move stick with a
// D-pad ring (double tap forward = LS), full-screen swipe look, A/B/X/Y
// diamond (hold B = RS), LT/RT that aim while held, LB/RB, Back and Start.
TouchLayoutModel CreateDefaultTouchLayout();

const char* TouchActionDisplayName(TouchAction action);
std::string TouchControlVisibleLabel(const TouchControlDefinition& control);
// Sets `action` and the buttons/triggers/look behavior it implies.
bool ConfigureTouchControlAction(TouchAction action, TouchControlDefinition* control);
float DefaultTouchHoldSecondsForTrigger(TouchInteractionTrigger trigger);

TouchRect ResolveTouchRect(const TouchRect& normalized_rect, const TouchLayoutSpace& space);
bool TouchRectContainsPoint(const TouchRect& rect, const TouchPoint& point);

}  // namespace rex::input::touch
