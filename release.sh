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
#      ./release.sh --no-build      reuse the APK already built (skip build_and_deploy)
#      ./release.sh --prerelease    mark the GitHub release as a pre-release
#
#  What it does:
#      1. Preflight: clean tree, on main, in sync, gh authed, tag is NEW,
#         buildable-clone check passes, RELEASE_NOTES.md present.
#      2. Read version from gnubg-app/app/build.gradle.kts (single source).
#      3. Build the debug APK via ./build_and_deploy.sh (--no-build to skip).
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
git fetch origin main --tags -q
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
  ./tools/check_buildable_clone.sh >/dev/null || die "buildable-clone check FAILED -- a fresh clone would not build"
  ok "buildable-clone check passes"
fi

# gh authed to the right account
command -v gh >/dev/null 2>&1 || die "gh (GitHub CLI) not found on PATH"
gh auth status >/dev/null 2>&1 || die "gh not authenticated -- run: gh auth login"
ok "gh authenticated"

# --- 2. build ----------------------------------------------------------------
if [ "$DO_BUILD" -eq 1 ]; then
  printf '%sbuilding debug APK via build_and_deploy.sh --native-only + apk...%s\n' "$B" "$X"
  # Build the APK but do NOT require a device: use gradle directly for the APK,
  # and let build_and_deploy handle the native lib. We only need the artifact.
  ./build_and_deploy.sh --native-only || die "native build failed"
  rm -rf "$APP_DIR/.gradle" "$APP_DIR/app/build"
  ( cd "$APP_DIR" && ./gradlew assembleDebug ) || die "gradle assembleDebug failed"
  ok "APK built"
else
  warn "skipping build (--no-build) -- using existing APK"
fi

APK="$(find "$APP_DIR/app/build/outputs/apk/debug" -name '*.apk' 2>/dev/null | head -n1)"
[ -n "$APK" ] && [ -f "$APK" ] || die "no APK found -- build first (drop --no-build)"
ok "APK: ${APK#$ROOT/}"

# a debug APK is signed with the debug key and installs; confirm it is not the
# unsigned release artifact by mistake
case "$APK" in
  *unsigned*) die "APK is UNSIGNED ($APK) -- it will not install. Use the debug APK." ;;
esac

# --- 3. tag + publish --------------------------------------------------------
if [ "$DRY" -eq 1 ]; then
  hr; ok "DRY RUN -- would tag $TAG and publish:"
  printf '    gh release create %s %s %s --title "%s" --notes-file %s\n' \
    "$TAG" "${APK#$ROOT/}" "$PRERELEASE" "$VNAME" "$NOTES"
  exit 0
fi

printf '%stagging %s...%s\n' "$B" "$TAG" "$X"
git tag -a "$TAG" -m "GNU Backgammon for Android $VNAME"
git push origin "$TAG"
ok "tag pushed"

printf '%screating GitHub release...%s\n' "$B" "$X"
TITLE="$(head -n1 "$NOTES" | sed 's/^#* *//')"
[ -n "$TITLE" ] || TITLE="$VNAME"
gh release create "$TAG" "$APK" $PRERELEASE \
  --title "$TITLE" \
  --notes-file "$NOTES" \
  || die "gh release create failed"

hr
ok "released $TAG"
gh release view "$TAG" --web >/dev/null 2>&1 || true
printf '%s  https://github.com/clavierhaus/gnubg-android/releases/tag/%s%s\n' "$G" "$TAG" "$X"
