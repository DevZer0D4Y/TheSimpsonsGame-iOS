#include <rex/input/input.h>
#include <rex/input/touch/touch_input_driver.h>
#include <rex/input/touch/touch_input_resolver.h>
#include <rex/input/touch/touch_layout.h>

#include <algorithm>
#include <cassert>
#include <iostream>

int main() {
  namespace touch = rex::input::touch;
  using rex::X_RESULT;
  const auto layout = touch::CreateDefaultTouchLayout();
  const auto button = std::find_if(layout.controls.begin(), layout.controls.end(),
                                  [](const auto& c) { return c.identifier == "b_button"; });
  assert(button != layout.controls.end());
  // The old layout deferred B until release and substituted R3 on a hold.
  assert(!touch::TouchControlUsesDeferredPrimaryTap(*button));
  assert(button->hold_while_captured);
  assert(!touch::TouchInteractionBehaviorConfigured(button->secondary_behavior));
  assert(button->drag_output == touch::TouchAnalogOutput::kNone);

  touch::TouchResolvedState held;
  held.gameplay_enabled = true;
  held.packet_number = 1;
  touch::ApplyTouchActionMapping(*button, &held);
  assert(held.buttons == rex::input::X_INPUT_GAMEPAD_B);
  assert(held.left_trigger == 0 && held.right_trigger == 0);

  // Exercise the guest input driver: B remains down across repeated polling,
  // concurrent movement/A, and then releases without a right-stick click.
  auto& model = touch::GetTouchRuntimeModel();
  model.set_attached(true);
  model.StoreResolvedState(held);
  touch::TouchInputDriver driver(nullptr, 0);
  std::vector<rex::input::DeviceInfo> devices;
  driver.EnumerateDevices(devices);
  assert(devices.size() == 1);
  rex::input::X_INPUT_STATE guest{};
  for (int poll = 0; poll < 1000; ++poll) {
    assert(driver.GetDeviceState(devices[0].id, &guest) == X_ERROR_SUCCESS);
    assert(uint16_t(guest.gamepad.buttons) == rex::input::X_INPUT_GAMEPAD_B);
    assert(uint32_t(guest.packet_number) == 1);
  }
  held.buttons |= rex::input::X_INPUT_GAMEPAD_A;
  held.thumb_lx = 16000;
  held.packet_number = 2;
  model.StoreResolvedState(held);
  assert(driver.GetDeviceState(devices[0].id, &guest) == X_ERROR_SUCCESS);
  assert(uint16_t(guest.gamepad.buttons) ==
         (rex::input::X_INPUT_GAMEPAD_B | rex::input::X_INPUT_GAMEPAD_A));
  assert(int16_t(guest.gamepad.thumb_lx) == 16000);
  held.buttons &= ~rex::input::X_INPUT_GAMEPAD_B;
  held.packet_number = 3;
  model.StoreResolvedState(held);
  assert(driver.GetDeviceState(devices[0].id, &guest) == X_ERROR_SUCCESS);
  assert(uint16_t(guest.gamepad.buttons) == rex::input::X_INPUT_GAMEPAD_A);
  assert(int16_t(guest.gamepad.thumb_lx) == 16000);
  model.StoreResolvedState({});
  assert(driver.GetDeviceState(devices[0].id, &guest) ==
         X_ERROR_DEVICE_NOT_CONNECTED);
  model.set_attached(false);

  for (const auto& control : layout.controls) {
    if (control.identifier == "a_button") {
      assert(control.mapped_buttons == rex::input::X_INPUT_GAMEPAD_A);
    } else if (control.identifier == "left_trigger_button") {
      assert(control.mapped_left_trigger == 255 && control.hold_while_captured);
    } else if (control.identifier == "right_trigger_button") {
      assert(control.mapped_right_trigger == 255 && control.hold_while_captured);
    }
  }
  std::cout << "PASS: immediate held-B layout, 1000 guest polls, movement/A coexistence, release and inactive input\n";
}
