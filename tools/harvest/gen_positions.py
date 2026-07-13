#!/usr/bin/env python3
# gen_positions.py -- Phase C.1 of CORPUS_HARVEST_PLAN: parametric position
# construction. For each MEASURED entry of docs/CORPUS_ENTRIES_DRAFT.md,
# generate board pairs (played = the mistake, best = the good play) along the
# entry's parameter axes, verify EVERY pair through tools/pilot/inputs_harness
# (this repo's own eval.c), and admit only pairs where each signature term
# moves the measured direction by at least MIN_DELTA. Output: one JSON per
# entry under tools/harvest/positions/, provenance "parametric".
#
# Boards are 26-int gnubg `set board simple` vectors:
#   [opp_bar, p1..p24 (+ = player on roll), player_bar]
#
# License: GPL-3.0-or-later, like the tree.

import json
import subprocess
import sys
import datetime
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from signatures import SIGNATURES  # schema v2: single source of truth

REPO = Path(__file__).resolve().parents[2]
HARNESS = REPO / "tools/pilot/inputs_harness"
OUTDIR = REPO / "tools/harvest/positions"
MIN_DELTA = 0.02          # above the pilot's observed noise floor (~0.01)
CLASS_CONTACT = 10
CLASS_RACE = 8


def board(me=None, opp=None, me_bar=0, opp_bar=0):
    """26-vector from point->count dicts (1-based points, player's view)."""
    v = [0] * 26
    v[0] = -abs(opp_bar)
    v[25] = abs(me_bar)
    for p, c in (me or {}).items():
        v[p] += c
    for p, c in (opp or {}).items():
        v[p] -= c
    if sum(c for c in v[1:25] if c > 0) + v[25] != 15:
        return None
    if -sum(c for c in v[1:25] if c < 0) - v[0] != 15:
        return None
    return v


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
            r["pips"] = (int(t[2]), int(t[4]))          # (me, opp)
        elif t[0] == "positionclass":
            r["class"] = int(t[1])
        elif t[0].startswith("I_") and len(t) == 3:
            r["me"][t[0]] = float(t[1])
            r["opp"][t[0]] = float(t[2])
    return r


def term_value(r, t):
    if t["term"] == "PipCount.opp":
        return float(r["pips"][1])
    return r[t["side"]][t["term"]]


def verify(sig, played, best):
    """Schema v2: per-term min/max gates, value ranges, class constraints."""
    if "class_played" in sig and played["class"] != sig["class_played"]:
        return None
    if "class_best" in sig and best["class"] != sig["class_best"]:
        return None
    deltas = {}
    for t in sig["terms"]:
        vp, vb = term_value(played, t), term_value(best, t)
        d = vb - vp
        deltas[f"{t['side']}.{t['term']}" if t["side"] else t["term"]] = round(d, 6)
        if t["direction"] == "any":
            pass                          # context term: range/max gates only
        elif t["direction"] == "up" and d < t["min_abs"]:
            return None
        if t["direction"] == "down" and d > -t["min_abs"]:
            return None
        if "max_abs" in t and abs(d) > t["max_abs"]:
            return None
        if "played_in" in t and not (t["played_in"][0] <= vp <= t["played_in"][1]):
            return None
        if "best_in" in t and not (t["best_in"][0] <= vb <= t["best_in"][1]):
            return None
    return deltas


# ---------------------------------------------------------------- templates

def prime_pairs(dissolve):
    for start in (3, 4, 5):                    # prime points start..start+4
        for depth in (1, 2, 3):
            if depth >= start:
                continue
            prime = {p: 2 for p in range(start, start + 5)}
            me = {**prime, 13: 3, 24: 2}
            opp = {depth: 2, 12: 5, 19: 3, 20: 3, 21: 2}
            best = board(me=me, opp=opp)
            if dissolve:
                kept = {start + 1: 2, start + 2: 2}
                played = board(me={**kept, 13: 9, 24: 2}, opp=opp)
                yield f"s{start}d{depth}", played, best, \
                    {"span": [start, start + 4], "trap": depth, "mode": "dissolved"}
            else:
                for brk, lbl in ((start + 4, "front"), (start + 2, "mid")):
                    m2 = dict(me)
                    del m2[brk]
                    m2[11] = m2.get(11, 0) + 2
                    played = board(me=m2, opp=opp)
                    yield f"s{start}d{depth}{lbl}", played, best, \
                        {"span": [start, start + 4], "trap": depth, "break": lbl}


def close_entry_pairs():
    for np_ in (2, 3, 4):
        for nbar in (1, 2):
            # NB pilot pair 1's board had only 13 player checkers -- fine
            # for signal discovery, invalid for a corpus position. Two
            # neutral checkers on the 11 complete it.
            me = {6: 2, 5: 2, 8: 4, 13: 5, 11: 2}
            opp = {19: 3, 20: 3, 21: 2, 24: 7 - nbar}
            played = board(me=me, opp=opp, opp_bar=nbar)
            m2 = {**me, 8: 2, np_: 2}
            best = board(me=m2, opp=opp, opp_bar=nbar)
            yield f"p{np_}b{nbar}", played, best, {"new_point": np_, "on_bar": nbar}


def blot_shot_pairs():
    for bp in (9, 10, 11):
        for d in (3, 4, 5, 6):
            sh = bp - d
            if sh < 1 or sh in (5, 6, 8):
                continue
            me = {5: 2, 6: 2, 8: 4, 13: 4, 24: 2, bp: 1}
            opp = {sh: 2, 18: 5, 19: 3, 20: 3, 21: 2}
            played = board(me=me, opp=opp)
            m2 = dict(me)
            del m2[bp]
            m2[8] += 1
            best = board(me=m2, opp=opp)
            yield f"b{bp}d{d}", played, best, {"blot": bp, "shot": d}


def blot_double_pairs():
    for sh in (3, 4):
        for b1, b2 in ((9, 10), (9, 11), (10, 11)):
            me = {5: 2, 6: 2, 8: 3, 13: 4, 24: 2, b1: 1, b2: 1}
            opp = {sh: 2, 18: 5, 19: 3, 20: 3, 21: 2}
            played = board(me=me, opp=opp)
            m2 = dict(me)
            del m2[b1], m2[b2]
            m2[8] += 2
            best = board(me=m2, opp=opp)
            yield f"s{sh}b{b1}-{b2}", played, best, {"shooter": sh, "blots": [b1, b2]}


def anchor_surrender_pairs():
    homes = {20: {12: 5, 17: 3, 19: 3, 21: 2, 23: 2},
             21: {12: 5, 17: 3, 19: 3, 20: 2, 23: 2},
             22: {12: 5, 17: 3, 19: 3, 20: 2, 21: 2}}
    for ap, opp in homes.items():
        me = {5: 2, 6: 2, 8: 4, 13: 5, ap: 2}
        best = board(me=me, opp=opp)
        m2 = dict(me)
        del m2[ap]
        s2 = ap + 2 if ap + 2 <= 24 and (ap + 2) not in opp else 24
        m2[ap] = 1
        m2[s2] = m2.get(s2, 0) + 1
        played = board(me=m2, opp=opp)
        yield f"a{ap}", played, best, {"anchor": ap, "split_to": [ap, s2]}


def anchor_golden_pairs():
    for ap in (19, 20, 21):
        for filler in ({13: 5}, {13: 3, 11: 2}):
            base = {5: 2, 6: 2, 8: 4}
            opp = {12: 5, 17: 3, 22: 3, 23: 2, 18: 2}
            played = board(me={**base, **filler, 24: 2}, opp=opp)
            best = board(me={**base, **filler, ap: 2}, opp=opp)
            yield f"a{ap}f{len(filler)}", played, best, {"anchor": ap}


def race_break_pairs():
    for r in (22, 24):
        for t in (10, 11):
            me = {4: 4, 5: 4, 6: 5}
            opp = {12: 5, 17: 2, 19: 3, 20: 3, 21: 2}
            played = board(me={**me, r: 2}, opp=opp)
            best = board(me={**me, t: 2}, opp=opp)
            yield f"r{r}t{t}", played, best, {"runner": r, "target": t}


def race_escape_pairs():
    for r in (23, 24):
        for e in (14, 16, 18):
            me = {4: 4, 5: 4, 6: 5}
            opp = {12: 5, 17: 2, 19: 3, 20: 3, 21: 2}
            played = board(me={**me, r: 2}, opp=opp)
            best = board(me={**me, e: 2}, opp=opp)
            yield f"r{r}e{e}", played, best, {"runner": r, "escaped_to": e}


def hit_pairs():
    for hp, src in ((9, 13), (10, 13), (11, 13), (18, 24), (20, 24)):
        me = {5: 2, 6: 2, 8: 4, 13: 5, 24: 2}
        opp_pts = {18: 5, 19: 3, 20: 3, 21: 2}
        if hp in opp_pts:
            opp_pts = {17: 5, 19: 3, 21: 3, 22: 2}
        opp = {**opp_pts, hp: 1}
        if sum(opp.values()) != 15:
            opp[max(opp_pts)] += 15 - sum(opp.values())
        played = board(me=me, opp=opp)
        m2 = dict(me)
        m2[src] -= 1
        m2[hp] = m2.get(hp, 0) + 1
        o2 = dict(opp)
        del o2[hp]
        best = board(me=m2, opp=o2, opp_bar=1)
        yield f"h{hp}s{src}", played, best, {"hit_point": hp, "from": src}


def backgame_timing_pairs():
    # A live backgame: I hold two rear anchors; the opponent is ahead,
    # bearing in AROUND my anchors (their home points fill the gaps between
    # them). played = a builder burned deep (timing spent); best = the same
    # builder held high (timing preserved). Anchors and opponent constant.
    for a1, a2 in ((20, 22), (20, 24), (21, 23)):
        # opponent home points: three of my 19..24 that are NOT my anchors
        homes = [p for p in (19, 20, 21, 22, 23, 24) if p not in (a1, a2)][:3]
        opp = {homes[0]: 3, homes[1]: 3, homes[2]: 2, 10: 3, 7: 4}
        base = {a1: 2, a2: 2, 13: 4, 8: 3, 6: 2}
        for low, high in ((5, 12), (4, 12), (3, 11), (5, 11)):
            played = board(me={**base, low: 2}, opp=opp)
            best = board(me={**base, high: 2}, opp=opp)
            yield f"a{a1}-{a2}l{low}h{high}", played, best, \
                {"anchors": [a1, a2], "burned_to": low, "held_at": high}


GENERATORS = {
    "backgame.timing": backgame_timing_pairs,
    "prime.break.5": lambda: prime_pairs(False),
    "prime.contain.lost": lambda: prime_pairs(True),
    "board.close.entry": close_entry_pairs,
    "blot.shot.given": blot_shot_pairs,
    "blot.double.given": blot_double_pairs,
    "anchor.surrender.back": anchor_surrender_pairs,
    "anchor.advance.golden": anchor_golden_pairs,
    "race.break.ahead": race_break_pairs,
    "race.escape.window": race_escape_pairs,
    "hit.declined": hit_pairs,
}


def main():
    if not HARNESS.exists():
        sys.exit("build the harness first: tools/pilot/build.sh")
    OUTDIR.mkdir(parents=True, exist_ok=True)
    today = datetime.date.today().isoformat()
    grand = 0
    for entry, gen in GENERATORS.items():
        sig = SIGNATURES[entry]
        kept, tried = [], 0
        for pid, played_v, best_v, params in gen():
            tried += 1
            if played_v is None or best_v is None:
                continue                              # checker-count violation
            pl, be = run_harness(played_v), run_harness(best_v)
            if pl is None or be is None:
                continue                              # harness rejected/aborted
            deltas = verify(sig, pl, be)
            if deltas is None:
                continue
            kept.append({"id": pid, "params": params,
                         "played": played_v, "best": best_v,
                         "deltas": deltas,
                         "pips_played": pl["pips"], "class_played": pl["class"],
                         "class_best": be["class"]})
        doc = {"entry": entry, "provenance": "parametric", "generated": today,
               "verifier": "tools/pilot/inputs_harness",
               "signature": sig,
               "pairs": kept}
        (OUTDIR / f"{entry}.json").write_text(json.dumps(doc, indent=1) + "\n")
        grand += len(kept)
        print(f"{entry:26s} {len(kept):3d}/{tried} pairs verified")
    print(f"total verified pairs: {grand}")


if __name__ == "__main__":
    main()
