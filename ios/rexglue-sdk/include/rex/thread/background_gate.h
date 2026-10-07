#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>

namespace rex::thread {
// Lifecycle pause is a cancellable request, independent of save-state fences.
// The UI never waits for a guest callback to return. A resume that arrives
// before the worker parks cancels the request rather than losing a wakeup.
class BackgroundGate {
 public:
  void Request(bool requested) {
    std::lock_guard<std::mutex> lock(mutex_);
    requested_.store(requested, std::memory_order_release);
    cv_.notify_all();
  }
  bool requested() const { return requested_.load(std::memory_order_acquire); }
  template <typename Prepare> bool Checkpoint(Prepare prepare) {
    if (!requested()) return false;
    prepare();
    std::unique_lock<std::mutex> lock(mutex_);
    quiescent_ = true;
    cv_.notify_all();
    cv_.wait(lock, [this] { return !requested(); });
    quiescent_ = false;
    return true;
  }
  bool WaitForQuiescence(std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock(mutex_);
    return cv_.wait_for(lock, timeout, [this] { return !requested() || quiescent_; });
  }
 private:
  std::atomic<bool> requested_{false};
  std::mutex mutex_;
  std::condition_variable cv_;
  bool quiescent_ = false;
};
}  // namespace rex::thread
