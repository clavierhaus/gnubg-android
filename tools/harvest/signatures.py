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
            {"term": "I_CONTAIN", "side": "me", "direction": "up",
             "min_abs": 0.05, "max_abs": 0.30, "weight": 1.0,
             "best_in": [0.85, 1.0]},
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
            {"term": "I_FORWARD_ANCHOR", "side": "me", "direction": "up",
             "min_abs": 0.30, "weight": 1.0,
             "played_in": [0.01, 0.40], "best_in": [0.50, 1.0]},
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
}
