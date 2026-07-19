# prompt_template.py -- Phase D prompt scaffolding (CORPUS_HARVEST_PLAN §4.D).
#
# The structure of SYSTEM_PROMPT and the sectioned user prompt is ported from
# yairwein/backgammon-teacher, src/lib/llm/prompt.ts (buildSystemPrompt /
# buildExplanationPrompt), whose race/board/threat presentation taxonomy and
# whose constitution-rule "Only reference facts provided in the data. Never
# invent strategic facts." this project adopts with credit. Per that
# project's LICENSE, its notice is included here verbatim:
#
# ---------------------------------------------------------------------------
# MIT License
#
# Copyright (c) 2024-2026 Yair Weinberger
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
# ---------------------------------------------------------------------------
#
# Everything below the ported structure -- the gnubg I_* vocabulary, the
# input meanings (from docs/INPUT_DICTIONARY.md), the phrase-oriented output
# schema and its Phase E constraints -- is this repository's own work:
# Copyright (C) 2025-2026 clavierhaus <gnubg@clavierhaus.at>,
# GPL-3.0-or-later. Offline tooling only; nothing here ships in the APK.

# Verified meanings, condensed from docs/INPUT_DICTIONARY.md (formula-first,
# measurement-second, comment-last). The prompt names gnubg's numbers in
# gnubg's vocabulary -- I_BACK_ANCHOR delta, not "anchors".
INPUT_MEANING = {
    "I_BREAK_CONTACT": "pip-mass of this side's checkers still engaged behind the opponent's rearmost checker (falls as they run past)",
    "I_BACK_CHEQUER": "position of the rearmost checker / 24 (1.0 = on the bar)",
    "I_BACK_ANCHOR": "position of the rearmost made point / 24",
    "I_FORWARD_ANCHOR": "most advanced anchor in the opponent's home zone, (24-j)/6; golden point = 0.833; values above 1.0 mean no anchor in the zone",
    "I_PIPLOSS": "expected pips this side's hits take off the opponent",
    "I_P1": "rolls with which this side hits at least one checker /36",
    "I_P2": "rolls with which this side hits at least two checkers /36",
    "I_BACKESCAPES": "rolls with which the OPPONENT's back checker escapes through this side's blockade /36",
    "I_CONTAIN": "this side's worst-case containment of the opponent's runner (1.0 = no escape roll anywhere)",
    "I_ENTER": "expected pip-loss while entering from the bar against the opponent's home board",
    "I_ENTER2": "closure of the board this side must enter against: 0.556 = two points made, 0.750 = three, 1.0 = closed",
    "I_TIMING": "timing reserve: rear pip-mass plus home-board spares",
    "I_BACKBONE": "connectivity of this side's rear points (falls as the structure fragments)",
    "I_BACKG": "backgame indicator: checker mass on two or more anchors in the opponent's home",
    "I_MOBILITY": "pip-weighted freedom of this side's outfield checkers through the opponent's blockade",
    "PipCount.opp": "the opponent's pip count",
}

# System prompt: structure ported from buildSystemPrompt (see header); the
# task, the length/voice constraints (Phase E rejection criteria, pushed into
# the draft so drafts start closer), and the vocabulary rule are ours.
SYSTEM_PROMPT = """You are a backgammon coach drafting one short teachable phrase.

RULES:
- Only reference facts provided in the data. Never invent strategic facts.
- The phrase asserts a principle; it does not counsel. Never write "You should consider..." or similar.
- At most 25 words. A full sentence, not a stub.
- Plain language a casual player understands; the numbers justify the phrase but do not appear in it. "You broke a 5-prime for a play that gains little" -- never "loses 0.081".
- Do not quote or closely paraphrase any backgammon book or author.

POLARITY -- read carefully: phrase_flag describes the MISTAKE the player
made (what the PLAYED position shows), never the good play. Example: for a
missed home-board point, flag = "Your board stayed open with a checker on
the bar", NOT "Making a home point traps the checker" -- that is the
praise. Two harvest runs inverted this; do not.

OUTPUT FORMAT (JSON, nothing else):
{
  "phrase_flag": "The phrase shown when the player's move fits this mistake pattern (or null if this entry praises)",
  "phrase_praise": "The phrase shown when the BEST move fits this pattern and was found (or null)",
  "category": "race|board|threat",
  "reasons": ["Why the data supports the phrase, briefly", "..."],
  "confidence": "low|medium|high"
}

Set confidence by how directly the provided feature deltas support the phrase:
- high: the deltas state the pattern plainly
- medium: the phrase is reasonable but reads beyond the strongest delta
- low: the pattern is only weakly present in the data"""


def _side_label(side):
    return {"me": "player on roll", "opp": "opponent", "": ""}[side]


def _board_lines(vec):
    me = ["%d:%d" % (p, vec[p]) for p in range(1, 25) if vec[p] > 0]
    opp = ["%d:%d" % (p, -vec[p]) for p in range(1, 25) if vec[p] < 0]
    lines = ["- Player points (point:checkers): " + (" ".join(me) or "none")]
    if vec[25]:
        lines.append("- Player on the bar: %d" % vec[25])
    lines.append("- Opponent points (player's numbering): " + (" ".join(opp) or "none"))
    if vec[0]:
        lines.append("- Opponent on the bar: %d" % -vec[0])
    return "\n".join(lines)


def build_user_prompt(entry_id, principle, category, pair, signature):
    """Sectioned in the shape of buildExplanationPrompt (see header); the
    content is gnubg's numbers named in gnubg's vocabulary."""
    sections = []
    sections.append("## Teaching target\n- Entry: %s\n- Principle: %s\n- Category: %s"
                    % (entry_id, principle, category))
    sections.append("## Position as PLAYED (the mistake)\n" + _board_lines(pair["played"]))
    sections.append("## Position after the BEST play\n" + _board_lines(pair["best"]))
    sections.append("## Pip counts (played position)\n- Player: %d\n- Opponent: %d"
                    % tuple(pair["pips_played"]))
    feat = []
    for t in signature["terms"]:
        if t.get("weight", 0) == 0:
            continue
        key = ("PipCount.opp" if t["term"] == "PipCount.opp"
               else "%s.%s" % (t["side"], t["term"]))
        d = pair["deltas"].get(key)
        if d is None:
            continue
        meaning = INPUT_MEANING.get(t["term"], "")
        feat.append("- %s (%s): best - played = %+0.3f -- %s"
                    % (t["term"], _side_label(t["side"]), d, meaning))
    sections.append("## gnubg feature deltas (the measured signature)\n" + "\n".join(feat))
    # Context: the OTHER gnubg inputs that moved. Same tier-1 source, shown so
    # the draft is grounded in the whole position, not a keyhole (the
    # documented "retreat" hallucination came from under-context). Threshold
    # shape after b-t's getNotableThreshold, values ours (normalised inputs).
    dall = pair.get("deltas_all")
    if dall:
        sig_keys = {("PipCount.opp" if t["term"] == "PipCount.opp"
                     else "%s.%s" % (t["side"], t["term"])) for t in signature["terms"]}
        ctx = []
        for key, d in sorted(dall.items(), key=lambda kv: -abs(kv[1])):
            if key in sig_keys:
                continue
            thr = 5.0 if key.startswith("PipCount") else 0.10
            if abs(d) < thr:
                continue
            term = key.split(".", 1)[1] if "." in key and not key.startswith("PipCount") else key
            side = key.split(".", 1)[0] if not key.startswith("PipCount") else ""
            meaning = INPUT_MEANING.get(term, "")
            ctx.append("- %s (%s): best - played = %+0.3f%s"
                       % (term, _side_label(side) if side in ("me","opp") else "pip count",
                          d, (" -- " + meaning) if meaning else ""))
            if len(ctx) >= 8:
                break
        if ctx:
            sections.append("## Other notable gnubg deltas (context, not the target)\n" + "\n".join(ctx))
    sections.append("## Task\nDraft the phrase(s) for this entry per the output format.")
    return "\n\n".join(sections)
