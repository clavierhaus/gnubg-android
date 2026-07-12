# Train with the Coach -- the fourth mode

Authoritative documentation for the Coach mode: what it is, what exists, what
was decided, and where every governing document lives. For live project state
see `docs/STATUS.md`; this file owns the mode's identity and its document map.

## What it is

**Train with the Coach** is the fourth hub entry, beside Play Tournament Match,
Analyse Position, and Review Match. It is playing gnubg *with the engine looking
over your shoulder*: after each of your moves, gnubg's own verdict -- silent when
your move was fine, and when it was not: what you played, what gnubg preferred,
what it cost, and *why*, shown first and foremost **on the board itself**.

The product promise (from the vision): *"I see what I missed."*
The governing constraint (from `CLAUDE.md`): gnubg is the sole authority for all
game logic and analysis; the app renders and sequences gnubg's output and never
computes a fact about the position itself.

## The document map

| Document | Role |
|---|---|
| `docs/TUTOR_VISION.md` | North star. Thesis, criticism analysis (C1-C6), design pillars P1-P5, the lexicon phases, engine surface. Amended 2026-07-11 with the two emphases below. |
| `docs/COACH_MODE_PLAN.md` | The commit plan: milestones M0-M5, verified engine dependencies (file:line), risks, locked decisions. |
| `docs/THREADING.md` | The threading boundary the vision now restates. |
| `docs/PHASE3_TUTOR_ANALYSIS.md` | The tutor facade internals the verdict verbs build on. |
| `CLAUDE.md` | The rules every line of this mode obeys. |

## The two amendments (2026-07-11)

**The visual WHY, in this board's own language** (vision P1, now its most
emphasized point). The answer to "why was my move worse?" is a visual
representation of moves fully adapted from the app's own board layout -- the
single BoardGeom geometry where one rectangle is both drawn and tapped. Played
vs best rendered as per-leg traced motion on the live board; a translucent ghost
of gnubg's destination checkers; the before/after toggle animated as movement,
not a cut. Every verbal layer accompanies this picture and never substitutes for
it. All of it renders gnubg-returned data (anMove pairs, the best-move board);
nothing is classified app-side.

**Threading is compartmentalized** (vision Section 6). Never for in-game move
calculation -- a live move is one serial, pruned, NEON-vectorised search gnubg
does not decompose. Threading is appropriate only where gnubg already decomposes
work (whole-match analysis, rollouts, behind progress bars), enabled
deliberately per `docs/THREADING.md`. The Coach feels fast by being asynchronous
and non-blocking, not by being parallel.

## Decisions (locked 2026-07-11, from the design discussion)

- The mode is named **"Train with the Coach"** -- the hub's verb+object grammar.
- **The one-point Chequer-Play Tutor stays permanently.** It is a movement-only
  companion -- no cube strategy -- and its narrowness is a feature. Tutor = quick
  movement practice; Coach = the full teaching loop. They sharpen each other.
- **V1 (0.12.0) is a compartmentalized, contained mode**: its own AppMode and
  screen, pane budgets measured before layout, zero changes to the Play path --
  the Analyse/Review pattern. Underneath it shares GameViewModel and the engine
  verbs: one game loop, two front ends. It ships to users; field reports drive
  this project.
- V1 plays a **single game vs a fixed Expert opponent** (0-ply clean, instant
  replies keep the coaching rhythm tight), with **no tournament ceremony**: no
  match length, no Crawford, **no cube** until the cube coach (M4).
- Coach analysis runs at the **fixed 2-ply analysis context**
  (`esAnalysisChequer`), the same context as the Review verdict.
- 0.12.0 scope: **M0 + M1**.

## What exists today (M0 -- done, build-verified on device toolchain)

`gnubg_mobile_coach_verdict(out[166])` -- gnubg's verdict on the last human
chequer move of the live game, carrying everything every disclosure level reads
from **one** evaluation (the vision's C6 fix): rank among all legal plays, both
equities, gnubg's severity (`Skill`), both moves (anMove pairs), the pre-move
board in `formatMove`'s frame, the per-move win/gammon/backgammon probability
vectors (`arEvalMove[NUM_ROLLOUT_OUTPUTS]`, eval.h:272) for played and best, and
the top-5 candidate rows. Exposed as `Engine.coachVerdict(): IntArray` with the
layout documented at the declaration. The verb marshals; it interprets nothing.

Verified foundation it stands on (all read against source, cited in the plan):
per-candidate `arEvalMove` exists (eval.h:272); `CalculateHalfInputs` was
already exported for the facade (eval.c:61/613) and `ClassifyPosition` is extern
(eval.h:413) -- the M2 features verb is a marshal, not an engine change; and the
vision's "2-ply returns inf" blocker is cleared -- the Review verdict runs
`esAnalysisChequer` at 2-ply today, since the per-player move-filter fix.

## What no other app has (field-checked 2026-07-11)

Backgammon Coach (iOS, gnubg-based), Backgammon Guru Pro, XG Mobile's tutor and
Backgammon Galaxy's AI Hint all surface *numbers on request*: probability
tables, color-coded error lists, hints. None explains why in grounded language,
none renders the reason on the board, none offers retry, none aggregates leaks.
The one explain-why attempt (yairwein/backgammon-teacher) requires a server-side
LLM and app-side feature counting -- the two mechanisms this project forbids.
Grounded, offline, board-first coaching -- every sentence licensed by a
gnubg-computed value -- exists in no shipping app. That is the mode's reason to
exist.

## Next

M1 scaffold: `AppMode.COACH`, the "Train with the Coach" hub entry, the
contained single-game CoachScreen; then the glance line over `coachVerdict`,
the visual WHY, and the try-again sandbox. See the plan for the full sequence.


Companion / natural-language layer: docs/COMPANION.md (analysis of
yairwein/backgammon-teacher and the local-LLM plan, phases A/B/C).
