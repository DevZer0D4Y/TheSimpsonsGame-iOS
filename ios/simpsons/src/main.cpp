// simpsons - ReXGlue Recompiled Project (iOS)

#include "generated/default/simpsons_init.h"

#include "simpsons_app.h"

#if REX_PLATFORM_IOS
#include <mach-o/dyld.h>
#include <mach-o/loader.h>
#include <mach-o/nlist.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <utility>
#include <vector>

// For the SDK's sampling profiler (it looks this up with dlsym). dladdr only
// knows exported symbols, and the recompiled functions are hidden, so it would
// name the nearest exported function instead. The executable keeps its full
// symbol table, so read it once from __LINKEDIT and answer for any PC in our
// own __TEXT: guest functions, the game hooks, the app and its threads.
extern "C" __attribute__((visibility("default"), used)) bool RexGuestSymbolize(
    uintptr_t pc, char* out, size_t out_size) {
  struct Symbols {
    uintptr_t text_lo = 0, text_hi = 0;
    std::vector<std::pair<uintptr_t, const char*>> by_address;
  };
  static const Symbols symbols = [] {
    Symbols result;
    const auto* header = reinterpret_cast<const mach_header_64*>(_dyld_get_image_header(0));
    const intptr_t slide = _dyld_get_image_vmaddr_slide(0);
    const symtab_command* symtab = nullptr;
    const segment_command_64* linkedit = nullptr;
    const auto* cmd = reinterpret_cast<const load_command*>(header + 1);
    for (uint32_t i = 0; i < header->ncmds; ++i) {
      if (cmd->cmd == LC_SYMTAB) {
        symtab = reinterpret_cast<const symtab_command*>(cmd);
      } else if (cmd->cmd == LC_SEGMENT_64) {
        const auto* segment = reinterpret_cast<const segment_command_64*>(cmd);
        if (std::strcmp(segment->segname, SEG_LINKEDIT) == 0) {
          linkedit = segment;
        } else if (std::strcmp(segment->segname, SEG_TEXT) == 0) {
          result.text_lo = segment->vmaddr + slide;
          result.text_hi = result.text_lo + segment->vmsize;
        }
      }
      cmd = reinterpret_cast<const load_command*>(reinterpret_cast<const uint8_t*>(cmd) +
                                                  cmd->cmdsize);
    }
    if (!symtab || !linkedit) {
      return result;
    }
    const uintptr_t linkedit_base = linkedit->vmaddr + slide - linkedit->fileoff;
    const auto* entries = reinterpret_cast<const nlist_64*>(linkedit_base + symtab->symoff);
    const char* strings = reinterpret_cast<const char*>(linkedit_base + symtab->stroff);
    for (uint32_t i = 0; i < symtab->nsyms; ++i) {
      if ((entries[i].n_type & N_STAB) || (entries[i].n_type & N_TYPE) != N_SECT) {
        continue;
      }
      result.by_address.emplace_back(entries[i].n_value + slide,
                                     strings + entries[i].n_un.n_strx);
    }
    std::sort(result.by_address.begin(), result.by_address.end());
    return result;
  }();
  if (pc < symbols.text_lo || pc >= symbols.text_hi || symbols.by_address.empty()) {
    return false;
  }
  auto it = std::upper_bound(symbols.by_address.begin(), symbols.by_address.end(),
                             std::make_pair(pc, static_cast<const char*>(nullptr)),
                             [](const auto& a, const auto& b) { return a.first < b.first; });
  if (it == symbols.by_address.begin()) {
    return false;
  }
  const char* name = std::prev(it)->second;
  if (name[0] == '_') {
    ++name;  // C symbol prefix
  }
  if (std::strncmp(name, "__imp__", 7) == 0) {
    name += 7;  // the original body of a guest function; same name as its thunk
  }
  std::snprintf(out, out_size, "%s", name);
  return true;
}
#endif

REX_DEFINE_APP(simpsons, SimpsonsApp::Create)
