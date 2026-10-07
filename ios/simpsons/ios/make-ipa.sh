#!/usr/bin/env bash
# Package the built app as an unsigned .ipa for a sideloading signer (ESign,
# AltStore, SideStore, Sideloadly, TrollStore), which signs it with its own
# certificate.
#
#   APP   built bundle (default: out/build/ios-arm64-release/simpsons.app)
#   OUT   output path (default: out/TheSimpsonsGame.ipa)
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
APP="${APP:-$HERE/../out/build/ios-arm64-release/simpsons.app}"
OUT="${OUT:-$HERE/../out/TheSimpsonsGame.ipa}"

[ -d "$APP" ] || { echo "Built app not found at $APP - run the build first." >&2; exit 1; }

STAGE="$(mktemp -d)"
trap 'rm -rf "$STAGE"' EXIT
mkdir -p "$STAGE/Payload"
# Clone the local APFS bundle to avoid duplicating the large executable.
# Fall back on filesystems that do not support copy-on-write clones.
if ! cp -cR "$APP" "$STAGE/Payload/" 2>/dev/null; then
  cp -R "$APP" "$STAGE/Payload/"
fi
BUNDLE="$STAGE/Payload/$(basename "$APP")"

# Strip anything that identifies the builder: a provisioning profile lists the
# team, name and device UDIDs, and a signature names the certificate.
rm -f "$BUNDLE/embedded.mobileprovision"
rm -rf "$BUNDLE/_CodeSignature"

# Ad hoc signature: names no certificate or team; the signer replaces it.
for d in "$BUNDLE"/Frameworks/*.dylib; do codesign -f -s - "$d"; done
# ESign can sign a main executable without a local ad hoc signature.
# ADHOC_MAIN_SIGN=0 is useful when local codesign cannot process the guest binary.
if [ "${ADHOC_MAIN_SIGN:-1}" = 1 ]; then
  codesign -f -s - "$BUNDLE"
fi

mkdir -p "$(dirname "$OUT")"
rm -f "$OUT"
(cd "$STAGE" && zip -qry "$OUT" Payload)
echo "Wrote $OUT"
