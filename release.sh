#!/usr/bin/env bash
# =============================================================================
#  release.sh
#
#  Cut a GitHub release end to end: verify, build, tag, publish the APK.
#  One command instead of a remembered checklist -- the release steps that
#  repeatedly went wrong (dirty tree, moved tags, wrong APK path, notes file
#  that did not exist, unsigned APK) are each a guard here.
#
#  Usage (from anywhere in the repo):
#      ./release.sh                 build from current build.gradle version, tag, publish
#      ./release.sh --dry-run       do everything EXCEPT tag/push/publish
#      ./release.sh --no-build      reuse the APK already built (skip the build)
#      ./release.sh --prerelease    mark the GitHub release as a pre-release
#
#  What it does:
#      1. Preflight: clean tree, on main, in sync, gh authed, tag is NEW,
#         buildable-clone check passes, RELEASE_NOTES.md present.
#      2. Read version from gnubg-app/app/build.gradle.kts (single source).
#      3. Build the signed release APK directly (--no-build to reuse existing).
#      4. Tag vX.Y.Z (annotated) and push it.
#      5. gh release create with the APK attached and RELEASE_NOTES.md as body.
#
#  The version is whatever build.gradle.kts says. Bump it (and roll CHANGELOG.md
#  into RELEASE_NOTES.md) in a commit BEFORE running this -- see RELEASING.md.
# =============================================================================
set -euo pipefail

if [ -t 1 ]; then B=$'\033[1m'; G=$'\033[32m'; R=$'\033[31m'; Y=$'\033[33m'; X=$'\033[0m'
else B=""; G=""; R=""; Y=""; X=""; fi
ok()   { printf '%s  ok %s%s\n' "$G" "$*" "$X"; }
warn() { printf '%s  !  %s%s\n' "$Y" "$*" "$X"; }
die()  { printf '%s  x  %s%s\n' "$R" "$*" "$X" >&2; exit 1; }
hr()   { printf '%s----------------------------------------------------------%s\n' "$B" "$X"; }

DRY=0; DO_BUILD=1; PRERELEASE=""
for arg in "$@"; do case "$arg" in
  --dry-run)     DRY=1 ;;
  --no-build)    DO_BUILD=0 ;;
  --prerelease)  PRERELEASE="--prerelease" ;;
  -h|--help)     sed -n '2,26p' "$0"; exit 0 ;;
  *) die "unknown flag: $arg (try --help)" ;;
esac; done

# --- locate repo root --------------------------------------------------------
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]:-$0}")" && pwd)"
ROOT=""; d="$SCRIPT_DIR"
while [ "$d" != "/" ]; do
  [ -d "$d/jni-bridge" ] && [ -d "$d/.git" ] && { ROOT="$d"; break; }
  d="$(dirname "$d")"
done
[ -n "$ROOT" ] || die "Could not find repo root."
cd "$ROOT"

GRADLE="gnubg-app/app/build.gradle.kts"
NOTES="RELEASE_NOTES.md"
APP_DIR="$ROOT/gnubg-app"

hr; printf '%sGNUbg - release%s\n' "$B" "$X"; hr

# --- 1. preflight ------------------------------------------------------------
# read version (single source of truth)
VNAME="$(grep -oP 'versionName\s*=\s*"\K[^"]+' "$GRADLE")" || die "cannot read versionName"
VCODE="$(grep -oP 'versionCode\s*=\s*\K[0-9]+' "$GRADLE")" || die "cannot read versionCode"
TAG="v$VNAME"
printf 'version  : %s (code %s)\n' "$VNAME" "$VCODE"
printf 'tag      : %s\n' "$TAG"
hr

# clean working tree -- a release must be reproducible from a commit
[ -z "$(git status --porcelain)" ] || die "working tree is dirty -- commit or stash first"
ok "working tree clean"

# on main, in sync with origin
BR="$(git rev-parse --abbrev-ref HEAD)"
[ "$BR" = "main" ] || die "not on main (on '$BR') -- release from main"
# NOTE: this fetch once killed the script SILENTLY -- a stale local tag
# (v0.10.0, left over from the git-history reset) made 'fetch --tags' exit 1
# with the rejection notice suppressed by -q, and set -e ended the run with no
# message at all. A guard that dies without naming itself is a defect: every
# failure here must say what refused and how to fix it.
git fetch origin main --tags -q \
  || die "git fetch --tags failed -- a local tag likely diverges from origin (stale from a history reset). Fix: git fetch origin --tags --force"
[ "$(git rev-parse HEAD)" = "$(git rev-parse origin/main)" ] \
  || die "local main is not in sync with origin/main -- pull/push first"
ok "on main, in sync with origin"

# tag must be NEW -- never silently move a release tag
if git rev-parse -q --verify "refs/tags/$TAG" >/dev/null \
   || git ls-remote --tags origin "$TAG" | grep -q "$TAG"; then
  die "tag $TAG already exists (local or remote). Bump the version in $GRADLE, or delete the tag deliberately."
fi
ok "tag $TAG is new"

# notes present
[ -f "$NOTES" ] || die "$NOTES missing -- roll CHANGELOG.md into it (see RELEASING.md)"
grep -q "$VNAME" "$NOTES" || warn "$NOTES does not mention $VNAME -- is it the right release's notes?"
ok "release notes present"

# buildable clone gate
if [ -x tools/check_buildable_clone.sh ]; then
  # Output NOT swallowed: the check names the offending file (same
  # named-failure principle as the tag-fetch guard above).
  ./tools/check_buildable_clone.sh || die "buildable-clone check FAILED -- a fresh clone would not build"
  ok "buildable-clone check passes"
fi

# gh authed to the right account
command -v gh >/dev/null 2>&1 || die "gh (GitHub CLI) not found on PATH"
# The guard tests ONLY the credential the release will actually use: the
# ACTIVE account's token, validated by one real API call. 'gh auth status'
# exits non-zero if ANY configured account is broken -- a stale, inactive
# second account (field incident: invalid default account OE1FEU-DF5JT beside
# a perfectly valid active clavierhaus) must never block a release.
ACTIVE_LOGIN="$(gh api user --jq .login 2>/dev/null)" \
  || die "gh cannot reach github.com with the ACTIVE account's token -- run: gh auth login  (a stale GH_TOKEN/GITHUB_TOKEN env var also overrides the keyring login; check: env | grep -E '^(GH|GITHUB)_TOKEN')"
ok "gh authenticated as $ACTIVE_LOGIN"

# --- 2. build ----------------------------------------------------------------
if [ "$DO_BUILD" -eq 1 ]; then
  printf '%sbuilding signed release APK...%s\n' "$B" "$X"

  # Native library, built directly here (no cross-script coupling): release
  # needs only the .so plus a signed assembleRelease, and shelling into
  # build_and_deploy.sh dragged in its debug/install path. Auto-discover the
  # NDK toolchain rather than pin a version/home that drifts between machines.
  CMAKE_BUILD="$ROOT/jni-bridge/build-android-arm64"
  JNILIBS="$APP_DIR/app/src/main/jniLibs/arm64-v8a"
  SDK_ROOT="${ANDROID_SDK_ROOT:-${ANDROID_HOME:-$HOME/Android/Sdk}}"
  NDK_TOOLCHAIN=""
  if [ -n "${ANDROID_NDK_HOME:-}" ] && [ -f "$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake" ]; then
    NDK_TOOLCHAIN="$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake"
  elif [ -d "$SDK_ROOT/ndk" ]; then
    ndk_dir="$(find "$SDK_ROOT/ndk" -maxdepth 1 -mindepth 1 -type d 2>/dev/null | sort -V | tail -n1)"
    [ -n "$ndk_dir" ] && NDK_TOOLCHAIN="$ndk_dir/build/cmake/android.toolchain.cmake"
  fi
  [ -n "$NDK_TOOLCHAIN" ] && [ -f "$NDK_TOOLCHAIN" ] \
    || die "Android NDK toolchain not found (set ANDROID_NDK_HOME or ANDROID_SDK_ROOT)"

  # Always wipe and reconfigure: a cached CMakeCache.txt pins the NDK toolchain
  # path, so reusing it after the NDK moves/changes fails with "not a full path
  # to an existing compiler". A release must be clean and reproducible anyway.
  rm -rf "$CMAKE_BUILD"
  cmake -B "$CMAKE_BUILD" \
    -DANDROID_ABI=arm64-v8a \
    -DANDROID_PLATFORM=android-23 \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_TOOLCHAIN_FILE="$NDK_TOOLCHAIN" \
    "$ROOT/jni-bridge/" || die "cmake configure failed"
  cmake --build "$CMAKE_BUILD" || die "native build failed"
  mkdir -p "$JNILIBS"
  cp "$CMAKE_BUILD/libgnubg-engine.so" "$JNILIBS/libgnubg-engine.so" || die "copying .so failed"
  ok "native library built"

  # Signed release APK. keystore.properties (in gnubg-app/, the gradle root)
  # supplies the signing config; without it gradle emits app-release-unsigned.apk
  # and the guard below fails loudly.
  rm -rf "$APP_DIR/.gradle" "$APP_DIR/app/build"
  ( cd "$APP_DIR" && ./gradlew assembleRelease ) || die "gradle assembleRelease failed"
  ok "signed release APK built"
else
  warn "skipping build (--no-build) -- using existing APK"
fi

# The release APK MUST be signed (not app-release-unsigned.apk): an unsigned
# release APK means gradle did not find keystore.properties (it must live in
# $APP_DIR/keystore.properties, the gradle root). Fail loudly rather than
# publish an unsigned or unusable artifact.
APK="$( { find "$APP_DIR/app/build/outputs/apk/release" -name 'app-release.apk' 2>/dev/null || true; } | head -n1)"
if [ -z "$APK" ] || [ ! -f "$APK" ]; then
  die "no SIGNED release APK found (only app-release-unsigned.apk?). gradle did not pick up the signing key -- ensure $APP_DIR/keystore.properties exists with real storeFile/passwords (see RELEASING.md)."
fi
APKSIGNER="$(find "${ANDROID_HOME:-$HOME/Android/Sdk}/build-tools" -name apksigner 2>/dev/null | sort -V | tail -n1)"
if [ -n "$APKSIGNER" ]; then
  "$APKSIGNER" verify --print-certs "$APK" >/dev/null 2>&1 \
    || die "release APK failed apksigner verify -- it is not correctly signed"
  RCERT="$("$APKSIGNER" verify --print-certs "$APK" 2>/dev/null | sed -n 's/.*SHA-256 digest: //p' | head -n1)"
  ok "release APK signed; signer SHA-256: ${RCERT:-unknown}"
else
  warn "apksigner not found -- skipping signature verification (install build-tools)"
fi
ok "release APK: ${APK#$ROOT/}"

# --- 2b. checksum ------------------------------------------------------------
# Publish a SHA256 sidecar so a downloader can verify the APK end-to-end.
# Written next to the APK, as "<apk>.sha256" in the standard `sha256sum -c`
# format (hash + two spaces + BASENAME, so `sha256sum -c` works from the
# download dir). sha256sum on Linux, shasum -a 256 on macOS.
if command -v sha256sum >/dev/null 2>&1; then
  ( cd "$(dirname "$APK")" && sha256sum "$(basename "$APK")" > "$(basename "$APK").sha256" )
elif command -v shasum >/dev/null 2>&1; then
  ( cd "$(dirname "$APK")" && shasum -a 256 "$(basename "$APK")" > "$(basename "$APK").sha256" )
else
  die "no sha256sum or shasum found -- cannot checksum the release APK"
fi
APK_SHA="$APK.sha256"
[ -s "$APK_SHA" ] || die "checksum file is empty: $APK_SHA"
ok "SHA256: $(cut -d' ' -f1 "$APK_SHA")"

# --- 3. tag + publish --------------------------------------------------------
if [ "$DRY" -eq 1 ]; then
  hr; ok "DRY RUN -- would tag $TAG and publish:"
  printf '    gh release create %s %s %s %s --title "%s" --notes-file %s\n' \
    "$TAG" "${APK#$ROOT/}" "${APK_SHA#$ROOT/}" "$PRERELEASE" "$VNAME" "$NOTES"
  exit 0
fi

printf '%stagging %s...%s\n' "$B" "$TAG" "$X"
# Signed tag when git is configured to sign (tag.gpgSign=true and a
# user.signingKey / GPG default is set up -- see docs/RELEASE_SIGNING.md and
# tools/setup_signing.sh). This is fire-and-forget: once configured, EVERY
# release tag is signed with no extra flag here. If signing is not set up,
# fall back to a plain annotated tag and warn, so a release is never blocked.
if [ "$(git config --bool tag.gpgSign 2>/dev/null)" = "true" ] \
   && { git config user.signingKey >/dev/null 2>&1 || gpg --list-secret-keys >/dev/null 2>&1; }; then
  git tag -s "$TAG" -m "GNU Backgammon for Android $VNAME" \
    || die "signed tag failed -- key set up but signing errored (check: git config user.signingKey; gpg --list-secret-keys)"
  ok "signed tag created"
else
  git tag -a "$TAG" -m "GNU Backgammon for Android $VNAME"
  warn "tag is UNSIGNED -- run tools/setup_signing.sh once to enable signing"
fi
git push origin "$TAG"
ok "tag pushed"

printf '%screating GitHub release...%s\n' "$B" "$X"
TITLE="$(head -n1 "$NOTES" | sed 's/^#* *//')"
[ -n "$TITLE" ] || TITLE="$VNAME"
gh release create "$TAG" "$APK" "$APK_SHA" $PRERELEASE \
  --title "$TITLE" \
  --notes-file "$NOTES" \
  || die "gh release create failed"

hr
ok "released $TAG"
gh release view "$TAG" --web >/dev/null 2>&1 || true
printf '%s  https://github.com/clavierhaus/gnubg-android/releases/tag/%s%s\n' "$G" "$TAG" "$X"
