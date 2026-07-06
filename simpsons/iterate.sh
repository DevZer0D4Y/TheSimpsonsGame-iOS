#!/bin/bash
# Autonomous iteration: codegen -> build -> boot-test (with trap interposer to
# enumerate any newly-revealed missing functions) -> report.
set +e
ROOT=/home/deck/simpsons-recomp
export PATH=$ROOT/tools/clang20/bin:$PATH
export LD_LIBRARY_PATH=$ROOT/tools/rexglue-bin/linux-amd64/lib:/home/.steamos/offload/nix/store/ah4525ca553drv47jhvgpl9sl87i7a1d-libxml2-2.13.8/lib:$LD_LIBRARY_PATH
BUILD=$ROOT/simpsons/out/build/linux-amd64-relwithdebinfo
GR=$ROOT/gamedata
cd $ROOT/simpsons

B=$ROOT/tools/rexglue-bin/linux-amd64/bin/rexglue
echo "[iterate] running codegen --force..."
"$B" -f codegen --force > codegen_iter.log 2>&1
echo "[iterate] codegen done emitted=$(find generated/default -name '*.cpp'|wc -l)"

echo "[iterate] building..."
ninja -C $BUILD -j3 -k 0 > build_iter.log 2>&1
echo "[iterate] build exit=$? errors=$(grep -cE 'error:' build_iter.log) exe=$(ls -la $BUILD/simpsons 2>/dev/null|awk '{print $5}')"

echo "[iterate] boot test (60s, trap interposer ON)..."
cd $BUILD
rm -f $ROOT/missing_funcs.log
LD_PRELOAD=$ROOT/tools/libtrap.so SDL_VIDEODRIVER=dummy timeout 60 ./simpsons --game_data_root $GR > boot_iter.log 2>&1
pkill -9 -f "linux-amd64-relwithdebinfo/simpsons" 2>/dev/null
echo "[iterate] boot done"
echo "=== NEW missing functions this pass (declare these next) ==="
sort -u $ROOT/missing_funcs.log 2>/dev/null
echo "=== count: $(sort -u $ROOT/missing_funcs.log 2>/dev/null | wc -l) ==="
LATEST=$(ls -t logs/simpsons_*.log 2>/dev/null | head -1)
echo "=== boot progress tail ==="
grep -vE "cvar: duplicate" "$LATEST" | grep -iE "pipeline|present|frame|render|draw|shader|fatal|error|guest address|launch|swap" | tail -20
echo "[iterate] DONE"
