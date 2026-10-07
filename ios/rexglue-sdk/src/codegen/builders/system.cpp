/**
 * @file        rexcodegen/builders/system.cpp
 * @brief       PPC system instruction code generation
 *
 * @copyright   Copyright (c) 2026 Tom Clay <tomc@tctechstuff.com>
 *              All rights reserved.
 *
 * @license     BSD 3-Clause License
 *              See LICENSE file in the project root for full license text.
 */

#include "builder_context.h"
#include "helpers.h"

#include <algorithm>
#include <set>
#include <vector>

#include <rex/codegen/binary_view.h>
#include <rex/codegen/function_node.h>
#include <rex/memory/utils.h>

namespace rex::codegen {

namespace {

//=============================================================================
// Interlocked sequences under "interrupts disabled"
//=============================================================================
//
// The Xbox 360 compiler wraps every interlocked operation in
//   mfmsr rX ; mtmsrd r13 ; lwarx ... stwcx. ; mtmsrd rX
// Disabling interrupts only kept the reservation from being lost to an
// interrupt on that hardware thread; it never excluded other cores. Here the
// lwarx/stwcx. pair is a host compare-and-swap, so taking the process-wide
// global lock around it adds nothing but contention - and Minecraft refcounts
// shared pointers this way on every thread, which cost the server thread a
// fifth of its time. Regions that provably contain nothing but that one
// atomic operation skip the lock; every other mtmsrd keeps it.

constexpr uint32_t kInterlockedWindowBytes = 32 * 4;

constexpr uint32_t PrimaryOp(uint32_t insn) {
  return insn >> 26;
}
constexpr uint32_t XO10(uint32_t insn) {
  return (insn >> 1) & 0x3FF;
}

bool IsLoadReserve(uint32_t insn) {
  return PrimaryOp(insn) == 31 && (XO10(insn) == 20 || XO10(insn) == 84);  // lwarx, ldarx
}
bool IsStoreConditional(uint32_t insn) {
  return PrimaryOp(insn) == 31 && (XO10(insn) == 150 || XO10(insn) == 214) &&
         (insn & 1);  // stwcx., stdcx.
}
bool IsMtmsrd(uint32_t insn) {
  return PrimaryOp(insn) == 31 && XO10(insn) == 178;
}
uint32_t MtmsrdSource(uint32_t insn) {
  return (insn >> 21) & 31;
}

// Integer ops that only read and write registers: no memory, SPRs or calls.
bool IsRegisterOnlyOp(uint32_t insn) {
  switch (PrimaryOp(insn)) {
    case 7:   // mulli
    case 8:   // subfic
    case 10:  // cmpli
    case 11:  // cmpi
    case 12:  // addic
    case 13:  // addic.
    case 14:  // addi
    case 15:  // addis
    case 20:  // rlwimi
    case 21:  // rlwinm
    case 23:  // rlwnm
    case 24:  // ori
    case 25:  // oris
    case 26:  // xori
    case 27:  // xoris
    case 28:  // andi.
    case 29:  // andis.
    case 30:  // rldicl, rldicr, rldic, rldimi, rldcl, rldcr
      return true;
    case 31:
      break;
    default:
      return false;
  }
  switch (XO10(insn)) {
    case 0:    // cmp
    case 24:   // slw
    case 26:   // cntlzw
    case 27:   // sld
    case 28:   // and
    case 32:   // cmpl
    case 58:   // cntlzd
    case 60:   // andc
    case 124:  // nor
    case 284:  // eqv
    case 316:  // xor
    case 412:  // orc
    case 444:  // or
    case 476:  // nand
    case 536:  // srw
    case 539:  // srd
    case 598:  // sync, lwsync
    case 792:  // sraw
    case 794:  // srad
    case 824:  // srawi
    case 922:  // extsh
    case 954:  // extsb
    case 986:  // extsw
      return true;
    default:
      break;
  }
  if (((insn >> 2) & 0x1FF) == 413) {  // sradi
    return true;
  }
  switch ((insn >> 1) & 0x1FF) {  // XO-form arithmetic, either OE setting
    case 8:    // subfc
    case 9:    // mulhdu
    case 10:   // addc
    case 11:   // mulhwu
    case 40:   // subf
    case 73:   // mulhd
    case 75:   // mulhw
    case 104:  // neg
    case 136:  // subfe
    case 138:  // adde
    case 200:  // subfze
    case 202:  // addze
    case 232:  // subfme
    case 233:  // mulld
    case 234:  // addme
    case 235:  // mullw
    case 266:  // add
    case 457:  // divdu
    case 459:  // divwu
    case 489:  // divd
    case 491:  // divw
      return true;
    default:
      return false;
  }
}

struct BranchInfo {
  bool is_branch = false;  // any b/bc/bclr/bcctr
  bool can_fall_through = true;
  bool has_target = false;  // relative, non-linking b/bc with a static target
  uint32_t target = 0;
  bool links = false;
};

BranchInfo DecodeBranch(uint32_t insn, uint32_t addr) {
  BranchInfo info;
  uint32_t op = PrimaryOp(insn);
  if (op == 18) {  // b
    info.is_branch = true;
    info.can_fall_through = false;
    info.links = insn & 1;
    if (!(insn & 2)) {
      int32_t li = int32_t(insn & 0x03FFFFFC);
      if (li & 0x02000000) {
        li -= 0x04000000;
      }
      info.has_target = true;
      info.target = addr + uint32_t(li);
    }
  } else if (op == 16) {  // bc
    info.is_branch = true;
    uint32_t bo = (insn >> 21) & 31;
    info.can_fall_through = (bo & 0x14) != 0x14;
    info.links = insn & 1;
    if (!(insn & 2)) {
      int32_t bd = int32_t(insn & 0xFFFC);
      if (bd & 0x8000) {
        bd -= 0x10000;
      }
      info.has_target = true;
      info.target = addr + uint32_t(bd);
    }
  } else if (op == 19 && (XO10(insn) == 16 || XO10(insn) == 528)) {  // bclr, bcctr
    info.is_branch = true;
    uint32_t bo = (insn >> 21) & 31;
    info.can_fall_through = (bo & 0x14) != 0x14;
    info.links = insn & 1;
  }
  return info;
}

struct InterlockedRegion {
  bool lockless = false;
  std::vector<uint32_t> exits;
};

// Proves that the mtmsrd r13 at `enter` guards a single lwarx/stwcx. atomic:
// every path from it reaches a restoring mtmsrd within a few instructions,
// running only register ops, the reservation pair and local branches, and
// nothing outside those paths can jump or fall into them.
InterlockedRegion AnalyzeInterlockedRegion(const BuilderContext& ctx, uint32_t enter) {
  const FunctionNode& fn = ctx.fn;
  const BinaryView& binary = ctx.emitCtx.binary;
  auto read = [&](uint32_t addr, uint32_t& insn) -> bool {
    if (addr < fn.base() || addr + 4 > fn.end()) {
      return false;
    }
    const uint8_t* p = binary.translate(addr);
    if (!p) {
      return false;
    }
    insn = rex::memory::load_and_swap<uint32_t>(p);
    return true;
  };

  InterlockedRegion region;
  uint32_t insn = 0;
  if (!read(enter, insn) || !IsMtmsrd(insn) || MtmsrdSource(insn) != 13) {
    return region;
  }
  if (!read(enter + 4, insn) || !IsLoadReserve(insn)) {
    return region;
  }

  // Walk every path from the first instruction after the enter.
  std::set<uint32_t> on_path;
  std::set<uint32_t> exits;
  std::vector<uint32_t> worklist{enter + 4};
  while (!worklist.empty()) {
    uint32_t addr = worklist.back();
    worklist.pop_back();
    if (addr <= enter || addr >= enter + kInterlockedWindowBytes) {
      return InterlockedRegion{};
    }
    if (!on_path.insert(addr).second) {
      continue;
    }
    if (!read(addr, insn)) {
      return InterlockedRegion{};
    }
    if (IsMtmsrd(insn)) {
      if (MtmsrdSource(insn) == 13) {
        return InterlockedRegion{};  // Nested enter.
      }
      exits.insert(addr);  // The path ends here.
      continue;
    }
    if (IsLoadReserve(insn) || IsStoreConditional(insn) || IsRegisterOnlyOp(insn)) {
      worklist.push_back(addr + 4);
      continue;
    }
    BranchInfo branch = DecodeBranch(insn, addr);
    if (!branch.is_branch || branch.links || !branch.has_target) {
      return InterlockedRegion{};  // Memory access, call, return or anything else.
    }
    worklist.push_back(branch.target);
    if (branch.can_fall_through) {
      worklist.push_back(addr + 4);
    }
  }
  if (exits.empty()) {
    return InterlockedRegion{};
  }

  // Nothing else may reach the paths: an instruction on them after the first
  // may only be entered by a branch on the paths, or by falling through from
  // a path instruction that isn't an exit.
  for (uint32_t addr : on_path) {
    if (addr == enter + 4) {
      continue;
    }
    uint32_t prev = 0;
    if (read(addr - 4, prev)) {
      bool prev_on_path = on_path.count(addr - 4) != 0;
      bool prev_is_exit = exits.count(addr - 4) != 0;
      BranchInfo prev_branch = DecodeBranch(prev, addr - 4);
      bool prev_falls_through = !prev_branch.is_branch || prev_branch.can_fall_through;
      if (prev_falls_through && (!prev_on_path || prev_is_exit)) {
        return InterlockedRegion{};
      }
    }
  }
  // Nor jump tables or mid-asm hooks.
  for (const JumpTable& table : fn.jumpTables()) {
    for (uint32_t target : table.targets) {
      if (on_path.count(target)) {
        return InterlockedRegion{};
      }
    }
  }
  for (const auto& [table_address, table] : ctx.config().switchTables) {
    for (uint32_t target : table.targets) {
      if (on_path.count(target)) {
        return InterlockedRegion{};
      }
    }
  }
  for (const auto& [hook_address, hook] : ctx.config().midAsmHooks) {
    if (hook_address == enter || on_path.count(hook_address) || on_path.count(hook.jumpAddress) ||
        on_path.count(hook.jumpAddressOnTrue) || on_path.count(hook.jumpAddressOnFalse)) {
      return InterlockedRegion{};
    }
  }
  // And no branch elsewhere in the function may target them.
  for (uint32_t addr = fn.base(); addr + 4 <= fn.end(); addr += 4) {
    if (on_path.count(addr) || !read(addr, insn)) {
      continue;
    }
    BranchInfo branch = DecodeBranch(insn, addr);
    if (branch.has_target && addr != enter && on_path.count(branch.target) &&
        branch.target != enter + 4) {
      return InterlockedRegion{};
    }
    if (branch.has_target && branch.target == enter + 4) {
      return InterlockedRegion{};  // Entering past the enter would skip it.
    }
  }

  region.lockless = true;
  region.exits.assign(exits.begin(), exits.end());
  return region;
}

// Whether the restoring mtmsrd at `addr` closes a lockless interlocked region.
bool IsLocklessInterlockedExit(const BuilderContext& ctx, uint32_t addr) {
  const FunctionNode& fn = ctx.fn;
  uint32_t lowest = addr > fn.base() + kInterlockedWindowBytes ? addr - kInterlockedWindowBytes
                                                               : fn.base();
  for (uint32_t enter = addr; enter >= lowest + 4;) {
    enter -= 4;
    InterlockedRegion region = AnalyzeInterlockedRegion(ctx, enter);
    if (region.lockless &&
        std::find(region.exits.begin(), region.exits.end(), addr) != region.exits.end()) {
      return true;
    }
  }
  return false;
}

}  // namespace

//=============================================================================
// No-ops and Sync Operations
//=============================================================================

bool build_nop(BuilderContext& ctx) {
  // Canonical PPC no-op (ori 0,0,0)
  (void)ctx;
  return true;
}

bool build_attn(BuilderContext& ctx) {
  // Xenon-specific debug breakpoint, no effect in recompiled code
  (void)ctx;
  return true;
}

bool build_sync(BuilderContext& ctx) {
  // Memory barrier, x86 has strong ordering so this is a no-op
  (void)ctx;
  return true;
}

bool build_lwsync(BuilderContext& ctx) {
  // Lightweight memory barrier, x86 has strong ordering so this is a no-op
  (void)ctx;
  return true;
}

bool build_eieio(BuilderContext& ctx) {
  // Enforce in-order execution of I/O, x86 has strong ordering so this is a no-op
  (void)ctx;
  return true;
}

bool build_db16cyc(BuilderContext& ctx) {
  // Xenon-specific 16-cycle delay hint, no effect in recompiled code
  (void)ctx;
  return true;
}

bool build_cctpl(BuilderContext& ctx) {
  // Xenon-specific cache control thread priority low, no effect in recompiled code
  (void)ctx;
  return true;
}

bool build_cctpm(BuilderContext& ctx) {
  // Xenon-specific cache control thread priority medium, no effect in recompiled code
  (void)ctx;
  return true;
}

bool build_cctph(BuilderContext& ctx) {
  // Xenon-specific cache control thread priority high, no effect in recompiled code
  (void)ctx;
  return true;
}

//=============================================================================
// Trap Instructions
// PPC trap instructions are assertion/debug checks. The TO field (bits 21-25)
// is a 5-bit mask specifying which conditions trigger: signed lt/gt, eq,
// unsigned lt/gt. We extract TO directly from the instruction word so that
// both generic (tw TO,rA,rB) and simplified (tweq rA,rB) forms work with
// the same builder.
//=============================================================================

bool build_tdi(BuilderContext& ctx) {
  uint32_t to = (ctx.insn.instruction >> 21) & 0x1F;
  uint32_t ra = (ctx.insn.instruction >> 16) & 0x1F;
  int64_t simm = static_cast<int16_t>(ctx.insn.instruction & 0xFFFF);
  emitTrap(ctx, to, fmt::format("{}.s64", ctx.r(ra)), fmt::format("{}.u64", ctx.r(ra)),
           fmt::format("{}ll", simm), fmt::format("{}ull", static_cast<uint64_t>(simm)));
  return true;
}

bool build_twi(BuilderContext& ctx) {
  uint32_t to = (ctx.insn.instruction >> 21) & 0x1F;
  uint32_t ra = (ctx.insn.instruction >> 16) & 0x1F;
  int32_t simm = static_cast<int16_t>(ctx.insn.instruction & 0xFFFF);

  // twi 31, r0, <imm> is an unconditional trap with service code in the immediate
  if (to == 0x1F && ra == 0) {
    uint16_t trap_type = static_cast<uint16_t>(simm);
    ctx.println("\tppc_trap(ctx, base, {});", trap_type);
    return true;
  }

  emitTrap(ctx, to, fmt::format("{}.s32", ctx.r(ra)), fmt::format("{}.u32", ctx.r(ra)),
           fmt::format("{}", simm), fmt::format("{}u", static_cast<uint32_t>(simm)));
  return true;
}

bool build_td(BuilderContext& ctx) {
  uint32_t to = (ctx.insn.instruction >> 21) & 0x1F;
  uint32_t ra = (ctx.insn.instruction >> 16) & 0x1F;
  uint32_t rb = (ctx.insn.instruction >> 11) & 0x1F;
  emitTrap(ctx, to, fmt::format("{}.s64", ctx.r(ra)), fmt::format("{}.u64", ctx.r(ra)),
           fmt::format("{}.s64", ctx.r(rb)), fmt::format("{}.u64", ctx.r(rb)));
  return true;
}

bool build_tw(BuilderContext& ctx) {
  uint32_t to = (ctx.insn.instruction >> 21) & 0x1F;
  uint32_t ra = (ctx.insn.instruction >> 16) & 0x1F;
  uint32_t rb = (ctx.insn.instruction >> 11) & 0x1F;
  emitTrap(ctx, to, fmt::format("{}.s32", ctx.r(ra)), fmt::format("{}.u32", ctx.r(ra)),
           fmt::format("{}.s32", ctx.r(rb)), fmt::format("{}.u32", ctx.r(rb)));
  return true;
}

//=============================================================================
// Cache Operations
//=============================================================================

bool build_dcbf(BuilderContext& ctx) {
  // Hint instruction, access violation callback handlers take care of this on write
  (void)ctx;
  return true;
}

bool build_dcbt(BuilderContext& ctx) {
  // Hint instruction, prefetch has no semantic effect
  (void)ctx;
  return true;
}

bool build_dcbtst(BuilderContext& ctx) {
  // Hint instruction, prefetch-for-store has no semantic effect
  (void)ctx;
  return true;
}

bool build_dcbz(BuilderContext& ctx) {
  // Compute EA, align to 32-byte cache line, apply physical offset
  ctx.print("\t{} = (", ctx.ea());
  if (ctx.insn.operands[0] != 0)
    ctx.print("{}.u32 + ", ctx.r(ctx.insn.operands[0]));
  ctx.println("{}.u32) & ~31;", ctx.r(ctx.insn.operands[1]));
  ctx.println("\tmemset((void*)REX_RAW_ADDR({}), 0, 32);", ctx.ea());
  return true;
}

bool build_dcbzl(BuilderContext& ctx) {
  // Compute EA, align to 128-byte cache line, apply physical offset
  ctx.print("\t{} = (", ctx.ea());
  if (ctx.insn.operands[0] != 0)
    ctx.print("{}.u32 + ", ctx.r(ctx.insn.operands[0]));
  ctx.println("{}.u32) & ~127;", ctx.r(ctx.insn.operands[1]));
  ctx.println("\tmemset((void*)REX_RAW_ADDR({}), 0, 128);", ctx.ea());
  return true;
}

bool build_dcbst(BuilderContext& ctx) {
  // Hint instruction, access violation callback handlers take care of this on write
  (void)ctx;
  return true;
}

//=============================================================================
// Move Register
//=============================================================================

bool build_mr(BuilderContext& ctx) {
  ctx.println("\t{}.u64 = {}.u64;", ctx.r(ctx.insn.operands[0]), ctx.r(ctx.insn.operands[1]));
  emitRecordFormCompare(ctx);

  // Propagates MMIO base flag from source to destination register
  if (ctx.locals.is_mmio_base(ctx.insn.operands[1]))
    ctx.locals.set_mmio_base(ctx.insn.operands[0]);
  else
    ctx.locals.clear_mmio_base(ctx.insn.operands[0]);

  return true;
}

//=============================================================================
// Move Register Field
//=============================================================================

bool build_mcrf(BuilderContext& ctx) {
  // Trivally copy one Control Register Field to another:
  ctx.println("\t{0} = {1};", ctx.cr(ctx.insn.operands[0]), ctx.cr(ctx.insn.operands[1]));
  return true;
}

//=============================================================================
// Move From Special Registers
//=============================================================================

bool build_mfxer(BuilderContext& ctx) {
  ctx.println("\t{}.u64 = ({}.so << 31) | ({}.ov << 30) | ({}.ca << 29);",
              ctx.r(ctx.insn.operands[0]), ctx.xer(), ctx.xer(), ctx.xer());
  return true;
}

bool build_mfctr(BuilderContext& ctx) {
  ctx.println("\t{}.u64 = {}.u64;", ctx.r(ctx.insn.operands[0]), ctx.ctr());
  return true;
}

bool build_mfcr(BuilderContext& ctx) {
  for (size_t i = 0; i < 32; i++) {
    constexpr std::string_view fields[] = {"lt", "gt", "eq", "so"};
    ctx.println("\t{}.u64 {}= {}.{} ? 0x{:X} : 0;", ctx.r(ctx.insn.operands[0]), i == 0 ? "" : "|",
                ctx.cr(i / 4), fields[i % 4], 1u << (31 - i));
  }
  return true;
}

bool build_mfocrf(BuilderContext& ctx) {
  // FXM is a one-hot mask: bit 7 = CR0, bit 6 = CR1, ..., bit 0 = CR7
  uint32_t fxm = ctx.insn.operands[1];
  uint32_t crField = 0;
  for (uint32_t i = 0; i < 8; i++) {
    if (fxm & (0x80u >> i)) {
      crField = i;
      break;
    }
  }
  uint32_t baseShift = 28 - 4 * crField;
  ctx.println("\t{}.u64 = ({}.lt << {}) | ({}.gt << {}) | ({}.eq << {}) | ({}.so << {});",
              ctx.r(ctx.insn.operands[0]), ctx.cr(crField), baseShift + 3, ctx.cr(crField),
              baseShift + 2, ctx.cr(crField), baseShift + 1, ctx.cr(crField), baseShift);
  return true;
}

bool build_mflr(BuilderContext& ctx) {
  if (!ctx.config().skipLr)
    ctx.println("\t{}.u64 = ctx.lr;", ctx.r(ctx.insn.operands[0]));
  return true;
}

bool build_mfmsr(BuilderContext& ctx) {
  if (!ctx.config().skipMsr) {
    // Memory barrier for MSR read
    ctx.println("\tstd::atomic_thread_fence(std::memory_order_seq_cst);");
    // Check global lock and return appropriate value
    // Returns 0x8000 if unlocked (interrupts enabled), 0 if locked
    ctx.println("\t{}.u64 = REX_CHECK_GLOBAL_LOCK();", ctx.r(ctx.insn.operands[0]));
  }
  return true;
}

bool build_mffs(BuilderContext& ctx) {
  ctx.println("\t{}.u64 = ctx.fpscr.loadFromHost();", ctx.f(ctx.insn.operands[0]));
  return true;
}

bool build_mftb(BuilderContext& ctx) {
  // Xbox 360 timebase runs at 50 MHz (guest tick frequency)
  // Using REX_QUERY_TIMEBASE() macro provides properly scaled timing from the runtime
  ctx.println("\t{}.u64 = REX_QUERY_TIMEBASE();", ctx.r(ctx.insn.operands[0]));
  return true;
}

bool build_mftbu(BuilderContext& ctx) {
  // Upper 32 bits of timebase
  ctx.println("\t{}.u64 = REX_QUERY_TIMEBASE() >> 32;", ctx.r(ctx.insn.operands[0]));
  return true;
}

//=============================================================================
// Move To Special Registers
//=============================================================================

bool build_mtcr(BuilderContext& ctx) {
  for (size_t i = 0; i < 32; i++) {
    constexpr std::string_view fields[] = {"lt", "gt", "eq", "so"};
    ctx.println("\t{}.{} = ({}.u32 & 0x{:X}) != 0;", ctx.cr(i / 4), fields[i % 4],
                ctx.r(ctx.insn.operands[0]), 1u << (31 - i));
  }
  return true;
}

bool build_mtcrf(BuilderContext& ctx) {
  uint32_t fxm = ctx.insn.operands[0];
  constexpr std::string_view names[] = {"lt", "gt", "eq", "so"};
  for (uint32_t field = 0; field < 8; field++) {
    if (fxm & (0x80u >> field)) {
      uint32_t base_bit = 28 - 4 * field;
      for (int b = 0; b < 4; b++) {
        ctx.println("\t{}.{} = ({}.u32 & 0x{:X}) != 0;", ctx.cr(field), names[b],
                    ctx.r(ctx.insn.operands[1]), 1u << (base_bit + 3 - b));
      }
    }
  }
  return true;
}

bool build_mtctr(BuilderContext& ctx) {
  ctx.println("\t{}.u64 = {}.u64;", ctx.ctr(), ctx.r(ctx.insn.operands[0]));
  return true;
}

bool build_mtlr(BuilderContext& ctx) {
  if (!ctx.config().skipLr)
    ctx.println("\tctx.lr = {}.u64;", ctx.r(ctx.insn.operands[0]));
  return true;
}

bool build_mtmsrd(BuilderContext& ctx) {
  if (!ctx.config().skipMsr) {
    // Memory barrier for MSR write
    ctx.println("\tstd::atomic_thread_fence(std::memory_order_seq_cst);");
    // Update MSR bits
    ctx.println("\tctx.msr = ({}.u32 & 0x8020) | (ctx.msr & ~0x8020);",
                ctx.r(ctx.insn.operands[0]));
    // Global lock mechanism:
    // R13 = enter lock (disable interrupts)
    // Other = leave lock (enable interrupts)
    // Except around a lone lwarx/stwcx. atomic, which needs no lock (see
    // AnalyzeInterlockedRegion).
    uint32_t src_reg = ctx.insn.operands[0];
    if (src_reg == 13) {
      if (AnalyzeInterlockedRegion(ctx, ctx.base).lockless) {
        ctx.println("\t// interlocked operation: host atomics, no global lock");
      } else {
        ctx.println("\tREX_ENTER_GLOBAL_LOCK();");
      }
    } else {
      if (IsLocklessInterlockedExit(ctx, ctx.base)) {
        ctx.println("\t// end of interlocked operation");
      } else {
        ctx.println("\tREX_LEAVE_GLOBAL_LOCK();");
      }
    }
  }
  return true;
}

bool build_mtfsf(BuilderContext& ctx) {
  uint32_t fm = ctx.insn.operands[0];
  uint32_t mask = 0;
  for (int j = 0; j < 8; j++) {
    if (fm & (1 << (7 - j)))
      mask |= 0xF << (4 * j);
  }
  if (mask == 0xFFFFFFFF) {
    ctx.println("\tctx.fpscr.storeFromGuest({}.u32);", ctx.f(ctx.insn.operands[1]));
  } else {
    ctx.println(
        "\tctx.fpscr.storeFromGuest((ctx.fpscr.loadFromHost() & 0x{:08X}) | ({}.u32 & 0x{:08X}));",
        ~mask, ctx.f(ctx.insn.operands[1]), mask);
  }
  return true;
}

bool build_mtxer(BuilderContext& ctx) {
  ctx.println("\t{}.so = ({}.u64 & 0x80000000) != 0;", ctx.xer(), ctx.r(ctx.insn.operands[0]));
  ctx.println("\t{}.ov = ({}.u64 & 0x40000000) != 0;", ctx.xer(), ctx.r(ctx.insn.operands[0]));
  ctx.println("\t{}.ca = ({}.u64 & 0x20000000) != 0;", ctx.xer(), ctx.r(ctx.insn.operands[0]));
  return true;
}

//=============================================================================
// Clear Left Double Word Immediate
//=============================================================================

bool build_clrldi(BuilderContext& ctx) {
  ctx.println("\t{}.u64 = {}.u64 & 0x{:X};", ctx.r(ctx.insn.operands[0]),
              ctx.r(ctx.insn.operands[1]), (1ull << (64 - ctx.insn.operands[2])) - 1);
  emitRecordFormCompare(ctx);
  return true;
}

}  // namespace rex::codegen
