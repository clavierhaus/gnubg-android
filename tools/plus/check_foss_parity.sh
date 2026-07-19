#!/usr/bin/env bash
# FOSS-parity audit: the plus branch must be byte-identical to the public
# FOSS main outside the paths in plus-overlay.paths, and private main must
# mirror public main exactly. ANY other difference is an ALARM (exit 1).
set -euo pipefail
R=$'\033[1;31m'; G=$'\033[1;32m'; X=$'\033[0m'
alarm() { printf '%s############  FOSS PARITY ALARM  ############%s\n' "$R" "$X"; printf '%s%s%s\n' "$R" "$*" "$X"; exit 1; }
FOSS_REF="${FOSS_REF:-upstream/main}"
PLUS_REF="${PLUS_REF:-HEAD}"
MIRROR_REF="${MIRROR_REF:-origin/main}"
git rev-parse --verify -q "$FOSS_REF" >/dev/null || alarm "FOSS ref $FOSS_REF not found -- fetch upstream first"
# 1. mirror equality
[ "$(git rev-parse "$MIRROR_REF")" = "$(git rev-parse "$FOSS_REF")" ] \
  || alarm "private main ($MIRROR_REF) != public main ($FOSS_REF) -- mirror broken"
# 2. plus contains every FOSS commit
git merge-base --is-ancestor "$FOSS_REF" "$PLUS_REF" \
  || alarm "plus branch is missing FOSS commits -- sync overdue"
# 3. content: zero differences outside the overlay manifest
DIFF=$(git diff --name-only "$FOSS_REF" "$PLUS_REF")
BAD=""
while IFS= read -r f; do
  [ -z "$f" ] && continue
  ok=0
  while IFS= read -r pat; do
    case "$pat" in ''|'#'*) continue;; esac
    case "$f" in "$pat"*|"$pat") ok=1; break;; esac
  done < plus-overlay.paths
  [ "$ok" = 1 ] || BAD="$BAD$f"$'\n'
done <<< "$DIFF"
[ -z "$BAD" ] || alarm "paths differ OUTSIDE the overlay manifest:"$'\n'"$BAD"
printf '%sFOSS PARITY OK%s  (mirror exact; plus = FOSS + manifest overlay only)\n' "$G" "$X"
