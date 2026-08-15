#!/usr/bin/env bash
# verify_reproducible.sh -- build this tree's HEAD twice, independently,
# and prove the unsigned release APKs are byte-identical.
#
# This is the gate for the claim "reproducible build", for either
# edition: run it in the gnubg-android tree and it verifies CBG; run it
# in the cbg-pro tree (plus branch) and it verifies CBG Pro. It contains
# NO build logic of its own -- each worktree runs the repository's own
# build scripts exactly as a release does (single-source law: this
# script orchestrates, it never re-derives).
#
# Method: two fresh `git worktree` checkouts of HEAD; in each, the full
# native build (glib + engine) and `gradle assembleRelease` with NO
# keystore.properties, so the output is the unsigned release APK -- the
# byte-comparable artifact (F-Droid's own definition: signatures are
# attached to identical bytes, never part of them). Then sha256 both.
#
# Honest cost: the native build runs TWICE, glib included. That is the
# point -- shared intermediates would prove nothing. Expect the runtime
# of two full release builds.
#
# Exit code: 0 = reproducible (bytes identical), 1 = not (worktrees are
# KEPT on failure so the two APKs can be diffed; diffoscope recommended).

set -u

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT" || exit 1

die() { echo "verify_reproducible: $*" >&2; exit 1; }

command -v git >/dev/null || die "git is required"
[ -x "$ROOT/build_native_android.sh" ] || die "build_native_android.sh not found -- run from a full checkout"
git -C "$ROOT" diff --quiet || die "worktree has uncommitted changes -- the claim is about a COMMIT; commit or stash first"

HEAD_SHA=$(git -C "$ROOT" rev-parse HEAD) || die "cannot resolve HEAD"
echo "== verifying reproducibility of commit $HEAD_SHA =="

WT="$ROOT/tmp/repro"
rm -rf "$WT"
git -C "$ROOT" worktree prune
mkdir -p "$WT"

build_one() {
    _name="$1"
    _dir="$WT/$_name"
    echo "== [$_name] worktree checkout =="
    git -C "$ROOT" worktree add --detach "$_dir" "$HEAD_SHA" || die "[$_name] worktree add failed"
    ( cd "$_dir" \
      && echo "== [$_name] native build ==" \
      && ./build_native_android.sh \
      && echo "== [$_name] gradle assembleRelease (unsigned) ==" \
      && cd gnubg-app \
      && ./gradlew --no-daemon :app:assembleRelease \
    ) || die "[$_name] build failed -- its log is above"
    _apk="$_dir/gnubg-app/app/build/outputs/apk/release/app-release-unsigned.apk"
    [ -f "$_apk" ] || die "[$_name] expected unsigned APK missing at $_apk (is keystore.properties present? it must NOT be, for this gate)"
}

build_one a
build_one b

APK_A="$WT/a/gnubg-app/app/build/outputs/apk/release/app-release-unsigned.apk"
APK_B="$WT/b/gnubg-app/app/build/outputs/apk/release/app-release-unsigned.apk"

SHA_A=$(sha256sum "$APK_A" | cut -d' ' -f1)
SHA_B=$(sha256sum "$APK_B" | cut -d' ' -f1)
echo "build a: $SHA_A"
echo "build b: $SHA_B"

if [ "$SHA_A" = "$SHA_B" ]; then
    echo "REPRODUCIBLE: two independent builds of $HEAD_SHA are byte-identical."
    git -C "$ROOT" worktree remove --force "$WT/a"
    git -C "$ROOT" worktree remove --force "$WT/b"
    rm -rf "$WT"
    exit 0
else
    echo "NOT REPRODUCIBLE: the two builds differ." >&2
    echo "Both worktrees are KEPT under $WT for diffing:" >&2
    echo "  diffoscope '$APK_A' '$APK_B'" >&2
    exit 1
fi
