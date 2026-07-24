# EXPERIMENT — phrase relicensing (branch experiment/phrase-relicense-audit)

STATUS: EXPERIMENT ONLY. Not merged, not on `plus`, not shipped.

Baseline: plus @ 9783ce59bd2a90d36fe82c8825330a35e63dd40f (audit harness, before
any phrase rewrite). This branch changes exactly two files:

  gnubg-app/app/src/main/assets/insights_v0.json   3 entries reworded
  tools/narrator/audit/audit.py                    3 predicates re-keyed

Result: 86,121 pairs, TOTAL VIOLATIONS 0 (was 30).

FULL REVERSAL, from any clone:

    git checkout plus
    git branch -D experiment/phrase-relicense-audit      # local only
    git push origin --delete experiment/phrase-relicense-audit   # if ever pushed

`plus` is untouched by this branch and needs no revert.

Not verified here: device behaviour, Kotlin gate (no SDK in the sandbox that ran
this), and whether the reworded sentences READ well to a player -- the audit
proves only that they are not false where they fire.
