#pragma once
#include <rex/development_stats.h>

namespace rex::graphics::vulkan {
inline bool DevelopmentStatsEnabled() {
  return rex::GpuDevelopmentStatsEnabled();
}
}  // namespace rex::graphics::vulkan
