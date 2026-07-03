# The GNU Backgammon Mobile Tutor -- Vision

Status: vision / north-star reference. Not a source of current state
(`docs/STATUS.md`) nor a commit plan (`docs/ROADMAP.md`). This document
harmonizes three inputs into one coherent direction:

1. the product philosophy in `docs/gnubg_mobile_tutor_mission_statement.tex`;
2. the documented criticism of gnubg's existing tutor (sources below); and
3. the extensible engine foundation already in place (the three-tier facade,
   `FindnSaveBestMoves`, the tutor replay path).

It is written to obey `CLAUDE.md` without exception. Where the original mission
statement and the ground rules disagree, this document resolves the conflict
explicitly (see "Reconciling the mission statement with the rules").


## 1. The thesis

GNU Backgammon is a world-class evaluator whose knowledge reaches the player
through a medium that fights comprehension: modal dialogs, cryptic numeric
tables, post-hoc analysis windows, and per-move waits long enough that the
community's own advice is to switch the tutor off and analyze afterward.

The engine is not the problem. The delivery is.

The mobile opportunity is therefore not "gnubg on a phone." It is to let gnubg
**point at the board**. Everything gnubg already computes -- best move, ranked
alternatives, per-move win/gammon/backgammon probabilities, cube equities,
skill severity -- can be rendered as spatial, tactile, board-first coaching
that the desktop never attempted. The phone's small screen, which the desktop
tutor treats as a constraint, becomes the reason the board stays central.

The product promise, unchanged from the mission statement:

> "I see what I missed."

The single governing constraint, from `CLAUDE.md`:

> gnubg is the sole authority for all game logic AND all analysis. The app
> renders and sequences gnubg's output. It never computes a fact about the
> position itself.

This document's job is to show these two are not in tension -- that the entire
coaching vision is reachable using only values gnubg hands us.


## 2. What the criticism tells us to fix

Synthesized from the gnubg manual's own tutor description, the bkgm.com "All
About GNU" review, the rec.games.backgammon threads on tutor speed, and the
bug-gnubg list. Each criticism maps to a design commitment.

| # | Documented criticism | Source | Design commitment |
|---|----------------------|--------|-------------------|
| C1 | Tutor is a modal dialog that fires only on error ("Play anyway / Rethink / Hint / End Tutor Mode") and breaks flow; nothing shown for good moves; analysis vanishes when the dialog closes. | gnubg manual, Tutor mode | Non-modal, always-present panel that lives beside the board. No dialog in the move loop. |
| C2 | Slow: world-class checking can take tens of seconds per move; the loop is "wait for GNU to move, wait for it to analyze, wait again if you rethink." Community advice: turn tutor off, analyze later. | rec.games.backgammon | Analysis is async on the engine thread after commit; the player is never blocked and never re-waits on rethink. |
| C3 | The hint window is a rank/equity/probability table the docs themselves call "complicated and cryptic"; understanding presupposes knowing EMG equity. | gnubg manual, hint window | Progressive disclosure: one plain fact first; the full table only on demand. |
| C4 | Config is expert-only: tutor flag in one menu, thresholds in another, settings lost unless manually saved. | bkgm.com review | Sensible defaults; a single strength choice already wired to gnubg presets; nothing to save-or-lose. |
| C5 | The "why" gap: gnubg names the best move but the player must work out why alone. | bkgm.com review | Show gnubg's own probability and equity deltas visually on the board -- the closest honest thing to "why" that gnubg actually proves. |
| C6 | Pressing Hint from the tutor dialog restarted the evaluation from scratch. | bug-gnubg 2009 | Evaluate once per move; cache the result; every disclosure level reads the same cached evaluation. |

The modern competitive bar (Backgammon Galaxy, XG-based tools) adds
expectations the desktop never met: a persistent blunder log, longitudinal
stats, on-demand cube hints, and retry-the-position practice. All of these are
achievable from gnubg data (Section 5).


## 3. Reconciling the mission statement with the rules

The mission statement predates the invention purge. Read literally, several of
its examples describe the app *computing* board facts:

- "the user's move leaves 17 hitting numbers while the best move leaves 6";
- "GNUbg's move makes the 5-point while the user's move does not";
- "feature-delta grounding" with an app-side vocabulary of shot count, blot
  count, point-making, race.

That is exactly the analysis layer we deleted (`FeatureExtractor`,
`FeatureVector`, `PositionType`, `FeatureDelta`, `TutorAnalyzer`). A Kotlin loop
that counts shots or detects a made point is reinvention, even in service of a
good UX, and it is now forbidden by the BRIGHT LINE.

**The resolution is a change of mechanism, not of vision.** Every UX goal in the
mission statement survives; the source of the numbers changes from
"app-computed feature deltas" to "gnubg-computed evaluation deltas."

The key realization is what gnubg already exposes per candidate move. In
`eval.h`, every entry of the move list carries:

```
arEvalMove[NUM_ROLLOUT_OUTPUTS]
```

which is gnubg's own output vector: probability of win, win-gammon,
win-backgammon, lose-gammon, lose-backgammon, plus equity. So the mission's
"grounded facts" can be restated as gnubg facts:

| Mission statement wanted (app-computed) | Restated as a gnubg-supplied value |
|-----------------------------------------|------------------------------------|
| "leaves 17 shots vs 6" (app shot count) | The played move's **loss-of-win-probability vs the best move**, from `arEvalMove` -- gnubg's own measure of how much safety the move gave up. |
| "makes the 5-point" (app point detection) | The best move's **notation** from `FormatMove` (`.../5 .../5`), shown beside the played move's notation. The app displays the difference; it does not classify it. |
| "gives up race equity" (app race calc) | The **cubeless/cubeful equity delta** gnubg already returns per move. |
| "gammon danger changes the cube" (app) | gnubg's `arDouble[]` and gammon-probability outputs from the cube path. |
| "leaves an extra blot" (app blot count) | Expressed via gnubg's own exposure feature, not an app count -- see "gnubg computes features natively" below. If no gnubg quantity supports it, we do not assert it: we show the equity/probability delta and the best-move arrow. |

### gnubg computes position features natively -- expose them, never recount

The deleted `FeatureExtractor` counted shots, blots, and primes in Kotlin. That
was reinvention. But gnubg does **not** leave these uncomputed -- it calculates
them itself as the named inputs to its own neural net, in `CalculateHalfInputs`
(eval.c), and classifies positions in `ClassifyPosition` (eval.h). The correct
mechanism is to expose gnubg's own feature values through a facade verb, never
to recompute them.

gnubg's named inputs are the authoritative, engine-side version of exactly the
"features" the mission statement wants:

| Mission "feature" | gnubg's own computation (authoritative) |
|-------------------|------------------------------------------|
| shot count / safety | `I_PIPLOSS` -- expected pip loss from being hit, computed in `CalculateHalfInputs` |
| back-checker containment | `I_BACKESCAPES` (`Escapes()`), `I_ACONTAIN` |
| board mobility / flexibility | `I_MOBILITY` |
| timing | `I_TIMING` |
| prime structure | `I_BACKBONE` |
| entering from the bar | `I_ENTER` |
| race / break-contact | `I_BREAK_CONTACT`, `I_FREEPIP`, `I_BACK_CHEQUER` |
| backgame | `I_BACKG`, `I_BACKG1` |
| pip count | `PipCount` |
| position class (race/contact/crashed/...) | `ClassifyPosition` -> `positionclass` |

So "your move leaves more shots" is expressible as the **delta in gnubg's own
`I_PIPLOSS`** between the played board and the best board -- gnubg computed it,
the facade exposes it, the app renders it. This is a port of a gnubg value, not
a Kotlin analysis. The BRIGHT LINE test still applies exactly: a Kotlin loop
over `board[]` computing pip loss is forbidden; a facade verb returning gnubg's
`afInput[I_PIPLOSS]` is a port and is allowed. The distinction is not "may we
show shot danger" but "who computed it" -- and the answer must always be gnubg.

A facade verb exposing these features must, per the NEW-FILE TRIPWIRE, name the
gnubg routine it wraps (`CalculateHalfInputs` / `baseInputs` / `ClassifyPosition`)
and hold no board interpretation of its own. It marshals gnubg's array across
the boundary; nothing more.

Where gnubg supplies no value, the mission statement's own fallback rule
applies and is now mandatory, not optional:

> If the app cannot identify a reliable reason, it should say less:
> "GNUbg prefers another move by 0.062. Show best move?"

This is the honest floor. Silence with an arrow beats an invented reason.

### Natural language: grounded, and only from gnubg deltas

The mission statement's "small vocabulary of reliable concepts" (Safety, Shot
count, Point-making, Exposure, Race, Cube take/pass, Gammon danger) is
retained, but each phrase must be triggered by a gnubg-supplied quantity, not
an app computation:

- "Safety" template fires when gnubg's **win-probability delta** dominates the
  equity loss.
- "Gammon danger" fires when the **gammon-probability delta** dominates.
- "Cube: take/pass" fires from the **cube decision** gnubg returns.
- Point-making is expressed only as the two **notations** side by side, never as
  an app-detected structural claim.

If none of the gnubg quantities crosses a template's trigger, no phrase is
emitted. The phrase library is a formatting layer over gnubg numbers, and the
checkpoint question (Q4) must be answerable for every phrase: *which gnubg value
triggered this sentence?* "The app decided" is a failing answer.


## 3A. The language layer -- a backgammon vocabulary that speaks

The mission statement's dream feature is human-sounding explanation ("Make your
5-point. It strengthens your board..."), while warning that a prose layer is
"the danger zone" because an interpretation that is wrong "teaches false lessons
with confidence." The resolution is a **backgammon-specific language database**
whose every utterance is bound to a gnubg-supplied quantity. Two phases.

### Inspiration and what we deliberately reject

`github.com/yairwein/backgammon-teacher` is a direct inspiration for the goal:
play gnubg, detect blunders, explain them in natural language. But its
architecture is the opposite of ours on two axes, and we reject both:

1. **It computes features in an app-side `features/` module** -- the same
   Kotlin-style shot/blot/prime counting we deleted. We instead expose gnubg's
   own `CalculateHalfInputs` / `ClassifyPosition` values (Section 3).
2. **It generates prose with a server-side LLM API call** (Claude / GPT-4). That
   is a client-server round-trip: unusable for an offline mobile port, and it
   places an ungrounded generator in the explanation path. We reject the server
   dependency entirely and, for the shippable product, the live generative model
   too.

backgammon-teacher shows the destination. Its mechanism is off-limits here.

### Phase 1 -- a curated lexicon (offline, static, authored, shippable)

A vetted, on-device database of backgammon language: for each concept a player
meets -- blot, prime, anchor, builder, blitz, back game, holding game, timing,
containment, take point, market loser, gammon price -- an authored definition
and a set of short explanatory templates, in plain vetted English.

Foundation corpus: the Wikipedia backgammon article and the references it cites
(Robertie, Magriel's *Backgammon*, Woolsey, the standard literature) provide the
canonical, well-reviewed vocabulary and definitions. This is a **content**
resource, not a model: authored phrases reviewed for correctness, stored as
data, shipped in the APK. No network, no generation, no inference.

The binding rule is what keeps it inside the ground rules: **a lexicon entry may
only be surfaced when a gnubg value licenses it.** The database supplies the
*words*; gnubg supplies the *trigger and the magnitude*. Examples:

- gnubg's `I_PIPLOSS` delta is large and dominates the equity loss -> surface the
  "safety / shots" template, worded from the lexicon, quantified by gnubg.
- `ClassifyPosition` returns a race class and the equity delta is a race loss ->
  surface the "race" template.
- The cube path returns double/pass with a close take -> surface the "market
  loser / take point" template with gnubg's numbers.

The lexicon never decides *that* a concept applies -- gnubg's value does. The
lexicon only decides *how to say it*. This is the mission statement's
"vocabulary of reliable concepts" realized as authored data over gnubg triggers,
and it passes checkpoint Q4 for every sentence: the triggering gnubg routine is
always nameable.

### Phase 2 -- a backgammon-specific generative model (long-term research)

The long-term research direction is an on-device model that generates
explanation from the same corpus -- a small, domain-specific language model
assembled from the Wikipedia article, its references, and the annotated-game
literature, running locally (no server, unlike backgammon-teacher).

This is explicitly **research, not the near-term product**, and it inherits a
hard constraint from the mission statement's own warning: a generative model
that invents reasons is the "false lessons with confidence" failure mode. So
even in Phase 2 the generation must remain **grounded**: the model may only
phrase facts that gnubg values assert (the `I_*` feature deltas, the evaluation
outputs, the cube decision, the skill severity). The model is a better
*phrasing* engine over gnubg-supplied facts -- never an independent source of
backgammon judgement. If it cannot ground a sentence in a gnubg quantity, it
must not say it.

The phased structure matters: Phase 1 ships and is safe by construction (static
authored data). Phase 2 is gated behind proving that a local generator can stay
grounded -- and it reuses Phase 1's gnubg-trigger discipline as its guardrail.


## 4. Design pillars (the mobile expression)

### P1 -- The board is the primary surface

The first explanation is visual and on the board, never a table. The tutor layer
draws *around and on top of* the board:

- an **arrow** from the played checker's source/destination to gnubg's
  preferred move (data: the best-move board `out[2..51]` our facade already
  returns; rendering is pure pixel work -- Kotlin's legitimate role);
- a **before/after toggle** flipping between the played position and gnubg's
  best (both boards are gnubg output);
- **move notation** for played vs best (`FormatMove`).

Highlights that assert a *classification* of a point ("this is a blot," "this
makes a prime") are out of scope unless gnubg hands us that classification.
Arrows and position toggles assert nothing -- they show gnubg's board -- so they
are always safe.

### P2 -- Progressive disclosure, one evaluation

Four depths over a single cached gnubg evaluation (fixing C3 and C6):

1. **Glance:** skill severity (only if gnubg flags it) + equity loss + best-move
   notation. Silent when gnubg does not flag the move.
2. **Reason:** the dominant gnubg delta rendered as one phrase + the best-move
   arrow.
3. **Numbers:** played equity, best equity, loss -- gnubg's figures.
4. **Full:** the ranked candidate list with per-move win/gammon/backgammon
   probabilities (the desktop hint window, touch-first).

The setting chooses which depth opens by default and how often the app
interrupts -- not different data and not different code paths. "Gentle Coach,"
"Serious Coach," and "Classic" are disclosure levels of one renderer over one
evaluation.

### P3 -- Non-modal, kind, asynchronous

No dialog in the move loop (C1). Analysis runs on the engine thread after commit
and updates the panel when ready (C2); the player keeps moving. Tone is precise,
not punitive: "Mistake: -0.084. GNUbg prefers 13/8 6/5." -- never "Blunder. Bad
move."

### P4 -- The Try-Again loop

On a significant error, offer Show best move / Try again / More detail. Try
again restores the pre-move position (replay via the `FixMatchState` +
`ApplyMoveRecord` pattern the facade already uses) and lets the player search
again. This converts "here is why you were wrong" into "look again" -- the
stronger learning structure, and it costs no new analysis, only a board reset.

### P5 -- The cube tutor as a distinct strength

Cube decisions are the weakest area of most mobile products and a place gnubg is
already strong. Rendered visually from `FindCubeDecision` / `GetCubeRecommendation`
and `arDouble[]`:

- current winning chances (gnubg output);
- take point and how close the take/pass is (`isCloseCubedecision`);
- gammon danger (gnubg gammon probabilities);
- the classification gnubg returns: no-double / double-take / double-pass /
  too-good.

A visual take/pass meter makes "why a take is barely correct" legible without a
table. All quantities are gnubg's.


## 5. Maximizing engine utilization -- the gnubg surface to expose

The vision is ambitious precisely because gnubg already computes almost
everything. The work is mostly *exposure and rendering*, not new logic. Each row
names the gnubg routine; none requires app-side board interpretation.

| Capability | gnubg routine / data | Status |
|------------|----------------------|--------|
| Score played move, find best | `FindnSaveBestMoves` (via `gnubg_mobile_tutor_analyze`) | Live (1-ply); log-only today |
| Best-move board for arrows | facade `out[2..51]` | Returned; not yet rendered |
| Per-move win/gammon/bg probabilities | `arEvalMove[NUM_ROLLOUT_OUTPUTS]` per move-list entry | In engine; not yet surfaced |
| Move notation (book form) | `FormatMove` / `FormatMovePlain` (drawboard.h) | In engine; not yet surfaced |
| Ranked candidate list | `movelist` from `FindnSaveBestMoves` | In engine; not yet surfaced |
| Skill severity label | `Skill()` / `aszSkillType` (via `Engine.skill`) | Live (`BlunderClassifier`) |
| Position features (shots/containment/timing/...) | `CalculateHalfInputs` named inputs (`I_PIPLOSS`, `I_BACKESCAPES`, `I_MOBILITY`, `I_TIMING`, `I_BACKBONE`, `I_ENTER`, ...) | In engine; not yet surfaced |
| Position class (race / contact / crashed / ...) | `ClassifyPosition` -> `positionclass` | In engine; not yet surfaced |
| Pip count | `PipCount` | In engine; not yet surfaced |
| Cube decision + equities | `FindCubeDecision`, `GetCubeRecommendation`, `arDouble[]` | In engine; cube path already routed (Phase 11) |
| Close-cube / missed-double flags | `isCloseCubedecision`, `isMissedDouble` | In engine; not yet surfaced |
| Position identity (for the log) | `PositionID` / `PositionKey` (positionid.c) | In engine |
| Retry a position | `FixMatchState` + `ApplyMoveRecord` replay | Pattern in use by the tutor facade |
| Per-roll temperature map | gnubg's own temperature-map evaluation (gtktempmap.c) | In engine; long-term |
| Deeper confidence | rollouts | In engine; advanced/deferred (Section 6) |

Two open engine items gate the top of this list:

- **2-ply tutor.** The prune/2-ply path currently returns `inf` in this build;
  the tutor runs at 1-ply (`fac_ec_default`). Raising tutor quality means fixing
  that path, and it must be fixed in the engine, not worked around in Kotlin.
- **Eval-context provenance.** Per the ROADMAP port audit (V1/V4), analysis and
  cube eval contexts must be sourced from gnubg's named instances
  (`GetEvalChequer`/`GetEvalCube`, `ap[].es*`), not a hand-built `fac_ec_default`.
  The tutor should consume whatever the audited path yields.


## 6. Longitudinal coaching -- pattern review from gnubg data

A single tutor event helps one move; a great tutor surfaces recurring leaks. The
mission statement's "pattern review" is buildable entirely from stored gnubg
outputs -- no app analysis, just aggregation of gnubg's own verdicts:

- **Blunder log:** per logged error, store only gnubg values -- `PositionID`,
  played and best notation (`FormatMove`), equity loss, skill severity, and the
  dominant `arEvalMove` delta component (win vs gammon). Re-open any entry to
  retry it.
- **Leak aggregation:** group logged errors by which gnubg quantity dominated
  (win-probability loss vs gammon-probability loss vs cube equity loss). "Your
  most common error this session: giving up winning chances while ahead" is a
  count over gnubg's own delta categories, not an app judgement about the
  position.
- **Cube tendencies:** from the cube path, aggregate missed doubles
  (`isMissedDouble`) and take/pass errors. "You pass too many takeable cubes" is
  a tally of gnubg cube verdicts.

The boundary stays bright: the app **counts and groups gnubg's verdicts**; it
never derives a new verdict of its own.

Rollouts remain advanced and deferred -- clearly labelled deeper analysis,
bounded, cancelable, never in the primary correction loop. The default coach
relies on fast evaluation.


## 7. Foundation fit -- why this is reachable from where we are

The three-tier facade already established (Kotlin UI -> `Engine.kt`/JNI ->
`native-lib.c` marshalling -> platform-neutral C facade -> vendored engine-core)
is exactly the substrate this vision needs. The engine is the authority; the
facade hands flat data across; Kotlin renders. The tutor already replays with
`FixMatchState` + `ApplyMoveRecord` and scores with `FindnSaveBestMoves`. What
remains is largely to widen the facade's return payload (add per-move
`arEvalMove`, `FormatMove` strings, the candidate list, the cube outputs) and to
build the board-first rendering on top -- pixels and sequencing, the app's
legitimate work.

New facade surface is additive and rule-bound: each new value returned must name
the gnubg routine that produced it (checkpoint Q1/Q4), and no new Kotlin or C
file may interpret the board (the NEW-FILE TRIPWIRE). A facade verb that returns
`FormatMove` output is a port; a Kotlin helper that counts shots is not.


## 8. First milestone -- what version one should prove

Not every idea. One experience, exactly as the mission statement frames it:

> "I made a move, gnubg judged it, and the app showed me visually and kindly
> what I missed."

Concretely, V1 of the visible tutor should:

1. detect a significant move error (gnubg skill severity -- live today);
2. surface, non-modally, the glance level: severity (when flagged) + equity loss
   + best-move notation (`FormatMove` -- new facade surface);
3. draw the best-move arrow on the board (from `out[2..51]` -- already returned);
4. offer Try again (board reset via the existing replay path).

That single loop attacks C1, C3, C5 and the mission's core promise at once,
using data the engine already produces, and it establishes the rendering
substrate every later pillar builds on.

Everything beyond -- candidate table, cube tutor, blunder log, pattern review,
temperature map, rollouts -- extends this spine. None of it requires the app to
become a second backgammon brain. All of it requires the app to become a better
window onto the first one.


## Sources

Criticism and competitive context (accessed for this document):

- GNU Backgammon Manual (tutor mode; hint window "complicated and cryptic"):
  https://www.gnu.org/software/gnubg/manual/gnubg.html
- "All About GNU Backgammon," bkgm.com (config friction; the "why" gap):
  https://bkgm.com/articles/GOL/Oct02/allgnu.htm
- rec.games.backgammon, tutor speed thread (tens of seconds per move; advice to
  turn tutor off and analyze later):
  https://rec.games.backgammon.narkive.com/stPF7Els/
- bug-gnubg (Hint restarts evaluation):
  https://lists.gnu.org/archive/html/bug-gnubg/2009-04/msg00073.html
- Backgammon Galaxy (modern learning-feature bar: blunder log, cube hints,
  stats): https://play.google.com/store/apps/details?id=com.backgammongalaxy.app
- yairwein/backgammon-teacher (inspiration for the goal; rejected mechanism --
  app-side feature extraction + server-side LLM):
  https://github.com/yairwein/backgammon-teacher
- Wikipedia "Backgammon" and its cited references (Magriel, Robertie, Woolsey,
  et al.) -- corpus foundation for the Phase 1 lexicon and Phase 2 research
  model: https://en.wikipedia.org/wiki/Backgammon

Internal:

- `docs/gnubg_mobile_tutor_mission_statement.tex` (product philosophy)
- `docs/PHASE3_TUTOR_ANALYSIS.md` (tutor facade internals)
- `docs/ROADMAP.md` (port audit V1/V4; tutor 2-ply item)
- `CLAUDE.md` (the ground rules this vision obeys)
