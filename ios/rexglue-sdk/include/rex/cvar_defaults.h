#pragma once
/**
 * @file        rex/cvar_defaults.h
 * @brief       App-supplied replacements for cvar defaults.
 *
 * @license     BSD 3-Clause License
 *              See LICENSE file in the project root for full license text.
 */

#include <string_view>

namespace rex::cvar {

// Replaces a flag's compiled-in default, for apps that ship different defaults
// than the SDK. It ranks below every other source, so the config file, the
// environment and the command line still override it. The value is also kept
// for flags that are not registered yet and applied when they register - which
// SetFlagByName cannot do, so this is the only way to default a flag owned by a
// runtime-loaded plugin (the GPU plugin loads after the config). Returns false
// when the value fails the flag's constraints.
bool SetDefaultByName(std::string_view name, std::string_view value);

}  // namespace rex::cvar
