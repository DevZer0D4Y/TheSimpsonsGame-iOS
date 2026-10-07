#include <rex/thread/background_gate.h>
#include <atomic>
#include <cassert>
#include <chrono>
#include <future>
#include <iostream>
#include <thread>
using namespace std::chrono_literals;
using rex::thread::BackgroundGate;

static bool WaitFor(const std::atomic<unsigned>& value, unsigned target) {
  const auto end = std::chrono::steady_clock::now() + 1s;
  while (value.load() < target && std::chrono::steady_clock::now() < end) std::this_thread::yield();
  return value.load() >= target;
}
int main() {
  // An uninterrupted stream of guest work must still honor each pause.
  BackgroundGate gate;
  std::atomic<bool> running{true};
  std::atomic<unsigned> progress{0}, drained{0};
  std::thread worker([&] {
    while (running) {
      gate.Checkpoint([&] { ++drained; });
      ++progress;
    }
  });
  for (unsigned i = 0; i < 100; ++i) {
    gate.Request(true);
    assert(gate.WaitForQuiescence(1s));
    auto frozen = progress.load();
    std::this_thread::sleep_for(1ms);
    assert(progress.load() == frozen);
    gate.Request(false);
    assert(WaitFor(progress, frozen + 1));
  }
  gate.Request(true);
  assert(gate.WaitForQuiescence(1s));
  running = false;
  gate.Request(false); // Shutdown wakes a parked worker.
  worker.join();
  assert(drained >= 100);

  // Foreground can arrive before a delayed worker even sees the pause.
  gate.Request(true);
  gate.Request(false);
  assert(!gate.Checkpoint([] { std::abort(); }));

  // A slow GPU drain may not block UIKit indefinitely; cancellation during
  // that drain must release the worker without requiring another resume.
  std::promise<void> entered, finish;
  auto release = finish.get_future();
  auto started = entered.get_future();
  gate.Request(true);
  auto task = std::async(std::launch::async, [&] {
    gate.Checkpoint([&] { entered.set_value(); release.wait(); });
  });
  assert(started.wait_for(1s) == std::future_status::ready);
  auto begin = std::chrono::steady_clock::now();
  assert(!gate.WaitForQuiescence(25ms));
  assert(std::chrono::steady_clock::now() - begin < 500ms);
  gate.Request(false);
  finish.set_value();
  assert(task.wait_for(1s) == std::future_status::ready);
  task.get();
  std::cout << "PASS: 100 pause/resume cycles under continuous work, shutdown while paused, early resume, bounded drain wait, cancellation during drain\n";
}
