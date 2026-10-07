/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2020 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 *
 * @modified    Tom Clay, 2026 - Adapted for ReXGlue runtime
 */

#include <algorithm>
#include <atomic>
#include <chrono>
#include <array>
#include <cstring>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <rex/assert.h>
#include <rex/audio/conversion.h>
#include <rex/audio/downmix.h>
#include <rex/audio/flags.h>
#include <rex/audio/sdl/sdl_audio_driver.h>
#include <rex/cvar.h>
#include <rex/dbg.h>
#include <rex/logging.h>
#include <rex/development_stats.h>
#include <rex/perf/counter.h>
#include <SDL3/SDL.h>

REXCVAR_DEFINE_BOOL(audio_mute, false, "Audio", "Mute audio output");

namespace rex::audio {
// DEVSTATS (local only), audio_system.cpp
extern std::atomic<uint64_t> g_rex_audio_cb_ns, g_rex_audio_cb_count, g_rex_audio_cb_max_ns;
}  // namespace rex::audio

namespace rex::audio {
extern std::atomic<uint64_t> g_rex_xma_out_bytes;  // DEVSTATS, xma_context.cpp
}

namespace rex::audio::sdl {
// DEVSTATS (local only)
std::atomic<uint32_t> g_rex_audio_peak_milli{0};


namespace {
// DEVSTATS (local only): underruns seen by the SDL callback, reported every 2 s.
struct RexAudioStats {
  uint64_t callbacks = 0, frames = 0, silence = 0, max_request = 0;
  uint64_t min_depth = UINT64_MAX;
  uint64_t last_ns = 0;
} g_rex_audio_stats;
}  // namespace

SDLAudioDriver::SDLAudioDriver(memory::Memory* memory, rex::thread::Semaphore* semaphore)
    : AudioDriver(memory), semaphore_(semaphore) {}

SDLAudioDriver::~SDLAudioDriver() {
  assert_true(frames_queued_.empty());
  assert_true(frames_unused_.empty());
}

bool SDLAudioDriver::Initialize() {
  // Set audio category for proper OS audio handling
  SDL_SetHint(SDL_HINT_AUDIO_CATEGORY, "playback");

  // Set app name for audio device identification
  SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_NAME_STRING, "rexglue");

  if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
    REXAPU_ERROR("SDL_InitSubSystem(SDL_INIT_AUDIO) failed: {}", SDL_GetError());
    return false;
  }
  sdl_initialized_ = true;

  SDL_AudioSpec desired_spec = {};
  SDL_AudioSpec obtained_spec = {};
  desired_spec.freq = frame_frequency_;
  desired_spec.format = SDL_AUDIO_F32LE;
  desired_spec.channels = frame_channels_;
  sdl_device_channels_ = frame_channels_;
  sdl_stream_ = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &desired_spec,
                                          SDLCallback, this);
  if (!sdl_stream_) {
    REXAPU_ERROR("SDL_OpenAudioDeviceStream() failed: {}", SDL_GetError());
    return false;
  }

  SDL_AudioDeviceID sdl_device = SDL_GetAudioStreamDevice(sdl_stream_);
  if (!sdl_device) {
    REXAPU_ERROR("SDL_GetAudioStreamDevice() failed: {}", SDL_GetError());
    return false;
  }

  if (!SDL_GetAudioDeviceFormat(sdl_device, &obtained_spec, NULL)) {
    REXAPU_WARN("SDL_GetAudioDeviceFormat() failed: {}", SDL_GetError());
    obtained_spec = desired_spec;
  }

  // A 1-channel device gets the stereo fold too, then SDL collapses to mono.
  // Handing it a 6ch stream instead would use SDL's own downmix.
  if (obtained_spec.channels <= 2) {
    SDL_DestroyAudioStream(sdl_stream_);
    sdl_stream_ = nullptr;
    desired_spec.channels = 2;
    sdl_device_channels_ = 2;
    sdl_stream_ = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &desired_spec,
                                            SDLCallback, this);
    if (!sdl_stream_) {
      REXAPU_ERROR("SDL_OpenAudioDeviceStream() stereo fallback failed: {}", SDL_GetError());
      return false;
    }
    sdl_device = SDL_GetAudioStreamDevice(sdl_stream_);
    if (!sdl_device) {
      REXAPU_ERROR("SDL_GetAudioStreamDevice() failed after stereo fallback: {}", SDL_GetError());
      return false;
    }
  }

  // The endpoint layout decides which mix the callback runs, and it is the
  // first thing worth knowing when a report says the balance is wrong on one
  // speaker setup and right on another.
  const char* device_name = SDL_GetAudioDeviceName(sdl_device);
  REXAPU_INFO("audio endpoint '{}': {} ch, {} Hz, format 0x{:04X}; submitting {} ch",
              device_name ? device_name : "?", obtained_spec.channels, obtained_spec.freq,
              static_cast<uint32_t>(obtained_spec.format), static_cast<int>(sdl_device_channels_));

  if (!SDL_ResumeAudioDevice(sdl_device)) {
    REXAPU_ERROR("SDL_ResumeAudioDevice() failed: {}", SDL_GetError());
    return false;
  }

  return true;
}

void SDLAudioDriver::SubmitFrame(uint32_t frame_ptr) {
  const auto input_frame = memory_->TranslateVirtual<float*>(frame_ptr);
  float* output_frame;
  {
    std::unique_lock<std::mutex> guard(frames_mutex_);
    if (frames_unused_.empty()) {
      output_frame = new float[frame_samples_];
    } else {
      output_frame = frames_unused_.top();
      frames_unused_.pop();
    }
  }

  std::memcpy(output_frame, input_frame, frame_samples_ * sizeof(float));
  {
    // DEVSTATS (local only): REX_DUMP_AUDIO=<seconds> writes the guest's PCM
    // exactly as submitted (256-sample, 6-channel planar, big-endian float
    // frames) to $HOME/Documents/audio_dump.raw for offline analysis.
    static FILE* rex_dump = nullptr;
    static uint32_t rex_dump_frames_left = 0;
    static bool rex_dump_checked = false;
    if (!rex_dump_checked) {
      rex_dump_checked = true;
      const char* seconds = std::getenv("REX_DUMP_AUDIO");
      const char* home = std::getenv("HOME");
      if (seconds && std::atoi(seconds) > 0 && home) {
        rex_dump_frames_left = uint32_t(std::atoi(seconds)) * 48000u / channel_samples_;
        rex_dump = std::fopen((std::string(home) + "/Documents/audio_dump.raw").c_str(), "wb");
      }
    }
    if (rex_dump && rex_dump_frames_left) {
      std::fwrite(output_frame, sizeof(float), frame_samples_, rex_dump);
      if (--rex_dump_frames_left == 0) {
        std::fclose(rex_dump);
        rex_dump = nullptr;
      }
    }
  }
  if (rex::AudioDevelopmentStatsEnabled()) {
    // DEVSTATS (local only): loudest sample the guest submitted, to tell real
    // sound from digital silence. Guest floats are big-endian.
    const uint32_t* raw = reinterpret_cast<const uint32_t*>(output_frame);
    float peak = 0.0f;
    for (size_t i = 0; i < frame_samples_; ++i) {
      const uint32_t v = __builtin_bswap32(raw[i]);
      float f;
      std::memcpy(&f, &v, sizeof(f));
      peak = std::max(peak, std::fabs(f));
    }
    const uint32_t milli = uint32_t(std::min(peak, 1000.0f) * 1000.0f);
    uint32_t prev = g_rex_audio_peak_milli.load(std::memory_order_relaxed);
    while (milli > prev &&
           !g_rex_audio_peak_milli.compare_exchange_weak(prev, milli, std::memory_order_relaxed)) {
    }
  }

  static uint32_t sdl_submit_count = 0;
  if (sdl_submit_count < 10) {
    REXAPU_DEBUG("SDLAudioDriver::SubmitFrame: frame_ptr={:08X} queued_count={}", frame_ptr,
                 frames_queued_.size() + 1);
    sdl_submit_count++;
  }

  {
    std::unique_lock<std::mutex> guard(frames_mutex_);
    frames_queued_.push(output_frame);
    PROFILE_BUFFER_QUEUE_DEPTH(static_cast<int64_t>(frames_queued_.size()));
  }
}

void SDLAudioDriver::Shutdown() {
  if (sdl_stream_) {
    SDL_DestroyAudioStream(sdl_stream_);
    sdl_stream_ = nullptr;
  }
  if (sdl_initialized_) {
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    sdl_initialized_ = false;
  }
  std::unique_lock<std::mutex> guard(frames_mutex_);
  while (!frames_unused_.empty()) {
    delete[] frames_unused_.top();
    frames_unused_.pop();
  }
  while (!frames_queued_.empty()) {
    delete[] frames_queued_.front();
    frames_queued_.pop();
  }
}

void SDLAudioDriver::SDLCallback(void* userdata, SDL_AudioStream* stream, int additional_amount,
                                 [[maybe_unused]] int total_amount) {
  SCOPE_profile_cpu_f("apu");
  if (!userdata || !stream) {
    REXAPU_ERROR("SDLAudioDriver::SDLCallback called with nullptr.");
    return;
  }
  const auto driver = static_cast<SDLAudioDriver*>(userdata);
  const bool stats_enabled = rex::AudioDevelopmentStatsEnabled();
  const int sample_count =
      static_cast<int>(channel_samples_ * std::max<uint8_t>(driver->sdl_device_channels_, 1));
  const int len = static_cast<int>(sizeof(float) * sample_count);
  float* data = SDL_stack_alloc(float, sample_count);
  if (!data) {
    REXAPU_ERROR("SDLAudioDriver::SDLCallback failed to allocate {} samples", sample_count);
    return;
  }
  // Snapshot once. A change mid-callback would split the frame across two mixes.
  const StereoFold fold = GetStereoFold();
  const SurroundMix mix = GetSurroundMix();
  const float gain = GetOutputGain();
  if (stats_enabled) {
    // DEVSTATS (local only)
    std::unique_lock<std::mutex> guard(driver->frames_mutex_);
    g_rex_audio_stats.callbacks++;
    g_rex_audio_stats.min_depth =
        std::min<uint64_t>(g_rex_audio_stats.min_depth, driver->frames_queued_.size());
    g_rex_audio_stats.max_request =
        std::max<uint64_t>(g_rex_audio_stats.max_request, uint64_t(std::max(additional_amount, 0)));
  }
  while (additional_amount > 0) {
    static uint32_t sdl_callback_count = 0;
    std::unique_lock<std::mutex> guard(driver->frames_mutex_);
    if (driver->frames_queued_.empty()) {
      if (stats_enabled) ++g_rex_audio_stats.silence;
      if (sdl_callback_count < 10) {
        REXAPU_DEBUG("SDLCallback: no frames queued (silence)");
        sdl_callback_count++;
      }
      std::memset(data, 0, len);
      if (!SDL_PutAudioStreamData(stream, data, len)) {
        REXAPU_ERROR("SDL_PutAudioStreamData() failed while filling silence: {}", SDL_GetError());
        break;
      }
      additional_amount -= len;
    } else {
      auto buffer = driver->frames_queued_.front();
      driver->frames_queued_.pop();
      if (stats_enabled) ++g_rex_audio_stats.frames;
      if (REXCVAR_GET(audio_mute)) {
        std::memset(data, 0, len);
      } else {
        switch (driver->sdl_device_channels_) {
          case 2:
            conversion::sequential_6_BE_to_interleaved_2_LE(data, buffer, channel_samples_, fold,
                                                            gain);
            break;
          case 6:
            conversion::sequential_6_BE_to_interleaved_6_LE(data, buffer, channel_samples_, mix,
                                                            gain);
            break;
          default:
            assert_unhandled_case(driver->sdl_device_channels_);
            break;
        }
      }
      if (!SDL_PutAudioStreamData(stream, data, len)) {
        REXAPU_ERROR("SDL_PutAudioStreamData() failed: {}", SDL_GetError());
        driver->frames_unused_.push(buffer);
        break;
      }
      driver->frames_unused_.push(buffer);

      auto ret = driver->semaphore_->Release(1, nullptr);
      assert_true(ret);
      additional_amount -= len;
    }
  }
  SDL_stack_free(data);
  if (stats_enabled) {
    // DEVSTATS (local only)
    const uint64_t rex_now = uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(
                                          std::chrono::steady_clock::now().time_since_epoch())
                                          .count());
    RexAudioStats& st = g_rex_audio_stats;
    if (!st.last_ns) {
      st.last_ns = rex_now;
    } else if (rex_now - st.last_ns >= 2000000000ull) {
      const double secs = double(rex_now - st.last_ns) / 1e9;
      const uint64_t cb_count = g_rex_audio_cb_count.exchange(0);
      const uint64_t cb_ns = g_rex_audio_cb_ns.exchange(0);
      const uint64_t cb_max = g_rex_audio_cb_max_ns.exchange(0);
      REXAPU_INFO("PERF audio level: guest peak {:.3f} | XMA decoded {:.0f} KB/s",
                  double(g_rex_audio_peak_milli.exchange(0)) / 1000.0,
                  double(rex::audio::g_rex_xma_out_bytes.exchange(0)) / 1024.0 / secs);
      REXAPU_INFO(
          "PERF audio: {:.1f} callbacks/s | {:.1f} frames/s | silence {} frames | min queue {} | "
          "max request {} B | guest callback {:.1f}/s avg {:.2f} ms max {:.2f} ms",
          double(st.callbacks) / secs, double(st.frames) / secs, st.silence,
          st.min_depth == UINT64_MAX ? 0 : st.min_depth, st.max_request, double(cb_count) / secs,
          cb_count ? double(cb_ns) / 1e6 / double(cb_count) : 0.0, double(cb_max) / 1e6);
      st = RexAudioStats{};
      st.last_ns = rex_now;
    }
  }
}

}  // namespace rex::audio::sdl
