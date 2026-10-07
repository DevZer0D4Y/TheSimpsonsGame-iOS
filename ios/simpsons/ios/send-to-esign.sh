#!/usr/bin/env bash
# Build the IPA and copy it into ESign's Documents folder on the phone (the
# "ESign" folder in the Files app), where ESign can sign and install it.
#
#   DEVICE  device id from `xcrun devicectl list devices` (default: first connected)
#   ESIGN   ESign's bundle id (default: p3.xyz.yyyue.esign)
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
ESIGN="${ESIGN:-p3.xyz.yyyue.esign}"
IPA="$HERE/../out/TheSimpsonsGame.ipa"
OUT="$IPA" bash "$HERE/make-ipa.sh"
if [ -z "${DEVICE:-}" ]; then
  DEVICE="$(xcrun devicectl list devices 2>/dev/null | awk 'NR>2 && /connected|available \(paired\)/ {for(i=1;i<=NF;i++) if ($i ~ /^[0-9A-F-]{25,}$/) {print $i; exit}}')"
fi
xcrun devicectl device copy to --device "$DEVICE" --domain-type appDataContainer \
  --domain-identifier "$ESIGN" --source "$IPA" --destination "Documents/TheSimpsonsGame.ipa"
echo "Copied to ESign: Documents/TheSimpsonsGame.ipa"
