/**
 * @file        system/ring_progress.cpp
 * @brief       Command ring progress notifications (see rex/graphics/ring_progress.h).
 */

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>

#include <rex/graphics/ring_progress.h>

namespace rex::graphics {

namespace {
// The mutex and condition variable are only touched when someone waits.
std::atomic<uint32_t> g_writebacks{0};
std::atomic<uint32_t> g_waiters{0};
std::atomic<uint32_t> g_read_pointer_writeback_address{0};
std::mutex g_mutex;
std::condition_variable g_condition;
}  // namespace

void NotifyRingProgress() {
  g_writebacks.fetch_add(1, std::memory_order_seq_cst);
  if (g_waiters.load(std::memory_order_seq_cst)) {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_condition.notify_all();
  }
}

uint32_t GetRingProgressCount() {
  return g_writebacks.load(std::memory_order_seq_cst);
}

void WaitForRingProgress(uint32_t seen_count, uint32_t timeout_us) {
  // Registered before checking the count, and the notifier increments the
  // count before checking for waiters, so a writeback can't be missed.
  g_waiters.fetch_add(1, std::memory_order_seq_cst);
  {
    std::unique_lock<std::mutex> lock(g_mutex);
    g_condition.wait_for(lock, std::chrono::microseconds(timeout_us), [&] {
      return g_writebacks.load(std::memory_order_seq_cst) != seen_count;
    });
  }
  g_waiters.fetch_sub(1, std::memory_order_seq_cst);
}

uint32_t GetReadPointerWritebackAddress() {
  return g_read_pointer_writeback_address.load(std::memory_order_relaxed);
}

void SetReadPointerWritebackAddress(uint32_t address) {
  g_read_pointer_writeback_address.store(address, std::memory_order_relaxed);
}

}  // namespace rex::graphics
