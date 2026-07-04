# Code Audit -- July 2026

Purpose: a deliberate pass over the three largest / most-suspect areas
(jni-bridge facade, GameViewModel, Board) to find (a) reinventions of gnubg
logic, (b) dead code, and (c) tangled responsibilities ("spaghetti"), ahead of
a de-spaghettification pass and a PROVENANCE refresh. Every finding cites a
file and line so the fix is actionable. This document is descriptive; it changes
no code.

Scope audited: jni-bridge/src/gnubg_mobile.c (990 lines, 48 verbs),
GameViewModel.kt (1044), Board.kt (925). Line numbers are as of commit 286c5f1.


## A. Reinventions (THE ONE RULE violations)

### A1. `gnubg_mobile_get_candidates` -- dead AND a reinvention (DELETE)
File: gnubg_mobile.c:787-855; JNI native-lib.c:339-390; Engine.kt:39-49.

Two problems compound:
1. **Dead code.** `Engine.getCandidates` is declared but never called anywhere
   in Kotlin (no `.getCandidates(` call site exists). The whole verb + JNI +
   external is unused.
2. **Reinvention.** Its body hand-rolls move scoring: for each candidate it
   does `SwapSides` + `EvaluatePosition` + negate to recover the mover's
   equity, at `GetEvalChequer()->ec` (0-ply play strength). But gnubg's
   `FindnSaveBestMoves`/`ScoreMoves` already scores and ranks every legal move
   (with `InvertEvaluationR` doing exactly this perspective flip), and the new
   `analyze_replay` helper already exposes that ranked movelist. So this verb
   re-implements ranking gnubg already did, at an inconsistent (weaker) ply.

Action: **delete** the verb, its JNI binding, and the Kotlin external. When
Tier-2 (the ranked candidate list) is built, source it from `analyze_replay`'s
already-ranked `ml.amMoves[]` -- same 2-ply analysis context as the tutor/detail
path -- not a second scoring loop.

### A2. `GameViewModel.unplayableDiceFor` -- die-playability re-derivation (REVIEW)
File: GameViewModel.kt:81-123.

Computes which remaining dice are unplayable (to grey them in the UI) by
replaying each movelist entry's sub-moves on a working board and resolving
bear-off dice against intermediate board states. This is intricate Kotlin logic
that reasons about die/move playability -- arguably re-deriving information the
movelist already encodes. It is UI (greying), not gameplay, so it does not
decide legality; but the replay-and-resolve is exactly the kind of "reasoning
about moves in Kotlin" the project avoids.

Action: **review** whether gnubg can report per-die playability directly (or
whether the fPartial movelist already distinguishes which die each sub-move
consumes) so this can shrink to a lookup rather than a replay. Lower priority
than A1 -- it is display-only and currently correct -- but flag for a faithful
rewrite.

### A3. Board.kt borne-off count via `15 - sum(board)` (MINOR)
File: Board.kt:442-445.

`humanBorneOff = 15 - (25..49).sumOf { board[it] }` derives the borne-off count
by arithmetic in Kotlin. gnubg tracks borne-off directly. This is trivial
arithmetic for a draw count (how many tray checkers to render), squarely in the
drawing exception, but a purist would source the count from gnubg. Very low
priority; note only.


## B. Dead code / post-seam cruft

### B1. `getCandidates` (see A1) -- dead.

### B2. `CubeDecision.kt` helpers -- possibly orphaned after Seam 1 removal
File: tutor/CubeDecision.kt (107 lines); imports were removed from
GameViewModel when `offerDouble` was rewritten (commit 286c5f1 lineage).

`cubeDecisionAction`, `CubeAction`, `BEAVER_DECISIONS` lost their GameViewModel
callers when the engine cube-response became gnubg's own decision. Confirm
whether anything still uses `CubeDecision.kt`; if not, it is dead and should be
removed (it encodes a Kotlin mapping of cube decisions -- a latent reinvention
if it lingers unused).

Action: grep for remaining users; delete if orphaned.

### B3. Verify no probe residue.
The 2-ply and drain probes (`gnubg-probe` tag) were reverted; confirm no
`probe`-tagged code or `System.nanoTime` perf probes remain in the tree.


## C. Tangled responsibilities (spaghetti)

### C1. GameViewModel is a god-object (1044 lines, ~40 functions)
File: GameViewModel.kt.

At least five distinct responsibilities live in one class:
1. **Board interaction** -- tapSource, dragMove, tryDestinationStackMove,
   swapDice, undo, confirm, passTurn (~350 lines, the intricate core).
2. **Turn/game flow** -- rollDice, startNewGame, offerDouble, acceptDouble,
   dropDouble, refreshFromEngineAfterControl.
3. **Command passthrough** -- commandNewGame/NewMatch/NewSession/EndGame/
   Resign/Next/Accept/Reject/Decline/Agree/Redouble (~11 one-line wrappers to
   Engine).
4. **Settings** -- setMatchLength/Crawford/Jacoby/AutomaticDoubles/Beavers/
   BoardTheme/ShowPointNumbers/ShowPipCount/Difficulty/TutorMode/Hint/
   ShowEquity/ShowMWC (~15 setters mutating `_settings`).
5. **Persistence** -- loadGame/saveGame/loadMatch/saveMatch/loadPosition/
   savePosition.

Groups 3, 4, 5 are mechanical and mixed in with the delicate interaction logic
of groups 1-2, which makes the file hard to read and the important code hard to
find.

Action (low-risk extractions, no behavior change):
- Extract group 4 (settings setters) into a `SettingsController` /
  delegate, or at minimum a `// region Settings` grouping.
- Extract group 3 (command passthroughs) similarly.
- Group 5 (persistence) into a `GamePersistence` helper.
- Leave groups 1-2 (interaction/flow) as the ViewModel core.
This is pure reorganization; each extraction is independently testable.

### C2. Board.kt mixes geometry, hit-testing, and rendering (925 lines)
File: Board.kt.

One composable holds: coordinate math (pointX, boardPointAt), legal-landing
derivation (landingPointsForSource, barEntryPoints), the giant gesture handler
(onTap/onLongPress/drag, ~250 lines inside BackgammonBoard), and all DrawScope
draw helpers (drawCube/Die/Checker/TrayChecker/Triangle). The gesture handler
in particular is a long inline block inside the composable.

Action: the draw helpers are already factored (good). The extraction target is
the **gesture/hit-testing block** -- pull tap/drag resolution into a small
pure helper (input coords + state -> action) so the composable body shrinks to
layout + draw. Lower priority than C1; Board.kt's length is mostly legitimate
rendering.


## D. Things that are RIGHT (audit reassurance)

- `gnubg_legal_sub_move` (eval.c:2625) is a clean one-line `LegalMove` wrapper --
  not a reinvention. (Should be recorded in PROVENANCE as a seam; see PROVENANCE
  tasks.)
- The facade's 48 verbs are overwhelmingly thin command/state wrappers.
- `analyze_replay` (gnubg_mobile.c:627) correctly centralizes the analysis
  replay for both tutor_analyze and analyze_played_move -- good de-duplication.
- Board.kt draw loops (442-546) compute pixel positions, not board facts -- the
  legitimate drawing exception, not reinvention.
- Landing points now derive from gnubg complete moves (fPartial=0) -- faithful.


## Priority order for the follow-up pass

1. **A1 / B1** -- delete dead+reinvented `get_candidates` (clean win, removes a
   reinvention and dead code in one stroke).
2. **B2** -- delete `CubeDecision.kt` if orphaned.
3. **PROVENANCE refresh** -- seams now diverge from what PROVENANCE describes
   (Seam 1 removed, 2/3 reworded; add `gnubg_legal_sub_move`, `analyze_replay`,
   `analyze_played_move`, the eval.c un-static divergences).
4. **C1** -- GameViewModel extractions (settings, commands, persistence).
5. **A2** -- review `unplayableDiceFor` for a faithful shrink.
6. **C2** -- Board.kt gesture-handler extraction.
7. **A3, B3** -- minor / verification.
