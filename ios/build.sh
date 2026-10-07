#!/usr/bin/env bash
# Build the iOS runtime, install its SDK, build the app and package a signer IPA.
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
SDK="$HERE/rexglue-sdk"
APP="$HERE/simpsons"
JOBS="${JOBS:-2}"
case "$JOBS" in ''|*[!0-9]*|0) echo 'JOBS must be a positive integer.' >&2; exit 1;; esac
[ "$(uname -s)" = Darwin ] || { echo 'Build on macOS with Xcode installed.' >&2; exit 1; }
for tool in cmake ninja python3 xcrun codesign zip; do
  command -v "$tool" >/dev/null || { echo "Required tool missing: $tool" >&2; exit 1; }
done
xcrun --sdk iphoneos --show-sdk-path >/dev/null
export CPM_SOURCE_CACHE="${CPM_SOURCE_CACHE:-$SDK/out/deps}"
(
  cd "$SDK"
  cmake --preset ios-arm64 -DREXGLUE_REGISTER_PACKAGE=OFF
  cmake --build --preset ios-arm64-release --parallel "$JOBS"
  cmake --install out/build/ios-arm64 --config Release
)
export REXGLUE_IOS_SDK="$SDK/out/install/ios-arm64"
(
  cd "$APP"
  cmake --preset ios-arm64-release -DSIMPSONS_REGENERATE_CODE=OFF
  cmake --build --preset ios-arm64-release --parallel "$JOBS"
  # ESign supplies the main signature; this avoids local codesign failures on
  # the large recompiled executable. Frameworks still get ad hoc signatures.
  ADHOC_MAIN_SIGN="${ADHOC_MAIN_SIGN:-0}" bash ios/make-ipa.sh
)
echo "IPA ready: $APP/out/TheSimpsonsGame.ipa"
