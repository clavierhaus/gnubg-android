#!/usr/bin/env bash
# Fails if any file the Android build compiles or includes is gitignored --
# i.e. if a fresh clone would be unbuildable. Run in pre-commit / CI.
set -u
cd "$(dirname "$0")/.."
bad=0
# Compiled .c from the CMake list + the jni-bridge sources.
mapfile -t csrc < <(grep -oE '\$\{ENGINE\}/[A-Za-z0-9_./-]+\.c' jni-bridge/CMakeLists.txt \
                    | sed 's#${ENGINE}#engine-core#')
csrc+=(jni-bridge/src/gnubg_mobile.c jni-bridge/src/native-lib.c \
       jni-bridge/src/android-app.c jni-bridge/src/stubs.c)
# Their local #include "..." targets, one level (covers the headers that bit us).
# A fresh clone contains a file iff it is TRACKED. git check-ignore was the
# wrong test: it matches per-clone (.git/info/exclude) and per-user (global
# excludesFile) patterns too, and it fires even on tracked files -- which are
# immune to ignore rules. That produced a machine-dependent false positive
# (release blocked on the maintainer's box, passing elsewhere, same commit).
check() { git ls-files --error-unmatch "$1" >/dev/null 2>&1 \
          || { echo "UNTRACKED but build-required: $1"; bad=1; }; }
for c in "${csrc[@]}"; do
  [ -f "$c" ] && check "$c"
  [ -f "$c" ] || continue
  while read -r h; do
    for d in "$(dirname "$c")" engine-core engine-core/lib jni-bridge jni-bridge/include jni-bridge/src; do
      [ -f "$d/$h" ] && { check "$d/$h"; break; }
    done
  done < <(grep -oE '#[[:space:]]*include[[:space:]]+"[^"]+"' "$c" | sed 's/.*"\(.*\)"/\1/')
done
if [ "$bad" -eq 0 ]; then echo "OK: every build-required file is tracked."; fi
exit "$bad"
