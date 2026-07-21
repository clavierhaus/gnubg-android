# Your Personal Stats — design note (stage 4)

*The engineering translation of docs/POSITION_YOUR_PERSONAL_STATS.md (the
philosophy) and the 2026-07-21 advocatus review (the design corrections).
This note governs the match-report screen. The feature's user-facing name is
"Your Personal Stats"; internal identifiers (MatchReport, matchStats) keep
their names. The screen is a Plus surface; all engine numbers are FOSS
capability (the verbs), per the established fence.*

## The laws (each traceable to a governing document)

**L1 — No unlabeled numbers, ever.** Every statistic renders with the
convention that produced it, in the same visual unit: "ER 12.4 (gnubg
scale)", "0.0248/move". A screenshot of any part of the screen must be
self-describing. (Position paper: "an unlabeled number is an invitation to
a wrong comparison.")

**L2 — Layers, not modes.** There is no display-format toggle. Rejected in
the advocatus review: modal state strips the convention from numbers at the
screenshot boundary — the same hidden-normalization sin our own paper
criticizes. Instead: co-presentation (number + rating word on one line) and
a details expander for secondary conventions. One report, layered rendering,
zero UI state to misread.

**L3 — The hero is the error list; the rating is context.** Over a single
match the error list is solid and instructive; the rating is mostly noise.
The screen leads with the receipts (worst moves, ranked; each row a future
doorway into Review, guardrail G5). The rating renders as ONE context line
whose noise caveat is part of the line itself, not a footnote elsewhere:
e.g. "one match — this number settles over ~20". (Position paper: "it has
not measured a change in you; it has measured variance.")

**L4 — The phrase layer is a separate, gated artifact.** Plain-language
sentences over the stats are NOT "just a render flavor" (advocatus
retraction): each phrase = a measured firing condition + a wording, and the
wording is an interpretive claim G4 does not validate. They require their own
playbook (DeltaNarrator pattern): conditions cited against the manual's
definitions, wordings reviewed, adopted rule by rule. v1 of the screen ships
with ZERO phrases. The playbook is its own future document.

**L5 — The verification affordance.** The details expander shows gnubg's
native per-move numbers — the exact figures desktop GNU Backgammon prints
for the same match file. Beside them, one line: how to check ("save this
match; analyse the same file in desktop GNU Backgammon; these numbers
match"). This is the position paper's posture made into UI: we do not ask
to be believed, we ask to be checked. The CBG-folder save is what makes the
check possible for every user.

**L6 — Never the bare token "PR".** Our x500 figure sits on gnubg's
denominator (anUnforcedMoves + anCloseCube, per player — getMWCFromError,
read at source); XG's PR uses a different, unpublished filter. Printing
"PR" invites a cross-check against XG that fails by definition and costs
us a public correctness fight over a non-bug. The x500 figure is labeled
"(gnubg scale)". A Snowie-equivalent rate exists natively in gnubg
(formatgs.c:411-421) and may ship as a second labeled convention AFTER a
G1 citation pass on its exact expression (the divisor n's definition
included). Until cited, it does not ship.

## Screen composition (v1 — phrase-free, the G4 vehicle)

Entry: an orange "Your Personal Stats" button (PlusUi convention) on the
match-over surface — the one moment the record still exists and the player
cares. No hub entry in v1 (a hub entry only becomes honest at stage 5, when
a ledger exists to show on days without a fresh match).

Order of content:
1. Header: match length, final score, games played.
2. THE ERROR LIST (hero): worst chequer decisions, worst first (verb order),
   each row: game/move locator, the move's cost (labeled, per L1), gnubg's
   skill word. Chequer-only until G6 lifts. Rows are static in v1; the
   Review jump (G5) arrives after the jump semantics are derived from
   Review's own stepping.
3. Rating context line (both players): "ER <x500> (gnubg scale) — <gnubg
   rating word>" + the inline noise caveat. gnubg's own aszRating word,
   licensed clause (a).
4. Luck, framed usefully: the luck-ADJUSTED result beside the actual result
   ("score 3–5; luck-adjusted you stand +0.8") — the honest number that
   defuses the "won big, rated badly" confusion. The raw luck rate lives in
   the details expander; the paranoia-inducing luck *rating* word is not
   surfaced (recon: "nothing to gain here except dice paranoia").
5. Details expander: native per-move rates (the G4/verification view),
   unforced/close-cube counts, skill histogram, cube error totals, the
   how-to-verify line (L5).

Cube numbers: displayed totals are gnubg's own accumulation and are covered
by G4's comparison; the error LIST stays chequer-only until G3+G4 have
passed and G6 lifts.

## Lifecycle (advocatus quirk 5, pinned)

- Trigger: the screen's open calls analyseCompletedMatch() IF no cached
  report exists (consumer-triggered; the free edition has no screen and
  therefore never analyses — the fence).
- Busy: the existing BusyKind.ANALYSING state renders a plain progress line
  for the seconds of analysis on first open.
- Cache: the report lives in the VM StateFlow; re-opening does not re-run.
- Reset: starting a new match clears the cached report.
- Ephemerality, stated honestly in-UI when relevant: the report covers the
  match just finished; history across matches arrives with the ledger
  (stage 5). Saving the match (CBG folder) preserves the underlying record
  regardless.

## The G4 gate (unchanged, now concrete)

Nothing beyond the plain v1 render ships until this passes:
1. Play a real match to completion on the Plus build; open Your Personal
   Stats (this triggers the analysis and logs `PR: games=.. valid=true`).
2. Save the same match to the CBG folder.
3. Analyse the same .sgf in desktop GNU Backgammon (same 2-ply analysis
   settings) and open its match statistics.
4. Compare field-for-field against the details expander / logcat: per-move
   error rates (the native, unscaled numbers), unforced move counts, close
   cube counts, chequer/cube error totals, luck totals, luck-adjusted
   results, skill histogram. Same engine, same formulas: they must match.
   G3's internal sum-check must simultaneously report valid=true.
Divergence anywhere = stop, read the field, fix, re-run. Only a clean pass
unlocks the phrase playbook (L4) and any styling beyond plain.

## Deferred, tracked

- Phrase playbook (own document, own adoption cycle) — after G4.
- Snowie-equivalent convention — after its G1 citation pass.
- Review jump from error rows — guardrail G5, from Review's own stepping.
- Cube rows in the error list — G6 lift after G3+G4.
- Ledger + hub entry + PR-over-time — stage 5, on the CBG folder.
- XG archive import investigation (the refugee's belongings) — stage-5
  adjacent; empirical question of what a stranded XG-Android user can still
  extract.
