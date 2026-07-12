#!/usr/bin/env python3
# matcher_proto.py -- offline prototype of the Phase G InsightMatcher
# (CORPUS_HARVEST_PLAN §4.G), run against the Phase C.1 verified pairs.
#
# The matcher's contract: it does NO position interpretation. It receives a
# delta vector (best - played, per named input, both sides, plus pip counts)
# and scores each corpus entry's signature against it: every term must pass
# its direction and min_abs_delta gate; the score is the weight-summed
# magnitude of the passing terms. Up to MAX_FIRE entries clearing the gate
# fire, or none.
#
# This prototype answers the question that gates the corpus scale-up: do the
# measured signatures DISCRIMINATE? For every pair of every entry it computes
# the full delta vector via the pilot harness and scores ALL entries,
# producing a confusion matrix. Sibling co-firing (prime.break with
# prime.contain) is by design; a FOREIGN entry outscoring the pair's own
# entry is a failure.
#
# License: GPL-3.0-or-later, like the tree.

import json
import subprocess
import sys
from collections import defaultdict
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from signatures import SIGNATURES

REPO = Path(__file__).resolve().parents[2]
HARNESS = REPO / "tools/pilot/inputs_harness"
POSDIR = REPO / "tools/harvest/positions"
MIN_ABS = 0.02
MAX_FIRE = 2


def run_harness(vec):
    out = subprocess.run([str(HARNESS), "x"] + [str(n) for n in vec],
                         capture_output=True, text=True, timeout=30)
    if out.returncode != 0:
        return None
    r = {"me": {}, "opp": {}}
    for line in out.stdout.splitlines():
        t = line.split()
        if not t:
            continue
        if t[0] == "PipCount":
            r["pips"] = (int(t[2]), int(t[4]))
        elif t[0] == "positionclass":
            r["class"] = int(t[1])
        elif t[0].startswith("I_") and len(t) == 3:
            r["me"][t[0]] = float(t[1])
            r["opp"][t[0]] = float(t[2])
    return r


def delta_vector(played, best):
    pl, be = run_harness(played), run_harness(best)
    if pl is None or be is None:
        return None
    d = {}
    for side in ("me", "opp"):
        for k in pl[side]:
            d[f"{side}.{k}"] = be[side][k] - pl[side][k]
    d["PipCount.opp"] = be["pips"][1] - pl["pips"][1]
    d["PipCount.me"] = be["pips"][0] - pl["pips"][0]
    return d


def score(sig, dv, played, best):
    """Schema v2: all gates must pass; capped, min-normalised contributions."""
    if "class_played" in sig and played["class"] != sig["class_played"]:
        return 0.0
    if "class_best" in sig and best["class"] != sig["class_best"]:
        return 0.0
    total = 0.0
    for t in sig["terms"]:
        if t["term"] == "PipCount.opp":
            vp, vb = float(played["pips"][1]), float(best["pips"][1])
        else:
            vp, vb = played[t["side"]][t["term"]], best[t["side"]][t["term"]]
        d = vb - vp
        if t["direction"] == "any":
            pass                          # context term: range/max gates only
        elif t["direction"] == "up" and d < t["min_abs"]:
            return 0.0
        if t["direction"] == "down" and d > -t["min_abs"]:
            return 0.0
        if "max_abs" in t and abs(d) > t["max_abs"]:
            return 0.0
        if "played_in" in t and not (t["played_in"][0] <= vp <= t["played_in"][1]):
            return 0.0
        if "best_in" in t and not (t["best_in"][0] <= vb <= t["best_in"][1]):
            return 0.0
        if t["weight"] > 0:
            mag = (min(abs(d) / (3.0 * t["min_abs"]), 1.0)
                   if "min_abs" in t else 1.0)   # range-only context: flat
            total += t["weight"] * mag
    return total


def main():
    files = sorted(POSDIR.glob("*.json"))
    if not files:
        sys.exit("no position files; run gen_positions.py first")
    corpora = {f.stem: json.loads(f.read_text()) for f in files}
    sigs = SIGNATURES

    top1_ok, top1_bad, fired_none = 0, 0, 0
    confusion = defaultdict(lambda: defaultdict(int))
    failures = []

    for own, c in corpora.items():
        for pair in c["pairs"]:
            pl = run_harness(pair["played"])
            be = run_harness(pair["best"])
            if pl is None or be is None:
                continue
            scores = sorted(((score(s, None, pl, be), len(s["terms"]), e)
                             for e, s in sigs.items()), reverse=True)
            scores = [(sc, e) for sc, _n, e in scores]
            firing = [(sc, e) for sc, e in scores if sc > 0.0][:MAX_FIRE]
            for _, e in firing:
                confusion[own][e] += 1
            if not firing:
                fired_none += 1
                failures.append((own, pair["id"], "NOTHING FIRED"))
            elif firing[0][1] == own:
                top1_ok += 1
            else:
                top1_bad += 1
                failures.append((own, pair["id"],
                                 f"top1={firing[0][1]} ({firing[0][0]:.3f})"))

    total = top1_ok + top1_bad + fired_none
    print(f"pairs scored: {total}   top-1 correct: {top1_ok}   "
          f"top-1 foreign: {top1_bad}   nothing fired: {fired_none}")
    print("\nconfusion (row = pair's own entry; cols = entries that fired):")
    for own in sorted(confusion):
        row = ", ".join(f"{e}:{n}" for e, n in
                        sorted(confusion[own].items(), key=lambda x: -x[1]))
        print(f"  {own:26s} -> {row}")
    if failures:
        print("\nfailures:")
        for own, pid, why in failures:
            print(f"  {own}/{pid}: {why}")


if __name__ == "__main__":
    main()
