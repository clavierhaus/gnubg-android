#!/bin/sh
# Compile the C we write, on the host, with no NDK.
#
# This exists because the assistant claimed it "could not compile" and therefore
# shipped a duplicate definition of gnubg_mobile_command_agree that a one-second
# syntax check would have caught. gcc, glib headers and a JDK are enough: the
# only NDK-specific header the facade needs is <android/log.h>, stubbed in
# tools/shim.
#
#   ./tools/syntax_check.sh
#
# Checks gnubg_mobile.c and native-lib.c -- the two files this port actually
# writes. android-app.c cannot be checked until engine-core/sound.h is in the
# repository (see .gitignore:64); it is included by play.c, set.c, analysis.c
# and android-app.c, so a fresh clone cannot build the native library at all.
set -e
cd "$(dirname "$0")/.."

GLIB=$(pkg-config --cflags glib-2.0 2>/dev/null) || {
    echo "need glib-2.0 dev headers: apt-get install libglib2.0-dev"; exit 2; }
JDK=$(dirname "$(find /usr/lib/jvm -name jni.h 2>/dev/null | head -1)" 2>/dev/null || true)
[ -n "$JDK" ] || { echo "need a JDK for jni.h: apt-get install openjdk-17-jdk-headless"; exit 2; }

# Include order MUST match jni-bridge/CMakeLists.txt: jni-bridge/ first, so its
# seven stub headers keep shadowing the GTK-dependent engine headers.
INC="-I tools/shim -I $JDK -I $JDK/linux -I jni-bridge -I jni-bridge/include -I engine-core -I engine-core/lib"

status=0
# android-app.c needs engine-core/export.h and movefilters.inc -- real engine
# content, not shimmable (exportsetup, MOVEFILTER_* initializers). Until those
# once-gitignored headers are tracked, say so instead of failing cryptically.
if [ -f engine-core/export.h ]; then
    FILES="jni-bridge/src/gnubg_mobile.c jni-bridge/src/native-lib.c jni-bridge/src/android-app.c"
else
    printf "  %-34s %s\n" "jni-bridge/src/android-app.c" "SKIPPED: engine-core/export.h absent (pull the tracked headers)"
    FILES="jni-bridge/src/gnubg_mobile.c jni-bridge/src/native-lib.c"
fi
for f in $FILES; do
    printf '  %-34s ' "$f"
    if gcc -fsyntax-only -Wall -DHAVE_CONFIG_H $INC $GLIB "$f" 2>tools/.err; then
        n=$(wc -l < tools/.err)
        [ "$n" -eq 0 ] && echo "clean" || { echo "$((n)) warning lines"; cat tools/.err; }
    else
        echo "FAILED"; cat tools/.err; status=1
    fi
done
rm -f tools/.err
exit $status
