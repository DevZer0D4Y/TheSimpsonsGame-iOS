// Draw-submission telemetry for the two guest D3D draw entry points.
//
// sub_82462810 and sub_824646E8 are the only two functions in the whole
// recompiled image that encode PM4 DRAW_INDX packets (verified by scanning
// for the opcode immediate) -- every draw the game issues flows through
// them, reached via the D3D device vtable. This file overrides both through
// the DEFINE_REX_FUNC weak-alias mechanism (a strong definition here wins
// over the generated weak one; __imp__* is the original body) to record how
// batchable the draw stream actually is BEFORE committing to a draw-merging
// hook: call volume, primitive-type mix, index-count distribution, and --
// the number that decides everything -- how long runs of consecutive draws
// with the same primitive type are.
//
// Behavior is byte-identical to the unhooked game: every call is forwarded
// to the original body with untouched registers; the only added work is a
// handful of relaxed atomics per draw and one log line every N draws.
//
// From the PM4 encoding in sub_82462810: r4 low 6 bits = primitive type,
// r5 low 16 bits = index/vertex count (packed into VGT_DRAW_INITIATOR as
// (count << 16) | prim).

#include <array>
#include <atomic>
#include <cstdint>

#include <rex/logging.h>
#include <rex/ppc.h>

REX_EXTERN(__imp__sub_82462810);
REX_EXTERN(__imp__sub_824646E8);
// Prior extern "C" declarations so the strong definitions below get C
// linkage, matching the generated weak symbols they override.
REX_EXTERN(sub_82462810);
REX_EXTERN(sub_824646E8);

namespace {

// The menu scene alone issues ~3.7k draws/frame (~170k/sec at 45fps), so
// keep the report interval high enough that the log stays readable.
constexpr uint32_t kReportEveryDraws = 200000;

std::atomic<uint64_t> g_total_calls{0};
std::atomic<uint64_t> g_calls_a{0};  // sub_82462810
std::atomic<uint64_t> g_calls_b{0};  // sub_824646E8

// Primitive types are 6 bits (xenos::PrimitiveType, 0..63).
std::array<std::atomic<uint64_t>, 64> g_prim_counts{};

// Index-count buckets: <=4, <=16, <=64, <=256, <=1024, >1024.
std::array<std::atomic<uint64_t>, 6> g_count_buckets{};

// Run-length tracking: consecutive calls with the same primitive type.
// The game submits draws from its render thread, so plain "last seen" state
// with relaxed atomics is accurate enough for telemetry purposes.
std::atomic<uint32_t> g_last_prim{UINT32_MAX};
std::atomic<uint64_t> g_current_run{0};
// Run-length buckets: 1, 2-4, 5-16, 17-64, >64.
std::array<std::atomic<uint64_t>, 5> g_run_buckets{};

inline size_t CountBucket(uint32_t count) {
  if (count <= 4) return 0;
  if (count <= 16) return 1;
  if (count <= 64) return 2;
  if (count <= 256) return 3;
  if (count <= 1024) return 4;
  return 5;
}

inline size_t RunBucket(uint64_t run) {
  if (run <= 1) return 0;
  if (run <= 4) return 1;
  if (run <= 16) return 2;
  if (run <= 64) return 3;
  return 4;
}

void FlushRun() {
  uint64_t run = g_current_run.exchange(0, std::memory_order_relaxed);
  if (run) {
    g_run_buckets[RunBucket(run)].fetch_add(1, std::memory_order_relaxed);
  }
}

void RecordDraw(uint32_t prim, uint32_t count) {
  g_prim_counts[prim & 63].fetch_add(1, std::memory_order_relaxed);
  g_count_buckets[CountBucket(count)].fetch_add(1, std::memory_order_relaxed);

  uint32_t last = g_last_prim.exchange(prim, std::memory_order_relaxed);
  if (prim == last) {
    g_current_run.fetch_add(1, std::memory_order_relaxed);
  } else {
    FlushRun();
    g_current_run.store(1, std::memory_order_relaxed);
  }

  uint64_t total = g_total_calls.fetch_add(1, std::memory_order_relaxed) + 1;
  if (total % kReportEveryDraws == 0) {
    // Snapshot (relaxed reads; telemetry precision, not accounting).
    uint64_t a = g_calls_a.load(std::memory_order_relaxed);
    uint64_t b = g_calls_b.load(std::memory_order_relaxed);
    // Top primitive types.
    uint32_t top_prim[3] = {64, 64, 64};
    uint64_t top_n[3] = {0, 0, 0};
    for (uint32_t p = 0; p < 64; ++p) {
      uint64_t n = g_prim_counts[p].load(std::memory_order_relaxed);
      for (int s = 0; s < 3; ++s) {
        if (n > top_n[s]) {
          for (int t = 2; t > s; --t) {
            top_n[t] = top_n[t - 1];
            top_prim[t] = top_prim[t - 1];
          }
          top_n[s] = n;
          top_prim[s] = p;
          break;
        }
      }
    }
    REXGPU_INFO(
        "[draw-telemetry] total={} (A={} B={}) prims: [{}]x{} [{}]x{} [{}]x{} "
        "counts(<=4/<=16/<=64/<=256/<=1024/>1024): {}/{}/{}/{}/{}/{} "
        "runs(1/2-4/5-16/17-64/>64): {}/{}/{}/{}/{}",
        total, a, b, top_prim[0], top_n[0], top_prim[1], top_n[1], top_prim[2], top_n[2],
        g_count_buckets[0].load(std::memory_order_relaxed),
        g_count_buckets[1].load(std::memory_order_relaxed),
        g_count_buckets[2].load(std::memory_order_relaxed),
        g_count_buckets[3].load(std::memory_order_relaxed),
        g_count_buckets[4].load(std::memory_order_relaxed),
        g_count_buckets[5].load(std::memory_order_relaxed),
        g_run_buckets[0].load(std::memory_order_relaxed),
        g_run_buckets[1].load(std::memory_order_relaxed),
        g_run_buckets[2].load(std::memory_order_relaxed),
        g_run_buckets[3].load(std::memory_order_relaxed),
        g_run_buckets[4].load(std::memory_order_relaxed));
  }
}

}  // namespace

// Strong overrides of the generated weak definitions. Registers are passed
// through to the original bodies untouched.

REX_FUNC(sub_82462810) {
  g_calls_a.fetch_add(1, std::memory_order_relaxed);
  RecordDraw(ctx.r4.u32 & 63, ctx.r5.u32 & 0xFFFF);
  __imp__sub_82462810(ctx, base);
}

REX_FUNC(sub_824646E8) {
  g_calls_b.fetch_add(1, std::memory_order_relaxed);
  RecordDraw(ctx.r4.u32 & 63, ctx.r5.u32 & 0xFFFF);
  __imp__sub_824646E8(ctx, base);
}
