#!/usr/bin/env bash
# Copy an extracted game folder into the app's Documents/game on the phone.
#   push-game-data.sh <extracted game folder>
#   DEVICE  device id (default: first connected)
#   BUNDLE  app bundle id (default: com.devz.simpsons)
set -uo pipefail
SRC="${1:?usage: push-game-data.sh <extracted game folder>}"
BUNDLE="${BUNDLE:-com.devz.simpsons}"
if [ -z "${DEVICE:-}" ]; then
  DEVICE="$(xcrun devicectl list devices 2>/dev/null | awk 'NR>2 && /connected|available \(paired\)/ {for(i=1;i<=NF;i++) if ($i ~ /^[0-9A-F-]{25,}$/) {print $i; exit}}')"
fi
fail=0
for item in "$SRC"/*; do
  name="$(basename "$item")"
  # The console dashboard update on the disc is not game data.
  [ "$name" = '$systemupdate' ] && continue
  echo "$(date +%H:%M:%S) copying $name"
  if ! xcrun devicectl device copy to --device "$DEVICE" --domain-type appDataContainer \
      --domain-identifier "$BUNDLE" --source "$item" --destination "Documents/game/$name" \
      >/dev/null 2>"/tmp/push-$$.err"; then
    echo "  FAILED: $(tail -3 /tmp/push-$$.err)"
    fail=1
  fi
done
rm -f "/tmp/push-$$.err"
echo "$(date +%H:%M:%S) done (failures: $fail)"
exit $fail
