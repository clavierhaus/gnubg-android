#!/usr/bin/env bash
#
# release_fdroid.sh -- one-command GitHub + F-Droid release.
#
#   ./release_fdroid.sh                       auto-bump patch (0.21.7 -> 0.21.8)
#   ./release_fdroid.sh --version 0.22.0      explicit version
#   ./release_fdroid.sh --summary "..."       fastlane changelog text
#   ./release_fdroid.sh --dry-run             show the plan, change nothing
#
# What it does, start to finish:
#   1. bump versionName/versionCode, roll CHANGELOG, write fastlane changelog
#   2. commit + push to GitHub
#   3. ./release.sh  (signed tag, local build, GitHub release -- the local APK
#      is a PLACEHOLDER reference; Fedora builds never match F-Droid's, see
#      docs/FDROID_SUBMISSION.md)
#   4. update the fdroiddata fork recipe to the new version, push -> CI builds
#      the app in F-Droid's own environment
#   5. wait for the 'fdroid build' job. It FAILS BY DESIGN (compares against
#      the placeholder), but uploads the unsigned APK artifact.
#   6. download the artifact, sign it with the project key, clobber it onto
#      the GitHub release. The reference is now F-Droid's own bytes, signed.
#      F-Droid's buildserver verification then passes by construction.
#
set -euo pipefail

APPID="com.clavierhaus.gnubg"
GL_PROJ="clavierhaus%2Fgnubg-android"      # GitLab fork (URL-encoded path)
GL_HOST="https://gitlab.com"
JOB_NAME="fdroid build"
FDROIDDATA="${FDROIDDATA:-$HOME/fdroiddata}"
BUILD_BRANCH="$APPID"                       # fork branch the CI builds from
POLL_SECS=30
POLL_MAX=90                                 # 45 min ceiling

B=$'\033[1m'; X=$'\033[0m'
ok()   { printf '  %sok%s %s\n' "$B" "$X" "$*"; }
warn() { printf '  %s!!%s %s\n' "$B" "$X" "$*"; }
die()  { printf '  %sx%s  %s\n' "$B" "$X" "$*" >&2; exit 1; }
hr()   { printf -- '----------------------------------------------------------\n'; }

VERSION=""; SUMMARY=""; DRY=0
while [ $# -gt 0 ]; do
  case "$1" in
    --version) VERSION="$2"; shift 2 ;;
    --summary) SUMMARY="$2"; shift 2 ;;
    --dry-run) DRY=1; shift ;;
    *) die "unknown argument: $1" ;;
  esac
done

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]:-$0}")" && pwd)"
ROOT=""; d="$SCRIPT_DIR"
while [ "$d" != / ]; do
  [ -d "$d/jni-bridge" ] && [ -d "$d/.git" ] && { ROOT="$d"; break; }
  d="$(dirname "$d")"
done
[ -n "$ROOT" ] || die "repo root not found"
cd "$ROOT"
GRADLE="gnubg-app/app/build.gradle.kts"

# --- 0. current + next version -----------------------------------------------
CUR_NAME="$(sed -n 's/.*versionName = "\(.*\)"/\1/p' "$GRADLE")"
CUR_CODE="$(sed -n 's/.*versionCode = \([0-9]*\).*/\1/p' "$GRADLE")"
[ -n "$CUR_NAME" ] && [ -n "$CUR_CODE" ] || die "cannot read version from $GRADLE"
if [ -z "$VERSION" ]; then
  VERSION="$(echo "$CUR_NAME" | awk -F. '{printf "%d.%d.%d", $1, $2, $3 + 1}')"
fi
NEW_CODE=$((CUR_CODE + 1))
TAG="v$VERSION"

hr
printf '%sGNUbg - GitHub + F-Droid release%s\n' "$B" "$X"
hr
printf 'current : %s (code %s)\n' "$CUR_NAME" "$CUR_CODE"
printf 'next    : %s (code %s), tag %s\n' "$VERSION" "$NEW_CODE" "$TAG"
hr

# --- 1. preflight --------------------------------------------------------------
[ -z "$(git status --porcelain)" ] || die "working tree is dirty -- commit or stash first"
[ "$(git rev-parse --abbrev-ref HEAD)" = "main" ] || die "not on main"
git fetch -q origin
[ "$(git rev-parse main)" = "$(git rev-parse origin/main)" ] || die "main not in sync with origin"
command -v gh >/dev/null || die "gh CLI required"
gh auth status >/dev/null 2>&1 || die "gh not authenticated"
[ -d "$FDROIDDATA/.git" ] || die "fdroiddata clone not found at $FDROIDDATA (set FDROIDDATA=...)"
[ -f "gnubg-app/keystore.properties" ] || die "gnubg-app/keystore.properties missing (signing)"
APKSIGNER="$(find "${ANDROID_HOME:-$HOME/Android/Sdk}/build-tools" -name apksigner 2>/dev/null | sort -V | tail -n1)"
[ -n "$APKSIGNER" ] || die "apksigner not found under build-tools"
ok "preflight clean"

if [ "$DRY" -eq 1 ]; then
  hr; ok "DRY RUN -- would do:"
  printf '    bump %s -> %s (code %s), roll CHANGELOG, fastlane %s.txt\n' "$CUR_NAME" "$VERSION" "$NEW_CODE" "$NEW_CODE"
  printf '    commit+push main, ./release.sh (tag %s, GitHub release)\n' "$TAG"
  printf '    update %s recipe -> push branch %s -> CI build\n' "$FDROIDDATA" "$BUILD_BRANCH"
  printf '    download CI unsigned APK, sign, clobber onto release %s\n' "$TAG"
  exit 0
fi

# --- 2. bump + changelog --------------------------------------------------------
sed -i "s/versionCode = $CUR_CODE/versionCode = $NEW_CODE/" "$GRADLE"
sed -i "s/versionName = \"$CUR_NAME\"/versionName = \"$VERSION\"/" "$GRADLE"
sed -i "s/^## \[Unreleased\]/## [Unreleased]\n\n## [$VERSION] -- $(date +%Y-%m-%d)/" CHANGELOG.md
mkdir -p fastlane/metadata/android/en-US/changelogs
printf '%s\n' "${SUMMARY:-Bug fixes and improvements.}" \
  > "fastlane/metadata/android/en-US/changelogs/$NEW_CODE.txt"
# keep the in-repo reference recipe in step with reality
if [ -f "fdroid/$APPID.yml" ]; then
  sed -i -e "s/versionName: $CUR_NAME/versionName: $VERSION/" \
         -e "s/versionCode: $CUR_CODE/versionCode: $NEW_CODE/" \
         -e "s/CurrentVersion: $CUR_NAME/CurrentVersion: $VERSION/" \
         -e "s/CurrentVersionCode: $CUR_CODE/CurrentVersionCode: $NEW_CODE/" \
         "fdroid/$APPID.yml"
fi
git add "$GRADLE" CHANGELOG.md "fastlane/metadata/android/en-US/changelogs/$NEW_CODE.txt" "fdroid/$APPID.yml" 2>/dev/null
git commit -q -m "release: $VERSION"
git push -q origin main
ok "version bumped, pushed"

# --- 3. GitHub release (tag + placeholder APK) ----------------------------------
./release.sh || die "release.sh failed"
ok "GitHub release $TAG published (placeholder reference APK)"

# F-Droid review rule: commit: must be the full commit hash, never a tag name.
TAG_SHA="$(git rev-list -n1 "$TAG")"
[ -n "$TAG_SHA" ] || die "cannot resolve $TAG to a commit"
if [ -f "fdroid/$APPID.yml" ]; then
  sed -i "s/^    commit: .*/    commit: $TAG_SHA/" "fdroid/$APPID.yml"
  git add "fdroid/$APPID.yml"
  git commit -q -m "docs: sync reference recipe commit to $TAG ($TAG_SHA)" || true
  git push -q origin main
fi

# --- 4. fdroiddata fork: recipe -> new version, push -> CI ----------------------
cd "$FDROIDDATA"
git remote get-url upstream >/dev/null 2>&1 || git remote add upstream https://gitlab.com/fdroid/fdroiddata.git
git fetch -q upstream master
git checkout -q -B "$BUILD_BRANCH" upstream/master
META="metadata/$APPID.yml"
if [ ! -f "$META" ]; then
  # app not merged upstream yet: seed from the in-repo reference recipe
  cp "$ROOT/fdroid/$APPID.yml" "$META"
  sed -i "s/^    commit: .*/    commit: $TAG_SHA/" "$META"
else
  sed -i -e "s/versionName: .*/versionName: $VERSION/" \
         -e "s/versionCode: [0-9]*/versionCode: $NEW_CODE/" \
         -e "s/^    commit: .*/    commit: $TAG_SHA/" \
         -e "s/CurrentVersion: [0-9.]*/CurrentVersion: $VERSION/" \
         -e "s/CurrentVersionCode: [0-9]*/CurrentVersionCode: $NEW_CODE/" \
         -e "/^    disable:/d" \
         "$META"
fi
git add "$META"
git commit -q -m "$APPID $VERSION ($NEW_CODE)"
git push -q -f origin "$BUILD_BRANCH"
SHA="$(git rev-parse HEAD)"
cd "$ROOT"
ok "fork recipe pushed ($SHA) -- CI building in F-Droid's environment"

# --- 5. wait for the 'fdroid build' job (fails by design, keeps artifact) -------
printf '  waiting for CI (up to %s min): ' "$((POLL_SECS * POLL_MAX / 60))"
JOB_ID=""; JOB_STATUS=""
for _ in $(seq 1 "$POLL_MAX"); do
  PIPE_ID="$(curl -sf "$GL_HOST/api/v4/projects/$GL_PROJ/pipelines?sha=$SHA&per_page=1" \
    | python3 -c "import json,sys; d=json.load(sys.stdin); print(d[0]['id'] if d else '')")" || PIPE_ID=""
  if [ -n "$PIPE_ID" ]; then
    read -r JOB_ID JOB_STATUS < <(curl -sf "$GL_HOST/api/v4/projects/$GL_PROJ/pipelines/$PIPE_ID/jobs?per_page=50" \
      | python3 -c "
import json,sys
for j in json.load(sys.stdin):
    if j['name'] == '$JOB_NAME':
        print(j['id'], j['status']); break
else:
    print(' ', 'pending')")
    case "$JOB_STATUS" in
      success|failed) break ;;
    esac
  fi
  printf '.'
  sleep "$POLL_SECS"
done
printf '\n'
[ -n "$JOB_ID" ] && [ "$JOB_ID" != " " ] || die "CI job '$JOB_NAME' not found for $SHA"
case "$JOB_STATUS" in
  success) ok "CI build verified on first pass (reference already matched)" ;;
  failed)  ok "CI build finished (comparison failed against placeholder -- expected)" ;;
  *) die "CI job did not finish in time (status: $JOB_STATUS). Re-run later steps manually." ;;
esac

# --- 6. fetch the CI-built unsigned APK, sign, clobber --------------------------
WORK="$(mktemp -d)"
curl -sfL -o "$WORK/artifacts.zip" \
  "$GL_HOST/clavierhaus/gnubg-android/-/jobs/$JOB_ID/artifacts/download" \
  || die "artifact download failed (job $JOB_ID)"
unzip -qo "$WORK/artifacts.zip" -d "$WORK"
# On a verified pass the unsigned APK lands in unsigned/; when the comparison
# fails (placeholder reference -- the normal first pass of every release) the
# built APK is kept as tmp/<appid>_<code>.apk instead. Accept either.
UNSIGNED="$(find "$WORK" -path "*unsigned*" -name "*.apk" | head -n1)"
[ -n "$UNSIGNED" ] || UNSIGNED="$(find "$WORK" -path "*tmp*" -name "${APPID}_${NEW_CODE}.apk" | head -n1)"
[ -n "$UNSIGNED" ] || die "no CI-built APK in artifacts (looked in unsigned/ and tmp/)"
ok "CI-built APK: $(basename "$UNSIGNED")"

KS_FILE="$(sed -n 's/^storeFile=//p' gnubg-app/keystore.properties)"
KS_PASS="$(sed -n 's/^storePassword=//p' gnubg-app/keystore.properties)"
KEY_PASS="$(sed -n 's/^keyPassword=//p' gnubg-app/keystore.properties)"
KEY_ALIAS="$(sed -n 's/^keyAlias=//p' gnubg-app/keystore.properties)"
"$APKSIGNER" sign --ks "$KS_FILE" --ks-key-alias "$KEY_ALIAS" \
  --ks-pass "pass:$KS_PASS" --key-pass "pass:$KEY_PASS" \
  --out "$WORK/app-release.apk" "$UNSIGNED" || die "apksigner failed"
( cd "$WORK" && sha256sum app-release.apk > app-release.apk.sha256 )
gh release upload "$TAG" "$WORK/app-release.apk" "$WORK/app-release.apk.sha256" --clobber \
  || die "gh release upload failed"
rm -rf "$WORK"
ok "reference APK replaced with signed CI build"

hr
ok "released $VERSION -- GitHub done; F-Droid's buildserver verifies + publishes on its next cycle"
hr
