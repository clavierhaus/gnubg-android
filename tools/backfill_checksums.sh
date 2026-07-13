#!/usr/bin/env bash
# backfill_checksums.sh -- attach a SHA256 sidecar to releases published
# before release.sh started doing it. For each tag given (or all releases if
# none given), download the release's .apk, compute .sha256, and upload it as
# a new asset. Idempotent: skips a release that already has the sidecar.
# Needs: gh (authenticated), sha256sum or shasum. Reads nothing secret.
set -eu

command -v gh >/dev/null || { echo "gh not found"; exit 1; }
gh auth status >/dev/null 2>&1 || { echo "gh not authenticated -- run: gh auth login"; exit 1; }
if command -v sha256sum >/dev/null; then SHA() { sha256sum "$1"; }
elif command -v shasum >/dev/null; then SHA() { shasum -a 256 "$1"; }
else echo "no sha256sum/shasum"; exit 1; fi

REPO="clavierhaus/gnubg-android"
TAGS="$*"
if [ -z "$TAGS" ]; then
  TAGS="$(gh release list --repo "$REPO" --limit 100 | cut -f3)"
fi

work="$(mktemp -d)"; trap 'rm -rf "$work"' EXIT
for TAG in $TAGS; do
  printf '\n=== %s ===\n' "$TAG"
  assets="$(gh release view "$TAG" --repo "$REPO" --json assets \
            --jq '.assets[].name' 2>/dev/null || true)"
  [ -n "$assets" ] || { echo "  no such release / no assets -- skip"; continue; }
  apk="$(printf '%s\n' "$assets" | grep -E '\.apk$' | head -n1 || true)"
  [ -n "$apk" ] || { echo "  no .apk asset -- skip"; continue; }
  if printf '%s\n' "$assets" | grep -qx "$apk.sha256"; then
    echo "  $apk.sha256 already present -- skip"; continue
  fi
  echo "  downloading $apk ..."
  ( cd "$work" && gh release download "$TAG" --repo "$REPO" --pattern "$apk" --clobber )
  ( cd "$work" && SHA "$apk" > "$apk.sha256" )
  echo "  sha256: $(cut -d' ' -f1 "$work/$apk.sha256")"
  gh release upload "$TAG" "$work/$apk.sha256" --repo "$REPO" --clobber
  echo "  uploaded $apk.sha256"
  rm -f "$work/$apk" "$work/$apk.sha256"
done
echo
echo "done."
