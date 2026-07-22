#!/usr/bin/env bash
# =============================================================================
#  build_and_deploy.sh
#
#  Build GNUbg Android from a pristine state and deploy to device.
#  Wipes all Gradle caches and build output before every compile.
#  No incremental builds. No stale state. Ever.
#
#  Usage (from anywhere in the repo):
#      ./build_and_deploy.sh                  full build + install + launch
#      ./build_and_deploy.sh --apk-only       skip native rebuild
#      ./build_and_deploy.sh --native-only    native rebuild only, no APK/deploy
#      ./build_and_deploy.sh --no-build       install existing APK only
#      ./build_and_deploy.sh --reconfigure    wipe CMake dir and reconfigure
#      ./build_and_deploy.sh --reinstall      uninstall first (clears app data)
#      ./build_and_deploy.sh --logcat         stream tutor + vm log after launch
#
#  Pipeline:
#      1. cmake --build jni-bridge/build-android-arm64
#      2. cp libgnubg-engine.so -> gnubg-app/app/src/main/jniLibs/arm64-v8a/
#      3. rm -rf gnubg-app/.gradle gnubg-app/app/build
#      4. ./gradlew assembleRelease  (assembleDebug if no keystore.properties)
#      5. adb install -r
#      6. adb shell monkey (launch)
#      7. adb logcat | grep gnubg  (if --logcat)
# =============================================================================
set -euo pipefail

if [ -t 1 ]; then B=$'\033[1m'; G=$'\033[32m'; R=$'\033[31m'; Y=$'\033[33m'; X=$'\033[0m'
else B=""; G=""; R=""; Y=""; X=""; fi
ok()   { printf '%s  ok %s%s\n' "$G" "$*" "$X"; }
warn() { printf '%s  !  %s%s\n' "$Y" "$*" "$X"; }
die()  { printf '%s  x  %s%s\n' "$R" "$*" "$X" >&2; exit 1; }
hr()   { printf '%s----------------------------------------------------------%s\n' "$B" "$X"; }

DO_NATIVE=1; DO_APK=1; DO_INSTALL=1; DO_RECONFIGURE=0; DO_LOGCAT=0; REINSTALL=0
for arg in "$@"; do case "$arg" in
  --apk-only)     DO_NATIVE=0 ;;
    --force-apk-only) DO_NATIVE=0; FORCE_APK_ONLY=1 ;;
  --native-only)  DO_APK=0; DO_INSTALL=0 ;;
  --no-build)     DO_NATIVE=0; DO_APK=0 ;;
  --reconfigure)  DO_RECONFIGURE=1 ;;
  --reinstall)    REINSTALL=1 ;;
  --logcat)       DO_LOGCAT=1 ;;
  -h|--help) sed -n '2,25p' "$0"; exit 0 ;;
  *) die "unknown flag: $arg (try --help)" ;;
esac; done

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]:-$0}")" && pwd)"
ROOT=""
d="$SCRIPT_DIR"
FORCE_APK_ONLY=0
while [ "$d" != "/" ]; do
  [ -d "$d/jni-bridge" ] && [ -d "$d/.git" ] && { ROOT="$d"; break; }
  d="$(dirname "$d")"
done
[ -n "$ROOT" ] || die "Could not find repo root."

CMAKE_BUILD="$ROOT/jni-bridge/build-android-arm64"
JNILIBS="$ROOT/gnubg-app/app/src/main/jniLibs/arm64-v8a"
NDK_TOOLCHAIN="/home/erweitert/android-sdk/ndk/27.0.11718014/build/cmake/android.toolchain.cmake"
APP_DIR="$ROOT/gnubg-app"
# The edition truth lives in gradle; hardcoding it here once launched the
# free app after installing the Plus build -- an entire debugging session
# chased silence in the wrong edition. Read, never assume.
APP_ID="$(sed -n 's/.*applicationId = "\([^"]*\)".*/\1/p' gnubg-app/app/build.gradle.kts | head -n1)"
[ -n "$APP_ID" ] || APP_ID="com.clavierhaus.gnubg"

hr
printf '%sGNUbg - clean build and deploy%s\n' "$B" "$X"
printf 'Repo : %s\n' "$ROOT"
hr

# --- 1. native build ---------------------------------------------------------
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
  cp "$CMAKE_BUILD/libgnubg-engine.so" "$JNILIBS/libgnubg-engine.so"
  ok "copied libgnubg-engine.so -> jniLibs/arm64-v8a/"
else
  # THE STALE-NATIVE GUARD (field lesson, 2026-07-20): a facade fix once
  # shipped in Kotlin while its C half stayed in the device's old native
  # libs -- reverted Kotlin read un-reverted C, and a four-hour ghost hunt
  # followed. Refuse --apk-only when native sources changed since the last
  # native build; --force-apk-only overrides deliberately.
  NATIVE_HASH=$( { git ls-files -s jni-bridge engine-core; git diff -- jni-bridge engine-core; } | sha1sum | cut -d' ' -f1 )
  STAMP_FILE=".native_build_stamp"
  if [ "$FORCE_APK_ONLY" != "1" ]; then
    if [ ! -f "$STAMP_FILE" ] || [ "$(cat "$STAMP_FILE")" != "$NATIVE_HASH" ]; then
      die "native sources changed since the last native build -- run ./build_native_android.sh first (or pass --force-apk-only if you know why this is safe)."
    fi
  fi
  warn "skipping native build (native sources unchanged since last native build)"
fi

# --- build type -------------------------------------------------------------
# A release-signed build installs over a release-signed one; a debug build does
# not, because Android refuses to change a package's signing identity. Since
# the releases handed to testers are signed with the project key, deploying a
# debug APK to the same device fails with INSTALL_FAILED_UPDATE_INCOMPATIBLE
# and costs an uninstall (and the app's data). So: sign with the project key
# whenever it is available, and fall back to debug only when it is not, which
# is the case for a contributor who has no keystore.
if [ -f "$APP_DIR/keystore.properties" ]; then
  BUILD_TYPE="release"; GRADLE_TASK="assembleRelease"
  APK_PATH="$APP_DIR/app/build/outputs/apk/release/app-release.apk"
else
  BUILD_TYPE="debug";   GRADLE_TASK="assembleDebug"
  APK_PATH="$APP_DIR/app/build/outputs/apk/debug/app-debug.apk"
  warn "no keystore.properties -- building DEBUG; it will not install over a release-signed app"
fi

# --- 2. wipe + gradle --------------------------------------------------------
if [ "$DO_APK" -eq 1 ]; then
  printf '%swiping Gradle cache and build output...%s\n' "$B" "$X"
  rm -rf "$APP_DIR/.gradle" "$APP_DIR/app/build"
  ok "wiped"
  printf '%sbuilding APK...%s\n' "$B" "$X"
  ( cd "$APP_DIR" && ./gradlew "$GRADLE_TASK" ) || die "gradle build failed."
  APK="$APK_PATH"
  [ -f "$APK" ] || die "expected $APK after $GRADLE_TASK -- not produced"
  ok "APK: ${APK#$ROOT/} ($BUILD_TYPE)"
elif [ "$DO_INSTALL" -eq 1 ]; then
  APK="$APK_PATH"
  warn "skipping build; using existing $BUILD_TYPE APK"
fi

if [ "$DO_INSTALL" -eq 1 ]; then
  [ -n "$APK" ] && [ -f "$APK" ] || die "no APK found -- build first"
fi

# --- 3. install + launch -----------------------------------------------------
if [ "$DO_INSTALL" -eq 1 ]; then
  mkdir -p "$ROOT/tmp"
  command -v adb >/dev/null 2>&1 || die "adb not found on PATH"
  adb start-server >/dev/null 2>&1 || true
  unauth="$(adb devices | awk 'NR>1 && $2=="unauthorized"{print $1}')"
  [ -z "$unauth" ] || die "device $unauth UNAUTHORIZED -- unlock phone and allow USB debugging"
  mapfile -t devs < <(adb devices | awk 'NR>1 && $2=="device"{print $1}')
  [ "${#devs[@]}" -gt 0 ] || die "no device connected"
  [ "${#devs[@]}" -eq 1 ] || warn "multiple devices; using ${devs[0]}"
  SERIAL="${ANDROID_SERIAL:-${devs[0]}}"; export ANDROID_SERIAL="$SERIAL"
  MODEL="$(adb -s "$SERIAL" shell getprop ro.product.model 2>/dev/null | tr -d '\r')"
  ok "device: $SERIAL ($MODEL)"

  if [ "$REINSTALL" -eq 1 ]; then
    warn "reinstall: uninstalling first (clears app data)"
    adb -s "$SERIAL" uninstall "$APP_ID" >/dev/null 2>&1 || true
    adb -s "$SERIAL" install "$APK" >"$ROOT/tmp/_adb_$$" 2>&1 \
      || { cat "$ROOT/tmp/_adb_$$" >&2; rm -f "$ROOT/tmp/_adb_$$"; die "install failed"; }
  else
    if ! adb -s "$SERIAL" install -r "$APK" >"$ROOT/tmp/_adb_$$" 2>&1; then
      if grep -qiE 'INSTALL_FAILED_UPDATE_INCOMPATIBLE|signatures do not match' "$ROOT/tmp/_adb_$$"; then
        cat "$ROOT/tmp/_adb_$$" >&2; rm -f "$ROOT/tmp/_adb_$$"
        die "signature mismatch -- re-run with --reinstall (clears app data)"
      fi
      cat "$ROOT/tmp/_adb_$$" >&2; rm -f "$ROOT/tmp/_adb_$$"; die "install failed"
    fi
  fi
  rm -f "$ROOT/tmp/_adb_$$"; ok "installed"

  printf '%slaunching...%s\n' "$B" "$X"
  if adb -s "$SERIAL" shell monkey -p "$APP_ID" -c android.intent.category.LAUNCHER 1 >/dev/null 2>&1; then
    ok "launched $APP_ID"
  else
    adb -s "$SERIAL" shell am start -n "$APP_ID/.MainActivity" >/dev/null 2>&1 \
      && ok "launched $APP_ID/.MainActivity" \
      || warn "could not auto-launch"
  fi
  printf '%s  on %s (%s)%s\n' "$G" "$MODEL" "$SERIAL" "$X"
fi

hr
ok "done."

# --- 4. logcat ---------------------------------------------------------------
if [ "$DO_LOGCAT" -eq 1 ]; then
  hr
  printf '%slogcat (tutor + vm) -- Ctrl-C to stop%s\n' "$B" "$X"
  adb logcat | grep --line-buffered -E "gnubg-tutor|gnubg-vm"
fi
