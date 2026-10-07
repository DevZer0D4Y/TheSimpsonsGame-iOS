/**
 * @file        rex/ui/surface_android.h
 * @brief       ANativeWindow-backed Surface for Android
 *
 * @license     BSD 3-Clause License
 *
 * @remarks     vulkan_presenter.cpp already handles
 *              Surface::kTypeIndex_AndroidNativeWindow and calls .window() to
 *              fill VkAndroidSurfaceCreateInfoKHR; only the class itself was
 *              missing.
 */

#pragma once

#include <rex/platform.h>

#if REX_PLATFORM_ANDROID

#include <android/native_window.h>

#include <rex/ui/surface.h>

namespace rex {
namespace ui {

class AndroidNativeWindowSurface final : public Surface {
 public:
  explicit AndroidNativeWindowSurface(ANativeWindow* window) : window_(window) {}

  TypeIndex GetType() const override { return kTypeIndex_AndroidNativeWindow; }
  ANativeWindow* window() const { return window_; }

 protected:
  bool GetSizeImpl(uint32_t& width_out, uint32_t& height_out) const override {
    if (!window_) {
      return false;
    }
    const int32_t width = ANativeWindow_getWidth(window_);
    const int32_t height = ANativeWindow_getHeight(window_);
    if (width <= 0 || height <= 0) {
      return false;
    }
    width_out = static_cast<uint32_t>(width);
    height_out = static_cast<uint32_t>(height);
    return true;
  }

 private:
  ANativeWindow* window_;
};

}  // namespace ui
}  // namespace rex

#endif  // REX_PLATFORM_ANDROID
