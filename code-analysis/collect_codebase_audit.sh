#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'HELP'
Usage:
  collect_codebase_audit.sh [--output-dir DIR] [--open]

Creates a complete whole-codebase audit bundle.

Options:
  --output-dir DIR   Write generated audit bundle to DIR.
  --open             Open 00_index.md in $PAGER after generation.
  -h, --help         Show help.

Default output:
  tmp/codebase_audit_<timestamp>/

The script writes only generated audit output. It does not reset, build,
install, clean, or modify project source files.
HELP
}

OPEN_INDEX=0
OUT_DIR=""

while [ "$#" -gt 0 ]; do
  case "$1" in
    --output-dir)
      shift
      [ "$#" -gt 0 ] || {
        echo "missing argument for --output-dir" >&2
        exit 2
      }
      OUT_DIR="$1"
      ;;
    --open)
      OPEN_INDEX=1
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "unknown argument: $1" >&2
      usage >&2
      exit 2
      ;;
  esac
  shift
done

SCRIPT_DIR="$(
  cd "$(dirname "${BASH_SOURCE[0]}")" &&
  pwd
)"

ROOT="$(
  git -C "$SCRIPT_DIR" rev-parse --show-toplevel
)"

cd "$ROOT"

STAMP="$(date +%Y%m%d-%H%M%S)"
if [ -z "$OUT_DIR" ]; then
  OUT_DIR="tmp/codebase_audit_${STAMP}"
fi

mkdir -p "$OUT_DIR/resource_binaries"
WORK_DIR="$OUT_DIR/.work"
mkdir -p "$WORK_DIR"

HEAD_FULL="$(git rev-parse HEAD)"
HEAD_SHORT="$(git rev-parse --short HEAD)"
HEAD_SUBJECT="$(git show -s --format=%s HEAD)"
HEAD_DATE="$(git show -s --format=%cI HEAD)"
GENERATED_AT="$(date -Iseconds)"

ALL_FILES="$WORK_DIR/all_candidate_files.txt"
TEXT_FILES="$WORK_DIR/text_files.txt"
BINARY_FILES="$WORK_DIR/binary_files.txt"
RESOURCE_BINARIES="$WORK_DIR/resource_binaries.txt"

is_excluded_path() {
  case "$1" in
    .git|.git/*) return 0 ;;
    .gradle|.gradle/*) return 0 ;;
    tmp|tmp/*) return 0 ;;
    external/backgammon-teacher|external/backgammon-teacher/*) return 0 ;;
    build|build/*) return 0 ;;
    */build|*/build/*) return 0 ;;
    jni-bridge/build-*) return 0 ;;
    *.apk|*.aab|*.aar|*.dex|*.class|*.o|*.lo|*.a|*.so|*.dylib|*.dll|*.exe) return 0 ;;
    *.zip|*.tar|*.tgz|*.gz|*.bz2|*.xz|*.7z) return 0 ;;
  esac
  return 1
}

is_text_file() {
  local path="$1"
  [ -f "$path" ] || return 1
  grep -Iq . "$path"
}

write_title() {
  local file="$1"
  local title="$2"

  {
    echo "# $title"
    echo
    echo "- Generated: $GENERATED_AT"
    echo "- Repository: $ROOT"
    echo "- Cutoff commit: $HEAD_FULL"
    echo "- Cutoff short: $HEAD_SHORT"
    echo "- Cutoff subject: $HEAD_SUBJECT"
    echo "- Cutoff date: $HEAD_DATE"
    echo
  } > "$file"
}

append_cmd() {
  local file="$1"
  local title="$2"
  shift 2

  {
    echo
    echo "## $title"
    echo
    echo '```text'
    "$@" 2>&1 || true
    echo '```'
  } >> "$file"
}

dump_file() {
  local out="$1"
  local path="$2"

  {
    echo
    echo "----- BEGIN FILE: $path -----"
    echo
    cat "$path" || true
    echo
    echo "----- END FILE: $path -----"
  } >> "$out"
}

redact_file() {
  local path="$1"

  sed -E \
    -e 's#^([[:alnum:]._-]*(password|passwd|secret|token|apikey|api_key|storepassword|keypassword|signingkey)[[:alnum:]._-]*[[:space:]]*=).*#\1 <REDACTED>#Ig' \
    -e 's#^(sdk\.dir[[:space:]]*=).*#\1 <REDACTED_LOCAL_PATH>#' \
    -e 's#^(ndk\.dir[[:space:]]*=).*#\1 <REDACTED_LOCAL_PATH>#' \
    "$path"
}

copy_resource_binary() {
  local path="$1"
  local dest="$OUT_DIR/resource_binaries/$path"

  mkdir -p "$(dirname "$dest")"
  cp "$path" "$dest"
}

emit_large_files() {
  while IFS= read -r f; do
    [ -f "$f" ] || continue
    lines="$(wc -l < "$f" 2>/dev/null || echo 0)"
    if [ "$lines" -ge 250 ]; then
      printf '%6s %s\n' "$lines" "$f"
    fi
  done < "$TEXT_FILES" | sort -nr
}

find . \
  \( -path './.git' \
     -o -path './.gradle' \
     -o -path './tmp' \
     -o -path './external/backgammon-teacher' \
     -o -path './build' \
     -o -path './jni-bridge/build-*' \
     -o -path '*/build' \) -prune \
  -o -type f -print \
  | sed 's#^\./##' \
  | sort > "$ALL_FILES"

: > "$TEXT_FILES"
: > "$BINARY_FILES"
: > "$RESOURCE_BINARIES"

while IFS= read -r path; do
  if is_excluded_path "$path"; then
    continue
  fi

  if is_text_file "$path"; then
    echo "$path" >> "$TEXT_FILES"
  else
    echo "$path" >> "$BINARY_FILES"
    case "$path" in
      gnubg-app/app/src/main/res/*)
        echo "$path" >> "$RESOURCE_BINARIES"
        copy_resource_binary "$path"
        ;;
    esac
  fi
done < "$ALL_FILES"

INDEX="$OUT_DIR/00_index.md"

cat > "$INDEX" <<EOF_INDEX
# GNUbg Android codebase audit index

This bundle is intended for second and third opinion review by external LLMs or
human reviewers.

It is a whole-codebase audit bundle.

It is not merely a JNI integration audit.
It is not merely a de-Androidification audit.

## Cutoff

- Commit: \`$HEAD_FULL\`
- Short: \`$HEAD_SHORT\`
- Subject: $HEAD_SUBJECT
- Commit date: $HEAD_DATE
- Generated: $GENERATED_AT

## Central source-of-truth rule

GNUbg is the gameplay authority.

For every gameplay-domain concept, reviewers should ask:

    Where is the authority?

If the answer is not GNUbg, or a documented faithful GNUbg-derived facade, the
code must be flagged.

This applies to:

- board representation;
- board orientation;
- move legality;
- dice use and dice consumption;
- bar and bear-off semantics;
- pip counts;
- cube ownership and cube actions;
- match score and match state;
- resignation;
- no-legal-move flow;
- turn transitions;
- equity/evaluation/analysis;
- tutor/review decisions;
- command sequencing;
- imported/exported positions.

## Whole-codebase review questions

1. Is the repository cleanly structured?
2. Is the build system coherent and reproducible?
3. Are Android/Kotlin, Java, resources, JNI, native bridge, and engine-core
   responsibilities cleanly separated?
4. Which concepts have multiple owners?
5. Where is gameplay logic reimplemented outside GNUbg?
6. Where does JNI/mobile/native code duplicate or reinterpret GNUbg concepts?
7. Where do docs describe an architecture that the code no longer follows?
8. What test gaps block reliable handover?
9. Which files or concepts are stale, dead, temporary, misleading, or overgrown?
10. What must be fixed before a coherent cleanup or de-Androidification sweep?

## Recommended review order

1. \`00_index.md\`
2. \`01_repo_structure.txt\`
3. \`03_build_system.md\`
4. \`11_gnubg_source_of_truth_audit.md\`
5. \`05_kotlin_sources.md\`
6. \`08_jni_bridge.md\`
7. \`10_engine_core_sources.md\`
8. \`12_reinvention_duplication_scan.md\`
9. \`13_architecture_state_ownership.md\`
10. \`15_documentation.md\`
11. \`17_test_coverage.md\`
12. \`14_code_quality_hotspots.md\`

## Files in this bundle

- \`00_index.md\`: master index
- \`01_repo_structure.txt\`: tree and inventory
- \`02_git_log.txt\`: git status, branches, remotes, recent log
- \`03_build_system.md\`: Gradle/CMake/build configuration
- \`04_android_manifest.xml\`: AndroidManifest.xml
- \`05_kotlin_sources.md\`: Kotlin sources
- \`06_java_sources.md\`: Java sources, if any
- \`07_android_resources.md\`: text Android resources
- \`08_jni_bridge.md\`: JNI/native bridge sources
- \`09_cmake_and_headers.md\`: CMake, make files, headers
- \`10_engine_core_sources.md\`: embedded GNUbg/engine-core sources
- \`11_gnubg_source_of_truth_audit.md\`: central source-of-truth audit
- \`12_reinvention_duplication_scan.md\`: duplicated concept scan
- \`13_architecture_state_ownership.md\`: architecture/state ownership map
- \`14_code_quality_hotspots.md\`: broad code quality scan
- \`15_documentation.md\`: docs, README, TeX, audit docs
- \`16_documentation_code_discrepancies.md\`: doc/code drift scan
- \`17_test_coverage.md\`: tests and test gaps
- \`18_binary_resource_inventory.md\`: binary/resource inventory
- \`19_all_text_sources.md\`: complete text source dump
- \`20_integrity_hashes.sha256\`: hashes of generated files
- \`21_archive.sha256\`: archive hash
- \`resource_binaries/\`: copied Android resource binaries
- \`codebase_audit_bundle.tar.gz\`: uploadable archive

## Exclusions

- \`.git/\`
- \`.gradle/\`
- \`tmp/\`
- build directories
- generated native/CMake output
- APK/AAB/class/object/shared-library build outputs
- archive files
- \`external/backgammon-teacher/\`
EOF_INDEX

repo_structure="$OUT_DIR/01_repo_structure.txt"
{
  echo "GNUbg Android repository structure"
  echo
  echo "Generated: $GENERATED_AT"
  echo "Cutoff: $HEAD_SHORT $HEAD_SUBJECT"
  echo
  echo "== directory tree =="
  find . \
    \( -path './.git' \
       -o -path './.gradle' \
       -o -path './tmp' \
       -o -path './external/backgammon-teacher' \
       -o -path './build' \
       -o -path './jni-bridge/build-*' \
       -o -path '*/build' \) -prune \
    -o -type d -print \
    | sed 's#^\./##' \
    | sort
  echo
  echo "== file counts =="
  echo "all candidates: $(wc -l < "$ALL_FILES")"
  echo "text files: $(wc -l < "$TEXT_FILES")"
  echo "binary files: $(wc -l < "$BINARY_FILES")"
  echo "resource binaries copied: $(wc -l < "$RESOURCE_BINARIES")"
  echo
  echo "== text files =="
  cat "$TEXT_FILES"
  echo
  echo "== binary files =="
  cat "$BINARY_FILES"
} > "$repo_structure"

git_log="$OUT_DIR/02_git_log.txt"
{
  echo "GNUbg Android git log overview"
  echo
  echo "Generated: $GENERATED_AT"
  echo "Cutoff: $HEAD_SHORT $HEAD_SUBJECT"
  echo
  echo "== status =="
  git status --short || true
  echo
  echo "== HEAD =="
  git --no-pager show -s --oneline --decorate HEAD || true
  echo
  echo "== remotes =="
  git remote -v || true
  echo
  echo "== branches =="
  git --no-pager branch --all --verbose --no-abbrev || true
  echo
  echo "== commit graph, last 100 =="
  git --no-pager log --graph --oneline --decorate --all -n 100 || true
} > "$git_log"

build_system="$OUT_DIR/03_build_system.md"
write_title "$build_system" "Build system"
{
  echo "## Build file dump"
} >> "$build_system"

while IFS= read -r path; do
  case "$path" in
    settings.gradle|settings.gradle.kts|build.gradle|build.gradle.kts|gradle.properties|\
    local.properties|gnubg-app/*.gradle|gnubg-app/*.gradle.kts|\
    gnubg-app/app/*.gradle|gnubg-app/app/*.gradle.kts|\
    gnubg-app/gradle.properties|jni-bridge/CMakeLists.txt|CMakeLists.txt|\
    engine-core/CMakeLists.txt|*/CMakeLists.txt|*.mk|*/Android.mk|*/Application.mk)
      if [ "$path" = "local.properties" ]; then
        {
          echo
          echo "----- BEGIN FILE: $path REDACTED -----"
          echo
          redact_file "$path"
          echo
          echo "----- END FILE: $path REDACTED -----"
        } >> "$build_system"
      else
        dump_file "$build_system" "$path"
      fi
      ;;
  esac
done < "$TEXT_FILES"

append_cmd "$build_system" "build dependency references" grep -RInE \
  'plugin|android|kotlin|ndk|cmake|sdk|compileSdk|minSdk|targetSdk|namespace|applicationId|jniLibs|externalNativeBuild|CMake|sourceSets|dependencies' \
  settings.gradle settings.gradle.kts build.gradle build.gradle.kts gradle.properties local.properties gnubg-app jni-bridge CMakeLists.txt engine-core 2>/dev/null

append_cmd "$build_system" "detected tool versions" bash -c '
for t in bash git java javac cmake ninja make adb tar sha256sum; do
  echo "### $t"
  if command -v "$t" >/dev/null 2>&1; then
    "$t" --version 2>&1 | sed -n "1,6p" || true
  else
    echo "not found"
  fi
  echo
done
if [ -x gnubg-app/gradlew ]; then
  echo "### gradle wrapper"
  (cd gnubg-app && ./gradlew --version) 2>&1 | sed -n "1,80p" || true
fi
'

manifest_out="$OUT_DIR/04_android_manifest.xml"
if [ -f gnubg-app/app/src/main/AndroidManifest.xml ]; then
  cat gnubg-app/app/src/main/AndroidManifest.xml > "$manifest_out"
else
  echo "AndroidManifest.xml not found" > "$manifest_out"
fi

kotlin_out="$OUT_DIR/05_kotlin_sources.md"
write_title "$kotlin_out" "Kotlin sources"
while IFS= read -r path; do
  case "$path" in
    *.kt|*.kts)
      dump_file "$kotlin_out" "$path"
      ;;
  esac
done < "$TEXT_FILES"

java_out="$OUT_DIR/06_java_sources.md"
write_title "$java_out" "Java sources"
while IFS= read -r path; do
  case "$path" in
    *.java)
      dump_file "$java_out" "$path"
      ;;
  esac
done < "$TEXT_FILES"

res_out="$OUT_DIR/07_android_resources.md"
write_title "$res_out" "Android text resources"
while IFS= read -r path; do
  case "$path" in
    gnubg-app/app/src/main/res/*)
      dump_file "$res_out" "$path"
      ;;
  esac
done < "$TEXT_FILES"

jni_out="$OUT_DIR/08_jni_bridge.md"
write_title "$jni_out" "JNI bridge and native adapter sources"
while IFS= read -r path; do
  case "$path" in
    jni-bridge/*|jni-bridge/src/*|jni-bridge/include/*)
      dump_file "$jni_out" "$path"
      ;;
  esac
done < "$TEXT_FILES"

cmake_headers="$OUT_DIR/09_cmake_and_headers.md"
write_title "$cmake_headers" "CMake, Android make files, and headers"
while IFS= read -r path; do
  case "$path" in
    CMakeLists.txt|*/CMakeLists.txt|*.mk|*/Android.mk|*/Application.mk|*.h|*.hpp)
      dump_file "$cmake_headers" "$path"
      ;;
  esac
done < "$TEXT_FILES"

engine_sources="$OUT_DIR/10_engine_core_sources.md"
write_title "$engine_sources" "Engine-core and embedded GNUbg sources"
while IFS= read -r path; do
  case "$path" in
    engine-core/*|engine-core/*/*|engine-core/*/*/*)
      dump_file "$engine_sources" "$path"
      ;;
  esac
done < "$TEXT_FILES"

sot="$OUT_DIR/11_gnubg_source_of_truth_audit.md"
write_title "$sot" "GNUbg source-of-truth audit"

cat >> "$sot" <<'EOF_SOT'
## Review mandate

This is the central audit chapter.

GNUbg must be treated as the gameplay source of truth.

For every gameplay-domain concept, reviewers should ask:

    Where is the authority?

If the authority is Kotlin, Java, Android UI state, JNI glue, or a mobile facade
that is not documented as a faithful GNUbg-derived exposure, flag it.

JNI is not exempt. JNI/native adapter code is allowed to translate and expose
GNUbg concepts. It is not allowed to become an independent backgammon engine.

## Gameplay-domain concepts that require authority review

- board representation
- point numbering
- player orientation
- move legality
- legal move generation
- dice ownership
- dice use and dice consumption
- bar handling
- bear-off rules
- pip counts
- cube state
- double/take/drop logic
- match score
- match length
- Crawford/post-Crawford state
- resignation
- no-legal-move flow
- turn transitions
- position import/export
- equity/evaluation
- analysis
- tutor/review recommendations
- SGF/game record semantics
EOF_SOT

append_cmd "$sot" "Android/Kotlin possible gameplay authority" grep -RInE \
  'pip.?count|pipCount|equity|eval|evaluate|evaluation|analysis|analyse|tutor|review|doubling.?cube|cube|double|take|drop|bear.?off|bearoff|bar|valid.?move|legal.?move|legalMoves|GenerateMoves|getLegalMoves|applySubMove|move.?legal|dice|die|remainingDice|blockedDice|consume|turn|match.?score|matchScore|matchLength|Crawford|resign|no.?legal|board\[|BoardState|point|orientation|swapBoard|source|destination|src|dst|position|PositionId|SGF' \
  gnubg-app/app/src/main 2>/dev/null

append_cmd "$sot" "JNI/native adapter possible gameplay authority" grep -RInE \
  'pip.?count|pipCount|equity|eval|evaluate|evaluation|analysis|analyse|tutor|review|doubling.?cube|cube|double|take|drop|bear.?off|bearoff|bar|valid.?move|legal.?move|GenerateMoves|ApplySubMove|CommandMove|CommandRoll|CommandDouble|CommandTake|CommandDrop|CommandResign|PositionKey|EqualKeys|FormatMove|ParseMove|SwapSides|PipCount|TanBoard|anBoard|anDice|ms\.|msBoard|NextTurn|TurnDone|match|Crawford|SGF|position' \
  jni-bridge engine-core 2>/dev/null

append_cmd "$sot" "Source-of-truth documentation claims" grep -RInE \
  'source of truth|GNUbg.*truth|bible|authority|upstream|engine-core|embedded|adapted|facade|JNI|native|Android.*logic|Kotlin.*logic|de-Android|reinvent|duplicate' \
  docs README* code-analysis gnubg-app jni-bridge engine-core 2>/dev/null

append_cmd "$sot" "Magic-number board/game logic outside engine-core" grep -RInE \
  '(^|[^A-Za-z0-9_])(24|25|26|49|50|96|144|1296)([^A-Za-z0-9_]|$)|point - 1|24 \+|indices step|until 8|IntArray\(' \
  gnubg-app jni-bridge 2>/dev/null

append_cmd "$sot" "Parallel state ownership indicators" grep -RInE \
  'board|Board|dice|Dice|cube|Cube|score|Score|match|Match|turn|Turn|phase|state|State|snapshot|Snapshot|cache|oldBoard|newBoard|current|previous|pending|confirmed|selected|available|legal|blocked' \
  gnubg-app jni-bridge docs 2>/dev/null

reinvention="$OUT_DIR/12_reinvention_duplication_scan.md"
write_title "$reinvention" "Reinvention and duplicated-concept scan"

cat >> "$reinvention" <<'EOF_REINV'
This section is a heuristic review map, not proof of defects.

It highlights where multiple layers may own the same concept, or where the app
may have implemented an almost-GNUbg instead of exposing GNUbg faithfully.
EOF_REINV

append_cmd "$reinvention" "Android-side gameplay semantics" grep -RInE \
  'legalMoves|remainingDice|blockedDice|applySubMove|findMove|formatMove|swapBoard|pipCount|getLegalMoves|source|destination|src|dst|bar|bear|cube|double|take|drop|resign|turn|moveStr|oldBoard|board\[|point|die|dice|match|score|analysis|tutor|review' \
  gnubg-app/app/src/main 2>/dev/null

append_cmd "$reinvention" "Native/facade gameplay semantics" grep -RInE \
  'GenerateMoves|ApplySubMove|FormatMove|ParseMove|CommandMove|CommandRoll|CommandDouble|CommandTake|CommandDrop|CommandResign|PositionKey|EqualKeys|SwapSides|PipCount|Bearoff|Cube|Legal|movelist|movefilter|TanBoard|anBoard|anDice|ms\.|snapshot' \
  jni-bridge engine-core 2>/dev/null

append_cmd "$reinvention" "Frequent identifiers across codebase" bash -c '
grep -RhoE "[A-Za-z_][A-Za-z0-9_]{3,}" gnubg-app jni-bridge engine-core docs code-analysis 2>/dev/null \
  | sort | uniq -c | sort -nr | sed -n "1,800p"
'

arch="$OUT_DIR/13_architecture_state_ownership.md"
write_title "$arch" "Architecture and state ownership"
append_cmd "$arch" "package/file ownership map" find gnubg-app jni-bridge engine-core docs code-analysis \
  \( -path '*/build' -o -path '*/tmp' \) -prune -o -type f -print 2>/dev/null

append_cmd "$arch" "state ownership keywords" grep -RInE \
  'state|State|BoardState|GameViewModel|Engine|snapshot|cache|remember|mutable|Mutable|Flow|LiveData|DataStore|Preferences|settings|phase|turn|match|score|dice|board|cube|analysis|review|mode|navigation|route|screen|Screen' \
  gnubg-app jni-bridge engine-core docs 2>/dev/null

append_cmd "$arch" "threading lifecycle synchronization" grep -RInE \
  'Thread|thread|Executor|Coroutine|Dispatchers|launch|suspend|mutex|pthread|lock|unlock|synchronized|volatile|lifecycle|onCreate|onDestroy|remember|LaunchedEffect|DisposableEffect|runBlocking|withContext' \
  gnubg-app jni-bridge engine-core docs 2>/dev/null

quality="$OUT_DIR/14_code_quality_hotspots.md"
write_title "$quality" "Code quality hotspots"
append_cmd "$quality" "large text files (>=250 lines)" emit_large_files

append_cmd "$quality" "TODO/FIXME/HACK/temporary markers" grep -RInE \
  'TODO|FIXME|HACK|XXX|temporary|temp|workaround|stub|placeholder|later|for now|not implemented|deprecated|dead|unused|remove|cleanup|clean-up' \
  gnubg-app jni-bridge engine-core docs code-analysis 2>/dev/null

append_cmd "$quality" "magic numbers and board constants in Kotlin/JNI" grep -RInE \
  '(^|[^A-Za-z0-9_])(24|25|26|49|50|96|144|1296)([^A-Za-z0-9_]|$)|point - 1|24 \+|indices step|until 8|IntArray\(' \
  gnubg-app/app/src/main jni-bridge 2>/dev/null

append_cmd "$quality" "error handling and logging" grep -RInE \
  'try|catch|throw|require|check|assert|Log\.|println|printf|fprintf|g_warning|error|fatal|return null|return@|?:' \
  gnubg-app jni-bridge engine-core docs 2>/dev/null

append_cmd "$quality" "imports and resource references" grep -RInE \
  '^package |^import |R\.drawable|R\.string|R\.color|painterResource|stringResource|colorResource' \
  gnubg-app/app/src/main gnubg-app/app/src/main/res 2>/dev/null

docs_out="$OUT_DIR/15_documentation.md"
write_title "$docs_out" "Documentation"
while IFS= read -r path; do
  case "$path" in
    *.md|*.tex|README*|docs/*|code-analysis/*)
      dump_file "$docs_out" "$path"
      ;;
  esac
done < "$TEXT_FILES"

disc="$OUT_DIR/16_documentation_code_discrepancies.md"
write_title "$disc" "Documentation/code discrepancy scan"

append_cmd "$disc" "documentation architecture claims" grep -RInE \
  'architecture|facade|JNI|native|Android|Kotlin|GNUbg|engine-core|source of truth|bible|authority|mode|tutor|analyse|analysis|cube|move|dice|board|bear|bar|documentation|handover|roadmap|TODO|FIXME' \
  docs README* code-analysis 2>/dev/null

append_cmd "$disc" "code terms likely needing documentation" grep -RInE \
  'HomeHub|GameViewModel|BoardState|Engine|SettingsScreen|Options|Analyse|Profile|Learn|Tutor|Review|Cube|Resign|Match|DataStore|Preferences|gnubg_mobile|native-lib|engine-core|CommandMove|GenerateMoves|ApplySubMove' \
  gnubg-app jni-bridge engine-core 2>/dev/null

{
  echo
  echo "## Automatic documentation gap flags"
  echo
  if grep -RInE 'upstream|engine-core|adapt|vendor|embedded|source of truth|authority' docs README* 2>/dev/null >/dev/null; then
    echo "- Upstream/GNUbg authority documentation: detected, review for accuracy."
  else
    echo "- Upstream/GNUbg authority documentation: not detected."
  fi

  if grep -RInE 'facade|JNI|native|Android|Kotlin|architecture' docs README* 2>/dev/null >/dev/null; then
    echo "- Architecture/JNI/native documentation: detected, review for accuracy."
  else
    echo "- Architecture/JNI/native documentation: not detected."
  fi

  if grep -RInE 'test|coverage|junit|androidTest|unit test' docs README* 2>/dev/null >/dev/null; then
    echo "- Test documentation: detected, review for accuracy."
  else
    echo "- Test documentation: not detected."
  fi
} >> "$disc"

tests="$OUT_DIR/17_test_coverage.md"
write_title "$tests" "Test coverage"
append_cmd "$tests" "test files" find . \
  \( -path './.git' -o -path './tmp' -o -path './.gradle' -o -path './external/backgammon-teacher' -o -path '*/build' -o -path './jni-bridge/build-*' \) -prune \
  -o \( -iname '*test*' -o -path '*androidTest*' -o -path '*test*' \) -print

while IFS= read -r path; do
  case "$path" in
    *Test.kt|*Test.java|*androidTest*|*test*|*Test.c|*test*.c|*Test.cpp|*test*.cpp)
      if [ -f "$path" ] && is_text_file "$path"; then
        dump_file "$tests" "$path"
      fi
      ;;
  esac
done < "$TEXT_FILES"

append_cmd "$tests" "test dependencies and tasks" grep -RInE \
  'testImplementation|androidTestImplementation|junit|espresso|mockk|robolectric|connectedAndroidTest|testDebugUnitTest|CTest|add_test|enable_testing' \
  . 2>/dev/null

binary_inv="$OUT_DIR/18_binary_resource_inventory.md"
write_title "$binary_inv" "Binary and resource inventory"

{
  echo "Binary files are inventoried by path, size, and SHA-256."
  echo
  echo "Android resource binaries are copied into resource_binaries/ for review."
  echo
  echo '```text'
  while IFS= read -r path; do
    [ -f "$path" ] || continue
    size="$(wc -c < "$path" | tr -d ' ')"
    sha="$(sha256sum "$path" | awk '{print $1}')"
    copied="no"
    if grep -Fxq "$path" "$RESOURCE_BINARIES"; then
      copied="yes"
    fi
    printf '%s  %s bytes  copied=%s  %s\n' "$sha" "$size" "$copied" "$path"
  done < "$BINARY_FILES"
  echo '```'
} >> "$binary_inv"

all_text="$OUT_DIR/19_all_text_sources.md"
write_title "$all_text" "All captured text sources"
while IFS= read -r path; do
  dump_file "$all_text" "$path"
done < "$TEXT_FILES"

integrity="$OUT_DIR/20_integrity_hashes.sha256"
TAR_PATH="$OUT_DIR/codebase_audit_bundle.tar.gz"
ARCHIVE_HASH="$OUT_DIR/21_archive.sha256"

(
  cd "$OUT_DIR"
  find . -type f \
    ! -path './.work/*' \
    ! -name '20_integrity_hashes.sha256' \
    ! -name 'codebase_audit_bundle.tar.gz' \
    ! -name '21_archive.sha256' \
    -print \
    | sed 's#^\./##' \
    | sort \
    | while IFS= read -r file; do
        sha256sum "$file"
      done
) > "$integrity"

(
  cd "$OUT_DIR"
  find . -mindepth 1 -type f \
    ! -path './.work/*' \
    ! -name 'codebase_audit_bundle.tar.gz' \
    ! -name '21_archive.sha256' \
    -print \
    | sed 's#^\./##' \
    | sort > ".work/tar_files.txt"

  tar -czf "codebase_audit_bundle.tar.gz" \
    -T ".work/tar_files.txt"
)

(
  cd "$OUT_DIR"
  sha256sum "codebase_audit_bundle.tar.gz"
) > "$ARCHIVE_HASH"

echo "Audit output directory:"
echo "$OUT_DIR"
echo
echo "Uploadable archive:"
echo "$TAR_PATH"
echo
echo "Archive hash:"
echo "$ARCHIVE_HASH"
echo
echo "Index:"
echo "$INDEX"
echo
echo "Generated deliverable files:"
find "$OUT_DIR" -maxdepth 2 -type f \
  ! -path "$OUT_DIR/.work/*" \
  -printf '%10s bytes  %p\n' \
  | sort -nr \
  | sed -n '1,220p'
echo
echo "Internal scratch files:"
find "$OUT_DIR/.work" -maxdepth 1 -type f \
  -printf '%10s bytes  %p\n' \
  | sort -nr \
  | sed -n '1,80p'
echo

if [ "$OPEN_INDEX" -eq 1 ]; then
  if [ -n "${PAGER:-}" ] && command -v "$PAGER" >/dev/null 2>&1; then
    "$PAGER" "$INDEX"
  elif command -v less >/dev/null 2>&1; then
    less "$INDEX"
  else
    sed -n '1,220p' "$INDEX"
  fi
fi
