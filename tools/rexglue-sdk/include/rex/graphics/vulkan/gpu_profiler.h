#pragma once
/**
 * @file        gpu_profiler.h
 * @brief       Per-frame GPU time attribution by work category (Vulkan).
 */

#include <array>
#include <cstdint>
#include <vector>

#include <rex/ui/vulkan/api.h>

namespace rex::ui::vulkan {
class VulkanDevice;
}

namespace rex::graphics::vulkan {

class DeferredCommandBuffer;

// Timestamps written into the frame's command stream split the GPU timeline
// into consecutive intervals, each attributed to the category named by the
// mark that starts it. Summed over a frame this answers where GPU time goes
// (draw passes vs EDRAM transfers vs resolves vs texture loads vs the swap),
// which is what decides which part of the renderer is worth replacing.
class VulkanGpuProfiler {
 public:
  enum class Category : uint8_t {
    kDraw,
    kTransfer,
    kResolve,
    kTextureLoad,
    kNativeResolve,
    kSwap,
    kOther,
    kCount,
  };

  static constexpr uint32_t kMaxMarksPerFrame = 1024;

  bool Initialize(const ui::vulkan::VulkanDevice* vulkan_device, uint32_t frames_in_flight);
  void Shutdown();

  bool enabled() const { return query_pool_ != VK_NULL_HANDLE; }

  // Records the query reset and the first mark of a frame. Must be the first
  // profiler call in the frame's first submission.
  void BeginFrame(DeferredCommandBuffer& command_buffer, uint64_t frame_index);
  // Ends the current interval and starts one attributed to `next`.
  void Mark(DeferredCommandBuffer& command_buffer, Category next);
  // Final mark of the frame, before the swap submission ends.
  void EndFrame(DeferredCommandBuffer& command_buffer);
  // Called once the frame's submissions have completed on the GPU.
  void FrameCompleted(uint64_t frame_index);

  static const char* CategoryName(Category category);

 private:
  struct FrameSlot {
    uint64_t frame_index = UINT64_MAX;
    uint32_t mark_count = 0;
    std::array<Category, kMaxMarksPerFrame> categories;
  };

  const ui::vulkan::VulkanDevice* vulkan_device_ = nullptr;
  VkQueryPool query_pool_ = VK_NULL_HANDLE;
  PFN_vkGetQueryPoolResults get_query_pool_results_ = nullptr;
  float timestamp_period_ns_ = 1.0f;
  uint32_t frames_in_flight_ = 0;
  std::vector<FrameSlot> slots_;
  FrameSlot* current_ = nullptr;
  uint32_t current_slot_index_ = 0;

  // Accumulated over the logging interval.
  uint32_t log_interval_frames_ = 0;
  uint32_t accumulated_frames_ = 0;
  double accumulated_total_ms_ = 0.0;
  std::array<double, size_t(Category::kCount)> accumulated_ms_{};
  std::array<uint32_t, size_t(Category::kCount)> accumulated_marks_{};
  std::vector<uint64_t> results_;
};

}  // namespace rex::graphics::vulkan
