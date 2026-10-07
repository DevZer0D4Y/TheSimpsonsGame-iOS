#pragma once
#include <cstdio>
#include <rex/filesystem.h>

namespace rex::graphics::vulkan {
// Darwin opens a+ streams with the initial read position at EOF. Explicitly
// rewind before reading headers, while preserving append-only writes.
inline FILE* OpenPersistentCacheFile(const std::filesystem::path& path) {
  FILE* file = rex::filesystem::OpenFile(path, "a+b");
  if (file && !rex::filesystem::Seek(file, 0, SEEK_SET)) {
    std::fclose(file);
    return nullptr;
  }
  return file;
}
}  // namespace rex::graphics::vulkan
