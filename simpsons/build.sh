#!/usr/bin/env bash
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
export PATH="$HERE/../tools/clang20/bin:$PATH"
cd "$HERE/out/build/linux-amd64-relwithdebinfo"
exec ninja -j"${JOBS:-3}" -k 0
