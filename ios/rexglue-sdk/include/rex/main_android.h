/**
 * @file        rex/main_android.h
 * @brief       Android runtime environment queries
 *
 * @license     BSD 3-Clause License
 *
 * @remarks     threading_posix.cpp and memory_posix.cpp carry Android blocks
 *              inherited from Xenia that gate dlsym lookups on the device API
 *              level (pthread_getname_np and ASharedMemory_create are both
 *              API 26+). Xenia sourced this from its own JNI bootstrap; the NDK
 *              exposes it directly from API 24 on, so no JNI is involved.
 */

#pragma once

#include <rex/platform.h>

#if REX_PLATFORM_ANDROID

#include <android/api-level.h>

namespace rex {

/// API level of the device the process is running on, or -1 if unavailable.
/// This is the *device* level, not the compile-time __ANDROID_API__ target.
inline int GetAndroidApiLevel() {
  return android_get_device_api_level();
}

}  // namespace rex

#endif  // REX_PLATFORM_ANDROID
