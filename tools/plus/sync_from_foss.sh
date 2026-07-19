#!/usr/bin/env bash
# The synchronisation law, executable: pull every FOSS change into this repo
# NOW -- mirror main, merge into plus, push both, audit. Run after EVERY
# commit/revert/push to the public repo. No exceptions.
set -euo pipefail
git fetch upstream
git fetch origin
git checkout -q main
git merge --ff-only upstream/main
git push -q origin main
git checkout -q plus
git merge --no-edit upstream/main || {
  echo "MERGE CONFLICT: resolve keeping the plus version for overlay files,"
  echo "the FOSS version for everything else, then: git commit, git push,"
  echo "and re-run tools/plus/check_foss_parity.sh"; exit 1; }
git push -q origin plus
exec ./tools/plus/check_foss_parity.sh
