#!/usr/bin/env bash
# Run against a macOS Release build of the matching SDK.
set -euo pipefail
SDK_SOURCE="${1:?Pass the SDK source directory}"
SDK_INSTALL="${2:?Pass the mac-arm64 SDK install directory}"
LIB_DIR="${3:-$SDK_SOURCE/out/mac-arm64/Release}"
HERE="$(cd "$(dirname "$0")" && pwd)"
TEST_WORK="$(mktemp -d)"
trap 'rm -rf "$TEST_WORK"' EXIT
clang++ -std=c++23 -O2 -DSPDLOG_FMT_EXTERNAL -DSPDLOG_COMPILED_LIB \
  "$HERE/runtime_tests.cpp" -I "$SDK_SOURCE/include" -I "$SDK_INSTALL/include" \
  -L "$LIB_DIR" -lrexruntime -Wl,-rpath,"$LIB_DIR" -o "$TEST_WORK/runtime_tests"
"$TEST_WORK/runtime_tests" "$TEST_WORK/fixtures"

clang++ -std=c++23 -O2 -DSPDLOG_FMT_EXTERNAL -DSPDLOG_COMPILED_LIB \
  "$HERE/touch_layout_tests.cpp" -I "$SDK_SOURCE/include" -I "$SDK_INSTALL/include" \
  -L "$LIB_DIR" -lrexruntime -Wl,-rpath,"$LIB_DIR" -o "$TEST_WORK/touch_layout_tests"
"$TEST_WORK/touch_layout_tests"

clang++ -std=c++23 -O2 -pthread -I "$SDK_SOURCE/include" \
  "$HERE/background_gate_tests.cpp" -o "$TEST_WORK/background_gate_tests"
"$TEST_WORK/background_gate_tests"

clang++ -std=c++23 -O2 -fsanitize=address,undefined -I "$SDK_SOURCE/include" \
  "$HERE/upload_order_tests.cpp" -o "$TEST_WORK/upload_order_tests"
"$TEST_WORK/upload_order_tests"

clang++ -std=c++23 -O2 -DSPDLOG_FMT_EXTERNAL -DSPDLOG_COMPILED_LIB \
  "$HERE/persistent_cache_tests.cpp" -I "$SDK_SOURCE/include" -I "$SDK_INSTALL/include" \
  -L "$LIB_DIR" -lrexruntime -Wl,-rpath,"$LIB_DIR" -o "$TEST_WORK/persistent_cache_tests"
"$TEST_WORK/persistent_cache_tests" "$TEST_WORK/cache-fixtures"

cmake -DSDK_SOURCE="$SDK_SOURCE" -DTEST_WORK="$TEST_WORK/version-fixtures" \
  -P "$HERE/source_archive_tests.cmake"
