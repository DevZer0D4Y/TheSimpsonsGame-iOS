/**
 * @file        rex/core/fiber_android.cpp
 * @brief       Android backend for rex::thread::Fiber
 *
 * @license     BSD 3-Clause License
 *
 * @remarks     bionic ships no getcontext/makecontext/swapcontext, so the
 *              switch itself is hand-written (core/fiber_aarch64.S). Semantics
 *              match the POSIX backend exactly, including staying on the same
 *              OS thread across a switch -- XThread keeps per-guest-thread
 *              state in thread_local storage that must survive it.
 */

#include <rex/platform.h>
#if REX_PLATFORM_ANDROID

#include <rex/thread/fiber.h>

#include <cassert>
#include <cstdint>
#include <cstdlib>

namespace rex::thread {

thread_local Fiber* Fiber::tls_current_ = nullptr;

namespace {
// Index of x30 (the link register) within RexFiberContext::gpr.
constexpr size_t kLinkRegisterSlot = 11;
}  // namespace

Fiber* Fiber::ConvertCurrentThread() {
  auto* f = new Fiber();
  // The running thread's context is captured on its first outgoing switch;
  // nothing needs seeding here.
  f->is_thread_fiber_ = true;
  tls_current_ = f;
  return f;
}

Fiber* Fiber::Create(size_t stack_size, void (*entry)(void*), void* arg) {
  auto* f = new Fiber();
  f->entry_ = entry;
  f->arg_ = arg;
  f->stack_.resize(stack_size);

  // AAPCS64 requires a 16-byte aligned stack pointer. Grow down from the top.
  auto top = reinterpret_cast<uintptr_t>(f->stack_.data()) + f->stack_.size();
  top &= ~static_cast<uintptr_t>(15);

  f->context_ = RexFiberContext{};
  f->context_.sp = top;
  // rex_fiber_switch ends in RET, so the first switch to this fiber "returns"
  // into the trampoline. Trampoline reads entry_/arg_ off tls_current_, which
  // SwitchTo sets before switching, so no argument needs to survive the switch.
  f->context_.gpr[kLinkRegisterSlot] = reinterpret_cast<uint64_t>(&Fiber::Trampoline);
  return f;
}

/*static*/ void Fiber::Trampoline() {
  Fiber* f = tls_current_;
  f->entry_(f->arg_);
  // The POSIX backend sets uc_link = nullptr, making a return from entry_
  // terminate the process. Entry points are not expected to return; failing
  // loudly beats returning into an unwound stack.
  assert(false && "fiber entry point returned");
  std::abort();
}

void Fiber::SwitchTo(Fiber* target) {
  Fiber* from = tls_current_;
  tls_current_ = target;
  rex_fiber_switch(&from->context_, &target->context_);
  // Resumed: another fiber switched back to us. tls_current_ was set to this
  // fiber by whoever did so.
}

void Fiber::Destroy() {
  if (is_thread_fiber_) {
    tls_current_ = nullptr;
  } else {
    assert(this != tls_current_ && "Destroy called on the currently running fiber");
  }
  delete this;
}

}  // namespace rex::thread

#endif  // REX_PLATFORM_ANDROID
