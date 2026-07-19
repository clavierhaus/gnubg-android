#!/usr/bin/env python3
# harvest.py -- Phase D of CORPUS_HARVEST_PLAN: the offline draft run.
#
# For every verified pair in tools/harvest/positions/*.json, build the
# credited prompt (prompt_template.py) and call BOTH Claude and GPT with
# identical prompts. Raw JSON per position per model lands in
# tools/harvest/output/raw/<entry>.<pair>.<model>.json (gitignored). The run
# is not reproducible bit-for-bit and does not need to be (plan §4.D); model
# strings are recorded in run metadata for workflow reproducibility.
#
# Keys are env-only, never in git:
#   ANTHROPIC_API_KEY   (model: ANTHROPIC_MODEL, default claude-sonnet-4-6)
#   OPENAI_API_KEY      (model: OPENAI_MODEL, REQUIRED if the key is set --
#                        no default is guessed here)
# A provider without a key is skipped with a notice. --dry-run prints one
# fully assembled prompt and exits; --limit N caps pairs per entry.
#
# License: GPL-3.0-or-later, like the tree. Nothing here runs on-device.

import json
import os
import sys
import urllib.request
import datetime
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from signatures import SIGNATURES
from prompt_template import SYSTEM_PROMPT, build_user_prompt

REPO = Path(__file__).resolve().parents[2]
POSDIR = REPO / "tools/harvest/positions"
RAWDIR = REPO / "tools/harvest/output/raw"

# Principle and category per entry, from docs/CORPUS_ENTRIES_DRAFT.md.
ENTRY_META = {
    "prime.break.5": ("A 5-prime is worth holding.", "board"),
    "bearoff.shot.left": ("Bear in without leaving the shot that loses a won race.", "threat"),
    "prime.extend.missed": ("When the builders are ready, the fifth prime point is the play.", "board"),
    "enter.fight.point": ("Enter making the advanced anchor, not splitting into the fight.", "board"),
    "hit.loose.homeboard": ("A loose hit in your board without cover invites the return shot.", "threat"),
    "contact.break.early": ("Far behind, the anchor is the game; don't break contact.", "race"),
    "prime.contain.lost": ("Don't let the trapped checker out for free.", "board"),
    "anchor.surrender.back": ("A back anchor is shelter; don't leave it without a reason.", "board"),
    "anchor.advance.golden": ("The 20-point anchor is worth fighting for.", "board"),
    "anchor.advance.mid": ("An advanced anchor beats a deep one.", "board"),
    "board.close.entry": ("Every home point you make keeps the hit man out longer.", "board"),
    "blot.shot.given": ("Count the shots before you leave the blot.", "threat"),
    "blot.double.given": ("One blot is a risk; two in range is a plan for disaster.", "threat"),
    "hit.declined": ("When the hit is right, take it -- pips on the bar are pips won.", "threat"),
    "race.break.ahead": ("Ahead in the race, break contact and run.", "race"),
    "race.escape.window": ("Escape rolls are a resource; don't spend the window doing nothing.", "race"),
    "backgame.timing": ("A backgame lives or dies on timing.", "board"),
    "timing.hold.crunch": ("A holding game survives on its timing.", "board"),
    "blitz.point.missed": ("When the blitz is on, points are made on heads.", "threat"),
}


def call_anthropic(key, model, user_prompt):
    req = urllib.request.Request(
        "https://api.anthropic.com/v1/messages",
        data=json.dumps({
            "model": model, "max_tokens": 1000,
            "system": SYSTEM_PROMPT,
            "messages": [{"role": "user", "content": user_prompt}],
        }).encode(),
        headers={"content-type": "application/json",
                 "x-api-key": key, "anthropic-version": "2023-06-01"})
    with urllib.request.urlopen(req, timeout=120) as r:
        body = json.load(r)
    return "".join(b.get("text", "") for b in body.get("content", []))


def call_openai(key, model, user_prompt):
    req = urllib.request.Request(
        "https://api.openai.com/v1/chat/completions",
        data=json.dumps({
            "model": model,
            "messages": [{"role": "system", "content": SYSTEM_PROMPT},
                         {"role": "user", "content": user_prompt}],
        }).encode(),
        headers={"content-type": "application/json",
                 "authorization": "Bearer " + key})
    with urllib.request.urlopen(req, timeout=120) as r:
        body = json.load(r)
    return body["choices"][0]["message"]["content"]


def main():
    dry = "--dry-run" in sys.argv
    limit = None
    if "--limit" in sys.argv:
        limit = int(sys.argv[sys.argv.index("--limit") + 1])

    providers = []
    # .strip(): a trailing newline from a copy-paste corrupts the auth
    # header and yields a misleading 401.
    ak = (os.environ.get("ANTHROPIC_API_KEY") or "").strip() or None
    if ak:
        providers.append(("anthropic",
                          os.environ.get("ANTHROPIC_MODEL", "claude-sonnet-4-6"),
                          lambda m, p, k=ak: call_anthropic(k, m, p)))
    else:
        print("ANTHROPIC_API_KEY unset -- skipping Claude")
    ok = (os.environ.get("OPENAI_API_KEY") or "").strip() or None
    if ok:
        om = os.environ.get("OPENAI_MODEL")
        if not om:
            sys.exit("OPENAI_API_KEY is set but OPENAI_MODEL is not -- name "
                     "the model explicitly; this tool does not guess one.")
        providers.append(("openai", om, lambda m, p, k=ok: call_openai(k, m, p)))
    else:
        print("OPENAI_API_KEY unset -- skipping GPT")

    files = sorted(POSDIR.glob("*.json"))
    if not files:
        sys.exit("no positions; run gen_positions.py first")

    if dry:
        c = json.loads(files[0].read_text())
        principle, cat = ENTRY_META[c["entry"]]
        print("=== SYSTEM ===\n" + SYSTEM_PROMPT)
        print("\n=== USER (%s / %s) ===" % (c["entry"], c["pairs"][0]["id"]))
        print(build_user_prompt(c["entry"], principle, cat,
                                c["pairs"][0], SIGNATURES[c["entry"]]))
        return

    if not providers:
        sys.exit("no provider keys in the environment; nothing to do")

    RAWDIR.mkdir(parents=True, exist_ok=True)
    meta = {"run": datetime.datetime.now().isoformat(timespec="seconds"),
            "models": {n: m for n, m, _ in providers}}
    (RAWDIR / "_run_meta.json").write_text(json.dumps(meta, indent=1) + "\n")

    for f in files:
        c = json.loads(f.read_text())
        entry = c["entry"]
        principle, cat = ENTRY_META[entry]
        pairs = c["pairs"][:limit] if limit else c["pairs"]
        for pair in pairs:
            prompt = build_user_prompt(entry, principle, cat, pair,
                                       SIGNATURES[entry])
            for name, model, call in providers:
                out = RAWDIR / ("%s.%s.%s.json" % (entry, pair["id"], name))
                if out.exists():
                    continue                      # resumable
                try:
                    text = call(model, prompt)
                    out.write_text(json.dumps(
                        {"entry": entry, "pair": pair["id"], "model": model,
                         "provider": name, "raw": text}, indent=1) + "\n")
                    print("ok  ", out.name)
                except Exception as e:            # noqa: BLE001 -- log & go on
                    print("FAIL", out.name, "--", e)


if __name__ == "__main__":
    main()
