#!/usr/bin/env bash
# =============================================================================
#  build.sh - build the GNUbg native library and package the Android APK.
#
#  This script owns the full native build pipeline. Run it whenever C sources
#  change (engine-core/, jni-bridge/src/, jni-bridge/include/).
#  For Kotlin-only changes, run_on_device.sh --no-build is sufficient.
#
#  Run from anywhere in the repo:
#      bash build.sh
#
#  Flags:
#      --native-only   build native library and copy to jniLibs; skip Gradle
#      --apk-only      skip native build; run Gradle only (Kotlin changes)
#      --reconfigure   wipe and reconfigure the CMake build directory first
#
#  Pipeline:
#      1. cmake --build jni-bridge/build-android-arm64
#      2. cp libgnubg-engine.so -> gnubg-app/app/src/main/jniLibs/arm64-v8a/
#      3. ./gradlew assembleDebug
#
#  After a successful build, run run_on_device.sh --no-build to install.
# =============================================================================
set -euo pipefail

if [ -t 1 ]; then B=$'\033[1m'; G=$'\033[32m'; R=$'\033[31m'; Y=$'\033[33m'; X=$'\033[0m'
else B=""; G=""; R=""; Y=""; X=""; fi
ok()   { printf '%s  ok %s%s\n' "$G" "$*" "$X"; }
warn() { printf '%s  !  %s%s\n' "$Y" "$*" "$X"; }
die()  { printf '%s  x  %s%s\n' "$R" "$*" "$X" >&2; exit 1; }
hr()   { printf '%s----------------------------------------------------------%s\n' "$B" "$X"; }

DO_NATIVE=1; DO_APK=1; DO_RECONFIGURE=0
for arg in "$@"; do case "$arg" in
  --native-only)  DO_APK=0 ;;
  --apk-only)     DO_NATIVE=0 ;;
  --reconfigure)  DO_RECONFIGURE=1 ;;
  -h|--help) sed -n '2,22p' "$0"; exit 0 ;;
  *) die "unknown flag: $arg (try --help)" ;;
esac; done

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]:-$0}")" && pwd)"
ROOT=""
d="$SCRIPT_DIR"
while [ "$d" != "/" ]; do
  [ -d "$d/jni-bridge" ] && [ -d "$d/.git" ] && { ROOT="$d"; break; }
  d="$(dirname "$d")"
done
[ -n "$ROOT" ] || die "Could not find repo root (a dir with jni-bridge/ and .git/)."

CMAKE_BUILD="$ROOT/jni-bridge/build-android-arm64"
JNILIBS="$ROOT/gnubg-app/app/src/main/jniLibs/arm64-v8a"
NDK_TOOLCHAIN="/home/erweitert/android-sdk/ndk/27.0.11718014/build/cmake/android.toolchain.cmake"
APP_DIR="$ROOT/gnubg-app"

hr
printf '%sGNUbg - native build + APK package%s\n' "$B" "$X"
printf 'Repo : %s\n' "$ROOT"
hr

if [ "$DO_NATIVE" -eq 1 ]; then
  if [ "$DO_RECONFIGURE" -eq 1 ]; then
    warn "reconfiguring: wiping $CMAKE_BUILD"
    rm -rf "$CMAKE_BUILD"
  fi
  if [ ! -f "$CMAKE_BUILD/CMakeCache.txt" ]; then
    printf '%sconfiguring CMake...%s\n' "$B" "$X"
    cmake -B "$CMAKE_BUILD" \
          -DANDROID_ABI=arm64-v8a \
          -DANDROID_PLATFORM=android-23 \
          -DCMAKE_BUILD_TYPE=Debug \
          -DCMAKE_TOOLCHAIN_FILE="$NDK_TOOLCHAIN" \
          "$ROOT/jni-bridge/" || die "cmake configure failed."
    ok "cmake configured"
  fi
  printf '%sbuilding native library...%s\n' "$B" "$X"
  cmake --build "$CMAKE_BUILD" || die "native build failed."
  ok "native library built"
  [ -d "$JNILIBS" ] || die "jniLibs dir not found: $JNILIBS"
  cp "$CMAKE_BUILD/libgnubg-engine.so" "$JNILIBS/libgnubg-engine.so"
  ok "copied libgnubg-engine.so -> jniLibs/arm64-v8a/"
else
  warn "skipping native build (--apk-only)"
fi

if [ "$DO_APK" -eq 1 ]; then
  printf '%spackaging APK...%s\n' "$B" "$X"
  ( cd "$APP_DIR" && ./gradlew assembleDebug ) || die "gradle build failed."
  APK="$(find "$APP_DIR/app/build/outputs/apk/debug" -name '*.apk' 2>/dev/null | head -n1)"
  ok "APK: ${APK#$ROOT/}"
else
  warn "skipping Gradle (--native-only)"
fi

hr
ok "done. Run run_on_device.sh --no-build to install."
