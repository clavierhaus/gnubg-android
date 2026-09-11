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
  printf '%sbuilding the release APK -- the recipe's build, then signed%s\n' "$B" "$X"

  # ONE build path. Until 2026-09-11 this script ran its own cmake configure
  # (CMAKE_BUILD_TYPE=Debug, android-23, no reproducibility flags, whichever
  # NDK sorted last) and never rebuilt glib, so the "reference APK" it
  # published was not the build F-Droid's recipe runs and not the build the
  # two-worktree proof had just verified: F-Droid's rebuild differed in every
  # native library. The recipe's build: line is `./build_native_android.sh`;
  # that is the only native build a release may use.
  ./build_native_android.sh || die "build_native_android.sh failed"
  ok "native libraries built by the recipe's own script"

  # The UNSIGNED release APK first, so its bytes can be compared with the
  # proof (verify_reproducible.sh builds this same unsigned artifact), then
  # signed with apksigner. keystore.properties is moved aside for the gradle
  # run so gradle cannot sign; the signature is attached to proven bytes.
  KSP="$APP_DIR/keystore.properties"
  [ -f "$KSP" ] || die "$KSP missing -- the release key is needed to sign (see docs/RELEASE_SIGNING.md)"
  mv "$KSP" "$KSP.release-aside"
  trap 'mv -f "$KSP.release-aside" "$KSP" 2>/dev/null || true' EXIT
  rm -rf "$APP_DIR/.gradle" "$APP_DIR/app/build"
  ( cd "$APP_DIR" && ./gradlew assembleRelease ) || die "gradle assembleRelease failed"
  mv -f "$KSP.release-aside" "$KSP"; trap - EXIT
  UNSIGNED_APK="$APP_DIR/app/build/outputs/apk/release/app-release-unsigned.apk"
  [ -f "$UNSIGNED_APK" ] || die "no app-release-unsigned.apk produced"
  UNSIGNED_SHA="$(sha256sum "$UNSIGNED_APK" | cut -d' ' -f1)"
  ok "unsigned release APK: $UNSIGNED_SHA"
  # If the caller (release_fdroid.sh) proved a hash, this build must be it.
  if [ -n "${EXPECT_UNSIGNED_SHA:-}" ] && [ "$EXPECT_UNSIGNED_SHA" != "$UNSIGNED_SHA" ]; then
    die "this build ($UNSIGNED_SHA) is not the proven build ($EXPECT_UNSIGNED_SHA); nothing is published"
  fi

  KS_FILE="$(sed -n 's/^storeFile=//p' "$KSP")"; KS_PASS="$(sed -n 's/^storePassword=//p' "$KSP")"
  KEY_PASS="$(sed -n 's/^keyPassword=//p' "$KSP")"; KEY_ALIAS="$(sed -n 's/^keyAlias=//p' "$KSP")"
  case "$KS_FILE" in /*) ;; *) KS_FILE="$APP_DIR/app/$KS_FILE" ;; esac   # gradle's file() resolves against app/
  APKSIGNER="$(find "${ANDROID_SDK_ROOT:-${ANDROID_HOME:-$HOME/Android/Sdk}}/build-tools" -name apksigner 2>/dev/null | sort -V | tail -n1)"
  [ -n "$APKSIGNER" ] || die "apksigner not found under build-tools"
  # v1 (JAR) signing OFF: it adds META-INF/*.SF/*.RSA/MANIFEST.MF as zip
  # ENTRIES, which shift every later entry's offset and re-pad the
  # alignment. F-Droid's verifier strips the v2/v3 signing block, not zip
  # entries, so with v1 on, two APKs whose contents were byte-identical
  # still failed the container digest (2026-09-11, the last difference).
  # minSdk 31 never reads v1 anyway. v2 + v3 are the signature.
  "$APKSIGNER" sign --ks "$KS_FILE" --ks-key-alias "$KEY_ALIAS" \
    --ks-pass "pass:$KS_PASS" --key-pass "pass:$KEY_PASS" \
    --v1-signing-enabled false --v2-signing-enabled true --v3-signing-enabled true \
    --out "$APP_DIR/app/build/outputs/apk/release/app-release.apk" "$UNSIGNED_APK" \
    || die "apksigner sign failed"
  # The signed APK must contain exactly the unsigned APK's entries: no v1 files.
  if unzip -l "$APP_DIR/app/build/outputs/apk/release/app-release.apk" | grep -qE 'META-INF/.*\.(SF|RSA|DSA|EC)$|META-INF/MANIFEST\.MF'; then
    die "signed APK carries v1 signature entries -- the container would not match F-Droid's build"
  fi
  ok "signed release APK built (signature attached to the proven bytes)"
else
  warn "skipping build (--no-build) -- using existing APK"
fi

# The release APK MUST be signed (not app-release-unsigned.apk): an unsigned
# release APK means gradle did not find keystore.properties (it must live in
# $APP_DIR/keystore.properties, the gradle root). Fail loudly rather than
# publish an unsigned or unusable artifact.
APK="$APP_DIR/app/build/outputs/apk/release/app-release.apk"
[ -f "$APK" ] || die "no signed release APK at $APK"
APKSIGNER="$(find "${ANDROID_SDK_ROOT:-${ANDROID_HOME:-$HOME/Android/Sdk}}/build-tools" -name apksigner 2>/dev/null | sort -V | tail -n1)"
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
# Title is MECHANICAL: fixed product name + the tag, nothing hand-typed.
# The old code took the title from RELEASE_NOTES.md's first line, so a stale
# heading (e.g. "...0.21.4") rotted into the title of a different version.
# GitHub shows the tag beside the title already, so the version lives in the
# tag alone -- one source of truth, nothing to drift.
TITLE="CBG - Clavierhaus BackGammon $TAG"
gh release create "$TAG" "$APK" "$APK_SHA" $PRERELEASE \
  --title "$TITLE" \
  --notes-file "$NOTES" \
  || die "gh release create failed"

hr
ok "released $TAG"
gh release view "$TAG" --web >/dev/null 2>&1 || true
printf '%s  https://github.com/clavierhaus/gnubg-android/releases/tag/%s%s\n' "$G" "$TAG" "$X"
