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

#include <atomic>
#include <algorithm>
#include <cstring>
#include <utility>
#include <vector>

#include <rex/assert.h>
#include <rex/cvar.h>
#include <rex/graphics/vulkan/command_processor.h>
#include <rex/graphics/vulkan/deferred_command_buffer.h>
#include <rex/graphics/vulkan/shared_memory.h>
#include <rex/graphics/vulkan/upload_policy.h>
#include <rex/graphics/vulkan/development_stats.h>
#include <rex/logging.h>
#include <rex/math.h>
#include <rex/ui/vulkan/util.h>

REXCVAR_DEFINE_BOOL(vulkan_sparse_shared_memory, true, "GPU/Vulkan",
                    "Use sparse shared memory on Vulkan")
    .lifecycle(rex::cvar::Lifecycle::kRequiresRestart);

namespace rex::graphics::vulkan {
extern std::atomic<uint64_t> g_rex_perf_upload_bytes;  // DEVSTATS, command_processor.cpp
extern std::atomic<uint64_t> g_rex_upload_region_bytes[512];  // DEVSTATS census, command_processor.cpp
extern std::atomic<uint64_t> g_rex_upload_calls, g_rex_upload_ranges;  // DEVSTATS
extern std::atomic<uint64_t> g_rex_direct_upload_bytes;  // DEVSTATS
extern std::atomic<uint64_t> g_rex_thumbsrc_uploads, g_rex_thumbsrc_upload_bytes;  // DEVSTATS

VulkanSharedMemory::VulkanSharedMemory(VulkanCommandProcessor& command_processor,
                                       memory::Memory& memory,
                                       VkPipelineStageFlags guest_shader_pipeline_stages)
    : SharedMemory(memory),
      command_processor_(command_processor),
      guest_shader_pipeline_stages_(guest_shader_pipeline_stages) {}

VulkanSharedMemory::~VulkanSharedMemory() {
  Shutdown(true);
}

REXCVAR_DEFINE_BOOL(vulkan_shared_memory_direct_upload, false, "GPU/Vulkan",
                    "On unified-memory GPUs, keep the guest memory buffer in host-visible "
                    "memory and write CPU-modified pages straight into it when no recorded or "
                    "in-flight GPU work reads them, instead of recording a copy - each copy "
                    "ends the render pass, which tile-based GPUs pay for heavily.")
    .lifecycle(rex::cvar::Lifecycle::kInitOnly);

void VulkanSharedMemory::RecordDeferredUploads(VkCommandBuffer command_buffer) {
  if (deferred_pages_.empty()) {
    return;
  }
  const ui::vulkan::VulkanDevice::Functions& dfn = command_processor_.GetVulkanDevice()->functions();
  const VkDeviceSize page_size = VkDeviceSize(1) << page_size_log2();
  VkBufferMemoryBarrier barrier = {};
  barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.buffer = buffer_;
  barrier.offset = 0;
  barrier.size = VK_WHOLE_SIZE;
  barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
  barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  dfn.vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                           VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 1, &barrier, 0, nullptr);
  upload_regions_.clear();
  VkBuffer source = deferred_pages_.front().buffer;
  for (const DeferredPage& deferred : deferred_pages_) {
    if (deferred.buffer != source) {
      dfn.vkCmdCopyBuffer(command_buffer, source, buffer_, uint32_t(upload_regions_.size()),
                          upload_regions_.data());
      upload_regions_.clear();
      source = deferred.buffer;
    }
    VkBufferCopy& region = upload_regions_.emplace_back();
    region.srcOffset = deferred.offset;
    region.dstOffset = VkDeviceSize(deferred.page) << page_size_log2();
    region.size = page_size;
    page_deferred_slot_[deferred.page] = 0;
    MarkPagesStagedCopy(deferred.page, 1);
  }
  dfn.vkCmdCopyBuffer(command_buffer, source, buffer_, uint32_t(upload_regions_.size()),
                      upload_regions_.data());
  upload_regions_.clear();
  barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
  dfn.vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                           VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 1, &barrier, 0,
                           nullptr);
  deferred_pages_.clear();
}

REXCVAR_DEFINE_BOOL(vulkan_defer_cpu_uploads, false, "GPU/Vulkan",
                    "With direct uploads, hold back CPU writes to pages earlier submissions still "
                    "read and write them right before the submission is queued, instead of a "
                    "copy command that ends the render pass.")
    .lifecycle(rex::cvar::Lifecycle::kHotReload);

bool VulkanSharedMemory::Initialize() {
  InitializeCommon();

  const ui::vulkan::VulkanDevice* const vulkan_device = command_processor_.GetVulkanDevice();
  const ui::vulkan::VulkanDevice::Functions& dfn = vulkan_device->functions();
  const VkDevice device = vulkan_device->device();

  const VkBufferCreateFlags sparse_flags =
      VK_BUFFER_CREATE_SPARSE_BINDING_BIT | VK_BUFFER_CREATE_SPARSE_RESIDENCY_BIT;

  // Try to create a sparse buffer.
  VkBufferCreateInfo buffer_create_info;
  buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  buffer_create_info.pNext = nullptr;
  buffer_create_info.flags = sparse_flags;
  buffer_create_info.size = kBufferSize;
  buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                             VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
  buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  buffer_create_info.queueFamilyIndexCount = 0;
  buffer_create_info.pQueueFamilyIndices = nullptr;
  if (REXCVAR_GET(vulkan_sparse_shared_memory) &&
      vulkan_device->properties().sparseResidencyBuffer) {
    if (dfn.vkCreateBuffer(device, &buffer_create_info, nullptr, &buffer_) == VK_SUCCESS) {
      VkMemoryRequirements buffer_memory_requirements;
      dfn.vkGetBufferMemoryRequirements(device, buffer_, &buffer_memory_requirements);
      if (rex::bit_scan_forward(buffer_memory_requirements.memoryTypeBits &
                                    vulkan_device->memory_types().device_local,
                                &buffer_memory_type_)) {
        uint32_t allocation_size_log2;
        rex::bit_scan_forward(std::max(uint64_t(buffer_memory_requirements.alignment), uint64_t(1)),
                              &allocation_size_log2);
        if (allocation_size_log2 < kBufferSizeLog2) {
          // Maximum of 1024 allocations in the worst case for all of the
          // buffer because of the overall 4096 allocation count limit on
          // Windows drivers.
          InitializeSparseHostGpuMemory(std::max(
              allocation_size_log2,
              std::max(kHostGpuMemoryOptimalSparseAllocationLog2, kBufferSizeLog2 - uint32_t(10))));
        } else {
          // Shouldn't happen on any real platform, but no point allocating the
          // buffer sparsely.
          dfn.vkDestroyBuffer(device, buffer_, nullptr);
          buffer_ = VK_NULL_HANDLE;
        }
      } else {
        REXGPU_ERROR(
            "Shared memory: Failed to get a device-local Vulkan memory type "
            "for the sparse buffer");
        dfn.vkDestroyBuffer(device, buffer_, nullptr);
        buffer_ = VK_NULL_HANDLE;
      }
    } else {
      REXGPU_ERROR("Shared memory: Failed to create the {} MB Vulkan sparse buffer",
                   kBufferSize >> 20);
    }
  }

  // Create a non-sparse buffer if there were issues with the sparse buffer.
  if (buffer_ == VK_NULL_HANDLE) {
    REXGPU_INFO(
        "Vulkan sparse binding is not used for shared memory emulation - video "
        "memory usage may increase significantly because a full {} MB buffer "
        "will be created",
        kBufferSize >> 20);
    buffer_create_info.flags &= ~sparse_flags;
    if (dfn.vkCreateBuffer(device, &buffer_create_info, nullptr, &buffer_) != VK_SUCCESS) {
      REXGPU_ERROR("Shared memory: Failed to create the {} MB Vulkan buffer", kBufferSize >> 20);
      Shutdown();
      return false;
    }
    VkMemoryRequirements buffer_memory_requirements;
    dfn.vkGetBufferMemoryRequirements(device, buffer_, &buffer_memory_requirements);
    const auto& memory_types = vulkan_device->memory_types();
    const uint32_t mappable_device_local = buffer_memory_requirements.memoryTypeBits &
                                           memory_types.device_local & memory_types.host_visible &
                                           memory_types.host_coherent;
    const bool direct_upload =
        REXCVAR_GET(vulkan_shared_memory_direct_upload) && mappable_device_local;
    if (direct_upload) {
      rex::bit_scan_forward(mappable_device_local, &buffer_memory_type_);
    } else if (!rex::bit_scan_forward(
            buffer_memory_requirements.memoryTypeBits & vulkan_device->memory_types().device_local,
            &buffer_memory_type_)) {
      REXGPU_ERROR(
          "Shared memory: Failed to get a device-local Vulkan memory type for "
          "the buffer");
      Shutdown();
      return false;
    }
    VkMemoryAllocateInfo buffer_memory_allocate_info;
    VkMemoryAllocateInfo* buffer_memory_allocate_info_last = &buffer_memory_allocate_info;
    buffer_memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    buffer_memory_allocate_info.pNext = nullptr;
    buffer_memory_allocate_info.allocationSize = buffer_memory_requirements.size;
    buffer_memory_allocate_info.memoryTypeIndex = buffer_memory_type_;
    VkMemoryDedicatedAllocateInfo buffer_memory_dedicated_allocate_info;
    if (vulkan_device->extensions().ext_1_1_KHR_dedicated_allocation) {
      buffer_memory_allocate_info_last->pNext = &buffer_memory_dedicated_allocate_info;
      buffer_memory_allocate_info_last =
          reinterpret_cast<VkMemoryAllocateInfo*>(&buffer_memory_dedicated_allocate_info);
      buffer_memory_dedicated_allocate_info.sType =
          VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO;
      buffer_memory_dedicated_allocate_info.pNext = nullptr;
      buffer_memory_dedicated_allocate_info.image = VK_NULL_HANDLE;
      buffer_memory_dedicated_allocate_info.buffer = buffer_;
    }
    VkDeviceMemory buffer_memory;
    if (dfn.vkAllocateMemory(device, &buffer_memory_allocate_info, nullptr, &buffer_memory) !=
        VK_SUCCESS) {
      REXGPU_ERROR(
          "Shared memory: Failed to allocate {} MB of memory for the Vulkan "
          "buffer",
          kBufferSize >> 20);
      Shutdown();
      return false;
    }
    buffer_memory_.push_back(buffer_memory);
    if (dfn.vkBindBufferMemory(device, buffer_, buffer_memory, 0) != VK_SUCCESS) {
      REXGPU_ERROR("Shared memory: Failed to bind memory to the Vulkan buffer");
      Shutdown();
      return false;
    }
    if (direct_upload) {
      void* mapping = nullptr;
      if (dfn.vkMapMemory(device, buffer_memory, 0, VK_WHOLE_SIZE, 0, &mapping) == VK_SUCCESS) {
        buffer_mapping_ = static_cast<uint8_t*>(mapping);
        EnableReadTracking();
        REXGPU_INFO("Shared memory: host-visible buffer, direct uploads enabled");
      }
    }
  }

  // The first usage will likely be uploading.
  last_usage_ = Usage::kTransferDestination;
  last_written_range_ = std::make_pair<uint32_t, uint32_t>(0, 0);

  upload_buffer_pool_ = std::make_unique<ui::vulkan::VulkanUploadBufferPool>(
      vulkan_device, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      rex::align(ui::vulkan::VulkanUploadBufferPool::kDefaultPageSize, size_t(1)
                                                                           << page_size_log2()));

  return true;
}

void VulkanSharedMemory::Shutdown(bool from_destructor) {
  upload_buffer_pool_.reset();

  const ui::vulkan::VulkanDevice* const vulkan_device = command_processor_.GetVulkanDevice();
  const ui::vulkan::VulkanDevice::Functions& dfn = vulkan_device->functions();
  const VkDevice device = vulkan_device->device();

  ui::vulkan::util::DestroyAndNullHandle(dfn.vkDestroyBuffer, device, buffer_);
  for (VkDeviceMemory memory : buffer_memory_) {
    dfn.vkFreeMemory(device, memory, nullptr);
  }
  buffer_memory_.clear();

  // If calling from the destructor, the SharedMemory destructor will call
  // ShutdownCommon.
  if (!from_destructor) {
    ShutdownCommon();
  }
}

void VulkanSharedMemory::CompletedSubmissionUpdated() {
  upload_buffer_pool_->Reclaim(command_processor_.GetCompletedSubmission());
}

void VulkanSharedMemory::EndSubmission() {
  upload_buffer_pool_->FlushWrites();
}

void VulkanSharedMemory::Use(Usage usage, std::pair<uint32_t, uint32_t> written_range) {
  written_range.first = std::min(written_range.first, kBufferSize);
  written_range.second = std::min(written_range.second, kBufferSize - written_range.first);
  assert_true(usage != Usage::kRead || !written_range.second);
  if (last_usage_ != usage || last_written_range_.second) {
    VkPipelineStageFlags src_stage_mask, dst_stage_mask;
    VkAccessFlags src_access_mask, dst_access_mask;
    GetUsageMasks(last_usage_, src_stage_mask, src_access_mask);
    GetUsageMasks(usage, dst_stage_mask, dst_access_mask);
    VkDeviceSize offset, size;
    if (last_usage_ == usage) {
      // Committing the previous write, while not changing the access mask
      // (passing false as whether to skip the barrier if no masks are changed
      // for this reason).
      offset = VkDeviceSize(last_written_range_.first);
      size = VkDeviceSize(last_written_range_.second);
    } else {
      // Changing the stage and access mask - all preceding writes must be
      // available not only to the source stage, but to the destination as well.
      offset = 0;
      size = VK_WHOLE_SIZE;
      last_usage_ = usage;
    }
    command_processor_.PushBufferMemoryBarrier(
        buffer_, offset, size, src_stage_mask, dst_stage_mask, src_access_mask, dst_access_mask,
        VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, false);
  }
  last_written_range_ = written_range;
}

bool VulkanSharedMemory::AllocateSparseHostGpuMemoryRange(uint32_t offset_allocations,
                                                          uint32_t length_allocations) {
  if (!length_allocations) {
    return true;
  }

  const ui::vulkan::VulkanDevice* const vulkan_device = command_processor_.GetVulkanDevice();
  const ui::vulkan::VulkanDevice::Functions& dfn = vulkan_device->functions();
  const VkDevice device = vulkan_device->device();

  VkMemoryAllocateInfo memory_allocate_info;
  memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  memory_allocate_info.pNext = nullptr;
  memory_allocate_info.allocationSize = length_allocations
                                        << host_gpu_memory_sparse_granularity_log2();
  memory_allocate_info.memoryTypeIndex = buffer_memory_type_;
  VkDeviceMemory memory;
  if (dfn.vkAllocateMemory(device, &memory_allocate_info, nullptr, &memory) != VK_SUCCESS) {
    REXGPU_ERROR("Shared memory: Failed to allocate sparse buffer memory");
    return false;
  }
  buffer_memory_.push_back(memory);

  VkSparseMemoryBind bind;
  bind.resourceOffset = offset_allocations << host_gpu_memory_sparse_granularity_log2();
  bind.size = memory_allocate_info.allocationSize;
  bind.memory = memory;
  bind.memoryOffset = 0;
  bind.flags = 0;
  VkPipelineStageFlags bind_wait_stage_mask =
      VK_PIPELINE_STAGE_VERTEX_INPUT_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT |
      VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT |
      VK_PIPELINE_STAGE_TRANSFER_BIT;
  if (vulkan_device->properties().tessellationShader) {
    bind_wait_stage_mask |= VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT;
  }
  command_processor_.SparseBindBuffer(buffer_, 1, &bind, bind_wait_stage_mask);

  return true;
}

bool VulkanSharedMemory::UploadRanges(
    const std::vector<std::pair<uint32_t, uint32_t>>& upload_page_ranges) {
  if (upload_page_ranges.empty()) {
    return true;
  }
  if (DevelopmentStatsEnabled()) {
    // DEVSTATS (local only)
    uint64_t rex_pages = 0;
    for (const auto& rex_range : upload_page_ranges) {
      rex_pages += rex_range.second;
    }
    g_rex_perf_upload_bytes.fetch_add(rex_pages << page_size_log2(), std::memory_order_relaxed);
    for (const auto& rex_range : upload_page_ranges) {  // DEVSTATS census by 1 MB region
      const uint64_t lo = uint64_t(rex_range.first) << page_size_log2();
      const uint64_t hi = uint64_t(rex_range.first + rex_range.second) << page_size_log2();
      for (uint64_t a = lo; a < hi;) {
        const uint64_t next = std::min<uint64_t>(hi, (a & ~uint64_t(0xFFFFF)) + 0x100000);
        g_rex_upload_region_bytes[(a >> 20) & 511].fetch_add(next - a, std::memory_order_relaxed);
        a = next;
      }
    }
    g_rex_upload_calls.fetch_add(1, std::memory_order_relaxed);
    g_rex_upload_ranges.fetch_add(upload_page_ranges.size(), std::memory_order_relaxed);
    // Uploads from guest RAM over the save-thumbnail source (1F34C000+398000),
    // which only the GPU writes. Counted here, logged from IssueSwap: never log
    // under this lock.
    for (const auto& rex_range : upload_page_ranges) {
      const uint64_t lo = uint64_t(rex_range.first) << page_size_log2();
      const uint64_t hi = uint64_t(rex_range.first + rex_range.second) << page_size_log2();
      const uint64_t a = std::max<uint64_t>(lo, 0x1F34C000), b = std::min<uint64_t>(hi, 0x1F6E4000);
      if (a < b) {
        g_rex_thumbsrc_uploads.fetch_add(1, std::memory_order_relaxed);
        g_rex_thumbsrc_upload_bytes.fetch_add(b - a, std::memory_order_relaxed);
      }
    }
  }
  // Pages no recorded or in-flight GPU work reads: write them straight into
  // the mapped buffer, no copy command - and so no render pass end.
  const std::vector<std::pair<uint32_t, uint32_t>>* staged_ranges = &upload_page_ranges;
  if (buffer_mapping_) {
    const uint64_t completed = command_processor_.GetCompletedSubmission();
    staged_upload_ranges_.clear();
    auto stage = [this](uint32_t page) {
      if (!staged_upload_ranges_.empty() &&
          staged_upload_ranges_.back().first + staged_upload_ranges_.back().second == page) {
        ++staged_upload_ranges_.back().second;
      } else {
        staged_upload_ranges_.emplace_back(page, 1);
      }
    };
    // Runs of directly writable pages are made valid and copied as one range -
    // MakeRangeValid re-protects the pages, a system call each time.
    uint32_t run_first = 0, run_count = 0;
    auto flush_run = [&]() {
      if (!run_count) {
        return;
      }
      const uint32_t offset = run_first << page_size_log2();
      const uint32_t length = run_count << page_size_log2();
      MakeRangeValid(offset, length, false);
      std::memcpy(buffer_mapping_ + offset, memory().TranslatePhysical(offset), length);
      if (DevelopmentStatsEnabled()) g_rex_direct_upload_bytes.fetch_add(length, std::memory_order_relaxed);  // DEVSTATS
      run_count = 0;
    };
    const uint32_t page_size = uint32_t(1) << page_size_log2();
    if (page_deferred_slot_.empty()) {
      page_deferred_slot_.assign(kBufferSize >> page_size_log2(), 0);
    }
    for (const auto& range : upload_page_ranges) {
      for (uint32_t page = range.first; page < range.first + range.second; ++page) {
        const uint32_t offset = page << page_size_log2();
        // Earlier submissions still read the old contents, but nothing recorded
        // in this one has touched the page yet: the copy can go at the very
        // start of this submission, ahead of every render pass, instead of
        // ending the current one.
        uint32_t& slot = page_deferred_slot_[page];
        const bool touched = PageTouchedInCurrentSubmission(page);
        // Never compare against the old mapped page to decide whether a
        // pending prelude snapshot can be changed after a recorded reader.
        const bool direct = !(slot && touched) &&
                            PageDirectWritable(page, completed, buffer_mapping_ + offset);
        if (DevelopmentStatsEnabled()) {
          extern std::atomic<uint64_t> g_rex_up_why[4];
          g_rex_up_why[direct ? 0 : slot ? 3 : touched ? 1 : 2].fetch_add(1, std::memory_order_relaxed);
        }
        const CpuUploadPath path = SelectCpuUploadPath(
            slot != 0, touched, REXCVAR_GET(vulkan_defer_cpu_uploads), direct);
        if (path == CpuUploadPath::kDeferred) {
          if (!slot) {
            DeferredPage deferred;
            deferred.page = page;
            VkDeviceSize size = 0;
            deferred.mapping = upload_buffer_pool_->RequestPartial(
                command_processor_.GetCurrentSubmission(), page_size, page_size, deferred.buffer,
                deferred.offset, size);
            if (!deferred.mapping || size < page_size) {
              flush_run();
              stage(page);
              continue;
            }
            deferred_pages_.push_back(deferred);
            slot = uint32_t(deferred_pages_.size());
          }
          flush_run();
          MakeRangeValid(offset, page_size, false);
          std::memcpy(deferred_pages_[slot - 1].mapping, memory().TranslatePhysical(offset),
                      page_size);
          if (DevelopmentStatsEnabled()) g_rex_direct_upload_bytes.fetch_add(page_size, std::memory_order_relaxed);  // DEVSTATS
          continue;
        }
        if (path == CpuUploadPath::kDirect) {
          if (run_count && run_first + run_count == page) {
            ++run_count;
          } else {
            flush_run();
            run_first = page;
            run_count = 1;
          }
        } else {
          flush_run();
          stage(page);
        }
      }
    }
    flush_run();
    if (staged_upload_ranges_.empty()) {
      return true;
    }
    for (const auto& range : staged_upload_ranges_) {
      MarkPagesStagedCopy(range.first, range.second);
    }
    staged_ranges = &staged_upload_ranges_;
  }
  const std::vector<std::pair<uint32_t, uint32_t>>& upload_page_ranges_staged = *staged_ranges;

  // upload_page_ranges are sorted, use them to determine the range for the
  // ordering barrier.
  Use(Usage::kTransferDestination,
      std::make_pair(upload_page_ranges_staged.front().first << page_size_log2(),
                     (upload_page_ranges_staged.back().first + upload_page_ranges_staged.back().second -
                      upload_page_ranges_staged.front().first)
                         << page_size_log2()));
  command_processor_.SubmitBarriers(true);
  DeferredCommandBuffer& command_buffer = command_processor_.deferred_command_buffer();
  uint64_t submission_current = command_processor_.GetCurrentSubmission();
  bool successful = true;
  upload_regions_.clear();
  VkBuffer upload_buffer_previous = VK_NULL_HANDLE;
  for (auto upload_range : upload_page_ranges_staged) {
    uint32_t upload_range_start = upload_range.first;
    uint32_t upload_range_length = upload_range.second;
    while (upload_range_length) {
      VkBuffer upload_buffer;
      VkDeviceSize upload_buffer_offset, upload_buffer_size;
      uint8_t* upload_buffer_mapping = upload_buffer_pool_->RequestPartial(
          submission_current, upload_range_length << page_size_log2(),
          size_t(1) << page_size_log2(), upload_buffer, upload_buffer_offset, upload_buffer_size);
      if (upload_buffer_mapping == nullptr) {
        REXGPU_ERROR("Shared memory: Failed to get a Vulkan upload buffer");
        successful = false;
        break;
      }
      MakeRangeValid(upload_range_start << page_size_log2(), uint32_t(upload_buffer_size), false);
      std::memcpy(upload_buffer_mapping,
                  memory().TranslatePhysical(upload_range_start << page_size_log2()),
                  upload_buffer_size);
      if (upload_buffer_previous != upload_buffer && !upload_regions_.empty()) {
        assert_true(upload_buffer_previous != VK_NULL_HANDLE);
        command_buffer.CmdVkCopyBuffer(upload_buffer_previous, buffer_,
                                       uint32_t(upload_regions_.size()), upload_regions_.data());
        upload_regions_.clear();
      }
      upload_buffer_previous = upload_buffer;
      VkBufferCopy& upload_region = upload_regions_.emplace_back();
      upload_region.srcOffset = upload_buffer_offset;
      upload_region.dstOffset = VkDeviceSize(upload_range_start << page_size_log2());
      upload_region.size = upload_buffer_size;
      uint32_t upload_buffer_pages = uint32_t(upload_buffer_size >> page_size_log2());
      upload_range_start += upload_buffer_pages;
      upload_range_length -= upload_buffer_pages;
    }
    if (!successful) {
      break;
    }
  }
  if (!upload_regions_.empty()) {
    assert_true(upload_buffer_previous != VK_NULL_HANDLE);
    command_buffer.CmdVkCopyBuffer(upload_buffer_previous, buffer_,
                                   uint32_t(upload_regions_.size()), upload_regions_.data());
    upload_regions_.clear();
  }
  return successful;
}

void VulkanSharedMemory::GetUsageMasks(Usage usage, VkPipelineStageFlags& stage_mask,
                                       VkAccessFlags& access_mask) const {
  switch (usage) {
    case Usage::kComputeWrite:
      stage_mask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
      access_mask = VK_ACCESS_SHADER_READ_BIT;
      return;
    case Usage::kTransferDestination:
      stage_mask = VK_PIPELINE_STAGE_TRANSFER_BIT;
      access_mask = VK_ACCESS_TRANSFER_WRITE_BIT;
      return;
    default:
      break;
  }
  stage_mask = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT | guest_shader_pipeline_stages_;
  access_mask = VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_SHADER_READ_BIT;
  switch (usage) {
    case Usage::kRead:
      stage_mask |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT;
      access_mask |= VK_ACCESS_TRANSFER_READ_BIT;
      break;
    case Usage::kGuestDrawReadWrite:
      access_mask |= VK_ACCESS_SHADER_WRITE_BIT;
      break;
    default:
      assert_unhandled_case(usage);
  }
}

}  // namespace rex::graphics::vulkan
