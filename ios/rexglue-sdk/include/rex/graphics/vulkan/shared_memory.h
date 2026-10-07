#pragma once
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
#include <memory>
#include <utility>
#include <vector>

#include <rex/graphics/shared_memory.h>
#include <rex/memory.h>
#include <rex/ui/vulkan/upload_buffer_pool.h>

namespace rex::graphics::vulkan {

class VulkanCommandProcessor;

class VulkanSharedMemory : public SharedMemory {
 public:
  VulkanSharedMemory(VulkanCommandProcessor& command_processor, memory::Memory& memory,
                     VkPipelineStageFlags guest_shader_pipeline_stages);
  ~VulkanSharedMemory() override;

  bool Initialize();
  void Shutdown(bool from_destructor = false);

  void CompletedSubmissionUpdated();
  void EndSubmission();

  // CPU writes held back until the submission is about to be queued (see
  // UploadRanges): whether there are any, and writing them into the buffer -
  // only once every earlier submission has completed.
  bool HasDeferredUploads() const { return !deferred_pages_.empty(); }
  // Records the held-back copies at the start of the submission's command
  // buffer, before anything recorded in it.
  void RecordDeferredUploads(VkCommandBuffer command_buffer);

  enum class Usage {
    // Index buffer, vfetch, compute read, transfer source.
    kRead,
    // Index buffer, vfetch, memexport.
    kGuestDrawReadWrite,
    kComputeWrite,
    kTransferDestination,
  };
  // Inserts a pipeline barrier for the target usage, also ensuring consecutive
  // read-write accesses are ordered with each other.
  void Use(Usage usage, std::pair<uint32_t, uint32_t> written_range = {});
  // UploadDirtyPages, then back to the usage the next draw had set up.
  bool UploadDirtyPagesKeepingUsage() {
    const Usage usage = last_usage_;
    const std::pair<uint32_t, uint32_t> written_range = last_written_range_;
    const bool uploaded = UploadDirtyPages();
    if (last_usage_ != usage) {
      Use(usage, written_range);
    }
    return uploaded;
  }

  VkBuffer buffer() const { return buffer_; }

 protected:
  bool AllocateSparseHostGpuMemoryRange(uint32_t offset_allocations,
                                        uint32_t length_allocations) override;

  bool UploadRanges(const std::vector<std::pair<uint32_t, uint32_t>>& upload_page_ranges) override;

 private:
  void GetUsageMasks(Usage usage, VkPipelineStageFlags& stage_mask,
                     VkAccessFlags& access_mask) const;

  VulkanCommandProcessor& command_processor_;
  VkPipelineStageFlags guest_shader_pipeline_stages_;

  VkBuffer buffer_ = VK_NULL_HANDLE;
  uint32_t buffer_memory_type_;
  // Persistent mapping of the buffer when it lives in host-visible coherent
  // memory (unified-memory GPUs), for uploads without copy commands.
  uint8_t* buffer_mapping_ = nullptr;
  std::vector<std::pair<uint32_t, uint32_t>> staged_upload_ranges_;
  // Page and its snapshot slot in deferred_data_, and the slot + 1 of each
  // page (0 when none).
  struct DeferredPage {
    uint32_t page;
    VkBuffer buffer;
    VkDeviceSize offset;
    uint8_t* mapping;
  };
  std::vector<DeferredPage> deferred_pages_;
  std::vector<uint32_t> page_deferred_slot_;
  // Single for non-sparse, every allocation so far for sparse.
  std::vector<VkDeviceMemory> buffer_memory_;

  Usage last_usage_;
  std::pair<uint32_t, uint32_t> last_written_range_;

  std::unique_ptr<ui::vulkan::VulkanUploadBufferPool> upload_buffer_pool_;
  std::vector<VkBufferCopy> upload_regions_;
};

}  // namespace rex::graphics::vulkan
