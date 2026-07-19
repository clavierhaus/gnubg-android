#!/usr/bin/env python3
# narrator_proto.py -- Phase A of docs/DELTA_NARRATOR_PLAYBOOK.md.
# The exact logic the Kotlin DeltaNarrator will mirror: deterministic rule
# application over gnubg deltas; race/board/threat triage; <=1 sentence per
# category, <=2 total; strongest normalised delta wins a category, table
# order breaks ties. License: GPL-3.0-or-later, like the tree.
import json, sys, pathlib
HERE = pathlib.Path(__file__).resolve().parent
RULES = json.load(open(HERE / "rules_draft.json"))["rules"]

def narrate(deltas, played_values=None):
    """deltas: {'side.term' or 'PipCount.opp': best-played float}.
    played_values: optional raw played-side values for played_in gates."""
    fired = []
    for r in RULES:
        key = r["term"] if r["term"].startswith("PipCount") else f"{r['side']}.{r['term']}"
        d = deltas.get(key)
        if d is None:
            continue
        if r["direction"] == "up" and d < r["notable"]:
            continue
        if r["direction"] == "down" and d > -r["notable"]:
            continue
        if "played_in" in r and played_values is not None:
            vp = played_values.get(key)
            if vp is None or not (r["played_in"][0] <= vp <= r["played_in"][1]):
                continue
        fired.append((abs(d) / r["notable"], r))
    best_per_cat = {}
    for score, r in fired:
        cur = best_per_cat.get(r["category"])
        if cur is None or score > cur[0]:
            best_per_cat[r["category"]] = (score, r)
    ranked = sorted(best_per_cat.values(), key=lambda t: -t[0])[:2]
    return [(r["id"], r["category"], r["sentence"]) for _, r in ranked]

if __name__ == "__main__":
    # evidence run over every verified corpus pair
    posdir = HERE.parent / "harvest" / "positions"
    fire_count, examples = {}, {}
    total = narrated = 0
    for f in sorted(posdir.glob("*.json")):
        doc = json.load(open(f))
        for pr in doc["pairs"]:
            total += 1
            deltas = pr.get("deltas_all") or pr["deltas"]
            out = narrate(deltas)
            if out:
                narrated += 1
            for rid, cat, _ in out:
                fire_count[rid] = fire_count.get(rid, 0) + 1
                examples.setdefault(rid, []).append(f"{doc['entry']}/{pr['id']}")
    print(f"pairs: {total}   narrator speaks on: {narrated}   silent: {total-narrated}")
    print(f"{'rule':28} {'fires':>5}   evidence (sample)")
    for r in RULES:
        rid = r["id"]
        n = fire_count.get(rid, 0)
        ex = ", ".join(examples.get(rid, [])[:3])
        print(f"{rid:28} {n:>5}   {ex}")
