#!/usr/bin/env bash
# Unattended run on a phone: launch with a scripted gamepad and periodic
# screenshots (src/test_harness.h), wait, pull the log and the screenshots,
# then stop the app.
#
#   device-test.sh <seconds> [autoplay script] [shot interval]
#   DEVICE  device id (default: first connected)
#   BUNDLE  app bundle id (default: com.devz.simpsons)
#   OUT     where results go (default: ../../device_runs/<time>)
#   EXTRA   more environment for the app, "KEY=value KEY2=value"
set -uo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
secs=${1:?seconds}
script=${2:-}
shots=${3:-0}
BUNDLE="${BUNDLE:-com.devz.simpsons}"
if [ -z "${DEVICE:-}" ]; then
  DEVICE="$(xcrun devicectl list devices 2>/dev/null | awk 'NR>2 && /connected|available \(paired\)/ {for(i=1;i<=NF;i++) if ($i ~ /^[0-9A-F-]{25,}$/) {print $i; exit}}')"
fi
OUT="${OUT:-$HERE/../../device_runs/$(date +%H%M%S)}"
mkdir -p "$OUT"

env_json=$(python3 -c 'import json,sys
e={}
if sys.argv[1]: e["SIMPSONS_AUTOPLAY"]=sys.argv[1]
if sys.argv[2] not in ("", "0"): e["SIMPSONS_SHOTS"]=sys.argv[2]
for kv in sys.argv[3].split():
    k,_,v=kv.partition("="); e[k]=v
print(json.dumps(e))' "$script" "$shots" "${EXTRA:-}")

files() {
  xcrun devicectl device info files --device "$DEVICE" --domain-type appDataContainer \
    --domain-identifier "$BUNDLE" 2>/dev/null
}
pull() {
  xcrun devicectl device copy from --device "$DEVICE" --domain-type appDataContainer \
    --domain-identifier "$BUNDLE" --source "$1" --destination "$2" >/dev/null 2>&1
}

before=$(files | grep -oE "Documents/userdata/logs/[^ ]+\.log" | sort | tail -1)
xcrun devicectl device process launch --device "$DEVICE" --terminate-existing -e "$env_json" \
  "$BUNDLE" >/dev/null 2>&1 || { echo "launch failed"; exit 1; }
echo "launched with $env_json; running ${secs}s"
sleep "$secs"
xcrun devicectl device capture screenshot --device "$DEVICE" --destination "$OUT/screen.png" \
  >/dev/null 2>&1 || true
pid=$(xcrun devicectl device info processes --device "$DEVICE" 2>/dev/null |
      grep "simpsons.app/simpsons" | awk '{print $1}')
if [ -n "$pid" ]; then
  xcrun devicectl device process terminate --device "$DEVICE" --pid "$pid" >/dev/null 2>&1
  sleep 2
else
  echo "the app was no longer running"
fi
listing=$(files)
log=$(echo "$listing" | grep -oE "Documents/userdata/logs/[^ ]+\.log" | sort | tail -1)
[ "$log" != "$before" ] || echo "warning: no new log (did the app start?)"
pull "$log" "$OUT/simpsons.log"
for s in $(echo "$listing" | grep -oE "Documents/userdata/shots/shot_[0-9]+\.ppm"); do
  pull "$s" "$OUT/$(basename "$s")"
done
echo "results in $OUT"
python3 "$HERE/../../run/summary.py" "$OUT/simpsons.log" 2>/dev/null || true
