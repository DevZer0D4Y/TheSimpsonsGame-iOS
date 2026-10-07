// Unattended testing: a scripted gamepad and periodic screenshots. Both are off
// unless the app is started with the environment variables below (on a phone:
// xcrun devicectl device process launch -e '{"SIMPSONS_AUTOPLAY":"..."}' ...).
//
// SIMPSONS_AUTOPLAY  comma-separated presses, times in seconds from app start:
//                      T:INPUT[:DURATION]        press INPUT at T for DURATION (0.15 s)
//                      T1-T2/P:INPUT[:DURATION]  press it every P seconds from T1 to T2
//                    INPUT is a button (A B X Y START BACK UP DOWN LEFT RIGHT LB RB
//                    LS RS), a trigger (LT RT) or a stick direction (LX+ LX- LY+ LY-
//                    RX+ RX- RY+ RY-, full deflection).
// SIMPSONS_SHOTS     seconds between captures of the guest image, written as
//                    <user data>/shots/shot_<seconds>.ppm.
//
// Adapted from the NFSMW port's autoplay driver.

#pragma once

#include <rex/input/device_assignment.h>
#include <rex/input/input_driver.h>
#include <rex/input/input_system.h>
#include <rex/logging.h>
#include <rex/ui/presenter.h>

#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace simpsons {

using namespace rex;         // X_STATUS, X_RESULT
using namespace rex::input;  // X_INPUT_*

class AutoplayInputDriver final : public rex::input::InputDriver {
 public:
  struct Step {
    double start = 0, end = 0, period = 0, duration = 0.15;
    uint16_t buttons = 0;
    bool lt = false, rt = false;
    int axis = -1;  // 0 lx, 1 ly, 2 rx, 3 ry
    int16_t value = 0;
  };

  static std::vector<Step> Parse(const char* script) {
    std::vector<Step> steps;
    std::string all(script);
    size_t pos = 0;
    while (pos <= all.size()) {
      size_t comma = all.find(',', pos);
      std::string item =
          all.substr(pos, comma == std::string::npos ? std::string::npos : comma - pos);
      pos = comma == std::string::npos ? all.size() + 1 : comma + 1;
      size_t colon = item.find(':');
      if (item.empty() || colon == std::string::npos) {
        continue;
      }
      Step step;
      std::string when = item.substr(0, colon);
      std::string input = item.substr(colon + 1);
      size_t colon2 = input.find(':');
      if (colon2 != std::string::npos) {
        step.duration = std::atof(input.c_str() + colon2 + 1);
        input = input.substr(0, colon2);
      }
      size_t dash = when.find('-');
      if (dash != std::string::npos) {
        size_t slash = when.find('/');
        step.start = std::atof(when.substr(0, dash).c_str());
        step.end = std::atof(when.substr(dash + 1, slash - dash - 1).c_str());
        step.period = slash != std::string::npos ? std::atof(when.c_str() + slash + 1) : 1.0;
      } else {
        step.start = step.end = std::atof(when.c_str());
      }
      if (!ApplyInput(input, step)) {
        REXLOG_WARN("[autoplay] unknown input '{}'", input);
        continue;
      }
      steps.push_back(step);
    }
    return steps;
  }

  explicit AutoplayInputDriver(std::vector<Step> steps)
      : InputDriver(nullptr, 0), steps_(std::move(steps)), start_(Clock::now()) {}

  X_STATUS Setup() override { return X_STATUS_SUCCESS; }

  void EnumerateDevices(std::vector<rex::input::DeviceInfo>& out) override {
    rex::input::DeviceInfo info;
    info.id = kDevice;
    info.name = "Autoplay";
    info.synthetic = true;
    out.push_back(info);
  }

  X_RESULT GetDeviceState(rex::input::DeviceId id, X_INPUT_STATE* out_state) override {
    if (id != kDevice) {
      return X_ERROR_DEVICE_NOT_CONNECTED;
    }
    const double now = std::chrono::duration<double>(Clock::now() - start_).count();
    uint16_t buttons = 0;
    uint8_t lt = 0, rt = 0;
    int16_t axes[4] = {0, 0, 0, 0};
    for (const Step& s : steps_) {
      if (!Active(s, now)) {
        continue;
      }
      buttons |= s.buttons;
      lt = s.lt ? 255 : lt;
      rt = s.rt ? 255 : rt;
      if (s.axis >= 0) {
        axes[s.axis] = s.value;
      }
    }
    if (out_state) {
      std::lock_guard<std::mutex> lock(mutex_);
      const uint64_t signature = uint64_t(buttons) | (uint64_t(lt) << 16) |
                                 (uint64_t(rt) << 24) | (uint64_t(uint16_t(axes[0])) << 32) |
                                 (uint64_t(uint16_t(axes[1])) << 48);
      if (signature != last_signature_) {
        last_signature_ = signature;
        ++packet_;
      }
      std::memset(out_state, 0, sizeof(*out_state));
      out_state->packet_number = packet_;
      out_state->gamepad.buttons = buttons;
      out_state->gamepad.left_trigger = lt;
      out_state->gamepad.right_trigger = rt;
      out_state->gamepad.thumb_lx = axes[0];
      out_state->gamepad.thumb_ly = axes[1];
      out_state->gamepad.thumb_rx = axes[2];
      out_state->gamepad.thumb_ry = axes[3];
    }
    return X_ERROR_SUCCESS;
  }

  X_RESULT GetDeviceCapabilities(rex::input::DeviceId id, uint32_t flags,
                                 X_INPUT_CAPABILITIES* out_caps) override {
    (void)flags;
    if (id != kDevice) {
      return X_ERROR_DEVICE_NOT_CONNECTED;
    }
    if (out_caps) {
      std::memset(out_caps, 0, sizeof(*out_caps));
      out_caps->type = 0x01;      // XINPUT_DEVTYPE_GAMEPAD
      out_caps->sub_type = 0x01;  // XINPUT_DEVSUBTYPE_GAMEPAD
      out_caps->gamepad.buttons = 0xF3FF;
      out_caps->gamepad.left_trigger = 0xFF;
      out_caps->gamepad.right_trigger = 0xFF;
      out_caps->gamepad.thumb_lx = int16_t(0xFFC0);
      out_caps->gamepad.thumb_ly = int16_t(0xFFC0);
      out_caps->gamepad.thumb_rx = int16_t(0xFFC0);
      out_caps->gamepad.thumb_ry = int16_t(0xFFC0);
    }
    return X_ERROR_SUCCESS;
  }

  X_RESULT SetDeviceVibration(rex::input::DeviceId id, X_INPUT_VIBRATION* vibration) override {
    (void)vibration;
    return id == kDevice ? X_ERROR_SUCCESS : X_ERROR_DEVICE_NOT_CONNECTED;
  }

  X_RESULT GetDeviceKeystroke(rex::input::DeviceId id, uint32_t flags,
                              X_INPUT_KEYSTROKE* out_keystroke) override {
    (void)flags;
    (void)out_keystroke;
    return id == kDevice ? X_ERROR_EMPTY : X_ERROR_DEVICE_NOT_CONNECTED;
  }

  // Adds the driver when SIMPSONS_AUTOPLAY is set. Every device then feeds
  // guest user 0, so the script drives whichever controller the game reads.
  static void InstallFromEnvironment(rex::system::IInputSystem* input_system) {
    const char* script = std::getenv("SIMPSONS_AUTOPLAY");
    if (!script || !script[0] || !input_system) {
      return;
    }
    auto steps = Parse(script);
    REXLOG_INFO("[autoplay] {} steps from SIMPSONS_AUTOPLAY", steps.size());
    auto* input = static_cast<rex::input::InputSystem*>(input_system);
    input->AddDriver(std::make_unique<AutoplayInputDriver>(std::move(steps)));
    input->SetDeviceAssignment(std::make_unique<rex::input::SharedAssignment>());
  }

 private:
  using Clock = std::chrono::steady_clock;
  static constexpr rex::input::DeviceId kDevice = static_cast<rex::input::DeviceId>(0x4155544F);

  static bool Active(const Step& s, double now) {
    if (now < s.start) {
      return false;
    }
    if (s.period <= 0 || s.end <= s.start) {
      return now < s.start + s.duration;
    }
    if (now > s.end + s.duration) {
      return false;
    }
    return std::fmod(now - s.start, s.period) < s.duration;
  }

  static bool ApplyInput(const std::string& name, Step& step) {
    struct Named {
      const char* name;
      uint16_t bit;
    };
    static const Named kButtons[] = {
        {"A", 0x1000},     {"B", 0x2000},     {"X", 0x4000},  {"Y", 0x8000},
        {"START", 0x0010}, {"BACK", 0x0020},  {"UP", 0x0001}, {"DOWN", 0x0002},
        {"LEFT", 0x0004},  {"RIGHT", 0x0008}, {"LB", 0x0100}, {"RB", 0x0200},
        {"LS", 0x0040},    {"RS", 0x0080},
    };
    for (const Named& b : kButtons) {
      if (name == b.name) {
        step.buttons = b.bit;
        return true;
      }
    }
    if (name == "LT" || name == "RT") {
      (name == "LT" ? step.lt : step.rt) = true;
      return true;
    }
    static const char* kAxes[] = {"LX", "LY", "RX", "RY"};
    for (int i = 0; i < 4; ++i) {
      if (name.size() == 3 && name.compare(0, 2, kAxes[i]) == 0) {
        step.axis = i;
        step.value = name[2] == '-' ? -32767 : 32767;
        return true;
      }
    }
    return false;
  }

  std::vector<Step> steps_;
  Clock::time_point start_;
  std::mutex mutex_;
  uint64_t last_signature_ = ~uint64_t(0);
  uint32_t packet_ = 0;
};

// Saves the guest image every SIMPSONS_SHOTS seconds while the app runs.
class ShotTaker {
 public:
  ~ShotTaker() { Stop(); }

  void StartFromEnvironment(rex::ui::Presenter* presenter, const std::filesystem::path& dir) {
    const char* env = std::getenv("SIMPSONS_SHOTS");
    const double interval = env ? std::atof(env) : 0.0;
    if (interval <= 0.0 || !presenter) {
      return;
    }
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    running_ = true;
    thread_ = std::thread([this, presenter, dir, interval] {
      const auto start = std::chrono::steady_clock::now();
      int index = 0;
      while (running_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        const double t =
            std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        if (t < interval * (index + 1)) {
          continue;
        }
        ++index;
        rex::ui::RawImage image;
        if (!presenter->CaptureGuestOutput(image) || !image.width || !image.height) {
          continue;
        }
        char name[64];
        std::snprintf(name, sizeof(name), "shot_%04d.ppm", int(t));
        if (FILE* f = std::fopen((dir / name).string().c_str(), "wb")) {
          std::fprintf(f, "P6\n%u %u\n255\n", image.width, image.height);
          std::vector<uint8_t> row(size_t(image.width) * 3);
          for (uint32_t y = 0; y < image.height; ++y) {
            const uint8_t* src = image.data.data() + image.stride * y;
            for (uint32_t x = 0; x < image.width; ++x) {
              std::memcpy(&row[x * 3], src + x * 4, 3);
            }
            std::fwrite(row.data(), 1, row.size(), f);
          }
          std::fclose(f);
          REXLOG_INFO("[shots] {} ({}x{})", name, image.width, image.height);
        }
      }
    });
  }

  void Stop() {
    running_ = false;
    if (thread_.joinable()) {
      thread_.join();
    }
  }

 private:
  std::atomic<bool> running_{false};
  std::thread thread_;
};

}  // namespace simpsons
