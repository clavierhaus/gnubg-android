#!/usr/bin/env python3
# signatures.py -- the single source of truth for entry signatures, consumed
# by gen_positions.py (verification gates) and matcher_proto.py (scoring).
# Schema v2, driven by the matcher prototype's confusion-matrix failures:
#
#   term:        input name, or "PipCount.opp"
#   side:        "me" / "opp" ("" for PipCount terms)
#   direction:   sign of (best - played)
#   min_abs:     delta gate (per term -- sibling entries separate here)
#   max_abs:     optional upper delta gate ("partial, not total")
#   weight:      contribution weight
#   played_in / best_in: optional [lo, hi] VALUE-range gates on the raw
#                input -- required for zone-encoded inputs (I_FORWARD_ANCHOR:
#                (0,1] = a real opp-home anchor, >1 = outfield fallback,
#                2.0 = none; see docs/INPUT_DICTIONARY.md) where raw deltas
#                across zones are meaningless.
#   class_best / class_played: optional positionclass constraint.
#
# Scoring (matcher): every term must pass ALL its gates or the entry scores
# 0. A passing term contributes weight * min(|delta| / (3*min_abs), 1.0) --
# capped and min-normalised so no single wide-range input dominates on raw
# magnitude (the golden-anchor bully of prototype run 1). Ties break toward
# the signature with MORE terms (specificity), then lexicographically.
#
# License: GPL-3.0-or-later, like the tree.

CLASS_RACE = 8
CLASS_CONTACT = 10

SIGNATURES = {
    "prime.break.5": {
        "terms": [
            {"term": "I_BACKESCAPES", "side": "me", "direction": "down",
             "min_abs": 0.05, "weight": 1.0},
            # played_in caps at the band seam vs prime.extend.missed
            {"term": "I_CONTAIN", "side": "me", "direction": "up",
             "min_abs": 0.05, "max_abs": 0.30, "weight": 1.0,
             "played_in": [0.0, 0.80], "best_in": [0.85, 1.0]},
        ],
    },
    "prime.contain.lost": {
        "terms": [
            {"term": "I_CONTAIN", "side": "me", "direction": "up",
             "min_abs": 0.35, "weight": 1.5},
            {"term": "I_BACKESCAPES", "side": "me", "direction": "down",
             "min_abs": 0.20, "weight": 1.0},
        ],
    },
    "board.close.entry": {
        "terms": [
            {"term": "I_ENTER2", "side": "opp", "direction": "up",
             "min_abs": 0.10, "weight": 1.5},
            {"term": "I_BACK_CHEQUER", "side": "opp", "direction": "any",
             "played_in": [1.0, 1.0], "best_in": [1.0, 1.0], "weight": 1.0},
        ],
    },
    "blot.shot.given": {
        "terms": [
            {"term": "I_FORWARD_ANCHOR", "side": "me", "direction": "any",
             "max_abs": 0.29, "weight": 0.0},
            {"term": "I_P1", "side": "opp", "direction": "down",
             "min_abs": 0.10, "weight": 1.0},
            {"term": "I_PIPLOSS", "side": "opp", "direction": "down",
             "min_abs": 0.05, "weight": 1.0},
        ],
    },
    "blot.double.given": {
        "terms": [
            {"term": "I_FORWARD_ANCHOR", "side": "me", "direction": "any",
             "max_abs": 0.29, "weight": 0.0},
            {"term": "I_P2", "side": "opp", "direction": "down",
             "min_abs": 0.05, "weight": 2.0},
            {"term": "I_P1", "side": "opp", "direction": "down",
             "min_abs": 0.05, "weight": 1.0},
            {"term": "I_PIPLOSS", "side": "opp", "direction": "down",
             "min_abs": 0.05, "weight": 1.0},
        ],
    },
    "anchor.surrender.back": {
        "terms": [
            # pip context (measured 2026-07-19): the surrender scenario lives
            # around opp 119-125; separates from contact.break.early (103)
            # and enter.fight.point (146). Provisional band, widen with pairs.
            {"term": "PipCount.opp", "side": "", "direction": "any",
             "played_in": [110, 135], "best_in": [110, 135], "weight": 0.0},
            {"term": "I_FORWARD_ANCHOR", "side": "me", "direction": "down",
             "min_abs": 0.30, "weight": 1.0,
             "played_in": [1.01, 2.0], "best_in": [0.01, 1.0]},
            {"term": "I_BACKBONE", "side": "me", "direction": "up",
             "min_abs": 0.20, "weight": 1.0},
            {"term": "I_P1", "side": "opp", "direction": "down",
             "min_abs": 0.10, "weight": 1.0},
        ],
    },
    "anchor.advance.golden": {
        "terms": [
            # best_in narrowed to THE golden point (0.833) -- the earlier
            # [0.5,1.0] fired the golden phrase on 21/22-point advances:
            # wrong words. Those now belong to anchor.advance.mid; the
            # 19-advance (their 6-point, 1.0) is deliberately uncovered
            # rather than mislabeled.
            {"term": "I_FORWARD_ANCHOR", "side": "me", "direction": "up",
             "min_abs": 0.30, "weight": 1.0,
             "played_in": [0.01, 0.40], "best_in": [0.80, 0.87]},
        ],
    },
    "anchor.advance.mid": {
        "terms": [
            {"term": "I_FORWARD_ANCHOR", "side": "me", "direction": "up",
             "min_abs": 0.25, "weight": 1.0,
             "played_in": [0.01, 0.40], "best_in": [0.45, 0.70]},
        ],
    },
    "race.break.ahead": {
        "class_played": CLASS_CONTACT,
        "class_best": CLASS_RACE,
        "terms": [
            {"term": "I_BREAK_CONTACT", "side": "me", "direction": "down",
             "min_abs": 0.10, "weight": 1.0},
        ],
    },
    "race.escape.window": {
        "class_best": CLASS_CONTACT,
        "terms": [
            {"term": "I_FORWARD_ANCHOR", "side": "me", "direction": "any",
             "best_in": [1.01, 2.0], "weight": 0.0},
            {"term": "I_BACK_CHEQUER", "side": "me", "direction": "down",
             "min_abs": 0.10, "weight": 1.0},
            {"term": "I_BACKESCAPES", "side": "opp", "direction": "up",
             "min_abs": 0.10, "weight": 1.0},
            {"term": "I_BREAK_CONTACT", "side": "me", "direction": "down",
             "min_abs": 0.02, "weight": 0.5},
        ],
    },
    "blitz.point.missed": {
        "terms": [
            {"term": "I_ENTER2", "side": "opp", "direction": "up",
             "min_abs": 0.10, "weight": 1.5},
            {"term": "I_BACK_CHEQUER", "side": "opp", "direction": "up",
             "min_abs": 0.10, "weight": 1.0},
            {"term": "I_ENTER", "side": "opp", "direction": "up",
             "min_abs": 0.10, "weight": 1.0},
            {"term": "PipCount.opp", "side": "", "direction": "up",
             "min_abs": 3.0, "weight": 1.0},
        ],
    },
    "timing.hold.crunch": {
        "terms": [
            # context: a HOLDING game -- exactly one rear anchor, both boards
            # (I_BACKG1 and I_BACKG are mutually exclusive by eval.c:1272)
            {"term": "I_BACKG1", "side": "me", "direction": "any",
             "played_in": [0.01, 5.0], "best_in": [0.01, 5.0], "weight": 0.5},
            # veto: a holding game is NOT a prime -- containment below the
            # 0.85 partition that prime.break.5 requires above (measured:
            # hold boards 0.58-0.67, prime boards 0.94)
            {"term": "I_CONTAIN", "side": "me", "direction": "any",
             "best_in": [0.0, 0.84], "weight": 0.0},
            {"term": "I_TIMING", "side": "me", "direction": "up",
             "min_abs": 0.10, "weight": 1.5},
        ],
    },
    "backgame.timing": {
        "terms": [
            # context: it IS a backgame -- two-plus rear anchors, both boards
            {"term": "I_BACKG", "side": "me", "direction": "any",
             "played_in": [0.01, 5.0], "best_in": [0.01, 5.0], "weight": 0.5},
            {"term": "I_TIMING", "side": "me", "direction": "up",
             "min_abs": 0.10, "weight": 1.5},
        ],
    },
    "hit.declined": {
        "terms": [
            {"term": "I_BACK_CHEQUER", "side": "opp", "direction": "up",
             "min_abs": 0.30, "weight": 1.5},
            {"term": "PipCount.opp", "side": "", "direction": "up",
             "min_abs": 4.0, "weight": 1.5},
            {"term": "I_ENTER", "side": "opp", "direction": "up",
             "min_abs": 0.02, "weight": 1.0},
        ],
    },

    # ------------------------------------------------ batch 2 (2026-07-19)
    "bearoff.shot.left": {
        "class_played": CLASS_CONTACT, "class_best": CLASS_CONTACT,
        "terms": [
            {"term": "I_P1", "side": "opp", "direction": "down",
             "min_abs": 0.04, "weight": 1.5,
             "played_in": [0.05, 1.0], "best_in": [0.0, 0.02]},
            # separator from blot.*: my rearmost is already home-side
            {"term": "I_BACK_CHEQUER", "side": "me", "direction": "any",
             "played_in": [0.0, 0.40], "best_in": [0.0, 0.40], "weight": 1.5},
        ],
    },
    "prime.extend.missed": {
        "terms": [
            # measured 2026-07-19: a real 4-prime already contains 0.80-0.86;
            # the extension to five reads +0.08..0.14 -> gates set to reality
            # band-split vs prime.break.5 (measured: break played 0.722-0.806,
            # extend played 0.806-0.861): the 0.80-0.82 seam stays uncovered
            {"term": "I_CONTAIN", "side": "me", "direction": "up",
             "min_abs": 0.06, "weight": 1.5,
             "played_in": [0.82, 0.92], "best_in": [0.90, 1.0]},
            {"term": "I_BACKESCAPES", "side": "me", "direction": "down",
             "min_abs": 0.04, "weight": 1.0},
        ],
    },
    "enter.fight.point": {
        "terms": [
            # played split into the fight (no anchor in zone), best made it
            {"term": "I_FORWARD_ANCHOR", "side": "me", "direction": "down",
             "min_abs": 0.30, "weight": 2.0,
             # measured: fight anchors 21/22 read 0.667/0.500
             "played_in": [1.0, 2.0], "best_in": [0.30, 0.70]},
            {"term": "I_P1", "side": "opp", "direction": "down",
             "min_abs": 0.03, "weight": 1.0},
            # separator vs anchor.surrender.back: here the played side has NO
            # made rear anchor at all (rearmost made point is the midpoint)
            {"term": "I_BACK_ANCHOR", "side": "me", "direction": "any",
             "played_in": [0.0, 0.60], "best_in": [0.0, 1.0], "weight": 0.0},
            # pip context (measured): near-level race ~146; separates from the
            # surrender (119-125) and lost-race (103) siblings. Provisional.
            {"term": "PipCount.opp", "side": "", "direction": "any",
             "played_in": [135, 160], "best_in": [135, 160], "weight": 0.0},
        ],
    },
    "hit.loose.homeboard": {
        "terms": [
            # both sides of the pair have the opponent on the bar
            {"term": "I_BACK_CHEQUER", "side": "opp", "direction": "any",
             "played_in": [0.95, 1.0], "best_in": [0.95, 1.0], "weight": 1.5},
            {"term": "I_P1", "side": "opp", "direction": "down",
             "min_abs": 0.05, "weight": 1.5,
             "played_in": [0.08, 1.0], "best_in": [0.0, 0.04]},
            {"term": "I_ENTER", "side": "opp", "direction": "up",
             "min_abs": 0.01, "weight": 1.0},
            # the "homeboard" in the name, measured: covering the hit steps
            # the quantized closure (+0.194); outfield double-hits stay flat
            {"term": "I_ENTER2", "side": "opp", "direction": "up",
             "min_abs": 0.15, "weight": 0.5},
        ],
    },
    # ------------------------------------------------ batch 3 (2026-07-19)
    "hit.double.declined": {
        "terms": [
            # the double-hit pip haul; floor calibrated by the b-t QA run
            # (+11 measured on the shallow construction). Seam vs
            # hit.declined set after measuring both clusters.
            {"term": "PipCount.opp", "side": "", "direction": "up",
             "min_abs": 10.0, "weight": 1.5},
            {"term": "I_ENTER", "side": "opp", "direction": "up",
             "min_abs": 0.03, "weight": 1.0},
            {"term": "I_BACK_CHEQUER", "side": "opp", "direction": "any",
             "played_in": [0.95, 1.0], "best_in": [0.95, 1.0], "weight": 1.0},
        ],
    },
    "board.crunch.spared": {
        "terms": [
            # measured 2026-07-19: the STRUCTURE crunch -- entry cost rises
            {"term": "I_ENTER", "side": "opp", "direction": "up",
             "min_abs": 0.05, "weight": 2.0},
            # the fingerprint vs board.close.entry: point COUNT kept, so the
            # quantized closure input stays flat (close.entry always steps)
            {"term": "I_ENTER2", "side": "opp", "direction": "any",
             "max_abs": 0.05, "weight": 0.5},
            # defined at the bar: the opponent is entering against this board
            {"term": "I_BACK_CHEQUER", "side": "opp", "direction": "any",
             "played_in": [0.95, 1.0], "best_in": [0.95, 1.0], "weight": 0.0},
        ],
    },
    "point.deep.wasted": {
        "terms": [
            # measured 2026-07-19: deep-point boards read played 0.556-0.583,
            # front-point best 0.694-0.806; prime.break.5 cannot fire here
            # (its best_in floor 0.85 sits above this whole band)
            {"term": "I_CONTAIN", "side": "me", "direction": "up",
             "min_abs": 0.05, "weight": 1.5,
             "played_in": [0.45, 0.62], "best_in": [0.65, 0.85]},
            # separator vs timing.hold.crunch: this is an early-structure
            # scenario, timing reserve still low (holding-crunch reads 0.49+)
            {"term": "I_TIMING", "side": "me", "direction": "any",
             "played_in": [0.0, 0.40], "best_in": [0.0, 1.0], "weight": 0.0},
        ],
    },
    "contact.break.early": {
        "terms": [
            # measured: holding reads 0.156, the break 0.09-0.10 -- small
            # absolute values, the DELTA is the pattern
            {"term": "I_BREAK_CONTACT", "side": "me", "direction": "up",
             "min_abs": 0.045, "weight": 1.5,
             "played_in": [0.0, 0.25], "best_in": [0.12, 1.0]},
            # the run abandons the high anchor: rearmost made point collapses
            # from the anchor to the midpoint (measured +0.29 best-played)
            {"term": "I_BACK_ANCHOR", "side": "me", "direction": "up",
             "min_abs": 0.10, "weight": 1.5},
            # the race is long and the opponent clearly ahead: holding is the game
            {"term": "PipCount.opp", "side": "", "direction": "any",
             "played_in": [85, 130], "best_in": [85, 130], "weight": 0.5},
        ],
    },
}
