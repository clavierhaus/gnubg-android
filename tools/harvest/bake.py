#!/usr/bin/env python3
# bake.py -- Phase F of CORPUS_HARVEST_PLAN: read the curated markdown, emit
# the corpus asset. The SHIPPING artifact (insights_v0.json) is refused while
# any phrase is still tier "proposed"; --dev emits insights_dev.json with the
# proposed wording for on-device testing on a branch, clearly tiered.
# License: GPL-3.0-or-later, like the tree.

import datetime
import json
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from signatures import SIGNATURES
from harvest import ENTRY_META

REPO = Path(__file__).resolve().parents[2]
CURATED = REPO / "tools/harvest/output/curated"
ASSETS = REPO / "gnubg-app/app/src/main/assets"

SEVERITY = {                       # gnubg skill bands per entry, from the draft
    "prime.break.5": ["doubtful", "bad"],
    "prime.contain.lost": ["doubtful", "bad", "very bad"],
    "board.close.entry": ["doubtful", "bad"],
    "blot.shot.given": ["doubtful", "bad", "very bad"],
    "blot.double.given": ["bad", "very bad"],
    "anchor.surrender.back": ["bad", "very bad"],
    "bearoff.shot.left": ["doubtful", "bad", "very bad"],
    "prime.extend.missed": ["doubtful", "bad"],
    "enter.fight.point": ["doubtful", "bad"],
    "hit.loose.homeboard": ["doubtful", "bad", "very bad"],
    "contact.break.early": ["bad", "very bad"],
    "hit.double.declined": ["doubtful", "bad", "very bad"],
    "board.crunch.spared": ["doubtful", "bad"],
    "point.deep.wasted": ["doubtful", "bad"],
    "blot.cover.missed": ["doubtful", "bad"],
    "anchor.split.straggler": ["doubtful", "bad", "very bad"],
    "anchor.advance.golden": ["doubtful"],
    "race.break.ahead": ["doubtful", "bad"],
    "race.escape.window": ["doubtful", "bad"],
    "hit.declined": ["doubtful", "bad"],
    "backgame.timing": ["bad", "very bad"],
    "timing.hold.crunch": ["doubtful", "bad"],
    "blitz.point.missed": ["doubtful", "bad"],
    "anchor.advance.mid": ["doubtful"],
}


def parse_curated(path):
    s = path.read_text()
    tier = "authored" if "tier: authored" in s else "proposed"
    flag = re.search(r"- phrase_flag: (.+)", s).group(1).strip()
    praise = re.search(r"- phrase_praise: (.+)", s).group(1).strip()
    return (None if flag == "null" else flag,
            None if praise == "null" else praise, tier)


def main():
    dev = "--dev" in sys.argv
    entries = []
    worst_tier = "authored"
    for entry, (principle, category) in ENTRY_META.items():
        f = CURATED / f"{entry}.md"
        if not f.exists():
            sys.exit(f"missing curated file: {f}")
        flag, praise, tier = parse_curated(f)
        if tier != "authored":
            worst_tier = "proposed"
        entries.append({
            "id": entry, "principle": principle,
            "phrase_flag": flag, "phrase_praise": praise,
            "category": category, "signature": SIGNATURES[entry],
            "severity_hint": SEVERITY[entry], "tier": tier,
        })
    if not dev and worst_tier != "authored":
        sys.exit("REFUSED: phrases still tier 'proposed'. The shipping asset "
                 "bakes only maintainer-authored wording. Use --dev for a "
                 "branch-test asset.")
    doc = {
        "version": 1,
        "corpus_license": "GPL-3.0-or-later",
        "generated": datetime.date.today().isoformat(),
        "tier": worst_tier,
        "authors": ["clavierhaus <gnubg@clavierhaus.at>",
                    "Clavierhaus Backgammon contributors"],
        "acknowledgements": {
            "yairwein/backgammon-teacher": {
                "license": "MIT",
                "copyright": "Copyright (c) 2024-2026 Yair Weinberger",
                "for": "presentation taxonomy (race/board/threat), "
                       "notable-delta threshold shape, harvest prompt "
                       "scaffolding and rules",
            }
        },
        "draft_assistance_note":
            "Entries were first-drafted with the assistance of large "
            "language models (claude-sonnet-4-6). Each entry is "
            "human-verified and edited into original expression before "
            "inclusion. No prose from copyrighted sources is included.",
        "entries": entries,
    }
    ASSETS.mkdir(parents=True, exist_ok=True)
    out = ASSETS / ("insights_dev.json" if dev else "insights_v0.json")
    out.write_text(json.dumps(doc, indent=1) + "\n")
    print(f"baked {out.relative_to(REPO)}  tier={worst_tier}  "
          f"entries={len(entries)}")


if __name__ == "__main__":
    main()
