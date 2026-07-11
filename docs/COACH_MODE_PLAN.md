# Coach Mode -- the implementation plan

Status: commit plan for realizing `docs/TUTOR_VISION.md`. This document is
sequenced and buildable; the vision stays the north star. Every engine claim
below was verified against the source on 2026-07-11 (file:line cited), per the
anti-fabrication rule.

## What it is

**Coach** is the fourth mode: Play, Analyse, Review -- and Coach, which is
playing gnubg *with the engine looking over your shoulder*. The hub today has
exactly three entries (verified: HomeHubScreen.kt:122-128); the chequer-play
tutor exists only as a toggle inside match setup, which is why nothing on the
front door says "gnubg will teach you." Coach is therefore a NEW fourth hub
entry, in the hub's own verb+object grammar -- working title **"Train with the
Coach"** (maintainer may rename) -- not an evolution of a tile that does not
exist.

The differentiator, confirmed against the 2026 field (Backgammon Coach iOS,
Guru Pro, XG Mobile, Galaxy AI Hint): every competitor shows *numbers on
request*. None explains why in grounded language, none draws the reason on the
board, none offers retry, none aggregates leaks. backgammon-teacher attempts
explanation but needs a server LLM and app-side feature counting -- both
rejected here. Coach's every sentence is licensed by a gnubg-computed value,
offline, on the board.

## Verified foundation (better than the vision knew)

- `move.arEvalMove[NUM_ROLLOUT_OUTPUTS]` -- per-candidate win/gammon/bg
  probabilities exist on every move-list entry (eval.h:272).
- `CalculateHalfInputs` is already non-static, with a comment saying it was
  exported *for the tutor facade* (eval.c:61, definition at eval.c:613).
  `ClassifyPosition` is extern (eval.h:413). The named inputs (`I_PIPLOSS`
  eval.c:1120, `I_BACKESCAPES` :1126, `I_MOBILITY` :1155, `I_TIMING` :816) are
  computed by gnubg itself.
- The vision's blocking item -- "2-ply tutor returns inf" -- is **cleared**: the
  review verdict runs `esAnalysisChequer` at 2-ply today
  (gnubg_mobile.c:807-812), working since the per-player move-filter fix.
- The verdict payload pattern (rank, equities, both anMoves, pre-move board,
  Skill) already exists in `gnubg_mobile_review_verdict`; Coach V1 largely
  re-targets it at the live game's last human move -- which is exactly what
  `analyze_replay(plTarget=NULL)` already selects.

## Milestones

### M0 -- widen the verdict payload (engine, small)

One verb, `gnubg_mobile_coach_verdict(out[])`, same construction as
review_verdict but over the live record tail (`analyze_replay(NULL)`), returning
in addition: `arEvalMove[7]` for the played and best moves, and the top-K
candidate rows (anMove + equity + arEvalMove each). Names its sources:
`FindnSaveBestMoves`, `Skill`, move-list `arEvalMove`. No new analysis -- one
evaluation, cached Kotlin-side, read by every disclosure level (fixes C6).

### M1 -- Coach V1, the vision's Section 8 loop  (release 0.12.0)

**Shape: a compartmentalized contained mode, not an overlay on Play.** Its own
AppMode + screen with a pane layout designed against measured budgets from day
one; ZERO changes to the Play path (the pattern that worked for Analyse and
Review). Underneath it shares GameViewModel and the engine verbs -- one game
loop, two front ends -- so no second game logic can drift. It ships to users
(field reports drive this project), so it is a contained first version, not a
lab build.

- New hub entry (fourth): a single game against a fixed sensible opponent
  (Expert, 0-ply clean -- fast replies keep the coaching loop tight), with the
  coach loop. Deliberately WITHOUT tournament ceremony: no match length
  selection, no Crawford, no cube pressure in V1 -- a game played to be
  coached, not to win. The cube arrives with the cube coach (M4).
- **Non-modal glance line** in the side panel after each committed human move:
  silent when gnubg does not flag it; else severity + equity loss + best-move
  notation (`FormatMove`). Asynchronous on the engine thread after the engine's
  own reply (single-threaded engine: coach eval queues behind the engine move --
  acceptable, the player is never blocked; see THREADING.md).
- **The visual WHY** (the vision's P1 emphasis): played-vs-best rendered as
  per-leg traced motion ON the existing BoardGeom geometry -- same points,
  palette, proportions -- from the anMove src/dst pairs the verdict already
  returns; a translucent ghost of gnubg's destination checkers as the second
  step. Pure rendering, the app's legitimate work. V1 ships the traced paths;
  the animated before/after toggle may land in M2 if geometry work demands it.
- **Try again**: NOT a game rewind. On a flagged error, "Try again" opens the
  pre-move position (returned by the verdict, mover frame) as a sandbox with the
  same dice -- the Analyse install path (`SetGNUbgID` via idsFromState) reused --
  play the move, get the same verdict treatment, then return to the live match,
  which has continued. gnubg's record is never falsified; desktop's "Rethink"
  fired pre-commit, ours is post-commit, so a sandbox is the honest equivalent.

### M2 -- the reason line  (0.13.0)

- Verb `gnubg_mobile_position_features(board[50], out)` wrapping
  `ClassifyPosition` + `CalculateHalfInputs` -- marshals gnubg's own feature
  array, interprets nothing (NEW-FILE TRIPWIRE applies).
- Kotlin renders the **dominant gnubg delta** between played and best as one
  phrase: win-probability loss vs gammon-probability loss (from `arEvalMove`),
  or safety (`I_PIPLOSS` delta). Subtracting two gnubg values is rendering;
  counting anything on `board[]` is forbidden.
- The honest floor everywhere: if no gnubg quantity dominates, say only
  "gnubg prefers <best> by <delta>" with the arrow.

### M3 -- lexicon + progressive disclosure  (0.14.0)

- `assets/coach/lexicon.json`: authored entries (blot, prime, anchor, timing,
  take point, market loser, ...) each with declarative gnubg-trigger conditions
  (which quantity, which threshold, which dominance). The trigger evaluator in
  Kotlin is a formatter over gnubg numbers; checkpoint Q4 must be answerable per
  sentence. Corpus: the standard literature via the Wikipedia article's cited
  references; authored and reviewed, shipped as data. No generation, no network.
- Disclosure depths over the one cached evaluation: Glance -> Reason -> Numbers
  -> Full candidate table (per-move probabilities, touch-first). A single
  setting selects the default depth ("Gentle / Serious / Classic").

### M4 -- cube coach + blunder log  (0.15.0)

- Cube: expose `isCloseCubedecision` / `isMissedDouble`; render the take/pass
  meter from `arDouble[]` and gammon probabilities. All gnubg quantities; the
  cube path is already routed.
- Blunder log (DataStore): per entry only gnubg outputs -- PositionID, both
  notations, equity loss, severity, dominant delta component. Re-open -> the M1
  sandbox. Leak aggregation = counting gnubg's verdict categories, never a new
  verdict.

### M5 -- grounded on-device phrasing (research, gated)

Deferred exactly as the vision gates it. Ships nothing until a local generator
provably cannot assert what gnubg has not.

## Risks, named

1. **Panel space.** The recurring failure class of this app is clipping. The
   coach panel is designed from measured budgets (the 0.858 ceiling method of
   issue #1) before any code, and the Full table is a separate overlay, not a
   pane resident.
2. **Latency stacking.** Coach eval (2-ply, ~1-2s) queues behind the engine's
   move on the single engine thread. At Grandmaster that means the glance line
   lands ~10s after commit. Acceptable for V1 (the player is playing, not
   waiting); the eventual fix is the analysis-side threading THREADING.md
   already scopes -- not a reason to thread now.
3. **Try-again record semantics.** Solved by design above (sandbox, not
   rewind); flagged because it is where an eager implementation would falsify
   gnubg's record.
4. **Lexicon overreach.** The failure mode is a phrase without a licensing
   quantity. The declarative trigger format makes Q4 mechanically checkable; a
   phrase without a named gnubg source cannot be merged.

## Decision points for the maintainer

- Naming: "Train with the Coach" (proposed), or another verb+object phrase.
- Coach analysis strength: fixed 2-ply analysis context (assumed, matches the
  review verdict), or follow a user-selectable level?
- Opponent in V1: fixed Expert (assumed -- fast, clean), or selectable?
- 0.12.0 scope: M0+M1 only (assumed -- one provable loop), or pull M2 in?
- The in-match tutor toggle: keep during the transition (assumed), retire once
  Coach covers it.
