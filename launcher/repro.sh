#!/bin/sh
# Autonomous crash-repro rig: boots the game with keyboard input enabled,
# drives the menu to Continue -> save load, and reports the outcome.
# Runs the game the same way the launcher does (host, systemd scope) so the
# environment matches user launches. Usage: repro.sh [extra RADV_DEBUG opts]
set -u
ROOT=/home/deck/simpsons-recomp
BUILD=$ROOT/simpsons/out/build/linux-amd64-relwithdebinfo
export LD_LIBRARY_PATH=$ROOT/tools/rexglue-bin/linux-amd64/lib:/home/.steamos/offload/nix/store/ah4525ca553drv47jhvgpl9sl87i7a1d-libxml2-2.13.8/lib
export DISABLE_LSFG=1
export RADV_DEBUG="${1:-hang}"
export DISPLAY=${DISPLAY:-:0}
cd "$BUILD"
./simpsons --game_data_root "$ROOT/gamedata" >/dev/null 2>&1 &
GAME=$!
echo "game pid $GAME (RADV_DEBUG=$RADV_DEBUG)"

key() {  # key <keysym> [repeats]
  n=${2:-1}
  i=0
  while [ $i -lt $n ]; do
    xdotool key --window "$WIN" "$1" 2>/dev/null || xdotool key "$1"
    i=$((i+1))
    sleep 0.6
  done
}

# wait for the game window
WIN=""
t=0
while [ -z "$WIN" ] && [ $t -lt 60 ]; do
  sleep 2; t=$((t+2))
  WIN=$(xdotool search --name "Simpsons|simpsons|Xenia" 2>/dev/null | head -1)
done
[ -z "$WIN" ] && { echo "NO WINDOW"; kill -9 $GAME 2>/dev/null; exit 2; }
xdotool windowactivate "$WIN" 2>/dev/null
echo "window $WIN"

# boot -> title -> menu -> continue -> confirm slot
sleep 40            # boot + shader warmup + attract
key Escape 2        # start past title
sleep 5
key space 2         # enter menu / select CONTINUE GAME (first item)
sleep 4
key space 1         # confirm
sleep 3
key Down 2          # move to the populated save slot if needed
key space 2         # load it (double-press to clear any prompt)

# watch for up to 120s: crash, hang dump, or survival
t=0
while [ $t -lt ${WATCH:-120} ]; do
  sleep 5; t=$((t+5))
  if ! kill -0 $GAME 2>/dev/null; then
    echo "OUTCOME: game exited at t=$t"
    exit 1
  fi
done
echo "OUTCOME: ALIVE after load window"
kill -9 $GAME 2>/dev/null
exit 0
