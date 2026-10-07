#!/usr/bin/env bash
# Sign the built simpsons.app with a development profile and install it on a
# connected device.
#
#   PROFILE   .mobileprovision covering the bundle ID (default: newest one whose
#             application-identifier ends in .com.devz.simpsons)
#   IDENTITY  codesigning identity (default: first "Apple Development")
#   DEVICE    device id from `xcrun devicectl list devices` (default: first connected)
#   APP       built bundle (default: app/out/build/ios-arm64-release/simpsons.app)
#   NO_INSTALL=1  sign only
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
APP="${APP:-$HERE/../out/build/ios-arm64-release/simpsons.app}"
BUNDLE_ID="$(/usr/libexec/PlistBuddy -c 'Print :CFBundleIdentifier' "$APP/Info.plist")"
WORK="$(mktemp -d)"; trap 'rm -rf "$WORK"' EXIT

decode_profile() {
  security cms -D -i "$1" > "$2" 2>/dev/null ||
    openssl cms -verify -inform DER -in "$1" -noverify -out "$2" 2>/dev/null
}

if [ -z "${PROFILE:-}" ]; then
  while IFS= read -r f; do
    decode_profile "$f" "$WORK/p.plist" || continue
    id="$(/usr/libexec/PlistBuddy -c 'Print :Entitlements:application-identifier' "$WORK/p.plist")"
    if [ "${id#*.}" = "$BUNDLE_ID" ]; then PROFILE="$f"; break; fi
  done < <(ls -t "$HOME/Library/Developer/Xcode/UserData/Provisioning Profiles/"*.mobileprovision)
fi
: "${PROFILE:?No provisioning profile for $BUNDLE_ID; set PROFILE}"
IDENTITY="${IDENTITY:-$(security find-identity -v -p codesigning | grep -o '"Apple Development:[^"]*"' | head -1 | tr -d '"')}"

decode_profile "$PROFILE" "$WORK/profile.plist"
/usr/libexec/PlistBuddy -x -c "Print :Entitlements" "$WORK/profile.plist" > "$WORK/ent.plist"
cp "$PROFILE" "$APP/embedded.mobileprovision"
for d in "$APP"/Frameworks/*.dylib; do codesign -f -s "$IDENTITY" "$d"; done
codesign -f -s "$IDENTITY" --entitlements "$WORK/ent.plist" "$APP"

[ "${NO_INSTALL:-0}" = 1 ] && exit 0
if [ -z "${DEVICE:-}" ]; then
  DEVICE="$(xcrun devicectl list devices 2>/dev/null | awk 'NR>2 && /connected|available \(paired\)/ {for(i=1;i<=NF;i++) if ($i ~ /^[0-9A-F-]{25,}$/) {print $i; exit}}')"
fi
xcrun devicectl device install app --device "$DEVICE" "$APP"
