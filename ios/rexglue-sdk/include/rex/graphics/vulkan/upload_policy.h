#pragma once

namespace rex::graphics::vulkan {

enum class CpuUploadPath { kDeferred, kDirect, kOrderedCopy };

// A prelude upload is a snapshot for every draw recorded after it. Once a
// command reads the page, keep that snapshot immutable; later CPU versions
// must be copied at their position in the command stream.
constexpr CpuUploadPath SelectCpuUploadPath(bool has_deferred_snapshot,
                                            bool touched_in_submission,
                                            bool defer_enabled,
                                            bool directly_writable) {
  if (has_deferred_snapshot && touched_in_submission) {
    return CpuUploadPath::kOrderedCopy;
  }
  if (has_deferred_snapshot ||
      (defer_enabled && !touched_in_submission && !directly_writable)) {
    return CpuUploadPath::kDeferred;
  }
  return directly_writable ? CpuUploadPath::kDirect : CpuUploadPath::kOrderedCopy;
}

}  // namespace rex::graphics::vulkan
