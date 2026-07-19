#!/usr/bin/env bash
# CBG PLUS build -- refuses to run in a tree that is not the Plus edition.
set -euo pipefail
B=$'\033[1;33m'; X=$'\033[0m'
grep -q 'applicationId = "at.clavierhaus.backgammon"' gnubg-app/app/build.gradle.kts \
  || { echo "This tree is NOT the Plus edition (applicationId mismatch). Refusing."; exit 1; }
printf '%s============================================%s\n' "$B" "$X"
printf '%s   CBG  P L U S   --  at.clavierhaus.backgammon%s\n' "$B" "$X"
printf '%s============================================%s\n' "$B" "$X"
exec ./build_and_deploy.sh "$@"
