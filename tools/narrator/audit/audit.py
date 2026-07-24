#!/usr/bin/env python3
"""
audit.py -- falsify the coach's authored sentences against gnubg's own boards.

Reads claim_audit's dump (one row per (variant, played) candidate pair, with
both boards' gnubg inputs and the board facts the sentences talk about),
replays the corpus signature logic exactly as InsightMatcher.match does, and
for every firing checks the sentence's LITERAL claim.

A violation is a pair where the signature fired but the words were false.
Zero violations over tens of thousands of pairs is not proof of truth, but it
is the strongest evidence available short of one: the claim survived every
attempt this tree could make to break it.

Restatement entries -- whose sentence says in words exactly what the signature
term measures (fewer shots, contained, timing) -- have no independent claim to
test and are reported as such rather than silently counted as passes.

GPLv3+ like the tree.
"""
import json, sys
from collections import defaultdict

INPUT_ORDER = ["I_OFF1","I_OFF2","I_OFF3","I_BREAK_CONTACT","I_BACK_CHEQUER",
    "I_BACK_ANCHOR","I_FORWARD_ANCHOR","I_PIPLOSS","I_P1","I_P2","I_BACKESCAPES",
    "I_ACONTAIN","I_ACONTAIN2","I_CONTAIN","I_CONTAIN2","I_MOBILITY","I_MOMENT2",
    "I_ENTER","I_ENTER2","I_TIMING","I_BACKBONE","I_BACKG","I_BACKG1","I_FREEPIP",
    "I_BACKRESCAPES"]
IDX = {n: i for i, n in enumerate(INPUT_ORDER)}
N = len(INPUT_ORDER)

# emit_board() layout: 50 inputs, pip0, pip1, cls, then 8 facts
BLOCK = 50 + 3 + 9
V0, P0 = 5, 5 + BLOCK
FACT = ["mehome","opphome","mebar","oppbar","meblots","meanch","rearanch","rearchk","fwdanch"]

def block(row, base):
    inp = [float(x) for x in row[base:base+50]]
    pip = (int(row[base+50]), int(row[base+51]))
    cls = int(row[base+52])
    f = {k: int(row[base+53+i]) for i, k in enumerate(FACT)}
    return inp, pip, cls, f

def value(term, side, inp, pip):
    if term == "PipCount.opp":
        return float(pip[0])
    i = IDX.get(term)
    if i is None:
        return 0.0
    return inp[N + i] if side == "me" else inp[i]

def fires(entry, vin, vpip, vcls, pin, ppip, pcls):
    """InsightMatcher.match, term-for-term. vp = played, vb = variant."""
    sig = entry["signature"]
    if "class_played" in sig and pcls != sig["class_played"]: return False
    if "class_best" in sig and vcls != sig["class_best"]: return False
    total = 0.0
    for t in sig["terms"]:
        vp = value(t["term"], t.get("side",""), pin, ppip)
        vb = value(t["term"], t.get("side",""), vin, vpip)
        d = vb - vp
        mn = float(t.get("min_abs", 0.0))
        dr = t.get("direction", "any")
        if dr == "up" and d < mn: return False
        if dr == "down" and d > -mn: return False
        if "max_abs" in t and abs(d) > float(t["max_abs"]): return False
        if "played_in" in t:
            lo, hi = t["played_in"]
            if vp < lo or vp > hi: return False
        if "best_in" in t:
            lo, hi = t["best_in"]
            if vb < lo or vb > hi: return False
        w = float(t.get("weight", 0.0))
        if w > 0:
            total += w * (min(abs(d) / (3.0 * mn), 1.0) if mn > 0 else 1.0)
    return total > 0.0

# ---- the literal claims, as predicates over the two boards ---------------
# V = the variant the sentence is about, P = the played move.
# None means: the sentence restates its own signature term, so there is no
# independent board fact to check (reported separately, never counted as pass).
CLAIMS = {
 "enter.fight.point":
   ("has a made point in the opponent's home board (the advanced anchor)",
    lambda V, P: V["meanch"] >= 1),
 "hit.loose.homeboard":
   ("has at least as many made home points as the played move",
    lambda V, P: V["mehome"] >= P["mehome"]),
 "contact.break.early":
   ("keeps a rearmost made point at least as far back as the played move's",
    lambda V, P: V["rearanch"] >= 0 and V["rearanch"] >= P["rearanch"]),
 "anchor.split.straggler":
   ("keeps its rearmost made point rather than breaking it",
    lambda V, P: V["rearanch"] >= 0 and V["rearanch"] >= P["rearanch"]),
 "prime.contain.lost": None,
 "anchor.advance.mid":
   ("its most advanced anchor in the opponent's home board is further advanced",
    lambda V, P: V["fwdanch"] >= 0 and P["fwdanch"] >= 0
                 and V["fwdanch"] < P["fwdanch"]),
 "board.close.entry":
   ("has made one more home point than the played move",
    lambda V, P: V["mehome"] > P["mehome"]),
 "blot.shot.given": None,
 "blot.double.given": None,
 "hit.declined":
   ("puts an opponent checker on the bar that the played move does not",
    lambda V, P: V["oppbar"] > P["oppbar"]),
 "race.escape.window":
   ("has its rearmost checker further advanced than the played move's",
    lambda V, P: V["rearchk"] >= 0 and P["rearchk"] >= 0
                 and V["rearchk"] < P["rearchk"]),
 "backgame.timing": None,
 "timing.hold.crunch": None,
 "blitz.point.missed":
   ("makes a home point the played move does not, with an opponent checker on the bar",
    lambda V, P: V["mehome"] > P["mehome"] and V["oppbar"] >= 1),
}

def main():
    corpus, dump = sys.argv[1], sys.argv[2]
    data = json.load(open(corpus))
    entries = data if isinstance(data, list) else data.get("entries", list(data.values()))
    fired = defaultdict(int)
    viol = defaultdict(int)
    examples = {}
    rows = 0
    with open(dump) as fh:
        fh.readline()
        for line in fh:
            row = line.rstrip("\n").split("\t")
            if len(row) < P0 + BLOCK: continue
            rows += 1
            vin, vpip, vcls, vf = block(row, V0)
            pin, ppip, pcls, pf = block(row, P0)
            for e in entries:
                if not fires(e, vin, vpip, vcls, pin, ppip, pcls): continue
                fired[e["id"]] += 1
                c = CLAIMS.get(e["id"])
                if c is None: continue
                if not c[1](vf, pf):
                    viol[e["id"]] += 1
                    examples.setdefault(e["id"], (row[0], vf, pf))

    print(f"pairs audited: {rows}\n")
    print(f"{'entry':26} {'fired':>7} {'violations':>11}   claim")
    print("-" * 100)
    for e in entries:
        i = e["id"]; c = CLAIMS.get(i)
        claim = c[0] if c else "(restatement of its own signature term -- nothing independent to test)"
        print(f"{i:26} {fired[i]:>7} {viol[i]:>11}   {claim}")
    total_v = sum(viol.values())
    print("-" * 100)
    print(f"TOTAL VIOLATIONS: {total_v}")
    for i, (pos, vf, pf) in examples.items():
        print(f"  counterexample {i}: pos={pos} V={vf} P={pf}")
    return 1 if total_v else 0

if __name__ == "__main__":
    sys.exit(main())
