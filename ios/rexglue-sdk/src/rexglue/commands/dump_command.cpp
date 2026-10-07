/**
 * @file        rexglue/commands/dump_command.cpp
 * @brief       Extracts the contents of an STFS package (XBLA container, title
 *              update, save) to a host directory.
 *
 * @copyright   Copyright (c) 2026 Tom Clay
 * @license     BSD 3-Clause License
 *
 * @remarks     Grown out of the unbuilt src/filesystem/vfs_dump.cpp, which still
 *              referenced a `vfs` namespace that no longer exists.
 */

#include "dump_command.h"

#include "../ui/ui.h"

#include <queue>
#include <span>
#include <vector>

#include <CLI/CLI.hpp>

#include <rex/filesystem.h>
#include <rex/filesystem/devices/stfs_container_device.h>
#include <rex/filesystem/entry.h>
#include <rex/filesystem/file.h>
#include <rex/logging.h>
#include <rex/string.h>

#include <fmt/format.h>

namespace rexglue::cli {

namespace fs = rex::filesystem;
using rex::X_STATUS;

// STFS entry paths are guest paths and use backslashes. On POSIX those are legal
// filename characters, so writing one verbatim yields a single file literally
// named "res\\1_2_2\\terrain.png" instead of a directory tree.
static std::filesystem::path HostRelativePath(const std::string& guest_path) {
  std::string s = guest_path;
  for (char& c : s) {
    if (c == '\\') {
      c = '/';
    }
  }
  while (!s.empty() && s.front() == '/') {
    s.erase(s.begin());
  }
  return std::filesystem::path(s);
}

Result<void> DumpPackage(const DumpOptions& opts, const CliContext& ctx) {
  std::filesystem::path source = rex::to_path(opts.source);
  if (!std::filesystem::exists(source)) {
    return rex::Err(rex::ErrorCategory::NotFound,
                    fmt::format("Package not found: {}", source.string()));
  }

  auto device = std::make_unique<fs::StfsContainerDevice>("", source);
  if (!device->Initialize()) {
    return rex::Err(rex::ErrorCategory::Format,
                    fmt::format("Not a readable STFS package: {}", source.string()));
  }

  std::filesystem::path base;
  if (!opts.list_only) {
    base = rex::to_path(opts.out_dir);
    std::error_code ec;
    std::filesystem::create_directories(base, ec);
    if (ec) {
      return rex::Err(rex::ErrorCategory::IO,
                      fmt::format("Cannot create output directory {}: {}", base.string(),
                                  ec.message()));
    }
  }

  // Breadth-first over the package tree. Directories are created eagerly so a
  // file entry never races ahead of its parent.
  std::queue<fs::Entry*> queue;
  queue.push(device->ResolvePath("/"));

  std::vector<uint8_t> buffer;
  size_t file_count = 0;
  size_t dir_count = 0;
  uint64_t byte_count = 0;

  while (!queue.empty()) {
    auto* entry = queue.front();
    queue.pop();
    if (!entry) {
      continue;
    }
    for (auto& child : entry->children()) {
      queue.push(child.get());
    }

    const bool is_dir = (entry->attributes() & fs::kFileAttributeDirectory) != 0;
    if (is_dir) {
      ++dir_count;
      if (!opts.list_only && !entry->path().empty()) {
        std::filesystem::create_directories(base / HostRelativePath(entry->path()));
      }
      continue;
    }

    ++file_count;
    byte_count += entry->size();
    if (opts.list_only || ctx.verbose) {
      REXLOG_INFO("{:>12}  {}", entry->size(), entry->path());
    }
    if (opts.list_only) {
      continue;
    }

    fs::File* in_file = nullptr;
    if (entry->Open(fs::FileAccess::kFileReadData, &in_file) != X_STATUS_SUCCESS) {
      return rex::Err(rex::ErrorCategory::IO,
                      fmt::format("Failed to open packaged file: {}", entry->path()));
    }

    auto dest = base / HostRelativePath(entry->path());
    std::filesystem::create_directories(dest.parent_path());
    FILE* out = rex::filesystem::OpenFile(dest, "wb");
    if (!out) {
      in_file->Destroy();
      return rex::Err(rex::ErrorCategory::IO, fmt::format("Failed to write: {}", dest.string()));
    }

    // STFS files are not contiguous on disk (hash blocks are interleaved every
    // 0xAA blocks), so read through the device rather than mapping.
    buffer.resize(entry->size());
    size_t bytes_read = 0;
    in_file->ReadSync(std::span<uint8_t>(buffer.data(), buffer.size()), 0, &bytes_read);
    if (bytes_read != entry->size()) {
      fclose(out);
      in_file->Destroy();
      return rex::Err(rex::ErrorCategory::IO,
                      fmt::format("Short read on {}: got {} of {} bytes", entry->path(),
                                  bytes_read, entry->size()));
    }
    fwrite(buffer.data(), 1, bytes_read, out);
    fclose(out);
    in_file->Destroy();
  }

  REXLOG_INFO("{} file(s), {} director(ies), {} bytes {}", file_count, dir_count, byte_count,
              opts.list_only ? "listed" : "extracted");
  return rex::Ok();
}

void RegisterDump(CLI::App& parent, const CliContext& ctx, DeferredAction& pending) {
  auto opts = std::make_shared<DumpOptions>();
  auto* cmd = parent.add_subcommand("dump", "Extract an STFS package (XBLA container, title update)");
  cmd->add_option("package", opts->source, "Path to the STFS package")
      ->required()
      ->type_name("PATH");
  cmd->add_option("-o,--out", opts->out_dir, "Directory to extract into")->type_name("PATH");
  cmd->add_flag("-l,--list", opts->list_only, "List contents without extracting");
  cmd->callback([opts, &ctx, &pending]() {
    if (!opts->list_only && opts->out_dir.empty()) {
      opts->out_dir = "extracted";
    }
    pending = [opts, &ctx]() { return DumpPackage(*opts, ctx); };
  });
}

}  // namespace rexglue::cli
