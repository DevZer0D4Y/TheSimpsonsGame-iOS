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

#include <rex/filesystem/devices/disc_image_device.h>
#include <rex/filesystem/devices/disc_image_entry.h>

#include <rex/literals.h>
#include <rex/logging.h>
#include <rex/math.h>
#include <rex/memory.h>

namespace rex::filesystem {

using namespace rex::literals;

const size_t kXESectorSize = 2_KiB;

DiscImageDevice::DiscImageDevice(const std::string_view mount_path,
                                 const std::filesystem::path& host_path)
    : Device(mount_path), name_("GDFX"), host_path_(host_path) {}

DiscImageDevice::~DiscImageDevice() = default;

bool DiscImageDevice::Initialize() {
  mmap_ = memory::MappedMemory::Open(host_path_, memory::MappedMemory::Mode::kRead);
  if (!mmap_) {
    REXFS_ERROR("Disc image could not be mapped");
    return false;
  }

  ParseState state = {};
  state.ptr = mmap_->data();
  state.size = mmap_->size();
  auto result = Verify(&state);
  if (result != Error::kSuccess) {
    REXFS_ERROR("Failed to verify disc image header: {}", static_cast<int>(result));
    return false;
  }

  result = ReadAllEntries(&state, state.ptr + state.root_offset);
  if (result != Error::kSuccess) {
    REXFS_ERROR("Failed to read all GDFX entries: {}", static_cast<int>(result));
    return false;
  }

  return true;
}

void DiscImageDevice::Dump(string::StringBuffer* string_buffer) {
  auto global_lock = global_critical_region_.Acquire();
  string_buffer->AppendFormat(
      "{}: {} files, {} bytes (game_offset={:#x}, root_sector={}, root_size={}, host_size={})\n",
      mount_path(), file_count_, total_file_size_, disc_info_.game_offset, disc_info_.root_sector,
      disc_info_.root_size, disc_info_.host_size);
}

Entry* DiscImageDevice::ResolvePath(const std::string_view path) {
  // The filesystem will have stripped our prefix off already, so the path will
  // be in the form:
  // some\PATH.foo
  REXFS_DEBUG("DiscImageDevice::ResolvePath({})", path);
  return root_entry_->ResolvePath(path);
}

DiscImageDevice::Error DiscImageDevice::Verify(ParseState* state) {
  // Find sector 32 of the game partition - try at a few points.
  static const size_t likely_offsets[] = {
      0x00000000, 0x0000FB20, 0x00020600, 0x02080000, 0x0FD90000,
  };
  bool magic_found = false;
  for (size_t n = 0; n < rex::countof(likely_offsets); n++) {
    state->game_offset = likely_offsets[n];
    if (VerifyMagic(state, state->game_offset + (32 * kXESectorSize))) {
      magic_found = true;
      break;
    }
  }
  if (!magic_found) {
    // File doesn't have the magic values - likely not a real GDFX source.
    return Error::kErrorFileMismatch;
  }

  // Read sector 32 to get FS state.
  if (state->game_offset + (32 * kXESectorSize) > state->size ||
      state->size - (state->game_offset + (32 * kXESectorSize)) < 28) {
    return Error::kErrorReadError;
  }
  uint8_t* fs_ptr = state->ptr + state->game_offset + (32 * kXESectorSize);
  state->root_sector = memory::load<uint32_t>(fs_ptr + 20);
  state->root_size = memory::load<uint32_t>(fs_ptr + 24);
  state->root_offset = state->game_offset + (state->root_sector * kXESectorSize);
  if (state->root_size < 14 || state->root_size > 32_MiB ||
      state->root_offset > state->size || state->root_size > state->size - state->root_offset) {
    return Error::kErrorDamagedFile;
  }

  disc_info_.game_offset = state->game_offset;
  disc_info_.root_sector = state->root_sector;
  disc_info_.root_size = state->root_size;
  disc_info_.host_size = state->size;

  return Error::kSuccess;
}

bool DiscImageDevice::VerifyMagic(ParseState* state, size_t offset) {
  if (offset > state->size || state->size - offset < 20) {
    return false;
  }

  // Simple check to see if the given offset contains the magic value.
  return std::memcmp(state->ptr + offset, "MICROSOFT*XBOX*MEDIA", 20) == 0;
}

DiscImageDevice::Error DiscImageDevice::ReadAllEntries(ParseState* state,
                                                       const uint8_t* root_buffer) {
  auto root_entry = new DiscImageEntry(this, nullptr, "", mmap_.get());
  root_entry->attributes_ = kFileAttributeDirectory;
  root_entry_ = std::unique_ptr<Entry>(root_entry);

  if (!ReadEntry(state, root_buffer, 0, root_entry, state->root_size)) {
    return Error::kErrorOutOfMemory;
  }

  return Error::kSuccess;
}

bool DiscImageDevice::ReadEntry(ParseState* state, const uint8_t* buffer, uint16_t ordinal,
                                DiscImageEntry* parent, size_t table_size, size_t depth) {
  const size_t offset = size_t(ordinal) * 4;
  const size_t absolute = size_t(buffer - state->ptr) + offset;
  if (depth > 512 || offset > table_size || table_size - offset < 14 ||
      !state->visited.insert(absolute).second) return false;
  const uint8_t* p = buffer + offset;
  const uint16_t left = memory::load<uint16_t>(p);
  const uint16_t right = memory::load<uint16_t>(p + 2);
  const size_t sector = memory::load<uint32_t>(p + 4);
  const size_t length = memory::load<uint32_t>(p + 8);
  const uint8_t attributes = p[12], name_length = p[13];
  if (!name_length || name_length > table_size - offset - 14) return false;
  const std::string name(reinterpret_cast<const char*>(p + 14), name_length);
  if (name == "." || name == ".." || name.find_first_of("/\\") != std::string::npos ||
      name.find('\0') != std::string::npos) return false;
  const size_t data_offset = state->game_offset + sector * kXESectorSize;
  if (data_offset > state->size || length > state->size - data_offset) return false;
  if (left && !ReadEntry(state, buffer, left, parent, table_size, depth + 1)) return false;
  auto entry = DiscImageEntry::Create(this, parent, name, mmap_.get());
  entry->attributes_ = attributes | kFileAttributeReadOnly;
  entry->size_ = length;
  entry->allocation_size_ = rex::round_up(length, bytes_per_sector());
  entry->create_timestamp_ = entry->access_timestamp_ = entry->write_timestamp_ =
      10000 * 11644473600000LL;
  if (attributes & kFileAttributeDirectory) {
    entry->data_offset_ = entry->data_size_ = 0;
    if (length && !ReadEntry(state, state->ptr + data_offset, 0, entry.get(), length, depth + 1))
      return false;
  } else {
    entry->data_offset_ = data_offset;
    entry->data_size_ = length;
    ++file_count_;
    total_file_size_ += length;
  }
  parent->children_.emplace_back(std::move(entry));
  return !right || ReadEntry(state, buffer, right, parent, table_size, depth + 1);
}

}  // namespace rex::filesystem
