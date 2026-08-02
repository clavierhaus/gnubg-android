#!/bin/sh
# Automated rollout-core tests -- run from the repo root:
#   tools/rollout_harness/run_tests.sh
# Builds the host harness (the port's exact engine subset) and asserts
# correct handling. Exit 0 = all green. See docs/MULTICORE_ANALYSIS.md 2.10.
set -e
cd "$(dirname "$0")/../.."
mkdir -p tmp

echo "== build =="
make -C tools/rollout_harness >/dev/null

# A position minted by the reference desktop gnubg (1-pt match, 66 rolled).
ID="4HPwBSDgc/ABMA:cAk7AAAAAAAE"

echo "== T1: candidates rollout, same seed twice -> byte-identical =="
./tools/rollout_harness/harness "$ID" 12345 2 36 2>/dev/null > tmp/ro_a.txt
./tools/rollout_harness/harness "$ID" 12345 2 36 2>/dev/null > tmp/ro_b.txt
diff tmp/ro_a.txt tmp/ro_b.txt
echo "PASS"

echo "== T2: different seed -> different numbers =="
./tools/rollout_harness/harness "$ID" 777 2 36 2>/dev/null > tmp/ro_c.txt
if diff -q tmp/ro_a.txt tmp/ro_c.txt >/dev/null; then
    echo "FAIL: seed had no effect"; exit 1
fi
echo "PASS"

echo "== T3: position rollout (the desktop-comparable core), deterministic =="
./tools/rollout_harness/harness "$ID" 12345 -1 36 2>/dev/null > tmp/ro_p1.txt
./tools/rollout_harness/harness "$ID" 12345 -1 36 2>/dev/null > tmp/ro_p2.txt
diff tmp/ro_p1.txt tmp/ro_p2.txt
grep -q "DONE 36" tmp/ro_p1.txt || { echo "FAIL: trials incomplete"; exit 1; }
echo "PASS"

echo "== T4: all trials complete and labeled =="
grep -q "DONE 36" tmp/ro_a.txt || { echo "FAIL"; exit 1; }
echo "PASS"

echo "ALL TESTS GREEN"
