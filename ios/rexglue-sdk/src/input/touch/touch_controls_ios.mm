/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 *
 * @modified    2026 - Ported from XeniOS (github.com/xenios-jp/XeniOS) to the
 *              ReXGlue runtime: the gameplay overlay, control shell and
 *              visibility handling. XeniOS's layout editor, layout library and
 *              pause menu are not part of the port.
 */

#include <rex/input/touch/touch_controls.h>

#import <GameController/GameController.h>
#import <QuartzCore/QuartzCore.h>
#import <UIKit/UIKit.h>

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>
#include <vector>

#include <SDL3/SDL.h>

#include <rex/cvar.h>
#include <rex/input/flags.h>
#include <rex/input/input.h>
#include <rex/input/touch/touch_input_resolver.h>
#include <rex/input/touch/touch_layout.h>
#include <rex/logging.h>
#include <rex/ui/window_sdl.h>

#if !__has_feature(objc_arc)
#error "touch_controls_ios.mm must be compiled with -fobjc-arc"
#endif

namespace touch = rex::input::touch;

namespace {

// A button re-tapped while its previous tap is still being held for the guest
// is released for this long first, so the guest sees two presses.
constexpr CFTimeInterval kTouchButtonRetapReleaseGapSeconds = 0.012;
constexpr CGFloat kComboStickRadiusFraction = 0.32f;

struct TouchCaptureState {
  // Compared, never messaged: the capture is dropped when its touch ends.
  __unsafe_unretained UITouch* touch = nil;
  NSUInteger control_index = NSNotFound;
  CGPoint anchor_point = CGPointZero;
  CGPoint current_point = CGPointZero;
  CFTimeInterval began_time = 0.0;
  CFTimeInterval last_motion_time = 0.0;
  bool secondary_behavior_triggered = false;
  touch::TouchComboSubzone combo_subzone = touch::TouchComboSubzone::kNone;
};

touch::TouchPoint ToTouchPoint(CGPoint point) {
  return touch::TouchPoint{static_cast<float>(point.x), static_cast<float>(point.y)};
}

CGPoint ToCGPoint(touch::TouchPoint point) {
  return CGPointMake(point.x, point.y);
}

CGRect CGRectFromTouchRect(const touch::TouchRect& rect) {
  return CGRectMake(rect.x, rect.y, rect.width, rect.height);
}

touch::TouchInputCapture ToInputCapture(const TouchCaptureState& capture) {
  touch::TouchInputCapture input_capture;
  input_capture.anchor_point = ToTouchPoint(capture.anchor_point);
  input_capture.current_point = ToTouchPoint(capture.current_point);
  input_capture.began_time = capture.began_time;
  input_capture.secondary_behavior_triggered = capture.secondary_behavior_triggered;
  input_capture.combo_subzone = capture.combo_subzone;
  return input_capture;
}

bool ContainsPoint(const touch::TouchControlDefinition& control, const touch::TouchRect& frame,
                   CGPoint point) {
  return touch::TouchControlContainsPoint(control, frame, ToTouchPoint(point));
}

float LookPointsPerFullScale() {
  return std::clamp(static_cast<float>(REXCVAR_GET(touch_look_points_per_full_scale)), 1.0f,
                    64.0f);
}

float LookVerticalScale() {
  return std::clamp(static_cast<float>(REXCVAR_GET(touch_look_vertical_scale)), 0.25f, 4.0f);
}

float LookHoldSeconds() {
  return std::clamp(static_cast<float>(REXCVAR_GET(touch_look_hold_seconds)), 0.016f, 0.25f);
}

float ButtonTapHoldSeconds() {
  return std::clamp(static_cast<float>(REXCVAR_GET(touch_button_tap_hold_seconds)), 0.016f,
                    0.25f);
}

CGFloat OpacityScale() {
  return std::clamp(static_cast<CGFloat>(REXCVAR_GET(touch_controls_opacity)), CGFloat(0.1),
                    CGFloat(1.0));
}

CGPoint SwipeLookVectorForDelta(CGPoint delta, float look_scale) {
  return ToCGPoint(touch::TouchSwipeLookVectorForDelta(
      ToTouchPoint(delta), look_scale, LookPointsPerFullScale(), LookVerticalScale()));
}

CGPoint TouchAnalogVectorForDelta(CGPoint delta, CFTimeInterval elapsed_seconds,
                                  const touch::TouchAnalogTuning& tuning) {
  const float safe_elapsed =
      std::clamp(static_cast<float>(elapsed_seconds), 1.0f / 240.0f, 0.25f);
  const float velocity_points_per_second =
      static_cast<float>(std::hypot(delta.x, delta.y)) / safe_elapsed;
  const float velocity_full_scales_per_second =
      velocity_points_per_second / std::max(LookPointsPerFullScale(), 1.0f);
  return ToCGPoint(touch::ApplyTouchAnalogTuningWithVelocity(
      ToTouchPoint(SwipeLookVectorForDelta(delta, 1.0f)), tuning,
      velocity_full_scales_per_second));
}

void BlendMaxMagnitude(CGPoint vector, CGPoint* accumulator) {
  if (std::abs(vector.x) > std::abs(accumulator->x)) {
    accumulator->x = vector.x;
  }
  if (std::abs(vector.y) > std::abs(accumulator->y)) {
    accumulator->y = vector.y;
  }
}

touch::TouchAnalogOutput EffectiveControlDragOutput(const touch::TouchControlDefinition& control) {
  if (control.drag_output != touch::TouchAnalogOutput::kNone) {
    return control.drag_output;
  }
  if (control.enables_relative_look) {
    return touch::TouchAnalogOutput::kLook;
  }
  if (control.type == touch::TouchControlType::kLookSwipeZone) {
    return control.action == touch::TouchAction::kMove ? touch::TouchAnalogOutput::kMove
                                                        : touch::TouchAnalogOutput::kLook;
  }
  return touch::TouchAnalogOutput::kNone;
}

bool IsDpadSubzone(touch::TouchComboSubzone subzone) {
  return subzone == touch::TouchComboSubzone::kDpadUp ||
         subzone == touch::TouchComboSubzone::kDpadDown ||
         subzone == touch::TouchComboSubzone::kDpadLeft ||
         subzone == touch::TouchComboSubzone::kDpadRight;
}

UIColor* BorderColorForControl(const touch::TouchControlDefinition& control) {
  switch (control.type) {
    case touch::TouchControlType::kMoveStick:
      return [UIColor colorWithWhite:1.0 alpha:0.38];
    case touch::TouchControlType::kLookSwipeZone:
      return [UIColor colorWithWhite:1.0 alpha:0.12];
    case touch::TouchControlType::kActionButton:
    default:
      return [UIColor colorWithWhite:1.0 alpha:0.32];
  }
}

UIColor* FillColorForControl(const touch::TouchControlDefinition& control, CGFloat opacity) {
  const CGFloat alpha = std::clamp(opacity, CGFloat(0.0), CGFloat(1.0));
  switch (control.type) {
    case touch::TouchControlType::kMoveStick:
      return [UIColor colorWithWhite:1.0 alpha:0.10 * alpha];
    case touch::TouchControlType::kLookSwipeZone:
      return UIColor.clearColor;
    case touch::TouchControlType::kActionButton:
    default:
      return [UIColor colorWithWhite:1.0 alpha:0.08 * alpha];
  }
}

// Controls are placed in the overlay's full bounds, clamped so none is pushed
// off screen.
touch::TouchRect ResolveControlFrame(const touch::TouchControlDefinition& control,
                                     const touch::TouchLayoutSpace& space) {
  touch::TouchRect frame = control.normalized_frame;
  const float max_size = control.type == touch::TouchControlType::kLookSwipeZone ? 1.0f : 0.98f;
  frame.width = std::clamp(frame.width, 0.05f, max_size);
  frame.height = std::clamp(frame.height, 0.05f, max_size);
  frame.x = std::clamp(frame.x, 0.0f, std::max(0.0f, 1.0f - frame.width));
  frame.y = std::clamp(frame.y, 0.0f, std::max(0.0f, 1.0f - frame.height));
  return touch::ResolveTouchRect(frame, space);
}

bool HasConnectedGameController() {
  for (GCController* controller in GCController.controllers) {
    if (controller.extendedGamepad || controller.microGamepad) {
      return true;
    }
  }
  return false;
}

}  // namespace

// One control's visuals: a translucent circle or capsule with a label, plus
// D-pad chevrons around the move stick.
@interface RexTouchControlShellView : UIView
- (instancetype)initWithControl:(const touch::TouchControlDefinition&)control
    NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithFrame:(CGRect)frame NS_UNAVAILABLE;
- (instancetype)initWithCoder:(NSCoder*)coder NS_UNAVAILABLE;
- (void)setTouchActive:(BOOL)active;
// Full-screen swipe zones draw nothing during gameplay; the view still exists
// for layout.
- (void)setChromeSuppressed:(BOOL)suppressed;
@end

@implementation RexTouchControlShellView {
  touch::TouchControlDefinition control_;
  UILabel* label_;
  NSArray<UIImageView*>* dpad_arrows_;  // up, down, left, right
  BOOL touch_active_;
  BOOL chrome_suppressed_;
}

- (instancetype)initWithControl:(const touch::TouchControlDefinition&)control {
  if (!(self = [super initWithFrame:CGRectZero])) {
    return nil;
  }
  control_ = control;
  self.backgroundColor = UIColor.clearColor;
  self.userInteractionEnabled = NO;
  self.layer.borderWidth = 1.5;

  label_ = [[UILabel alloc] initWithFrame:CGRectZero];
  label_.backgroundColor = UIColor.clearColor;
  label_.textAlignment = NSTextAlignmentCenter;
  label_.font = [UIFont systemFontOfSize:12.0 weight:UIFontWeightSemibold];
  label_.textColor = [UIColor colorWithWhite:1.0 alpha:0.92];
  label_.adjustsFontSizeToFitWidth = YES;
  label_.minimumScaleFactor = 0.6;
  const std::string label_text = touch::TouchControlVisibleLabel(control_);
  label_.text = label_text.empty() ? nil : [NSString stringWithUTF8String:label_text.c_str()];
  label_.hidden = label_.text.length == 0;
  [self addSubview:label_];

  if (control_.type == touch::TouchControlType::kMoveStick && control_.move_with_dpad_ring) {
    UIImageSymbolConfiguration* config =
        [UIImageSymbolConfiguration configurationWithPointSize:18.0
                                                        weight:UIImageSymbolWeightBold];
    NSMutableArray<UIImageView*>* arrows = [NSMutableArray arrayWithCapacity:4];
    for (NSString* name in @[ @"chevron.up", @"chevron.down", @"chevron.left", @"chevron.right" ]) {
      UIImageView* arrow =
          [[UIImageView alloc] initWithImage:[UIImage systemImageNamed:name
                                                     withConfiguration:config]];
      arrow.contentMode = UIViewContentModeCenter;
      arrow.userInteractionEnabled = NO;
      arrow.alpha = 0.85;
      arrow.tintColor = UIColor.whiteColor;
      [self addSubview:arrow];
      [arrows addObject:arrow];
    }
    dpad_arrows_ = arrows;
  }

  [self refreshVisualState];
  return self;
}

- (CGFloat)baseVisualAlpha {
  if (control_.type == touch::TouchControlType::kLookSwipeZone) {
    return 1.0;
  }
  return std::clamp(static_cast<CGFloat>(control_.visual_opacity), CGFloat(0.0), CGFloat(1.0)) *
         OpacityScale();
}

- (void)refreshVisualState {
  if (chrome_suppressed_) {
    [UIView animateWithDuration:0.10
                          delay:0.0
                        options:(UIViewAnimationOptionCurveEaseOut |
                                 UIViewAnimationOptionAllowUserInteraction |
                                 UIViewAnimationOptionBeginFromCurrentState)
                     animations:^{
                       self.alpha = 0.0;
                       self.backgroundColor = UIColor.clearColor;
                       self.transform = CGAffineTransformIdentity;
                     }
                     completion:nil];
    self.layer.borderColor = UIColor.clearColor.CGColor;
    self.layer.borderWidth = 0.0;
    return;
  }

  const bool is_look_zone = control_.type == touch::TouchControlType::kLookSwipeZone;
  const CGFloat base_alpha = [self baseVisualAlpha];
  const CGFloat target_alpha = touch_active_ ? MIN(base_alpha + 0.18, 1.0) : base_alpha;
  CGFloat fill_alpha = touch_active_ ? MIN(base_alpha + 0.45, 1.0) : base_alpha;
  if (is_look_zone) {
    fill_alpha = touch_active_ ? 0.55 : 0.18;
  }
  UIColor* target_fill = FillColorForControl(control_, fill_alpha);
  UIColor* target_border = BorderColorForControl(control_);
  if (touch_active_) {
    target_border = is_look_zone ? UIColor.whiteColor : [UIColor colorWithWhite:1.0 alpha:0.78];
  }
  const CGFloat target_border_width =
      is_look_zone ? (touch_active_ ? 1.8 : 1.0) : (touch_active_ ? 2.0 : 1.5);
  const CGAffineTransform target_transform =
      touch_active_ ? CGAffineTransformMakeScale(1.03, 1.03) : CGAffineTransformIdentity;

  [UIView animateWithDuration:0.08
                        delay:0.0
                      options:(UIViewAnimationOptionCurveEaseOut |
                               UIViewAnimationOptionAllowUserInteraction |
                               UIViewAnimationOptionBeginFromCurrentState)
                   animations:^{
                     self.alpha = target_alpha;
                     self.backgroundColor = target_fill;
                     self.transform = target_transform;
                   }
                   completion:nil];

  [CATransaction begin];
  [CATransaction setAnimationDuration:0.08];
  [CATransaction
      setAnimationTimingFunction:[CAMediaTimingFunction
                                     functionWithName:kCAMediaTimingFunctionEaseOut]];
  self.layer.borderColor = target_border.CGColor;
  self.layer.borderWidth = target_border_width;
  [CATransaction commit];
}

- (void)setTouchActive:(BOOL)active {
  if (touch_active_ == active) {
    return;
  }
  touch_active_ = active;
  [self refreshVisualState];
}

- (void)setChromeSuppressed:(BOOL)suppressed {
  if (chrome_suppressed_ == suppressed) {
    return;
  }
  chrome_suppressed_ = suppressed;
  label_.hidden = suppressed || label_.text.length == 0;
  for (UIImageView* arrow in dpad_arrows_) {
    arrow.hidden = suppressed;
  }
  [self refreshVisualState];
}

- (void)layoutSubviews {
  [super layoutSubviews];

  const CGRect bounds = self.bounds;
  const CGFloat short_side = MIN(CGRectGetWidth(bounds), CGRectGetHeight(bounds));
  if (control_.shape == touch::TouchControlShape::kCircle) {
    self.layer.cornerRadius = short_side * 0.5;
  } else {
    self.layer.cornerRadius = control_.type == touch::TouchControlType::kLookSwipeZone ? 22.0 : 16.0;
  }
  label_.frame = CGRectInset(bounds, 8.0, 8.0);

  if (dpad_arrows_.count == 4) {
    const CGFloat stick_radius = short_side * kComboStickRadiusFraction;
    const CGFloat outer_radius = short_side * 0.5;
    const CGFloat arrow_radius = (stick_radius + outer_radius) * 0.5;
    const CGFloat arrow_size = MAX(short_side * 0.18, 24.0);
    const CGFloat centre_x = CGRectGetMidX(bounds);
    const CGFloat centre_y = CGRectGetMidY(bounds);
    const CGPoint centres[4] = {
        CGPointMake(centre_x, centre_y - arrow_radius),
        CGPointMake(centre_x, centre_y + arrow_radius),
        CGPointMake(centre_x - arrow_radius, centre_y),
        CGPointMake(centre_x + arrow_radius, centre_y),
    };
    for (NSUInteger i = 0; i < 4; ++i) {
      dpad_arrows_[i].frame = CGRectMake(centres[i].x - arrow_size * 0.5,
                                         centres[i].y - arrow_size * 0.5, arrow_size, arrow_size);
    }
  }
}

@end

// The gameplay overlay: hit-tests every finger against the layout, turns the
// captures into a pad state for TouchInputDriver, and animates the controls.
@interface RexTouchControlsOverlayView : UIView
- (instancetype)initWithRuntimeModel:(touch::TouchRuntimeModel*)runtime_model
    NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithFrame:(CGRect)frame NS_UNAVAILABLE;
- (instancetype)initWithCoder:(NSCoder*)coder NS_UNAVAILABLE;
- (void)setGameplayOverlayVisible:(BOOL)visible animated:(BOOL)animated;
// Off while a system dialog is up, so taps outside the buttons reach it.
- (void)setLookZoneEnabled:(BOOL)enabled;
- (void)shutdown;
@end

@implementation RexTouchControlsOverlayView {
  touch::TouchRuntimeModel* runtime_model_;
  NSMutableArray<RexTouchControlShellView*>* control_views_;
  UIView* move_knob_;
  CADisplayLink* display_link_;
  std::vector<touch::TouchRect> resolved_control_frames_;
  std::vector<uint8_t> visually_active_control_indices_;
  std::vector<CFTimeInterval> recent_action_press_times_;
  std::vector<CFTimeInterval> recent_action_suppressed_until_times_;
  std::vector<CFTimeInterval> recent_secondary_press_times_;
  std::vector<CFTimeInterval> recent_secondary_candidate_times_;
  std::vector<CGPoint> recent_look_vectors_;
  std::vector<CFTimeInterval> recent_look_motion_times_;
  std::vector<CGPoint> recent_move_vectors_;
  std::vector<CFTimeInterval> recent_move_motion_times_;
  std::vector<TouchCaptureState> active_captures_;
  touch::TouchResolvedState last_published_state_;
  uint32_t next_packet_number_;
  BOOL gameplay_overlay_active_;
  BOOL look_zone_enabled_;
  NSUInteger move_control_index_;
  UIImpactFeedbackGenerator* haptic_press_;        // buttons and D-pad arrows
  UIImpactFeedbackGenerator* haptic_press_light_;  // stick engage
}

- (instancetype)initWithRuntimeModel:(touch::TouchRuntimeModel*)runtime_model {
  if (!(self = [super initWithFrame:CGRectZero])) {
    return nil;
  }
  // Drawn over running gameplay: keep system chrome dark whatever the device
  // appearance.
  self.overrideUserInterfaceStyle = UIUserInterfaceStyleDark;

  runtime_model_ = runtime_model;
  control_views_ = [NSMutableArray array];
  move_control_index_ = NSNotFound;
  next_packet_number_ = 1;
  gameplay_overlay_active_ = NO;
  look_zone_enabled_ = YES;

  move_knob_ = [[UIView alloc] initWithFrame:CGRectZero];
  move_knob_.hidden = YES;
  move_knob_.userInteractionEnabled = NO;
  move_knob_.backgroundColor = [UIColor colorWithWhite:1.0 alpha:0.18];
  move_knob_.layer.borderWidth = 1.0;
  move_knob_.layer.borderColor = [UIColor colorWithWhite:1.0 alpha:0.72].CGColor;
  [self addSubview:move_knob_];

  self.backgroundColor = UIColor.clearColor;
  self.opaque = NO;
  self.alpha = 0.0;
  self.hidden = YES;
  self.userInteractionEnabled = YES;
  self.multipleTouchEnabled = YES;

  // Created up front so the first press does not pay generator setup latency;
  // -prepare after each impact keeps them warm.
  haptic_press_ = [[UIImpactFeedbackGenerator alloc] initWithStyle:UIImpactFeedbackStyleMedium];
  haptic_press_light_ =
      [[UIImpactFeedbackGenerator alloc] initWithStyle:UIImpactFeedbackStyleLight];

  [self createDisplayLinkIfNeeded];
  [self refreshLayoutModel];
  return self;
}

- (void)createDisplayLinkIfNeeded {
  if (display_link_) {
    return;
  }
  // Republishes every frame while visible: look swipes decay and tapped
  // buttons release on a timer, with no touch event to drive them.
  display_link_ = [CADisplayLink displayLinkWithTarget:self selector:@selector(displayLinkFired:)];
  display_link_.paused = !gameplay_overlay_active_;
  [display_link_ addToRunLoop:NSRunLoop.mainRunLoop forMode:NSRunLoopCommonModes];
}

- (void)shutdown {
  // The display link retains its target; breaking it here is what lets the
  // overlay deallocate.
  [display_link_ invalidate];
  display_link_ = nil;
  gameplay_overlay_active_ = NO;
  [self resetInteractionState];
  if (runtime_model_) {
    runtime_model_->StoreResolvedState(touch::TouchResolvedState{});
  }
}

- (void)willMoveToWindow:(UIWindow*)new_window {
  if (!new_window) {
    [display_link_ invalidate];
    display_link_ = nil;
  } else {
    [self createDisplayLinkIfNeeded];
  }
  [super willMoveToWindow:new_window];
}

- (void)playPressHaptic {
  if (!REXCVAR_GET(touch_haptics)) {
    return;
  }
  [haptic_press_ impactOccurred];
  [haptic_press_ prepare];
}

- (void)playLightPressHaptic {
  if (!REXCVAR_GET(touch_haptics)) {
    return;
  }
  [haptic_press_light_ impactOccurred];
  [haptic_press_light_ prepare];
}

- (void)refreshLayoutModel {
  [self resetInteractionState];
  for (UIView* control_view in control_views_) {
    [control_view removeFromSuperview];
  }
  [control_views_ removeAllObjects];
  move_control_index_ = NSNotFound;
  if (!runtime_model_) {
    return;
  }

  const auto& controls = runtime_model_->layout().controls;
  const size_t count = controls.size();
  resolved_control_frames_.assign(count, touch::TouchRect{});
  visually_active_control_indices_.assign(count, 0);
  recent_action_press_times_.assign(count, 0.0);
  recent_action_suppressed_until_times_.assign(count, 0.0);
  recent_secondary_press_times_.assign(count, 0.0);
  recent_secondary_candidate_times_.assign(count, 0.0);
  recent_look_vectors_.assign(count, CGPointZero);
  recent_look_motion_times_.assign(count, 0.0);
  recent_move_vectors_.assign(count, CGPointZero);
  recent_move_motion_times_.assign(count, 0.0);

  for (NSUInteger control_index = 0; control_index < count; ++control_index) {
    const touch::TouchControlDefinition& control = controls[control_index];
    RexTouchControlShellView* shell_view =
        [[RexTouchControlShellView alloc] initWithControl:control];
    [control_views_ addObject:shell_view];
    [self addSubview:shell_view];
    if (control.type == touch::TouchControlType::kMoveStick &&
        control.action != touch::TouchAction::kLook && move_control_index_ == NSNotFound) {
      move_control_index_ = control_index;
    }
  }
  [self bringSubviewToFront:move_knob_];
  [self setNeedsLayout];
}

- (BOOL)controlIndexAcceptsTouches:(NSUInteger)control_index {
  if (!runtime_model_ || control_index >= runtime_model_->layout().controls.size()) {
    return NO;
  }
  const auto& control = runtime_model_->layout().controls[control_index];
  return look_zone_enabled_ || control.type != touch::TouchControlType::kLookSwipeZone;
}

- (BOOL)isControlIndexCaptured:(NSUInteger)control_index {
  return std::any_of(active_captures_.begin(), active_captures_.end(),
                     [control_index](const TouchCaptureState& capture) {
                       return capture.control_index == control_index;
                     });
}

- (float)doubleTapWindowSecondsForControl:(const touch::TouchControlDefinition&)control {
  return std::clamp(control.secondary_behavior.hold_seconds, 0.12f, 0.60f);
}

- (void)triggerSecondaryBehaviorPulseForControlIndex:(NSUInteger)control_index
                                              atTime:(CFTimeInterval)current_time {
  if (!runtime_model_ || control_index >= recent_secondary_press_times_.size()) {
    return;
  }
  const auto& controls = runtime_model_->layout().controls;
  if (control_index >= controls.size() ||
      controls[control_index].secondary_behavior.action == touch::TouchAction::kNone) {
    return;
  }
  recent_secondary_press_times_[control_index] = current_time;
}

- (BOOL)hasPendingDoubleTapCandidateForControlIndex:(NSUInteger)control_index
                                             atTime:(CFTimeInterval)current_time {
  if (!runtime_model_ || control_index >= recent_secondary_candidate_times_.size()) {
    return NO;
  }
  const auto& controls = runtime_model_->layout().controls;
  if (control_index >= controls.size()) {
    return NO;
  }
  const CFTimeInterval candidate_time = recent_secondary_candidate_times_[control_index];
  if (candidate_time <= 0.0) {
    return NO;
  }
  return (current_time - candidate_time) <=
         [self doubleTapWindowSecondsForControl:controls[control_index]];
}

- (BOOL)consumeDoubleTapCandidateForControlIndex:(NSUInteger)control_index
                                          atTime:(CFTimeInterval)current_time {
  if (![self hasPendingDoubleTapCandidateForControlIndex:control_index atTime:current_time]) {
    return NO;
  }
  recent_secondary_candidate_times_[control_index] = 0.0;
  [self triggerSecondaryBehaviorPulseForControlIndex:control_index atTime:current_time];
  return YES;
}

- (void)storeDoubleTapCandidateForControlIndex:(NSUInteger)control_index
                                        atTime:(CFTimeInterval)current_time {
  if (control_index < recent_secondary_candidate_times_.size()) {
    recent_secondary_candidate_times_[control_index] = current_time;
  }
}

- (void)clearLookMotionState {
  std::fill(recent_look_vectors_.begin(), recent_look_vectors_.end(), CGPointZero);
  std::fill(recent_look_motion_times_.begin(), recent_look_motion_times_.end(), 0.0);
  std::fill(recent_move_vectors_.begin(), recent_move_vectors_.end(), CGPointZero);
  std::fill(recent_move_motion_times_.begin(), recent_move_motion_times_.end(), 0.0);
}

- (void)clearLookMotionStateForControlIndex:(NSUInteger)control_index {
  if (control_index >= recent_look_vectors_.size() ||
      control_index >= recent_look_motion_times_.size() ||
      control_index >= recent_move_vectors_.size() ||
      control_index >= recent_move_motion_times_.size()) {
    return;
  }
  recent_look_vectors_[control_index] = CGPointZero;
  recent_look_motion_times_[control_index] = 0.0;
  recent_move_vectors_[control_index] = CGPointZero;
  recent_move_motion_times_[control_index] = 0.0;
}

- (void)storeAnalogMotion:(CGPoint)vector
                   output:(touch::TouchAnalogOutput)output
          forControlIndex:(NSUInteger)control_index
                   atTime:(CFTimeInterval)current_time
                   tuning:(const touch::TouchAnalogTuning&)tuning {
  auto smooth_vector = [&](CGPoint previous_vector, CFTimeInterval previous_time) {
    const float smoothing = std::clamp(tuning.smoothing, 0.0f, 0.95f);
    if (smoothing <= 0.001f || previous_time <= 0.0 ||
        current_time - previous_time >= LookHoldSeconds()) {
      return vector;
    }
    const float sample_count =
        std::clamp(static_cast<float>((current_time - previous_time) * 60.0), 0.25f, 4.0f);
    const float retain_previous = std::pow(smoothing, sample_count);
    return CGPointMake(previous_vector.x * retain_previous + vector.x * (1.0f - retain_previous),
                       previous_vector.y * retain_previous + vector.y * (1.0f - retain_previous));
  };

  switch (output) {
    case touch::TouchAnalogOutput::kLook:
      if (control_index >= recent_look_vectors_.size() ||
          control_index >= recent_look_motion_times_.size()) {
        return;
      }
      recent_look_vectors_[control_index] = smooth_vector(recent_look_vectors_[control_index],
                                                          recent_look_motion_times_[control_index]);
      recent_look_motion_times_[control_index] = current_time;
      return;
    case touch::TouchAnalogOutput::kMove:
      if (control_index >= recent_move_vectors_.size() ||
          control_index >= recent_move_motion_times_.size()) {
        return;
      }
      recent_move_vectors_[control_index] = smooth_vector(recent_move_vectors_[control_index],
                                                          recent_move_motion_times_[control_index]);
      recent_move_motion_times_[control_index] = current_time;
      return;
    case touch::TouchAnalogOutput::kNone:
    default:
      return;
  }
}

- (void)resetInteractionState {
  active_captures_.clear();
  std::fill(recent_action_press_times_.begin(), recent_action_press_times_.end(), 0.0);
  std::fill(recent_action_suppressed_until_times_.begin(),
            recent_action_suppressed_until_times_.end(), 0.0);
  std::fill(recent_secondary_press_times_.begin(), recent_secondary_press_times_.end(), 0.0);
  std::fill(recent_secondary_candidate_times_.begin(), recent_secondary_candidate_times_.end(),
            0.0);
  [self clearLookMotionState];
  move_knob_.hidden = YES;
  for (RexTouchControlShellView* control_view in control_views_) {
    [control_view setTouchActive:NO];
  }
}

- (void)applyCaptureVisualState {
  if (!runtime_model_) {
    move_knob_.hidden = YES;
    return;
  }

  const auto& controls = runtime_model_->layout().controls;
  const NSUInteger control_count =
      MIN(control_views_.count, static_cast<NSUInteger>(controls.size()));
  if (visually_active_control_indices_.size() != control_count) {
    visually_active_control_indices_.resize(control_count);
  }
  std::fill(visually_active_control_indices_.begin(), visually_active_control_indices_.end(), 0);
  const TouchCaptureState* move_capture = nullptr;
  for (const TouchCaptureState& capture : active_captures_) {
    if (capture.control_index >= control_count) {
      continue;
    }
    visually_active_control_indices_[capture.control_index] = 1;
    if (capture.control_index == move_control_index_) {
      move_capture = &capture;
    }
  }
  for (NSUInteger control_index = 0; control_index < control_views_.count; ++control_index) {
    const BOOL active =
        control_index < control_count && visually_active_control_indices_[control_index];
    [control_views_[control_index] setTouchActive:active];
  }

  // The knob follows the thumb from where it landed, clamped to the stick's
  // travel. D-pad arrow presses do not move it.
  if (!move_capture || move_control_index_ == NSNotFound ||
      move_control_index_ >= resolved_control_frames_.size() ||
      IsDpadSubzone(move_capture->combo_subzone)) {
    move_knob_.hidden = YES;
    return;
  }
  const touch::TouchRect& move_frame = resolved_control_frames_[move_control_index_];
  const touch::TouchControlDefinition& move_control = controls[move_control_index_];
  const CGFloat outer_radius =
      MIN(move_frame.width, move_frame.height) * MAX(move_control.activation_radius, 0.24f);
  CGPoint delta = CGPointMake(move_capture->current_point.x - move_capture->anchor_point.x,
                              move_capture->current_point.y - move_capture->anchor_point.y);
  const CGFloat distance = std::hypot(delta.x, delta.y);
  if (distance > outer_radius && distance > 0.0) {
    const CGFloat scale = outer_radius / distance;
    delta.x *= scale;
    delta.y *= scale;
  }
  const CGFloat knob_size = MIN(move_frame.width, move_frame.height) * 0.28;
  move_knob_.bounds = CGRectMake(0.0, 0.0, knob_size, knob_size);
  move_knob_.center =
      CGPointMake(move_capture->anchor_point.x + delta.x, move_capture->anchor_point.y + delta.y);
  move_knob_.layer.cornerRadius = knob_size * 0.5;
  move_knob_.alpha = OpacityScale();
  move_knob_.hidden = NO;
}

- (void)publishResolvedState {
  if (!runtime_model_) {
    return;
  }

  touch::TouchResolvedState state = {};
  state.gameplay_enabled = gameplay_overlay_active_;

  const auto& controls = runtime_model_->layout().controls;
  const NSUInteger control_count =
      MIN(control_views_.count, static_cast<NSUInteger>(controls.size()));
  const CFTimeInterval current_time = CACurrentMediaTime();

  // Held controls can scale look / move output (none of the defaults do).
  float held_look_scale = 1.0f;
  float held_move_scale = 1.0f;
  for (const TouchCaptureState& capture : active_captures_) {
    if (capture.control_index >= control_count ||
        capture.control_index >= resolved_control_frames_.size()) {
      continue;
    }
    const touch::TouchControlDefinition& control = controls[capture.control_index];
    if (control.type != touch::TouchControlType::kActionButton) {
      continue;
    }
    const touch::TouchRect& frame = resolved_control_frames_[capture.control_index];
    const touch::TouchInteractionBehaviorState secondary_state =
        touch::ResolveTouchInteractionBehaviorState(control.secondary_behavior,
                                                    ToInputCapture(capture), current_time);
    const bool touch_active = control.hold_while_captured || secondary_state.active ||
                              ContainsPoint(control, frame, capture.current_point);
    if (!touch_active) {
      continue;
    }
    held_look_scale *= std::clamp(control.held_look_scale, 0.25f, 4.0f);
    held_move_scale *= std::clamp(control.held_move_scale, 0.25f, 4.0f);
  }
  held_look_scale = std::clamp(held_look_scale, 0.1f, 4.0f);
  held_move_scale = std::clamp(held_move_scale, 0.1f, 4.0f);

  for (const TouchCaptureState& capture : active_captures_) {
    if (capture.control_index >= control_count ||
        capture.control_index >= resolved_control_frames_.size()) {
      continue;
    }
    const NSUInteger control_index = capture.control_index;
    const touch::TouchControlDefinition& control = controls[control_index];
    const touch::TouchRect& frame = resolved_control_frames_[control_index];

    switch (control.type) {
      case touch::TouchControlType::kMoveStick: {
        // A touch that landed on a D-pad arrow presses that direction for as
        // long as it is held, however far the finger drifts.
        switch (capture.combo_subzone) {
          case touch::TouchComboSubzone::kDpadUp:
            state.buttons |= rex::input::X_INPUT_GAMEPAD_DPAD_UP;
            continue;
          case touch::TouchComboSubzone::kDpadDown:
            state.buttons |= rex::input::X_INPUT_GAMEPAD_DPAD_DOWN;
            continue;
          case touch::TouchComboSubzone::kDpadLeft:
            state.buttons |= rex::input::X_INPUT_GAMEPAD_DPAD_LEFT;
            continue;
          case touch::TouchComboSubzone::kDpadRight:
            state.buttons |= rex::input::X_INPUT_GAMEPAD_DPAD_RIGHT;
            continue;
          case touch::TouchComboSubzone::kStick:
          case touch::TouchComboSubzone::kNone:
            break;
        }
        const touch::TouchPoint move_unit =
            touch::MoveStickUnitVectorForCapture(control, frame, ToInputCapture(capture));
        if (move_unit.x != 0.0f || move_unit.y != 0.0f) {
          if (control.action == touch::TouchAction::kLook) {
            state.thumb_rx = touch::TouchAxisFromUnit(move_unit.x * held_look_scale);
            state.thumb_ry = touch::TouchAxisFromUnit(-move_unit.y * held_look_scale);
          } else {
            state.thumb_lx = touch::TouchAxisFromUnit(move_unit.x * held_move_scale);
            state.thumb_ly = touch::TouchAxisFromUnit(-move_unit.y * held_move_scale);
          }
        }
      } break;

      case touch::TouchControlType::kActionButton: {
        const touch::TouchInteractionBehaviorState secondary_state =
            touch::ResolveTouchInteractionBehaviorState(control.secondary_behavior,
                                                        ToInputCapture(capture), current_time);
        const bool touch_active = control.hold_while_captured || secondary_state.active ||
                                  ContainsPoint(control, frame, capture.current_point);
        const bool primary_suppressed =
            control_index < recent_action_suppressed_until_times_.size() &&
            current_time < recent_action_suppressed_until_times_[control_index];
        if (touch_active) {
          const bool deferred = touch::TouchControlUsesDeferredPrimaryTap(control);
          if (!primary_suppressed && !deferred) {
            touch::ApplyTouchActionMapping(control, &state);
          }
          if (secondary_state.active &&
              control.secondary_behavior.action != touch::TouchAction::kNone) {
            touch::ApplyTouchActionMappingForAction(control.secondary_behavior.action, &state);
          }
          if (!control.hold_while_captured && !primary_suppressed && !deferred &&
              control_index < recent_action_press_times_.size()) {
            recent_action_press_times_[control_index] = current_time;
          }
        }
      } break;

      case touch::TouchControlType::kLookSwipeZone:
      default:
        break;
    }
  }

  // Tapped buttons stay pressed for a minimum time after release.
  const float button_tap_hold_seconds = ButtonTapHoldSeconds();
  for (NSUInteger control_index = 0; control_index < control_count; ++control_index) {
    const touch::TouchControlDefinition& control = controls[control_index];
    // Held B still needs a minimum pulse for taps shorter than one guest
    // frame. Its timestamp is touch-down, so a long hold releases immediately.
    if (control.type != touch::TouchControlType::kActionButton ||
        (control.hold_while_captured && control.action != touch::TouchAction::kButtonB) ||
        control_index >= recent_action_press_times_.size()) {
      continue;
    }
    if ((current_time - recent_action_press_times_[control_index]) < button_tap_hold_seconds) {
      if (control_index < recent_action_suppressed_until_times_.size() &&
          current_time < recent_action_suppressed_until_times_[control_index]) {
        continue;
      }
      touch::ApplyTouchActionMapping(control, &state);
    }
  }

  // Double-tap and double-tap-forward behaviors fire as a short pulse.
  for (NSUInteger control_index = 0; control_index < control_count; ++control_index) {
    const touch::TouchControlDefinition& control = controls[control_index];
    if (control.secondary_behavior.action == touch::TouchAction::kNone ||
        control_index >= recent_secondary_press_times_.size()) {
      continue;
    }
    if ((current_time - recent_secondary_press_times_[control_index]) < button_tap_hold_seconds) {
      touch::ApplyTouchActionMappingForAction(control.secondary_behavior.action, &state);
    }
  }

  // Swipes: every control with a recent motion contributes, blended per axis
  // by magnitude, and each fades out over the hold time so a title polling at
  // a low frame rate still sees the motion.
  const float look_hold_seconds = LookHoldSeconds();
  const NSUInteger look_state_count =
      MIN(control_count, static_cast<NSUInteger>(recent_look_motion_times_.size()));
  CGPoint accumulated_right_thumb = CGPointZero;
  for (NSUInteger control_index = 0; control_index < look_state_count; ++control_index) {
    const CFTimeInterval motion_time = recent_look_motion_times_[control_index];
    if (motion_time <= 0.0) {
      continue;
    }
    const CFTimeInterval age = current_time - motion_time;
    if (age >= look_hold_seconds) {
      continue;
    }
    const float decay =
        std::clamp(1.0f - static_cast<float>(age / look_hold_seconds), 0.0f, 1.0f);
    const CGPoint vector = recent_look_vectors_[control_index];
    BlendMaxMagnitude(
        CGPointMake(vector.x * decay * held_look_scale, vector.y * decay * held_look_scale),
        &accumulated_right_thumb);
  }
  const NSUInteger move_state_count =
      MIN(control_count, static_cast<NSUInteger>(recent_move_motion_times_.size()));
  CGPoint accumulated_left_thumb = CGPointZero;
  bool any_left_swipe = false;
  for (NSUInteger control_index = 0; control_index < move_state_count; ++control_index) {
    const CFTimeInterval motion_time = recent_move_motion_times_[control_index];
    if (motion_time <= 0.0) {
      continue;
    }
    const CFTimeInterval age = current_time - motion_time;
    if (age >= look_hold_seconds) {
      continue;
    }
    const float decay =
        std::clamp(1.0f - static_cast<float>(age / look_hold_seconds), 0.0f, 1.0f);
    const CGPoint vector = recent_move_vectors_[control_index];
    const CGPoint decayed =
        CGPointMake(vector.x * decay * held_move_scale, vector.y * decay * held_move_scale);
    BlendMaxMagnitude(decayed, &accumulated_left_thumb);
    if (decayed.x != 0.0 || decayed.y != 0.0) {
      any_left_swipe = true;
    }
  }
  const int16_t swipe_rx =
      touch::TouchAxisFromUnit(static_cast<float>(accumulated_right_thumb.x));
  const int16_t swipe_ry =
      touch::TouchAxisFromUnit(static_cast<float>(accumulated_right_thumb.y));
  if (std::abs(static_cast<int>(swipe_rx)) > std::abs(static_cast<int>(state.thumb_rx))) {
    state.thumb_rx = swipe_rx;
  }
  if (std::abs(static_cast<int>(swipe_ry)) > std::abs(static_cast<int>(state.thumb_ry))) {
    state.thumb_ry = swipe_ry;
  }
  // Only override the stick's left thumb when a swipe actually moved it.
  if (any_left_swipe) {
    const int16_t swipe_lx =
        touch::TouchAxisFromUnit(static_cast<float>(accumulated_left_thumb.x));
    const int16_t swipe_ly =
        touch::TouchAxisFromUnit(static_cast<float>(accumulated_left_thumb.y));
    if (std::abs(static_cast<int>(swipe_lx)) > std::abs(static_cast<int>(state.thumb_lx))) {
      state.thumb_lx = swipe_lx;
    }
    if (std::abs(static_cast<int>(swipe_ly)) > std::abs(static_cast<int>(state.thumb_ly))) {
      state.thumb_ly = swipe_ly;
    }
  }

  // Titles detect input edges by packet number: bump it only on a change.
  if (!touch::TouchStatesEqualIgnoringPacket(state, last_published_state_)) {
    state.packet_number = next_packet_number_++;
    if (next_packet_number_ == 0) {
      next_packet_number_ = 1;
    }
    last_published_state_ = state;
  } else {
    state.packet_number = last_published_state_.packet_number;
  }

  runtime_model_->StoreResolvedState(state);
  [self applyCaptureVisualState];
}

- (void)setGameplayOverlayVisible:(BOOL)visible animated:(BOOL)animated {
  if (visible == gameplay_overlay_active_ && self.hidden == !visible) {
    return;
  }
  gameplay_overlay_active_ = visible;

  if (visible) {
    if (self.hidden) {
      self.alpha = 0.0;
    }
    self.hidden = NO;
    self.userInteractionEnabled = YES;
    display_link_.paused = NO;
    [self publishResolvedState];
    if (!animated) {
      self.alpha = 1.0;
      return;
    }
    UIViewPropertyAnimator* show_animator =
        [[UIViewPropertyAnimator alloc] initWithDuration:0.35
                                            dampingRatio:0.85
                                              animations:^{
                                                self.alpha = 1.0;
                                              }];
    [show_animator startAnimation];
    return;
  }

  display_link_.paused = YES;
  self.userInteractionEnabled = NO;
  [self resetInteractionState];
  [self publishResolvedState];
  if (!animated || self.hidden) {
    self.hidden = YES;
    self.alpha = 0.0;
    return;
  }
  UIViewPropertyAnimator* hide_animator =
      [[UIViewPropertyAnimator alloc] initWithDuration:0.22
                                          dampingRatio:1.0
                                            animations:^{
                                              self.alpha = 0.0;
                                            }];
  __weak RexTouchControlsOverlayView* weak_self = self;
  [hide_animator addCompletion:^(UIViewAnimatingPosition position) {
    (void)position;
    RexTouchControlsOverlayView* strong_self = weak_self;
    // Shown again before the fade finished: stay visible.
    if (strong_self && !strong_self->gameplay_overlay_active_) {
      strong_self.hidden = YES;
    }
  }];
  [hide_animator startAnimation];
}

- (void)setLookZoneEnabled:(BOOL)enabled {
  if (look_zone_enabled_ == enabled) {
    return;
  }
  look_zone_enabled_ = enabled;
  if (enabled || !runtime_model_) {
    return;
  }
  // Drop a swipe that is in progress so it cannot keep turning the camera
  // behind the dialog.
  const auto& controls = runtime_model_->layout().controls;
  active_captures_.erase(
      std::remove_if(active_captures_.begin(), active_captures_.end(),
                     [&controls](const TouchCaptureState& capture) {
                       return capture.control_index < controls.size() &&
                              controls[capture.control_index].type ==
                                  touch::TouchControlType::kLookSwipeZone;
                     }),
      active_captures_.end());
  [self clearLookMotionState];
  [self publishResolvedState];
}

- (void)displayLinkFired:(CADisplayLink*)display_link {
  (void)display_link;
  [self publishResolvedState];
}

- (UIView*)hitTest:(CGPoint)point withEvent:(UIEvent*)event {
  (void)event;
  if (self.hidden || self.alpha <= 0.01 || !self.userInteractionEnabled ||
      !gameplay_overlay_active_ || !runtime_model_) {
    return nil;
  }
  // Only claim touches that land on a control; the rest go to the view
  // underneath (the dialogs, while the look zone is off).
  const auto& controls = runtime_model_->layout().controls;
  const NSUInteger control_count =
      MIN(control_views_.count, static_cast<NSUInteger>(controls.size()));
  for (NSUInteger control_index = 0; control_index < control_count; ++control_index) {
    if (control_index >= resolved_control_frames_.size() ||
        ![self controlIndexAcceptsTouches:control_index]) {
      continue;
    }
    if (ContainsPoint(controls[control_index], resolved_control_frames_[control_index], point)) {
      return self;
    }
  }
  return nil;
}

- (void)touchesBegan:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event {
  (void)event;
  if (!runtime_model_ || !gameplay_overlay_active_) {
    return;
  }

  const auto& controls = runtime_model_->layout().controls;
  const NSUInteger control_count =
      MIN(control_views_.count, static_cast<NSUInteger>(controls.size()));
  for (UITouch* touch_object in touches) {
    const CGPoint point = [touch_object locationInView:self];

    // The highest-priority free control under the finger takes it.
    NSInteger best_control_index = -1;
    uint8_t best_priority = 0;
    for (NSUInteger control_index = 0; control_index < control_count; ++control_index) {
      if (control_index >= resolved_control_frames_.size() ||
          ![self controlIndexAcceptsTouches:control_index] ||
          [self isControlIndexCaptured:control_index]) {
        continue;
      }
      const touch::TouchControlDefinition& control = controls[control_index];
      if (!ContainsPoint(control, resolved_control_frames_[control_index], point)) {
        continue;
      }
      if (best_control_index < 0 || control.capture_priority > best_priority) {
        best_control_index = static_cast<NSInteger>(control_index);
        best_priority = control.capture_priority;
      }
    }
    if (best_control_index < 0) {
      continue;
    }

    TouchCaptureState capture;
    capture.touch = touch_object;
    capture.control_index = static_cast<NSUInteger>(best_control_index);
    capture.anchor_point = point;
    capture.current_point = point;
    capture.began_time = CACurrentMediaTime();
    capture.last_motion_time = capture.began_time;
    const touch::TouchControlDefinition& control = controls[capture.control_index];
    // Pinned for the capture's lifetime: a finger drifting from an arrow to
    // the centre does not turn a D-pad press into a stick deflection.
    capture.combo_subzone = touch::ResolveTouchComboSubzone(
        control, resolved_control_frames_[capture.control_index], ToTouchPoint(point));

    if (control.type == touch::TouchControlType::kActionButton &&
        capture.control_index < recent_action_press_times_.size()) {
      const CFTimeInterval tap_tail_age =
          capture.began_time - recent_action_press_times_[capture.control_index];
      if (tap_tail_age >= 0.0 && tap_tail_age < ButtonTapHoldSeconds()) {
        recent_action_press_times_[capture.control_index] = 0.0;
        if (capture.control_index < recent_action_suppressed_until_times_.size()) {
          recent_action_suppressed_until_times_[capture.control_index] =
              capture.began_time + kTouchButtonRetapReleaseGapSeconds;
        }
      }
    }

    if (control.hold_while_captured && control.action == touch::TouchAction::kButtonB &&
        capture.control_index < recent_action_press_times_.size()) {
      recent_action_press_times_[capture.control_index] = capture.began_time;
    }

    // The look zone is continuous and would buzz constantly, so no haptic.
    switch (control.type) {
      case touch::TouchControlType::kActionButton:
        [self playPressHaptic];
        break;
      case touch::TouchControlType::kMoveStick:
        if (IsDpadSubzone(capture.combo_subzone)) {
          [self playPressHaptic];
        } else {
          [self playLightPressHaptic];
        }
        break;
      case touch::TouchControlType::kLookSwipeZone:
      default:
        break;
    }
    active_captures_.push_back(capture);
  }

  [self publishResolvedState];
}

- (void)touchesMoved:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event {
  (void)event;
  if (!runtime_model_ || !gameplay_overlay_active_) {
    return;
  }

  const auto& controls = runtime_model_->layout().controls;
  const CFTimeInterval current_time = CACurrentMediaTime();
  for (UITouch* touch_object in touches) {
    auto capture_it = std::find_if(
        active_captures_.begin(), active_captures_.end(),
        [touch_object](const TouchCaptureState& capture) { return capture.touch == touch_object; });
    if (capture_it == active_captures_.end() || capture_it->control_index >= controls.size()) {
      continue;
    }

    const CGPoint new_point = [touch_object locationInView:self];
    const NSUInteger control_index = capture_it->control_index;
    const touch::TouchControlDefinition& control = controls[control_index];
    TouchCaptureState behavior_capture = *capture_it;
    behavior_capture.current_point = new_point;
    if (control.type == touch::TouchControlType::kMoveStick &&
        control.secondary_behavior.trigger == touch::TouchInteractionTrigger::kDoubleTapForward &&
        control_index < resolved_control_frames_.size() &&
        !capture_it->secondary_behavior_triggered &&
        [self hasPendingDoubleTapCandidateForControlIndex:control_index atTime:current_time] &&
        touch::MoveStickCaptureQualifiesForDoubleTapForward(
            control, resolved_control_frames_[control_index], ToInputCapture(behavior_capture),
            current_time)) {
      capture_it->secondary_behavior_triggered =
          [self consumeDoubleTapCandidateForControlIndex:control_index atTime:current_time];
    }
    const touch::TouchInteractionBehaviorState secondary_state =
        touch::ResolveTouchInteractionBehaviorState(control.secondary_behavior,
                                                    ToInputCapture(behavior_capture), current_time);
    const CGPoint delta = CGPointMake(new_point.x - capture_it->current_point.x,
                                      new_point.y - capture_it->current_point.y);
    const CFTimeInterval elapsed_seconds =
        std::max(current_time - capture_it->last_motion_time, 1.0 / 240.0);
    const touch::TouchAnalogOutput primary_output = EffectiveControlDragOutput(control);
    if (primary_output != touch::TouchAnalogOutput::kNone) {
      [self storeAnalogMotion:TouchAnalogVectorForDelta(delta, elapsed_seconds,
                                                        control.analog_tuning)
                       output:primary_output
              forControlIndex:control_index
                       atTime:current_time
                       tuning:control.analog_tuning];
    }
    if (secondary_state.active && secondary_state.analog_output != touch::TouchAnalogOutput::kNone) {
      [self storeAnalogMotion:TouchAnalogVectorForDelta(delta, elapsed_seconds,
                                                        secondary_state.analog_tuning)
                       output:secondary_state.analog_output
              forControlIndex:control_index
                       atTime:current_time
                       tuning:secondary_state.analog_tuning];
    }
    capture_it->current_point = new_point;
    capture_it->last_motion_time = current_time;
  }

  [self publishResolvedState];
}

- (void)touchesEnded:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event {
  (void)event;
  [self finalizeTouches:touches cancelled:NO];
}

- (void)touchesCancelled:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event {
  (void)event;
  [self finalizeTouches:touches cancelled:YES];
}

- (void)finalizeTouches:(NSSet<UITouch*>*)touches cancelled:(BOOL)cancelled {
  if (!runtime_model_) {
    return;
  }

  const auto& controls = runtime_model_->layout().controls;
  const CFTimeInterval current_time = CACurrentMediaTime();
  for (UITouch* touch_object in touches) {
    auto capture_it = std::find_if(
        active_captures_.begin(), active_captures_.end(),
        [touch_object](const TouchCaptureState& capture) { return capture.touch == touch_object; });
    if (capture_it == active_captures_.end()) {
      continue;
    }
    const CGPoint release_point = [touch_object locationInView:self];
    capture_it->current_point = release_point;
    if (cancelled) {
      [self clearLookMotionStateForControlIndex:capture_it->control_index];
      // Cancellation must not leave a short B pulse behind.
      const NSUInteger index = capture_it->control_index;
      if (index < controls.size() && controls[index].action == touch::TouchAction::kButtonB &&
          index < recent_action_press_times_.size()) {
        recent_action_press_times_[index] = 0.0;
      }
    }
    if (!cancelled && capture_it->control_index < controls.size() &&
        capture_it->control_index < resolved_control_frames_.size()) {
      const NSUInteger control_index = capture_it->control_index;
      const touch::TouchControlDefinition& control = controls[control_index];
      const touch::TouchRect& frame = resolved_control_frames_[control_index];
      switch (control.type) {
        case touch::TouchControlType::kActionButton: {
          const bool ended_inside = ContainsPoint(control, frame, release_point);
          if (touch::TouchControlUsesDeferredPrimaryTap(control) &&
              control_index < recent_action_press_times_.size()) {
            // Held past the hold time, the release belongs to the hold.
            const touch::TouchInteractionBehaviorState secondary_state =
                touch::ResolveTouchInteractionBehaviorState(
                    control.secondary_behavior, ToInputCapture(*capture_it), current_time);
            if (ended_inside && !secondary_state.active) {
              recent_action_press_times_[control_index] = current_time;
            }
          } else if (ended_inside && !control.hold_while_captured &&
                     control_index < recent_action_press_times_.size()) {
            CFTimeInterval press_time = current_time;
            if (control_index < recent_action_suppressed_until_times_.size()) {
              press_time =
                  std::max(press_time, recent_action_suppressed_until_times_[control_index]);
            }
            recent_action_press_times_[control_index] = press_time;
          }

          const bool quick_tap = (current_time - capture_it->began_time) <=
                                 [self doubleTapWindowSecondsForControl:control];
          if (ended_inside && quick_tap &&
              control.secondary_behavior.trigger == touch::TouchInteractionTrigger::kDoubleTap &&
              touch::TouchInteractionBehaviorConfigured(control.secondary_behavior)) {
            if (![self consumeDoubleTapCandidateForControlIndex:control_index
                                                         atTime:current_time]) {
              [self storeDoubleTapCandidateForControlIndex:control_index atTime:current_time];
            }
          }
        } break;

        case touch::TouchControlType::kMoveStick: {
          // The first quick forward flick arms the double tap; the second one
          // (in touchesMoved, or here) fires it.
          if (control.secondary_behavior.trigger ==
                  touch::TouchInteractionTrigger::kDoubleTapForward &&
              touch::TouchInteractionBehaviorConfigured(control.secondary_behavior) &&
              !capture_it->secondary_behavior_triggered &&
              touch::MoveStickCaptureQualifiesForDoubleTapForward(
                  control, frame, ToInputCapture(*capture_it), current_time)) {
            if (![self consumeDoubleTapCandidateForControlIndex:control_index
                                                         atTime:current_time]) {
              [self storeDoubleTapCandidateForControlIndex:control_index atTime:current_time];
            }
          }
        } break;

        case touch::TouchControlType::kLookSwipeZone:
        default:
          break;
      }
    }
    active_captures_.erase(capture_it);
  }

  [self publishResolvedState];
}

- (void)layoutSubviews {
  [super layoutSubviews];
  if (!runtime_model_) {
    return;
  }

  const CGRect bounds = self.bounds;
  const touch::TouchLayoutSpace space{0.0f, 0.0f,
                                      static_cast<float>(MAX(bounds.size.width, 0.0)),
                                      static_cast<float>(MAX(bounds.size.height, 0.0))};
  const auto& controls = runtime_model_->layout().controls;
  const NSUInteger control_count =
      MIN(control_views_.count, static_cast<NSUInteger>(controls.size()));
  resolved_control_frames_.resize(control_count);
  for (NSUInteger control_index = 0; control_index < control_count; ++control_index) {
    const touch::TouchControlDefinition& control = controls[control_index];
    const touch::TouchRect frame = ResolveControlFrame(control, space);
    resolved_control_frames_[control_index] = frame;
    RexTouchControlShellView* control_view = control_views_[control_index];
    control_view.frame = CGRectIntegral(CGRectFromTouchRect(frame));
    // The full-screen look zone never draws during gameplay.
    const bool is_fullscreen_look = control.type == touch::TouchControlType::kLookSwipeZone &&
                                    control.normalized_frame.width >= 0.95f &&
                                    control.normalized_frame.height >= 0.95f;
    [control_view setChromeSuppressed:is_fullscreen_look];
  }
  [self applyCaptureVisualState];
}

@end

// Keeps the overlay on top of SDL's view and decides when it shows: the
// `touch_controls` setting, hardware controllers coming and going, and
// system dialogs opening and closing.
@interface RexTouchControlsController : NSObject
- (instancetype)initWithWindow:(UIWindow*)window
                         hooks:(touch::TouchControlsHooks)hooks NS_DESIGNATED_INITIALIZER;
- (instancetype)init NS_UNAVAILABLE;
- (void)refresh;
- (void)shutdown;
@end

@implementation RexTouchControlsController {
  __weak UIWindow* window_;
  RexTouchControlsOverlayView* overlay_;
  NSTimer* timer_;
  touch::TouchControlsHooks hooks_;
  id<NSObject> connect_observer_;
  id<NSObject> disconnect_observer_;
  id<NSObject> inactive_observer_;
  id<NSObject> active_observer_;
}

- (instancetype)initWithWindow:(UIWindow*)window hooks:(touch::TouchControlsHooks)hooks {
  if (!(self = [super init])) {
    return nil;
  }
  window_ = window;
  hooks_ = std::move(hooks);
  overlay_ =
      [[RexTouchControlsOverlayView alloc] initWithRuntimeModel:&touch::GetTouchRuntimeModel()];
  overlay_.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;

  __weak RexTouchControlsController* weak_self = self;
  NSNotificationCenter* center = NSNotificationCenter.defaultCenter;
  connect_observer_ = [center addObserverForName:GCControllerDidConnectNotification
                                          object:nil
                                           queue:NSOperationQueue.mainQueue
                                      usingBlock:^(NSNotification* note) {
                                        (void)note;
                                        [weak_self refresh];
                                      }];
  disconnect_observer_ = [center addObserverForName:GCControllerDidDisconnectNotification
                                             object:nil
                                              queue:NSOperationQueue.mainQueue
                                         usingBlock:^(NSNotification* note) {
                                           (void)note;
                                           [weak_self refresh];
                                         }];
  inactive_observer_ = [center addObserverForName:UIApplicationWillResignActiveNotification
      object:nil queue:NSOperationQueue.mainQueue usingBlock:^(NSNotification* note) {
    RexTouchControlsController* controller = weak_self;
    if (controller) [controller->overlay_ setGameplayOverlayVisible:NO animated:NO];
  }];
  active_observer_ = [center addObserverForName:UIApplicationDidBecomeActiveNotification
      object:nil queue:NSOperationQueue.mainQueue usingBlock:^(NSNotification* note) {
    [weak_self refresh];
  }];
  // For what has no notification: SDL replacing its view (a new Metal view
  // would cover the overlay), the setting changing, dialogs opening.
  timer_ = [NSTimer timerWithTimeInterval:0.25
                                  repeats:YES
                                    block:^(NSTimer* timer) {
                                      (void)timer;
                                      [weak_self refresh];
                                    }];
  [NSRunLoop.mainRunLoop addTimer:timer_ forMode:NSRunLoopCommonModes];

  touch::GetTouchRuntimeModel().set_attached(true);
  [self refresh];
  return self;
}

- (void)refresh {
  UIWindow* window = window_;
  UIView* host = window.rootViewController.view;
  if (!host) {
    return;
  }
  if (overlay_.superview != host) {
    [overlay_ removeFromSuperview];
    overlay_.frame = host.bounds;
    [host addSubview:overlay_];
  } else if (host.subviews.lastObject != overlay_) {
    [host bringSubviewToFront:overlay_];
  }

  const std::string mode = REXCVAR_GET(touch_controls);
  const bool active = window.windowScene.activationState == UISceneActivationStateForegroundActive;
  const bool visible = active && (mode == "on" || (mode == "auto" && !HasConnectedGameController()));
  [overlay_ setGameplayOverlayVisible:visible animated:YES];
  const bool system_ui_active = hooks_.is_system_ui_active && hooks_.is_system_ui_active();
  [overlay_ setLookZoneEnabled:!system_ui_active];
}

- (void)shutdown {
  [timer_ invalidate];
  timer_ = nil;
  NSNotificationCenter* center = NSNotificationCenter.defaultCenter;
  if (connect_observer_) {
    [center removeObserver:connect_observer_];
    connect_observer_ = nil;
  }
  if (disconnect_observer_) {
    [center removeObserver:disconnect_observer_];
    disconnect_observer_ = nil;
  }
  if (inactive_observer_) [center removeObserver:inactive_observer_];
  if (active_observer_) [center removeObserver:active_observer_];
  inactive_observer_ = active_observer_ = nil;
  touch::GetTouchRuntimeModel().set_attached(false);
  [overlay_ shutdown];
  [overlay_ removeFromSuperview];
  overlay_ = nil;
}

@end

namespace {
RexTouchControlsController* g_touch_controls = nil;
}  // namespace

namespace rex::input::touch {

void InstallTouchControls(rex::ui::Window* window, TouchControlsHooks hooks) {
  if (g_touch_controls || !window) {
    return;
  }
  // SDL is the only windowing backend on iOS.
  SDL_Window* sdl_window = static_cast<rex::ui::WindowSDL*>(window)->sdl_window();
  if (!sdl_window) {
    return;
  }
  UIWindow* ui_window = (__bridge UIWindow*)SDL_GetPointerProperty(
      SDL_GetWindowProperties(sdl_window), SDL_PROP_WINDOW_UIKIT_WINDOW_POINTER, nullptr);
  if (!ui_window) {
    REXLOG_WARN("Touch controls: SDL window has no UIKit window");
    return;
  }
  g_touch_controls = [[RexTouchControlsController alloc] initWithWindow:ui_window
                                                                  hooks:std::move(hooks)];
  REXLOG_INFO("Touch controls installed ({} controls, touch_controls = {})",
              GetTouchRuntimeModel().layout().controls.size(), REXCVAR_GET(touch_controls));
}

void RemoveTouchControls() {
  [g_touch_controls shutdown];
  g_touch_controls = nil;
}

}  // namespace rex::input::touch
