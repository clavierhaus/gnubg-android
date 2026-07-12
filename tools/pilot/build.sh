#!/bin/sh
# Build the §9.4 pilot harness against THIS repository's engine-core.
# Needs: gcc, libglib2.0-dev. Output: tools/pilot/inputs_harness
set -e
cd "$(dirname "$0")/../.."

GLIB=$(pkg-config --cflags glib-2.0) || {
    echo "need glib-2.0 dev headers: apt-get install libglib2.0-dev"; exit 2; }
INC="-I tools/shim -I jni-bridge -I jni-bridge/include -I engine-core -I engine-core/lib"
OUT=tools/pilot
TMP=$(mktemp -d)

for f in engine-core/positionid.c \
         engine-core/lib/cache.c engine-core/lib/neuralnet.c \
         tools/pilot/inputs_harness.c tools/pilot/pilot_stubs.c; do
    gcc -c -O1 -DHAVE_CONFIG_H $INC $GLIB "$f" -o "$TMP/$(basename "$f" .c).o"
done
# eval.c at -O0: -O1 inlines the static ComputeTable into its sole caller,
# leaving no symbol for objcopy to globalize. Pilot speed is irrelevant.
gcc -c -O0 -DHAVE_CONFIG_H $INC $GLIB engine-core/eval.c -o "$TMP/eval.o"
# eval.c's ComputeTable is static; the harness must call it (see its header
# comment). Globalize the symbol post-compile instead of editing the engine.
objcopy --globalize-symbol=ComputeTable "$TMP/eval.o"
gcc "$TMP"/*.o -lm $(pkg-config --libs glib-2.0) -o "$OUT/inputs_harness"
rm -rf "$TMP"
echo "built $OUT/inputs_harness"
echo "self-check (expect pips 167/167, CLASS_CONTACT):"
"$OUT/inputs_harness" opening 0 -2 0 0 0 0 5 0 3 0 0 0 -5 5 0 0 0 -3 0 -5 0 0 0 0 2 0 | head -3
