#!/usr/bin/env bash
# CBG PLUS -- cut a private release on github.com/clavierhaus/cbg-plus so an
# invited collaborator can download a current APK.
#
# Deliberately NOT versioned: Plus tracks whatever versionCode/versionName the
# FOSS release carries, and the tag is a timestamp. There is no release
# bookkeeping to keep straight -- run it whenever something is worth handing
# over, as often as you like.
#
#   ./tools/plus/release_plus.sh            build + publish
#   ./tools/plus/release_plus.sh --no-build  publish the APK already built
#
# The APK installs ALONGSIDE free CBG: applicationId at.clavierhaus.backgammon
# vs com.clavierhaus.gnubg, so a tester can keep both.
set -euo pipefail

G=$'\033[1;32m'; Y=$'\033[1;33m'; R=$'\033[1;31m'; X=$'\033[0m'
ok()   { printf '%sok%s   %s\n'   "$G" "$X" "$1"; }
warn() { printf '%swarn%s %s\n'   "$Y" "$X" "$1"; }
die()  { printf '%sFAIL%s %s\n'   "$R" "$X" "$1" >&2; exit 1; }

REPO="clavierhaus/cbg-plus"
APP_DIR="gnubg-app"
DO_BUILD=1
[ "${1:-}" = "--no-build" ] && DO_BUILD=0

# --- guards ---------------------------------------------------------------
[ -f "$APP_DIR/app/build.gradle.kts" ] || die "run this from the repo root"

grep -q 'applicationId = "at.clavierhaus.backgammon"' "$APP_DIR/app/build.gradle.kts" \
  || die "this tree is NOT the Plus edition (applicationId mismatch)"
ok "Plus tree confirmed"

[ -z "$(git status --porcelain)" ] || die "working tree is dirty -- commit first"
ok "working tree clean"

BRANCH="$(git rev-parse --abbrev-ref HEAD)"
[ "$BRANCH" = "plus" ] || die "on '$BRANCH', expected 'plus'"

# Remotes are named differently depending on which clone this is run from: in
# the FOSS working copy cbg-plus is a secondary remote (usually 'plus'), while
# a dedicated cbg-plus clone has it as 'origin'. Detect by URL rather than
# assuming a name.
PLUS_REMOTE=""; FOSS_REMOTE=""
for r in $(git remote); do
  u="$(git remote get-url "$r")"
  case "$u" in
    *cbg-plus*)       PLUS_REMOTE="$r" ;;
    *gnubg-android*)  FOSS_REMOTE="$r" ;;
  esac
done
[ -n "$PLUS_REMOTE" ] || die "no remote points at cbg-plus"
ok "plus remote is '$PLUS_REMOTE'"

git fetch -q "$PLUS_REMOTE"
LOCAL="$(git rev-parse HEAD)"; REMOTE="$(git rev-parse "$PLUS_REMOTE/plus")"
if [ "$LOCAL" != "$REMOTE" ]; then
  # Out of sync in one of three ways, each needing a different action. Saying
  # "push first" when the tree is merely behind sends you the wrong way.
  BASE="$(git merge-base HEAD "$PLUS_REMOTE/plus")"
  if [ "$LOCAL" = "$BASE" ]; then
    die "plus is BEHIND $PLUS_REMOTE -- run: git pull"
  elif [ "$REMOTE" = "$BASE" ]; then
    die "plus is AHEAD of $PLUS_REMOTE -- run: git push"
  else
    die "plus has DIVERGED from $PLUS_REMOTE -- reconcile before releasing"
  fi
fi
ok "plus branch in sync ($(git rev-parse --short HEAD))"

# Standing order: a red parity audit outranks all other work. The FOSS leg
# needs a remote pointing at the upstream repo; a dedicated cbg-plus clone may
# not have one, in which case say so rather than pass silently.
if [ -n "$FOSS_REMOTE" ]; then
  git fetch -q "$FOSS_REMOTE"
  FOSS_REF="$FOSS_REMOTE/main" MIRROR_REF="$PLUS_REMOTE/main" PLUS_REF=HEAD \
    ./tools/plus/check_foss_parity.sh || die "FOSS parity audit is RED -- fix before releasing"
else
  FOSS_REF="$PLUS_REMOTE/main" MIRROR_REF="$PLUS_REMOTE/main" PLUS_REF=HEAD \
    ./tools/plus/check_foss_parity.sh || die "overlay audit is RED -- fix before releasing"
  warn "no upstream FOSS remote in this clone: plus-vs-mirror checked, mirror-vs-FOSS NOT verified"
fi

# Authored coach text must pass every gate that a shipped defect taught us to
# check. A tester must never receive a build whose phrases can lie.
python3 ./tools/plus/check_phrases.py || die "phrase gate failed -- fix before releasing"
# (add --require-grammar above once language_tool_python is installed, to make a
#  missing grammar gate fatal rather than merely loud)
ok "phrase gate clean"

command -v gh >/dev/null || die "gh not found"
gh api user -q .login >/dev/null 2>&1 \
  || die "gh cannot reach github.com -- run: gh auth login"
ok "gh authenticated as $(gh api user -q .login)"

# --- build ----------------------------------------------------------------
# Signing key. keystore.properties is gitignored, so a separate cbg-plus clone
# will not have it even though the FOSS working copy does. Without it gradle
# silently emits app-release-unsigned.apk -- check before spending a build on it.
KSP="$APP_DIR/keystore.properties"
[ -f "$KSP" ] || die "missing $KSP (gitignored, so a fresh clone lacks it). Copy it over:
       cp /home/erweitert/gnubg-android/gnubg-app/keystore.properties $APP_DIR/"
KS_NAMED="$(sed -n 's/^storeFile=//p' "$KSP" | head -n1)"
case "$KS_NAMED" in
  /*) KS_PATH="$KS_NAMED" ;;
  *)  KS_PATH="$APP_DIR/$KS_NAMED" ;;   # gradle resolves against rootProject = gnubg-app/
esac
[ -f "$KS_PATH" ] || die "keystore.properties names storeFile=$KS_NAMED, which resolves to
       $KS_PATH and does not exist. Copy the keystore across too, or make storeFile absolute."
ok "signing key present ($KS_PATH)"

if [ "$DO_BUILD" = 1 ]; then
  # jniLibs is gitignored: the .so are produced by build_native_android.sh and
  # live only in the working tree. A release built without them would ship an
  # APK with no engine.
  SO_COUNT="$(find "$APP_DIR/app/src/main/jniLibs" -name '*.so' 2>/dev/null | wc -l)"
  [ "$SO_COUNT" -gt 0 ] \
    || die "no native libraries in $APP_DIR/app/src/main/jniLibs -- run ./build_native_android.sh first"
  ok "native libraries present ($SO_COUNT)"

  ( cd "$APP_DIR" && ./gradlew assembleRelease ) || die "gradle assembleRelease failed"
fi

APK="$(find "$APP_DIR/app/build/outputs/apk/release" -name 'app-release.apk' 2>/dev/null | head -n1)"
[ -n "$APK" ] && [ -f "$APK" ] \
  || die "no signed app-release.apk -- keystore.properties must live in $APP_DIR/ (an unsigned build produces app-release-unsigned.apk)"

# An unsigned APK will not install. Fail loudly rather than hand a tester a dud.
if command -v apksigner >/dev/null 2>&1; then
  apksigner verify "$APK" >/dev/null 2>&1 || die "APK failed apksigner verify"
  ok "APK is signed"
else
  warn "apksigner not on PATH -- signature not verified"
fi

# --- publish --------------------------------------------------------------
TAG="plus-$(date +%Y%m%d-%H%M)"
VNAME="$(grep -oP 'versionName\s*=\s*"\K[^"]+' "$APP_DIR/app/build.gradle.kts" | head -n1)"
ASSET="$(dirname "$APK")/cbg-plus-$TAG.apk"
cp -f "$APK" "$ASSET"
sha256sum "$ASSET" | awk '{print $1}' > "$ASSET.sha256"

gh release create "$TAG" "$ASSET" "$ASSET.sha256" \
  --repo "$REPO" \
  --target plus \
  --title "CBG Plus -- $TAG" \
  --notes "Private test build of CBG Plus, from plus @ $(git rev-parse --short HEAD) (engine version $VNAME).

Installs alongside the free CBG app: this is at.clavierhaus.backgammon, the
free edition is com.clavierhaus.gnubg. Both can sit on the same device.

Android will warn about installing outside the Play Store; allow it for your
browser or file manager once. Download the .apk and open it.

Not versioned: tags are timestamps, and the newest release is always the
current one." \
  || die "gh release create failed"

ok "published $TAG"
gh release view "$TAG" --repo "$REPO" --json url -q .url
