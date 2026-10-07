/**
 * @file        rexglue/commands/dump_command.h
 * @brief       STFS package extraction command interface
 *
 * @copyright   Copyright (c) 2026 Tom Clay
 * @license     BSD 3-Clause License
 */

#pragma once

#include "../cli_utils.h"

#include <string>

#include <rex/result.h>

namespace CLI {
class App;
}

namespace rexglue::cli {

using rex::Result;

struct DumpOptions {
  std::string source;
  std::string out_dir;
  bool list_only = false;
};

Result<void> DumpPackage(const DumpOptions& opts, const CliContext& ctx);

void RegisterDump(CLI::App& parent, const CliContext& ctx, DeferredAction& pending);

}  // namespace rexglue::cli
