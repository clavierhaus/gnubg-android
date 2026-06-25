#!/usr/bin/env bash
# =============================================================================
#  run_on_device.sh
#
#  Build (debug), install, and launch the GNUbg app on a connected device
#  (your Pixel 8 Pro over adb).
#
#  Run from anywhere in the repo:
#      bash claude-final-integration/run_on_device.sh
#
#  Flags:
#      --no-build     skip the Gradle build; just install the existing APK
#      --clean        do a clean build (./gradlew clean assembleDebug)
#      --logcat       after launch, stream the app's logcat (Ctrl-C to stop)
#      --reinstall    adb install -r (keep data) is default; this forces a
#                     full uninstall+install (clears app data)
#
#  Does nothing destructive to your repo. Only builds, installs, launches.
# =============================================================================

set -euo pipefail

# ---- pretty output ----------------------------------------------------------
if [ -t 1 ]; then
  B=$'\033[1m'; G=$'\033[32m'; R=$'\033[31m'; Y=$'\033[33m'; X=$'\033[0m'
else
  B=""; G=""; R=""; Y=""; X=""
fi
ok()   { printf '%s  ok %s%s\n'  "$G" "$*" "$X"; }
warn() { printf '%s  !  %s%s\n'  "$Y" "$*" "$X"; }
die()  { printf '%s  x  %s%s\n'  "$R" "$*" "$X" >&2; exit 1; }
hr()   { printf '%s----------------------------------------------------------%s\n' "$B" "$X"; }

# ---- config -----------------------------------------------------------------
APP_ID="com.clavierhaus.gnubg"

DO_BUILD=1
DO_CLEAN=0
DO_LOGCAT=0
FORCE_REINSTALL=0
for arg in "$@"; do
  case "$arg" in
    --no-build)   DO_BUILD=0 ;;
    --clean)      DO_CLEAN=1 ;;
    --logcat)     DO_LOGCAT=1 ;;
    --reinstall)  FORCE_REINSTALL=1 ;;
    -h|--help)
      sed -n '2,20p' "$0"; exit 0 ;;
    *) die "unknown flag: $arg (try --help)" ;;
  esac
done

# ---- locate repo root -------------------------------------------------------
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]:-$0}")" && pwd)"
find_root() {
  local d="$SCRIPT_DIR"
  while [ "$d" != "/" ]; do
    [ -d "$d/gnubg-app" ] && [ -d "$d/.git" ] && { printf '%s\n' "$d"; return 0; }
    d="$(dirname "$d")"
  done
  return 1
}
ROOT="$(find_root)" || die "Could not find repo root (a dir with gnubg-app/ and .git/)."
APP_DIR="$ROOT/gnubg-app"

hr
printf '%sGNUbg - build, install, launch on device%s\n' "$B" "$X"
printf 'Repo : %s\n' "$ROOT"
printf 'App  : %s\n' "$APP_ID"
hr

# ---- 1. adb present & device connected --------------------------------------
command -v adb >/dev/null 2>&1 || die "adb not found on PATH. Install platform-tools or add it to PATH."
ok "adb present"

# Start the server quietly, then enumerate devices.
adb start-server >/dev/null 2>&1 || true

# Count devices that are in 'device' state (not 'unauthorized'/'offline').
mapfile -t DEVLINES < <(adb devices | awk 'NR>1 && $2=="device"{print $1}')
UNAUTH="$(adb devices | awk 'NR>1 && $2=="unauthorized"{print $1}')"
OFFLINE="$(adb devices | awk 'NR>1 && $2=="offline"{print $1}')"

if [ -n "$UNAUTH" ]; then
  die "Device $UNAUTH is UNAUTHORIZED. Unlock the Pixel and tap 'Allow USB debugging' (check 'always allow'), then re-run."
fi
if [ "${#DEVLINES[@]}" -eq 0 ]; then
  [ -n "$OFFLINE" ] && warn "device(s) offline: $OFFLINE"
  die "No device in 'device' state. Plug in the Pixel 8 Pro, ensure USB debugging is on, and 'adb devices' lists it."
fi

if [ "${#DEVLINES[@]}" -gt 1 ]; then
  warn "multiple devices connected: ${DEVLINES[*]}"
  warn "using the first one: ${DEVLINES[0]}  (set ANDROID_SERIAL to override)"
fi
SERIAL="${ANDROID_SERIAL:-${DEVLINES[0]}}"
export ANDROID_SERIAL="$SERIAL"

# Friendly device name
MODEL="$(adb -s "$SERIAL" shell getprop ro.product.model 2>/dev/null | tr -d '\r')"
ok "device: $SERIAL  ($MODEL)"

# ---- 2. build ---------------------------------------------------------------
if [ "$DO_BUILD" -eq 1 ]; then
  cd "$APP_DIR"
  if [ "$DO_CLEAN" -eq 1 ]; then
    printf '%sbuilding (clean)...%s\n' "$B" "$X"
    ./gradlew clean assembleDebug || die "Gradle build failed."
  else
    printf '%sbuilding...%s\n' "$B" "$X"
    ./gradlew assembleDebug || die "Gradle build failed."
  fi
  ok "build successful"
  cd "$ROOT"
else
  warn "skipping build (--no-build)"
fi

# ---- 3. locate the APK ------------------------------------------------------
APK="$(find "$APP_DIR/app/build/outputs/apk/debug" -name '*.apk' 2>/dev/null | head -n1)"
[ -n "$APK" ] && [ -f "$APK" ] || die "No debug APK found under app/build/outputs/apk/debug/. Build first (drop --no-build)."
ok "apk: ${APK#$ROOT/}"

# ---- 4. install -------------------------------------------------------------
if [ "$FORCE_REINSTALL" -eq 1 ]; then
  warn "force reinstall: uninstalling first (clears app data)"
  adb -s "$SERIAL" uninstall "$APP_ID" >/dev/null 2>&1 || true
  printf '%sinstalling...%s\n' "$B" "$X"
  adb -s "$SERIAL" install "$APK" >/tmp/adbinstall_$$ 2>&1 || { cat /tmp/adbinstall_$$ >&2; rm -f /tmp/adbinstall_$$; die "install failed."; }
else
  printf '%sinstalling (keep data, -r)...%s\n' "$B" "$X"
  if ! adb -s "$SERIAL" install -r "$APK" >/tmp/adbinstall_$$ 2>&1; then
    # Common cause: signature mismatch vs an existing install. Offer the fix.
    if grep -qiE 'INSTALL_FAILED_UPDATE_INCOMPATIBLE|signatures do not match' /tmp/adbinstall_$$; then
      cat /tmp/adbinstall_$$ >&2; rm -f /tmp/adbinstall_$$
      die "Signature mismatch with the installed copy. Re-run with --reinstall to uninstall+install (this clears app data)."
    fi
    cat /tmp/adbinstall_$$ >&2; rm -f /tmp/adbinstall_$$
    die "install failed."
  fi
fi
rm -f /tmp/adbinstall_$$
ok "installed"

# ---- 5. launch --------------------------------------------------------------
# Resolve and start the default launcher activity from the package alone, so we
# don't have to hard-code the activity class.
printf '%slaunching...%s\n' "$B" "$X"
if adb -s "$SERIAL" shell monkey -p "$APP_ID" -c android.intent.category.LAUNCHER 1 >/dev/null 2>&1; then
  ok "launched $APP_ID"
else
  # Fallback: try the conventional MainActivity.
  if adb -s "$SERIAL" shell am start -n "$APP_ID/.MainActivity" >/dev/null 2>&1; then
    ok "launched $APP_ID/.MainActivity"
  else
    warn "could not auto-launch. List activities with:"
    warn "  adb shell cmd package resolve-activity --brief $APP_ID"
  fi
fi

hr
ok "done."
printf 'On device : %s (%s)\n' "$MODEL" "$SERIAL"
printf 'APK       : %s\n' "${APK#$ROOT/}"
if [ "$DO_LOGCAT" -eq 1 ]; then
  hr
  printf '%sstreaming logcat for %s - Ctrl-C to stop%s\n' "$B" "$APP_ID" "$X"
  # Clear, then follow only this app's PID.
  PID="$(adb -s "$SERIAL" shell pidof "$APP_ID" 2>/dev/null | tr -d '\r')"
  if [ -n "$PID" ]; then
    adb -s "$SERIAL" logcat --pid="$PID"
  else
    warn "app PID not found yet; showing unfiltered gnubg-tagged lines instead"
    adb -s "$SERIAL" logcat | grep -i --line-buffered 'gnubg'
  fi
else
  printf 'Tip       : add --logcat to stream the app log, --clean for a clean build.\n'
fi
