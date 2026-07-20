#!/bin/sh
# Build the amendment-2 candidate harness against THIS repo's engine-core,
# with the REAL evaluator (neural net + bearoff + MET), unlike the pilot which
# stubs them. Needs: gcc, libglib2.0-dev. Output: tools/narrator/quorum/authority_harness
set -e
cd "$(dirname "$0")/../../.."

GLIB=$(pkg-config --cflags glib-2.0) || {
    echo "need glib-2.0 dev headers: apt-get install libglib2.0-dev"; exit 2; }
INC="-I tools/shim -I jni-bridge -I jni-bridge/include -I engine-core -I engine-core/lib"
OUT=tools/narrator/quorum
TMP=$(mktemp -d)

# The real evaluation world: eval, position id, neural net, bearoff, MET,
# inputs, isaac RNG, md5, cache. Add sources here as the linker demands them.
SRCS="engine-core/eval.c \
      engine-core/positionid.c \
      engine-core/bearoff.c \
      engine-core/bearoffgammon.c \
      engine-core/matchequity.c \
      engine-core/mec.c \
      engine-core/lib/neuralnet.c \
      engine-core/lib/cache.c \
      engine-core/lib/inputs.c \
      engine-core/lib/isaac.c \
      engine-core/lib/md5.c \
      engine-core/dice.c \
      engine-core/lib/SFMT.c \
      engine-core/lib/list.c \
      engine-core/matchid.c \
      jni-bridge/src/stubs.c \
      tools/narrator/quorum/authority_harness.c \
      tools/narrator/quorum/quorum_stubs.c"

for f in $SRCS; do
    gcc -c -O1 -DHAVE_CONFIG_H $INC $GLIB "$f" -o "$TMP/$(basename "$f" .c).o"
done
gcc "$TMP"/*.o -lm $(pkg-config --libs glib-2.0) -o "$OUT/authority_harness"
rm -rf "$TMP"
echo "built $OUT/authority_harness"
