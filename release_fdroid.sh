#!/usr/bin/env bash
#
# release_fdroid.sh -- one-command GitHub + F-Droid release, reproducible.
#
#   ./release_fdroid.sh --version 1.0.2 --summary "..."   the release
#   ./release_fdroid.sh --dry-run ...                     preflight + plan only
#   ./release_fdroid.sh --resume ...                      redo steps 4-5 after an abort
#
# The cycle (2026-09-11; replaces the placeholder design of 0.21-1.0.1):
#   0. preflight: clean tree, gh auth, engine gates (syntax_check, the rollout
#      harness at the shipped worker count), and the REPRODUCIBILITY PROOF --
#      tools/verify_reproducible.sh builds this commit twice in independent
#      worktrees; the unsigned APKs must be byte-identical. No proof, no release.
#   1. bump versionName/versionCode, roll CHANGELOG, write fastlane changelog,
#      commit + push.
#   2. ./release.sh -- signed tag, the same build again, GitHub release with
#      the SIGNED APK. That APK is the reference F-Droid compares against:
#      the recipe's Binaries: points at it, AllowedAPKSigningKeys names our key.
#   3. append a build block for this version to the fdroiddata fork recipe,
#      push the branch -> F-Droid's CI rebuilds the app in its own environment
#      and compares its bytes with the reference.
#   4. wait for the 'fdroid build' job. success = F-Droid reproduced our APK;
#      the merge request can be opened with a green pipeline and nothing to
#      explain. failed = a real finding: the job log says whether the build
#      failed (recipe/environment) or differed (diffoscope names the file);
#      it is fixed in the build and re-tagged. NOTHING is ever uploaded over
#      the reference to make the comparison pass -- that was the old design,
#      and it hid a non-reproducible glib build for three releases.
#
# Why the old design existed: glib compiled its install prefix (a path under
# the checkout) into libglib/libgio/libgirepository, so no two checkouts
# matched and the script shipped F-Droid's own build re-signed. Fixed in
# build_glib_android.sh (prefix "/", placed by DESTDIR). See
# CLAUDE.md, THE F-DROID BUILD CHECK, for the version table this relies on.
#
set -euo pipefail

APPID="com.clavierhaus.gnubg"
GL_PROJ="clavierhaus%2Fgnubg-android"      # GitLab fork (URL-encoded path)
GL_HOST="https://gitlab.com"
JOB_NAME="fdroid build"
FDROIDDATA="${FDROIDDATA:-}"                # default set below: sibling of the repo, never $HOME
BUILD_BRANCH="$APPID"                       # fork branch the CI builds from
POLL_SECS=30
POLL_MAX=90                                 # 45 min ceiling

B=$'\033[1m'; X=$'\033[0m'
ok()   { printf '  %sok%s %s\n' "$B" "$X" "$*"; }
warn() { printf '  %s!!%s %s\n' "$B" "$X" "$*"; }
die()  { printf '  %sx%s  %s\n' "$B" "$X" "$*" >&2; exit 1; }
hr()   { printf -- '----------------------------------------------------------\n'; }

VERSION=""; SUMMARY=""; DRY=0; RESUME=0
while [ $# -gt 0 ]; do
  case "$1" in
    --version) VERSION="$2"; shift 2 ;;
    --summary) SUMMARY="$2"; shift 2 ;;
    --dry-run) DRY=1; shift ;;
    # --resume: the bump is committed and the tag + GitHub release exist
    # (steps 1-2 done); redo only the fdroiddata push and the CI wait. Refuses unless the tree is AT --version and the
    # tag resolves. Added 2026-09-11 after step 4 aborted on a dirty
    # fdroiddata clone and a plain re-run would have bumped 1.0.2 -> 1.0.3.
    --resume) RESUME=1; shift ;;
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
# The fdroiddata clone lives BESIDE the repository (/home/erweitert/fdroiddata
# next to /home/erweitert/gnubg-android), never under $HOME: the shell user
# and the tree's owner differ on the maintainer's machine, and $HOME pointed
# at a clone owned by another uid (git: "dubious ownership", 2026-09-11).
[ -n "$FDROIDDATA" ] || FDROIDDATA="$(dirname "$ROOT")/fdroiddata"

# --- 0. current + next version -----------------------------------------------
CUR_NAME="$(sed -n 's/.*versionName = "\(.*\)"/\1/p' "$GRADLE")"
CUR_CODE="$(sed -n 's/.*versionCode = \([0-9]*\).*/\1/p' "$GRADLE")"
[ -n "$CUR_NAME" ] && [ -n "$CUR_CODE" ] || die "cannot read version from $GRADLE"
if [ -z "$VERSION" ]; then
  VERSION="$(echo "$CUR_NAME" | awk -F. '{printf "%d.%d.%d", $1, $2, $3 + 1}')"
fi
# If the tree is already staged at the requested version (the version
# jump was committed separately), release AS-IS: keep the versionCode
# and skip the bump edits -- the 1.0.0 path.
STAGED=0
if [ "$VERSION" = "$CUR_NAME" ]; then
  STAGED=1
  NEW_CODE=$CUR_CODE
else
  NEW_CODE=$((CUR_CODE + 1))
fi
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
# Engine gates. The harness runs its determinism test at the SHIPPED worker
# count (single source: jni-bridge/src/stubs.c GNUBG_ROLLOUT_WORKERS) and
# refuses on a host with fewer cores than that -- a gate imitated on a
# one-core host is not a gate (2026-09-10). The host is recorded here so
# the release log says where the gate ran.
./tools/syntax_check.sh >/dev/null 2>&1 || die "tools/syntax_check.sh failed -- run it to see why"
printf 'engine gate host: %s cores (%s)\n' "$(nproc)" "$(uname -srm)"
./tools/rollout_harness/run_tests.sh > tmp/release_harness.log 2>&1 \
  || die "tools/rollout_harness/run_tests.sh failed -- see tmp/release_harness.log"
grep -q "ALL TESTS GREEN" tmp/release_harness.log || die "harness did not report ALL TESTS GREEN"
grep -E "^host cores:|DIFFER at|identical at" tmp/release_harness.log
# The reproducibility proof: two independent worktrees of this commit build
# the same unsigned APK. Without it there is no claim to publish, and the
# APK release.sh attaches is what F-Droid's verifier will compare against.
# On --resume the tag exists: prove THAT commit, not HEAD (which has moved on
# by the recipe-sync commit at least); a different commit is a different APK.
PROVE_REF=HEAD; [ "$RESUME" -eq 1 ] && PROVE_REF="$TAG"
./tools/verify_reproducible.sh "$PROVE_REF" > tmp/release_repro.log 2>&1 \
  || die "tools/verify_reproducible.sh $PROVE_REF: NOT REPRODUCIBLE -- see tmp/release_repro.log; fix the build, never the release"
REPRO_SHA="$(sed -n 's/^build a: //p' tmp/release_repro.log | head -n1)"
[ -n "$REPRO_SHA" ] || die "could not read the unsigned APK sha256 from tmp/release_repro.log"
ok "reproducible: unsigned APK $REPRO_SHA (two independent worktrees)"
ok "preflight clean"

if [ "$DRY" -eq 1 ]; then
  hr; ok "DRY RUN -- would do:"
  printf '    bump %s -> %s (code %s), roll CHANGELOG, fastlane %s.txt\n' "$CUR_NAME" "$VERSION" "$NEW_CODE" "$NEW_CODE"
  printf '    commit+push main, ./release.sh (tag %s, GitHub release)\n' "$TAG"
  printf '    update %s recipe -> push branch %s -> CI build\n' "$FDROIDDATA" "$BUILD_BRANCH"
  printf '    wait for the fdroid build job: success = verified, open the MR; failed = fix the build, re-tag\n'
  exit 0
fi

if [ "$RESUME" -eq 1 ]; then
  [ "$STAGED" -eq 1 ] || die "--resume: tree is at $CUR_NAME, not $VERSION -- nothing to resume"
  git rev-parse -q --verify "refs/tags/$TAG" >/dev/null || die "--resume: tag $TAG does not exist"
  ok "resume: bump and tag $TAG already done, skipping to fdroiddata"
fi

# --- 2. bump + changelog --------------------------------------------------------
if [ "$RESUME" -eq 0 ]; then
if [ "$STAGED" -eq 0 ]; then
  sed -i "s/versionCode = $CUR_CODE/versionCode = $NEW_CODE/" "$GRADLE"
  sed -i "s/versionName = \"$CUR_NAME\"/versionName = \"$VERSION\"/" "$GRADLE"
  sed -i "s/^## \[Unreleased\]/## [Unreleased]\n\n## [$VERSION] -- $(date +%Y-%m-%d)/" CHANGELOG.md
fi
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
git commit -q -m "release: $VERSION" || ok "nothing to commit: tree already staged at $VERSION"
git push -q origin main
ok "version bumped, pushed"

# --- 3. GitHub release (tag + the reference APK, signed) ------------------------
./release.sh || die "release.sh failed"
ok "GitHub release $TAG published with the reference APK (unsigned bytes $REPRO_SHA, signed by our key)"
fi # RESUME

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
META="metadata/$APPID.yml"
# The fork clone must be clean before the branch is rebuilt from
# upstream/master. Only our own recipe may be dirty -- it is a leftover of a
# previous run and is regenerated below, so it is restored, and said so.
# Anything else dirty is not ours to discard.
if [ -n "$(git status --porcelain -- "$META")" ]; then
  warn "restoring local edits to $META in $FDROIDDATA (leftover of a previous run; regenerated below)"
  git checkout -q -- "$META"
fi
# Untracked files are fdroid's own working dirs (tmp/, unsigned/, logs/,
# build/ ...) and do not affect checkout -B; only TRACKED modifications do.
[ -z "$(git status --porcelain --untracked-files=no)" ] \
  || die "fdroiddata clone has tracked modifications beyond $META -- inspect: git -C $FDROIDDATA status --porcelain --untracked-files=no"
git remote get-url upstream >/dev/null 2>&1 || git remote add upstream https://gitlab.com/fdroid/fdroiddata.git
git fetch -q upstream master
git checkout -q -B "$BUILD_BRANCH" upstream/master
if [ ! -f "$META" ]; then
  # app not merged upstream yet: seed from the in-repo reference recipe
  cp "$ROOT/fdroid/$APPID.yml" "$META"
  sed -i "s/^    commit: .*/    commit: $TAG_SHA/" "$META"
else
  # APPEND a build block: the recipe keeps every published version, so a
  # global sed over versionName/versionCode/commit rewrites ALL of them to the
  # new one -- three identical blocks, "Builds has non-unique elements",
  # "Found invalid versionCodes", no APK (1.0.2, 2026-09-10). The last block
  # is copied with the three fields replaced and any disable: line dropped;
  # everything above it is untouched. Re-runs are idempotent: an existing
  # block for this versionCode is replaced, not duplicated.
  python3 "$ROOT/tools/fdroid_recipe_append.py" "$META" "$VERSION" "$NEW_CODE" "$TAG_SHA" || die "recipe update failed"
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
  success)
    ok "F-Droid's CI rebuilt $TAG and it MATCHED the reference APK -- the reproducibility claim, proven by F-Droid" ;;
  failed)
    hr
    printf '  x  F-Droid'"'"'s CI build of %s did NOT match the reference APK.\n' "$TAG" >&2
    printf '     Job: %s/clavierhaus/gnubg-android/-/jobs/%s\n' "$GL_HOST" "$JOB_ID" >&2
    printf '     Read the job log. If the build failed before comparing, it is the recipe or\n' >&2
    printf '     the environment. If it built and differed, download the artifact and run\n' >&2
    printf '     diffoscope against gnubg-app/app/build/outputs/apk/release/app-release-unsigned.apk;\n' >&2
    printf '     the differing file names the component (CLAUDE.md, THE F-DROID BUILD CHECK).\n' >&2
    printf '     Nothing is replaced on the GitHub release: the reference APK is what we\n' >&2
    printf '     claim, and the claim is fixed in the build, then re-tagged -- never by\n' >&2
    printf '     uploading F-Droid'"'"'s own build as if it were ours.\n' >&2
    die "release $TAG is NOT verified; do not open the fdroiddata merge request" ;;
  *) die "CI job did not finish in time (status: $JOB_STATUS). Re-run with --resume later." ;;
esac

hr
ok "released $VERSION -- verified by F-Droid's CI. Open the merge request from branch $BUILD_BRANCH of the fork; its pipeline is already green."
hr
