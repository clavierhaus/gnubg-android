#!/usr/bin/env bash
set -euo pipefail

stamp="$(date +%Y%m%d_%H%M%S)"
status_dir="tmp/current_review_status_${stamp}"
mkdir -p "$status_dir"

status_md="$status_dir/CURRENT_STATUS_FOR_REVIEW.md"
run_log="$status_dir/debug_packet_run.log"

section() {
  local title="$1"
  {
    echo
    echo "## $title"
    echo
  } >> "$status_md"
}

code_block() {
  {
    echo '```text'
    cat
    echo '```'
    echo
  } >> "$status_md"
}

repo_file() {
  local path="$1"
  [ -f "$path" ]
}

count_jni_refs() {
  grep -RIl \
    'JNIEnv\|jstring\|jintArray\|JNIEXPORT' \
    jni-bridge/src jni-bridge/include 2>/dev/null \
    | while IFS= read -r f; do
        count="$(
          grep -nE \
            'JNIEnv|jstring|jintArray|JNIEXPORT' \
            "$f" 2>/dev/null \
            | wc -l \
            | tr -d ' '
        )"
        printf '%5s %s\n' "$count" "$f"
      done \
    | sort -nr
}

native_boundary_scan() {
  local file="jni-bridge/src/native-lib.c"

  if [ ! -f "$file" ]; then
    echo "missing: $file"
    return 0
  fi

  python3 - "$file" <<'PY'
from pathlib import Path
import re
import sys

path = Path(sys.argv[1])
text = path.read_text(errors="replace").splitlines()

symbols = re.compile(
    r"\b("
    r"ms|plGame|plLastMove|lMatch|"
    r"ApplySubMove|NextTurn|FormatMove|"
    r"CommandLoad[A-Za-z]*|CommandSave[A-Za-z]*|"
    r"ClearMatch|ListCreate"
    r")\b"
)

allowed = re.compile(
    r"\b("
    r"gnubg_mobile_|"
    r"JNIEXPORT|JNIEnv|jobject|jstring|jintArray|jint|"
    r"pack_board|unpack_board|NewIntArray|SetIntArrayRegion|"
    r"GetIntArrayRegion|GetStringUTFChars|ReleaseStringUTFChars"
    r")"
)

in_block_comment = False
findings = []

for lineno, line in enumerate(text, 1):
    raw = line
    s = line

    # Strip block comments conservatively enough for audit output.
    cleaned = ""
    i = 0
    while i < len(s):
        if in_block_comment:
            end = s.find("*/", i)
            if end == -1:
                i = len(s)
            else:
                in_block_comment = False
                i = end + 2
        else:
            start = s.find("/*", i)
            slash = s.find("//", i)

            if slash != -1 and (start == -1 or slash < start):
                cleaned += s[i:slash]
                i = len(s)
            elif start != -1:
                cleaned += s[i:start]
                in_block_comment = True
                i = start + 2
            else:
                cleaned += s[i:]
                i = len(s)

    s = cleaned.strip()
    if not s:
        continue

    # Ignore extern declarations; they are still worth cleaning later,
    # but they are not executable boundary violations.
    if s.startswith("extern "):
        continue

    # Ignore function prototypes.
    if s.endswith(";") and "(" in s and ")" in s and "{" not in s:
        if re.match(r"^(static\s+)?[A-Za-z_][A-Za-z0-9_ *]+\(", s):
            continue

    if symbols.search(s):
        findings.append((lineno, raw.rstrip()))

if not findings:
    print("native-lib.c executable direct-engine scan: clean")
else:
    print("native-lib.c executable direct-engine scan:")
    for lineno, line in findings:
        print(f"{path}:{lineno}:{line}")
PY
}

facade_exports() {
  grep -nE \
    '^int gnubg_mobile_|^const char \*gnubg_mobile_' \
    jni-bridge/include/gnubg_mobile.h 2>/dev/null \
    || true
}

packet_checks() {
  local packet="$1"

  if [ ! -f "$packet" ]; then
    echo "no packet generated"
    return 0
  fi

  bytes="$(wc -c < "$packet" | tr -d ' ')"
  lines="$(wc -l < "$packet" | tr -d ' ')"

  echo "packet: $packet"
  echo "bytes:  $bytes"
  echo "lines:  $lines"
  echo
  echo "leak checks:"
  printf '  %-42s %s\n' \
    "upstream-source content paths" \
    "$(grep -c '^upstream-source/' "$packet" || true)"
  printf '  %-42s %s\n' \
    "engine-core FILE dumps" \
    "$(grep -c 'BEGIN FILE: engine-core/' "$packet" || true)"
  printf '  %-42s %s\n' \
    "gnubg-app/tmp paths" \
    "$(grep -c '^gnubg-app/tmp' "$packet" || true)"
  printf '  %-42s %s\n' \
    "shell command-not-found errors" \
    "$(grep -c 'command not found' "$packet" || true)"
  printf '  %-42s %s\n' \
    "review_files manifest bug" \
    "$(grep -c 'review_files: command not found' "$packet" || true)"
}

{
  echo "# Current GNUbg Android review status"
  echo
  echo "Generated: $(date -Iseconds)"
  echo "Repository: $(pwd)"
} > "$status_md"

section "Git"
{
  echo '```text'
  git --no-pager branch --show-current 2>/dev/null || true
  git rev-parse HEAD 2>/dev/null || true
  git --no-pager log -1 --oneline 2>/dev/null || true
  echo
  git status --short
  echo '```'
} >> "$status_md"

section "Recent commits"
git --no-pager log --oneline -20 2>/dev/null | code_block

section "Assessment scripts"
{
  echo '```text'
  ls -l \
    code-analysis/collect_debug_packet.sh \
    code-analysis/collect_current_review_status.sh \
    code-analysis/collect_codebase_audit.sh \
    code-analysis/README.md 2>/dev/null || true
  echo

  if git ls-files --error-unmatch \
       code-analysis/collect_debug_packet.sh >/dev/null 2>&1
  then
    echo "collect_debug_packet.sh is tracked"
  else
    echo "collect_debug_packet.sh is NOT tracked"
  fi

  if bash -n code-analysis/collect_debug_packet.sh; then
    echo "collect_debug_packet.sh syntax: OK"
  else
    echo "collect_debug_packet.sh syntax: FAIL"
  fi

  if bash -n code-analysis/collect_current_review_status.sh; then
    echo "collect_current_review_status.sh syntax: OK"
  else
    echo "collect_current_review_status.sh syntax: FAIL"
  fi
  echo '```'
} >> "$status_md"

echo "== run debug packet collector =="
if bash code-analysis/collect_debug_packet.sh >"$run_log" 2>&1; then
  packet_exit=0
else
  packet_exit=$?
fi

latest_packet_dir="$(
  ls -dt tmp/debug_packet_* 2>/dev/null | head -n 1 || true
)"

section "Debug packet run"
{
  echo '```text'
  echo "exit: $packet_exit"
  echo
  cat "$run_log"
  echo '```'
} >> "$status_md"

section "Latest debug packet"
{
  echo '```text'
  if [ -n "$latest_packet_dir" ]; then
    echo "$latest_packet_dir"
    find "$latest_packet_dir" -maxdepth 1 -type f \
      -printf '%10s bytes  %p\n' \
      | sort -nr
  else
    echo "no tmp/debug_packet_* directory found"
  fi
  echo '```'
} >> "$status_md"

packet=""
if [ -n "$latest_packet_dir" ] &&
   [ -f "$latest_packet_dir/00_debug_packet.md" ]; then
  packet="$latest_packet_dir/00_debug_packet.md"
fi

section "Packet checks"
packet_checks "$packet" | code_block

section "Current source-boundary counts"
{
  echo "JNI references by C file:"
  count_jni_refs
  echo
  echo "native-lib.c direct engine/global executable scan:"
  native_boundary_scan
  echo
  echo "mobile facade exported functions:"
  facade_exports
} | code_block

section "App/JNI source-of-truth scan"
grep -RInE \
  --exclude-dir=.gradle \
  --exclude-dir=.kotlin \
  --exclude-dir=build \
  --exclude-dir=tmp \
  --exclude-dir=glib \
  'legal|illegal|valid|invalid|dice|die|move|turn|cube|double|take|drop|resign|bear|bar|pip|score|match|game|board|position|orientation|swap|findMove|getLegalMoves|remainingDice|apply|confirm|command' \
  gnubg-app jni-bridge 2>/dev/null \
  | sed -n '1,260p' \
  | code_block

echo "current review status written:"
echo "$status_md"

if [ -n "$packet" ]; then
  echo "latest debug packet:"
  echo "$packet"
fi
