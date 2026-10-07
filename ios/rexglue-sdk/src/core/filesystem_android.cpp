/**
 * @file        rex/core/filesystem_android.cpp
 * @brief       Android filesystem entry points declared in rex/filesystem.h
 *
 * @license     BSD 3-Clause License
 *
 * @remarks     Inherited from Xenia, whose Android build resolved content://
 *              URIs through JNI (ContentResolver.openFileDescriptor). ReXGlue
 *              has no JNI bridge, so content URIs are reported as unsupported
 *              and callers fall back to ordinary paths --
 *              MappedMemory::OpenForAndroidContentUri already returns nullptr on
 *              a negative descriptor. Ordinary filesystem access is unaffected:
 *              it goes through filesystem_posix.cpp like every other POSIX
 *              target. Wire a JNI bridge here if the app is ever handed content
 *              URIs (e.g. a document picker) rather than plain paths.
 */

#include <rex/platform.h>
#if REX_PLATFORM_ANDROID

#include <rex/filesystem.h>
#include <rex/logging.h>

namespace rex {
namespace filesystem {

void AndroidInitialize() {}

void AndroidShutdown() {}

bool IsAndroidContentUri(const std::string_view source) {
  // Scheme test only; recognising the form costs nothing and lets callers
  // report something better than "file not found".
  return source.starts_with("content://");
}

int OpenAndroidContentFileDescriptor(const std::string_view uri, const char* mode) {
  (void)mode;
  REXLOG_ERROR("content:// URIs need a JNI ContentResolver bridge, which this build does not have: {}",
               uri);
  return -1;
}

}  // namespace filesystem
}  // namespace rex

#endif  // REX_PLATFORM_ANDROID
