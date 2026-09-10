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

# The shipped worker count, read from its single source in stubs.c.
WORKERS=$(sed -n 's/^#define GNUBG_ROLLOUT_WORKERS[[:space:]]*\([0-9][0-9]*\).*/\1/p' jni-bridge/src/stubs.c)
[ -n "$WORKERS" ] || { echo "cannot read GNUBG_ROLLOUT_WORKERS from stubs.c"; exit 2; }
CORES=$(nproc 2>/dev/null || echo 1)
echo "host cores: $CORES; shipped rollout workers: $WORKERS"
if [ "$CORES" -lt "$WORKERS" ]; then
    echo "REFUSED: this host has $CORES core(s); the determinism gate at $WORKERS workers"
    echo "cannot be run here, only imitated. Run it on a host with >= $WORKERS cores."
    exit 3
fi
export GNUBG_ROLLOUT_THREADS=$WORKERS

echo "== T1: candidates rollout, same seed twice -> byte-identical (at $WORKERS workers) =="
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

echo "== M1 (informational, never gates): the pool at every core ($CORES), same seed twice =="
# Documents the race the serial pool avoids (stubs.c gnubg_init_rollout):
# the NoLocking evaluation family on one shared cEval. Expected to DIFFER
# on a multi-core host until the WithLocking family is compiled in.
GNUBG_ROLLOUT_THREADS=$CORES ./tools/rollout_harness/harness "$ID" 12345 2 36 2>/dev/null > tmp/ro_m1a.txt
GNUBG_ROLLOUT_THREADS=$CORES ./tools/rollout_harness/harness "$ID" 12345 2 36 2>/dev/null > tmp/ro_m1b.txt
if diff -q tmp/ro_m1a.txt tmp/ro_m1b.txt >/dev/null; then
    echo "identical at $CORES workers on this host (probability, not proof -- see stubs.c)"
else
    echo "DIFFER at $CORES workers on this host -- the documented race; the shipped pool runs $WORKERS"
fi

echo "ALL TESTS GREEN"
