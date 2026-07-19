#!/usr/bin/env python3
# Bake the adopted narrator rules into the app asset. Refuses tier != authored
# (DELTA_NARRATOR_PLAYBOOK.md L4); asserts the rule count loudly (L7).
import json, pathlib, sys
HERE = pathlib.Path(__file__).resolve().parent
REPO = HERE.parents[1]
src = json.load(open(HERE / "rules_draft.json"))
if src.get("tier") != "authored":
    sys.exit("REFUSED: rules tier is '%s', not 'authored'" % src.get("tier"))
n = len(src["rules"])
assert n == 10, "rule count %d != expected 10 -- update the assertion deliberately" % n
out = REPO / "gnubg-app/app/src/main/assets/narrator_rules_v0.json"
json.dump(src, open(out, "w"), indent=1)
print("baked %s  tier=%s  rules=%d" % (out.relative_to(REPO), src["tier"], n))
