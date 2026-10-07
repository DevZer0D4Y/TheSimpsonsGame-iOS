#pragma once
#include <cstdlib>

namespace rex {
inline bool GpuDevelopmentStatsEnabled() {
  static const bool enabled = std::getenv("REX_GPU_STATS") ||
                              std::getenv("REX_GPU_PROFILE") ||
                              std::getenv("REX_FRAME_DUMP");
  return enabled;
}
inline bool AudioDevelopmentStatsEnabled() {
  static const bool enabled = std::getenv("REX_AUDIO_STATS") ||
                              GpuDevelopmentStatsEnabled();
  return enabled;
}
}  // namespace rex
