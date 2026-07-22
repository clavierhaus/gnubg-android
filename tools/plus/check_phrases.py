#!/usr/bin/env python3
"""Phrase gate for the coach's authored text.

Fails on every defect class that has actually reached a screen. It does NOT
prove a phrase is licensed -- G-LICENSE in docs/PHRASE_RESEARCH_ROUTINE.md
still needs a human reading eval.c. What it does is make each shipped defect
unrepeatable, which is the part that was missing: all four classes were caught
by playing, none by any check.

    ./tools/plus/check_phrases.py            all gates (grammar needs Java)
    ./tools/plus/check_phrases.py --no-grammar

Exit status is non-zero on any finding, so it can gate a build.
"""
import json, re, sys, os

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
NARRATOR = os.path.join(ROOT, "gnubg-app/app/src/main/assets/narrator_rules_v0.json")
CORPUS   = os.path.join(ROOT, "gnubg-app/app/src/main/assets/insights_v0.json")

findings = []
def fail(gate, where, msg, text=""):
    findings.append((gate, where, msg, text))

# ---------------------------------------------------------------------------
# Input semantics, each read from engine-core/eval.c. "sign" is what a HIGHER
# value means. "rolls" marks inputs that are literally n/36 and may therefore
# be spoken as a count of numbers; the others may not (see the routine doc).
# ---------------------------------------------------------------------------
INPUTS = {
    "I_P1":            ("n1/36 -- rolls hitting at least one chequer", "more hitting numbers", True),
    "I_P2":            ("n2/36 -- rolls hitting two or more",          "more double-hit numbers", True),
    "I_BACKESCAPES":   ("Escapes/36",                                  "more escaping numbers", True),
    "I_CONTAIN":       ("(36-n)/36, n = minimum escapes",              "better containment", True),
    "I_ENTER":         ("loss/(36*49/6) -- equity weighted",           "costlier entry", False),
    "I_ENTER2":        ("(36-(n-6)^2)/36, n = made home points",       "more home points made", False),
    "I_BACK_CHEQUER":  ("nBack/24",                                    "rearmost checker further back", False),
    "I_BACK_ANCHOR":   ("i/24 -- rearmost anchor's point",             "rearmost anchor further back", False),
    "I_FORWARD_ANCHOR":("n==0 ? 2.0 : n/6 -- 2.0 is a SENTINEL for NONE", "LESS advanced anchor", False),
    "I_BACKBONE":      ("1-w/(tot*11) over gaps between made points",  "MORE scattered (disconnection)", False),
    "I_BREAK_CONTACT": ("np/167, own checkers still behind opp rearmost", "MORE contact retained", False),
    "PipCount.opp":    ("opponent's pip count",                        "opponent further behind", False),
}

# Audited directions (2026-07-22, each checked against eval.c). A change here
# must be accompanied by re-reading the input's computation -- this table is
# what caught board.structure.connected asserting the opposite of its trigger.
EXPECTED_DIRECTION = {
    "threat.shots.given": "down", "threat.hit.available": "up",
    "threat.entry.cost": "up",    "threat.risk.accepted": "up",
    "board.point.closed": "up",   "board.contain.tighter": "up",
    "board.anchor.kept": "up",    "board.anchor.made": "down",
    "board.structure.connected": "down",
    "race.pips.taken": "up",      "race.contact.held": "up",
}

CONVENTIONS = [
    (r"\bplays?\b",                  "'play' is banned -- use 'variant'"),
    (r"\btheir\b|\bthem\b|\bthey\b", "opponent pronoun -- use 'the opponent'"),
    (r"\bhis\b|\bhim\b|\bher\b",     "gendered third person"),
    (r"\bthe best (play|move|variant)\b", "non-neutral referent -- use {REF}"),
]
# Words established as wrong (not the glossary's term) or unmeasurable by any
# input we have. Each was found in a phrase that shipped.
BANNED_VOCAB = [
    (r"\bblockade\b",      "glossary term for I_CONTAIN is 'contain'"),
    (r"\bback checkers\b", "glossary term is 'runners'"),
    (r"\bre-?enter\b",     "glossary verb is 'enter'"),
    (r"\brear (point|structure)\b", "not glossary terms -- 'anchor', 'connected'"),
    (r"\bcrunch",          "no input measures a crunch"),
    (r"\bspare\b",         "no input measures a spare checker"),
    (r"\bdead point",      "no input measures a dead point"),
    (r"\bbuilder\b",       "no input identifies a builder"),
]
# Claims that need a specific term present in the entry's own signature.
CLAIM_REQUIRES = [
    (r"(both|two)\b[^.]{0,20}\bblots?\b|\bblots?\b[^.]{0,20}\b(both|two)\b",
     ["I_P2"], "a two-blot claim needs I_P2 (rolls hitting >1)"),
    (r"\brace\b",         ["PipCount.opp"], "a race claim needs a pip term"),
    (r"\bpips?\b",        ["PipCount.opp"], "a pip claim needs a pip term"),
]
TENSE = [
    (r"\bwill\b|\bgoing to\b", "prediction -- the matcher measures the present position only"),
    (r"\bwas\b|\bwere\b",      "past tense implies a PRE-move fact; match() sees post-move boards only"),
]

def gates_on_text(where, text, terms):
    for pat, msg in CONVENTIONS:
        if re.search(pat, text, re.I): fail("G-CONVENTION", where, msg, text)
    for pat, msg in BANNED_VOCAB:
        if re.search(pat, text, re.I): fail("G-DICTIONARY", where, msg, text)
    for pat, msg in TENSE:
        if re.search(pat, text, re.I): fail("G-LICENSE", where, msg, text)
    for pat, need, msg in CLAIM_REQUIRES:
        if re.search(pat, text, re.I) and not any(n in t for t in terms for n in need):
            fail("G-LICENSE", where, msg, text)
    # "numbers"/"rolls" may only be spoken of inputs that ARE n/36
    if re.search(r"\b(more|fewer|extra|\d+)\s+(numbers|rolls)\b|\b(numbers|rolls)\s+that\b", text, re.I):
        if not any(INPUTS.get(t.split(".")[-1], (None, None, False))[2] for t in terms):
            fail("G-LICENSE", where, "counts rolls, but no term in the signature is n/36", text)

# --- narrator ---------------------------------------------------------------
d = json.load(open(NARRATOR))
rules = d if isinstance(d, list) else d["rules"]
seen = set()
for r in rules:
    rid, s = r["id"], r["sentence"]
    seen.add(rid)
    exp = EXPECTED_DIRECTION.get(rid)
    if exp is None:
        fail("G-DIRECTION", rid, "new rule: add it to EXPECTED_DIRECTION after reading eval.c")
    elif r.get("direction") != exp:
        fail("G-DIRECTION", rid,
             f"direction is '{r.get('direction')}', audited value is '{exp}'. "
             f"{r['term']} higher means: {INPUTS.get(r['term'], ('','?',False))[1]}")
    if r["term"] not in INPUTS:
        fail("G-DIRECTION", rid, f"unknown input {r['term']} -- read eval.c and add it")
    if "{REF}" not in s and not s.startswith("Your move"):
        fail("G-CONVENTION", rid, "no {REF} token: the compared variant needs a neutral referent", s)
    gates_on_text(rid, s, [r["term"]])
for rid in EXPECTED_DIRECTION:
    if rid not in seen:
        fail("G-DIRECTION", rid, "audited rule has disappeared from the rules file")

# --- corpus -----------------------------------------------------------------
c = json.load(open(CORPUS))
for e in c["entries"]:
    terms = [f"{t.get('side','')}.{t['term']}" for t in e["signature"]["terms"]]
    for key in ("phrase_flag", "phrase_praise"):
        if e.get(key):
            gates_on_text(f"{e['id']}[{key}]", e[key], terms)

# --- grammar ----------------------------------------------------------------
if "--no-grammar" not in sys.argv:
    try:
        import language_tool_python as lt
        tool = lt.LanguageTool("en-GB")
        IGNORE = {"EN_QUOTES", "WHITESPACE_RULE", "DASH_RULE"}
        # Backgammon vocabulary absent from a general en-GB dictionary. Every
        # word here is a Backgammon Galore glossary entry; the glossary, not
        # LanguageTool, is the authority on the game's terms.
        DOMAIN = {"backgame", "backgames", "blot", "blots", "gammon", "gammons",
                  "backgammon", "pip", "pips", "chequer", "chequers", "bearoff",
                  "blitz", "prime", "primes", "anchor", "anchors", "midpoint",
                  "runner", "runners", "bar", "cube", "doubler", "gnubg"}
        def check(where, text):
            for m in tool.check(text):
                rid_ = getattr(m, "rule_id", None) or getattr(m, "ruleId", "?")
                off = getattr(m, "offset", 0)
                ln  = getattr(m, "errorLength", None) or getattr(m, "error_length", 0)
                word = text[off:off+ln].strip().lower() if ln else ""
                if rid_.startswith("MORFOLOGIK") and word in DOMAIN:
                    continue
                if rid_ not in IGNORE:
                    fail("G-GRAMMAR", where, rid_ + ": " + m.message, text)
        for r in rules:
            for fillv in ("the best variant", "this variant"):
                s = r["sentence"].replace("{REF}", fillv)
                check(r["id"] + f" [{fillv}]", s[0].upper() + s[1:])
        for e in c["entries"]:
            for key in ("phrase_flag", "phrase_praise"):
                if e.get(key): check(f"{e['id']}[{key}]", e[key])
        tool.close()
    except ImportError:
        print("note: language_tool_python not installed -- grammar gate skipped")

# --- report -----------------------------------------------------------------
if findings:
    print(f"\nPHRASE GATE: {len(findings)} finding(s)\n")
    for gate, where, msg, text in findings:
        print(f"  [{gate}] {where}")
        print(f"      {msg}")
        if text: print(f"      :: {text}")
    sys.exit(1)

print(f"PHRASE GATE: clean ({len(rules)} narrator rules, {len(c['entries'])} corpus entries)")
