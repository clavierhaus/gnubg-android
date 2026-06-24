#!/usr/bin/env bash
set -euo pipefail

STAMP="$(date +%Y%m%d_%H%M%S)"
OUT_DIR="tmp/debug_packet_${STAMP}"
WORK_DIR="$OUT_DIR/.work"
PACKET="$OUT_DIR/00_debug_packet.md"
ARCHIVE="$OUT_DIR/debug_packet_archive.tar.gz"
MAX_PACKET_BYTES="${MAX_PACKET_BYTES:-1500000}"

mkdir -p "$OUT_DIR" "$WORK_DIR"

show_title() {
  local title="$1"
  {
    echo
    echo
    echo "================================================================"
    echo "$title"
    echo "================================================================"
    echo
  } >> "$PACKET"
}

append_cmd() {
  local title="$1"
  shift

  show_title "$title"
  {
    echo '```text'
    "$@" 2>&1 || true
    echo '```'
  } >> "$PACKET"
}

dump_file() {
  local path="$1"

  if [ ! -f "$path" ]; then
    return 0
  fi

  show_title "FILE: $path"
  {
    echo '```text'
    sed -n '1,260p' "$path"
    lines="$(wc -l < "$path" | tr -d ' ')"
    if [ "$lines" -gt 260 ]; then
      echo
      echo "[truncated: $lines total lines]"
    fi
    echo '```'
  } >> "$PACKET"
}

sha_line() {
  local path="$1"

  if [ -f "$path" ]; then
    bytes="$(wc -c < "$path" | tr -d ' ')"
    sha="$(sha256sum "$path" | awk '{print $1}')"
    printf '%-78s %12s %s\n' "$path" "$bytes" "$sha"
  fi
}

review_files() {
  find \
    gnubg-app \
    jni-bridge \
    docs \
    doc \
    code-analysis \
    test-harness \
    -type f \
    ! -path '*/build/*' \
    ! -path '*/tmp/*' \
    ! -path '*/.gradle/*' \
    ! -path '*/.kotlin/*' \
    ! -path 'jni-bridge/external/glib/*' \
    ! -name '*.apk' \
    ! -name '*.aab' \
    ! -name '*.class' \
    ! -name '*.o' \
    ! -name '*.so' \
    ! -name '*.jar' \
    ! -name '*.pdf' \
    ! -name '*.png' \
    ! -name '*.jpg' \
    ! -name '*.jpeg' \
    ! -name '*.webp' \
    ! -name '*.ttf' \
    2>/dev/null \
    | sort
}

app_jni_grep() {
  local pattern="$1"

  grep -RInE \
    --exclude-dir=.git \
    --exclude-dir=.gradle \
    --exclude-dir=.kotlin \
    --exclude-dir=tmp \
    --exclude-dir=build \
    --exclude-dir=glib \
    "$pattern" \
    gnubg-app jni-bridge 2>/dev/null || true
}

compiled_engine_files() {
  awk '
    /\$\{ENGINE\}\// {
      line = $0
      gsub(/[ \t\r]/, "", line)
      gsub(/\$\{ENGINE\}\//, "engine-core/", line)
      gsub(/[),]/, "", line)
      if (line ~ /^engine-core\/.*\.(c|h)$/) {
        print line
      }
    }
  ' jni-bridge/CMakeLists.txt 2>/dev/null \
    | sort -u
}

compiled_engine_dump_manifest() {
  tmp="$WORK_DIR/compiled_engine_files.txt"
  compiled_engine_files > "$tmp"

  while IFS= read -r f; do
    [ -f "$f" ] || continue
    sha_line "$f"
    case "$f" in
      *.c)
        h="${f%.c}.h"
        [ -f "$h" ] && sha_line "$h"
        ;;
    esac
  done < "$tmp" \
    | sort -u
}

write_header() {
  head="$(git rev-parse --short HEAD 2>/dev/null || echo unknown)"
  full="$(git rev-parse HEAD 2>/dev/null || echo unknown)"
  subject="$(git log -1 --pretty=%s 2>/dev/null || echo unknown)"

  {
    echo "# GNUbg Android debug packet"
    echo
    echo "Generated: $(date -Iseconds)"
    echo "Repository: $(pwd)"
    echo "Commit: $full"
    echo "Short: $head"
    echo "Subject: $subject"
    echo
    echo "## Scope"
    echo
    echo "This is a compact debugging and review packet, not a full source dump."
    echo
    echo "Included:"
    echo
    echo "- repo status and filtered structure"
    echo "- Gradle/CMake/JNI overview"
    echo "- key Kotlin app files"
    echo "- JNI/mobile facade files"
    echo "- packaged native library symbol index"
    echo "- JNI-compiled engine-core source manifest"
    echo "- source-of-truth and duplication scans"
    echo "- app/JNI-only code-quality hotspots"
    echo "- text manifest with paths, sizes, and SHA-256 hashes"
    echo
    echo "Excluded from the packet:"
    echo
    echo "- upstream-source/"
    echo "- raw engine-core dumps"
    echo "- raw all-text dumps"
    echo "- vendored glib tree"
    echo "- Gradle/Kotlin/build/tmp caches"
    echo
    echo "The tar archive beside this packet is the backing store for full files."
  } > "$PACKET"
}

write_packet() {
  write_header

  append_cmd "Git status" git status --short
  append_cmd "Recent commits" git --no-pager log --oneline -20

  append_cmd "Filtered top-level structure" bash -c '
    find . -maxdepth 4 \
      \( -path "./.git" \
         -o -path "./tmp" \
         -o -path "./upstream-source" \
         -o -path "*/.gradle" \
         -o -path "*/.kotlin" \
         -o -path "*/build" \
         -o -path "*/tmp" \
         -o -path "*/external/glib" \) \
      -prune -o -type d -print \
      | sed "s#^\./##" \
      | sort
  '

  append_cmd "Build files" bash -c '
    for f in \
      gnubg-app/settings.gradle.kts \
      gnubg-app/build.gradle.kts \
      gnubg-app/app/build.gradle.kts \
      gnubg-app/gradle/libs.versions.toml \
      gnubg-app/gradle.properties \
      jni-bridge/CMakeLists.txt \
      test-harness/CMakeLists.txt
    do
      [ -f "$f" ] || continue
      echo "----- $f -----"
      sed -n "1,220p" "$f"
      echo
    done
  '

  for f in \
    gnubg-app/app/src/main/kotlin/com/clavierhaus/gnubg/Engine.kt \
    gnubg-app/app/src/main/kotlin/com/clavierhaus/gnubg/engine/BoardState.kt \
    gnubg-app/app/src/main/kotlin/com/clavierhaus/gnubg/engine/GameViewModel.kt \
    gnubg-app/app/src/main/kotlin/com/clavierhaus/gnubg/play/Board.kt \
    gnubg-app/app/src/main/kotlin/com/clavierhaus/gnubg/play/GameLayout.kt \
    jni-bridge/include/gnubg_mobile.h \
    jni-bridge/src/gnubg_mobile.c \
    jni-bridge/src/native-lib.c \
    jni-bridge/src/android-app.c \
    jni-bridge/src/com/clavierhaus/gnubg/Engine.kt
  do
    dump_file "$f"
  done

  append_cmd "Packaged native library info" bash -c '
    so="gnubg-app/app/src/main/jniLibs/arm64-v8a/libgnubg-engine.so"
    if [ -f "$so" ]; then
      file "$so"
      ls -lh "$so"
      sha256sum "$so"
    else
      echo "missing: $so"
    fi
  '

  append_cmd "Native symbols: likely public/JNI/mobile/gameplay" bash -c '
    so="gnubg-app/app/src/main/jniLibs/arm64-v8a/libgnubg-engine.so"
    [ -f "$so" ] || exit 0

    if command -v nm >/dev/null 2>&1; then
      nm -D --defined-only "$so" \
        | grep -Ei "Java_|gnubg|mobile|move|cube|roll|dice|board|match|score|pip|eval|position|resign|accept|reject|double|take|drop|next|game|command" \
        | sort || true
    fi
  '

  append_cmd "Native symbols: undefined imports" bash -c '
    so="gnubg-app/app/src/main/jniLibs/arm64-v8a/libgnubg-engine.so"
    [ -f "$so" ] || exit 0

    if command -v nm >/dev/null 2>&1; then
      nm -D --undefined-only "$so" | sort || true
    fi
  '

  append_cmd "JNI-compiled engine-core source manifest" \
    compiled_engine_dump_manifest

  append_cmd "Source-of-truth scan in app/JNI" bash -c '
    grep -RInE \
      --exclude-dir=.gradle \
      --exclude-dir=.kotlin \
      --exclude-dir=build \
      --exclude-dir=tmp \
      --exclude-dir=glib \
      "legal|illegal|valid|invalid|dice|die|move|turn|cube|double|take|drop|resign|bear|bar|pip|score|match|game|board|position|orientation|swap|findMove|getLegalMoves|remainingDice|apply|confirm|command" \
      gnubg-app jni-bridge 2>/dev/null || true
  '

  append_cmd "Duplication/reinvention scan in app/JNI" bash -c '
    grep -RInE \
      --exclude-dir=.gradle \
      --exclude-dir=.kotlin \
      --exclude-dir=build \
      --exclude-dir=tmp \
      --exclude-dir=glib \
      "data class|enum class|sealed class|fun .*legal|fun .*move|fun .*dice|fun .*cube|fun .*board|class .*Board|class .*Move|remainingDice|fromPoint|toPoint|bar|bear|pip|orientation|swap" \
      gnubg-app jni-bridge 2>/dev/null || true
  '

  append_cmd "Code-quality hotspots in app/JNI only" bash -c '
    grep -RInE \
      --exclude-dir=.gradle \
      --exclude-dir=.kotlin \
      --exclude-dir=build \
      --exclude-dir=tmp \
      --exclude-dir=glib \
      "TODO|FIXME|HACK|XXX|temporary|temp|workaround|stub|placeholder|later|for now|not implemented|deprecated|dead|unused|remove|cleanup|clean-up|println|Log\.|android\.util\.Log|fprintf\(|printf\(|catch|throw|Exception|Throwable|return false|return NULL|return -1|failed|failure|error|panic|fatal|assert\(" \
      gnubg-app jni-bridge 2>/dev/null || true
  '

  append_cmd "Documentation and test manifest" bash -c '
    find docs doc test-harness code-analysis \
      -type f \
      ! -path "*/build/*" \
      ! -path "*/tmp/*" \
      2>/dev/null \
      | sort \
      | while IFS= read -r f; do
          bytes="$(wc -c < "$f" | tr -d " ")"
          sha="$(sha256sum "$f" | awk "{print \$1}")"
          printf "%-78s %12s %s\n" "$f" "$bytes" "$sha"
        done
  '

  append_cmd "All reviewed text files manifest" bash -c '
    printf "%-78s %12s %s\n" "path" "bytes" "sha256"

    find \
      gnubg-app \
      jni-bridge \
      docs \
      doc \
      code-analysis \
      test-harness \
      -type f \
      ! -path "*/build/*" \
      ! -path "*/tmp/*" \
      ! -path "*/.gradle/*" \
      ! -path "*/.kotlin/*" \
      ! -path "jni-bridge/external/glib/*" \
      ! -name "*.apk" \
      ! -name "*.aab" \
      ! -name "*.class" \
      ! -name "*.o" \
      ! -name "*.so" \
      ! -name "*.jar" \
      ! -name "*.pdf" \
      ! -name "*.png" \
      ! -name "*.jpg" \
      ! -name "*.jpeg" \
      ! -name "*.webp" \
      ! -name "*.ttf" \
      2>/dev/null \
      | sort \
      | while IFS= read -r f; do
          [ -f "$f" ] || continue
          bytes="$(wc -c < "$f" | tr -d " ")"
          sha="$(sha256sum "$f" | awk "{print \$1}")"
          printf "%-78s %12s %s\n" "$f" "$bytes" "$sha"
        done
  '
}

make_archive() {
  review_files > "$WORK_DIR/review_files.txt"

  tar -czf "$ARCHIVE" \
    -T "$WORK_DIR/review_files.txt" \
    "$PACKET" \
    2>/dev/null || true

  sha256sum "$ARCHIVE" > "$OUT_DIR/debug_packet_archive.sha256"
  sha256sum "$PACKET" > "$OUT_DIR/00_debug_packet.sha256"
}

acceptance_checks() {
  bytes="$(wc -c < "$PACKET" | tr -d ' ')"

  echo "packet: $PACKET"
  echo "packet bytes: $bytes"
  echo "archive: $ARCHIVE"

  if [ "$bytes" -gt "$MAX_PACKET_BYTES" ]; then
    echo "FAIL: packet exceeds MAX_PACKET_BYTES=$MAX_PACKET_BYTES" >&2
    exit 1
  fi

  if grep -q '^upstream-source/' "$PACKET"; then
    echo "FAIL: packet contains upstream-source paths" >&2
    exit 1
  fi

  if grep -q '^gnubg-app/tmp' "$PACKET"; then
    echo "FAIL: packet contains gnubg-app/tmp paths" >&2
    exit 1
  fi

  if grep -q 'review_files: command not found' "$PACKET"; then
    echo "FAIL: manifest attempted to call unavailable review_files function" >&2
    exit 1
  fi

  if grep -q 'BEGIN GENERATED SECTION' "$PACKET"; then
    echo "FAIL: packet looks like old full review context" >&2
    exit 1
  fi

  if grep -q 'BEGIN FILE: engine-core/' "$PACKET"; then
    echo "FAIL: packet dumps engine-core contents" >&2
    exit 1
  fi

  echo "acceptance checks passed"
}

write_packet
make_archive
acceptance_checks
