# Design: the Analyse destination (position entry, match save, match review)

Status: proposal. Written before implementation, to fix the architecture before
any code. Supersedes nothing; extends `ARCHITECTURE.md`.

## Why this document exists

The first public preview drew a clear, consistent piece of feedback: three
features are missing, and they are the features people actually open a
backgammon app for.

1. **Set up an arbitrary position and have gnubg evaluate it.** Reported as the
   reason 99% of people still use XG Mobile. True Backgammon's developer has
   stated he will not implement it. BGNJ does not offer it either.
2. **Save the match afterwards**, to review on a larger screen or to catalogue.
   BGNJ and True Backgammon both offer this.
3. **Step through a match afterwards (or during play) inside the app.** BGNJ
   offers this; True Backgammon does not.

The engine work for all three is small. **The design problem is the UI and the
information architecture**, and that is what this document settles.

## The finding that drives the design

The current Home Hub offers four entries:

    Play Tournament Match | Live Game Analysis | Options | Profile

Two problems:

- **"Live Game Analysis" is not a destination.** It is `GameLayout` with
  `tutorMode = true` -- a *mode of playing*, occupying a top-level menu slot as
  though it were a place.
- **`AppMode` already contains `LEARN` and `ANALYSE`**, both wired to real
  screens (`LearnScreen`, `AnalyseScreen`), and **neither is reachable from the
  hub**. `AnalyseScreen`'s first scaffold section reads, verbatim:

      "review position -- Future entry point for examining a board position
       with GNUbg analysis."

The destination for feature 1 was designed and then left unreachable. The
architecture already anticipated this work.

## What the hub currently says the app is

A menu is a positioning statement. The present one says: *this is an app you
play games against.* The three requested features are not about playing; they
are about **working with positions and matches**. They do not belong under Play,
and Settings would be worse. They need a home.

Giving them one changes what the app declares itself to be: not just an
opponent, but a **tool** -- play, and also study positions and matches. That
second identity is precisely the one XG Mobile vacated.

## Proposed structure

    Play      -- a match against gnubg. Tutor becomes a toggle here,
                 where it already technically lives.
    Analyse   -- the new destination.
                   Position : enter a position, gnubg evaluates it.   [1]
                   Match    : open a saved match, step through it,
                              gnubg's verdict on each move.           [3]
    Options
    Profile

**Saving a match [2] is not a menu point.** It is an *action* -- offered at the
end of a game and from within match review -- that produces the artifacts
`Analyse > Match` consumes.

This is what makes the three features one coherent story rather than three
bolted-on additions:

> Play a match -> save it -> open it in Analyse -> step through gnubg's verdict
> on every move. And, independently: paste any position -> get gnubg's verdict.

### Cost of the change

Promoting `ANALYSE` to the hub and demoting "Live Game Analysis" to a toggle is
a visible UX change, not a free one. It is still correct: the slot is worth more
as a destination than as a duplicate of Play. `LEARN` stays unreachable for now;
it is a separate question.

## Engine backing (verified, not assumed)

Every claim below was checked against the vendored, compiled sources.

### [1] Position entry -- fully supported

| Need | gnubg routine | Vendored? |
|---|---|---|
| Board from a Position ID | `PositionFromID(TanBoard, const char*)` (`positionid.c`) | yes, compiled |
| Position ID from a board | `PositionID(const TanBoard)` (`positionid.c`) | yes, compiled |
| Match state (score, cube, dice, turn, Crawford) from a Match ID | `MatchFromID(...)` (`matchid.c`) | yes, compiled |
| Match ID from current state | `MatchIDFromMatchState(const matchstate*)` (`matchid.c`) | yes, compiled |

Together, `PositionFromID` + `MatchFromID` reconstruct exactly what a serious
player copies around: the board *and* the surrounding match context. This is the
right primitive, and it is already in the build.

**Do not route position entry through `CommandSetBoard`.** Two blockers, both
verified:

- `SetBoard` (`set.c:601`) refuses unless `ms.gs == GAME_PLAYING`
  ("There must be a game in progress to set the board").
- `SetBoard` calls `ParsePosition`, which is **declared** in `backgammon.h:482`
  but **not defined in any vendored file** (it lives in upstream `backgammon.c`,
  which was not vendored). Wiring it as-is would fail to link.

`PositionFromID` avoids both. It also has the free validation we want nearby:
`CorrectNumberOfChequers` exists for rejecting illegal boards.

### [2] Save match -- mostly supported, one honest gap

| Need | gnubg routine | Vendored? |
|---|---|---|
| Save a match (gnubg native `.sgf`) | `CommandSaveMatch` (`sgf.c:2365`) | yes, compiled |
| Save a single game | `CommandSaveGame`, `SaveGame` | yes, compiled |
| Read `.sgf` | `sgf.c`, `sgf_l.c`, `sgf_y.c` | yes, compiled |

So **`.sgf` in both directions is already there.** The remaining work is Android
plumbing -- a Storage Access Framework save/share intent -- not engine work.

**`.mat` (Jellyfish) is a real gap, not a small one.** Neither `import.c` nor
`export.c` exists in the vendored tree at all. "Detect the format and convert
both ways" therefore means vendoring new translation units and their
dependencies, not writing a small adapter. This must be scoped separately before
it is promised to anyone.

Recommended stance: **ship `.sgf` first** (free, and it opens in gnubg desktop
and Backgammon Studio, which is the actual workflow being asked for). Treat
`.mat` as a follow-on with its own investigation.

### [3] Match review -- data is present, UI is the whole job

The engine holds the entire match in memory while you play:

- `lMatch` -- list of games (`backgammon.h:314`)
- `plGame` -- list of `moverecord`s for the current game (`backgammon.h:306`)
- `moverecord` (`backgammon.h:204`)

Walking and re-evaluating them is exactly the pattern gnubg's own `AnalyzeGame`
uses: `FixMatchState` + `ApplyMoveRecord`. Per the one rule, review must replay
through those routines and read gnubg's own evaluation -- never recompute a
verdict in Kotlin.

The engine is not the risk here. The risk is a cramped phone UI.

## The UI bar

This is where the project either differentiates itself or repeats the incumbent's
mistakes. Prior research on XG Mobile found its interface to be its weakest
point: an "infuriating" toolbar, configuration overlaid on the board, and painful
drag-the-chequers position setup. Meanwhile its *engine* and forgiving chequer
movement were praised.

That is the opening. Position entry and match review done with a **clean,
unobstructive** interface are not parity with XG -- they are the differentiator.

Two constraints follow, and they are binding:

- **Position entry must not be drag-the-chequers-only.** Pasting a Position ID
  (and optionally a Match ID) is instant, precise, and the format players already
  trade. A tap-to-edit board is a good *addition*, never the only path.
- **Match review must not become a cramped mess.** Board + move list + analysis +
  navigation, in landscape, on a phone. If all four cannot coexist legibly,
  something is progressively disclosed. Design before code.

## Sequence

Descending order of (validated demand x cheapness), and each step feeds the next:

1. **Build the `Analyse` destination shell** and reach it from the hub.
2. **[1] Position entry + evaluate.** Cheapest, uses analysis that already
   exists, and fills the one gap a competitor has publicly refused to fill.
3. **[2] Save match as `.sgf`.** Near-free; produces the artifacts step 4 needs.
   `.mat` deferred pending its own scoping.
4. **[3] Match review.** The largest build. By then serialization exists, and
   (per `ROADMAP_ANALYSIS_PARITY.md`) so does the ranked candidate-move list it
   wants to display.

If only one ships, it is [1]: the demand is validated and explicitly unserved.

## Risk analysis for [1], verified against the code

Three risks were identified before design. All three were traced to their source.

### Risk 1: does the evaluation path mutate the global `ms`? -- NO

`analyze_replay` (`gnubg_mobile.c:652`) is the existing analysis chain. It
operates entirely on a **caller-supplied** `matchstate *pmsAnalyse` and never
reads or writes the global `ms`. Its evaluation call is:

    GetMatchStateCubeInfo(&ci, pmsAnalyse);   /* gnubg derives ci from a LOCAL ms */
    FindnSaveBestMoves(pml, d0, d1, pmsAnalyse->anBoard, &key, TRUE,
                       arSkillLevel[SKILL_DOUBTFUL], &ci,
                       &esAnalysisChequer.ec, aamfAnalysis);

Every context object is either a local (`pmsAnalyse`, `ci`) or a gnubg **named
instance** (`esAnalysisChequer`, `aamfAnalysis`, `arSkillLevel`). Nothing is
invented or reparameterised. This is already the Q2-compliant pattern, and it is
the pattern position analysis must copy.

**Consequence:** analysing a pasted position cannot corrupt a game in progress,
provided the new verb likewise confines itself to a local matchstate.

### Risk 2: can position entry reuse `analyze_replay`? -- NO, and it should not

`analyze_replay` depends on the global **game history**, not on global `ms`:

    if (!plGame || plGame->plNext == plGame) return 0;   /* needs a live game */
    ... scan plGame backwards for the last human MOVE_NORMAL ...
    AnalyzeMove(pmr, pmsAnalyse, plGame, psc, ...)       /* replays plGame */

It is, by construction, *"analyse the last played move of the current game."* A
pasted position has no played move and no `plGame`. **Position entry therefore
needs its own facade verb.** This is the correct conclusion; `analyze_replay` is
not broken and must not be refactored to accommodate a different question.

Note also that `CommandSetBoard` is doubly unusable, as recorded above: it
requires `GAME_PLAYING`, and it calls `ParsePosition`, which is declared
(`backgammon.h:482`) but defined in no vendored file. Neither the command path
nor the replay path serves position entry. The ID decoders do.

### Risk 3: the silent money-play trap -- REAL, and the main hazard

`GetMatchStateCubeInfo(&ci, pms)` derives the cube context from the matchstate's
`nMatchTo`, `anScore`, `nCube`, `fCubeOwner`, `fCrawford`, `fJacoby`. If a bare
Position ID is pasted and those fields are left zeroed, gnubg will evaluate
happily -- **as money play, cube centred, score 0-0** -- and return numbers that
look authoritative while answering a different question than the user asked.

This is the single most dangerous failure mode of the feature, because it fails
*silently* and *plausibly*. It is precisely the class of error `CLAUDE.md`
exists to prevent, arriving through data rather than through invented code.

**Mitigation is a UI requirement, not a nicety:** the match context (match
length, score, cube value and owner, Crawford, Jacoby) must be either supplied
(via a Match ID) or *explicitly chosen and shown* by the user. The screen must
never quietly default. "Money play" is a legitimate answer; an unstated
assumption of it is not.

## The clean design for [1]

The `matchstate` struct (`lib/gnubg-types.h:41`) and `MatchFromID`'s out-params
line up field for field -- unsurprisingly, since a Match ID *is* a serialised
matchstate. That correspondence is the whole design.

**Two decoders, one evaluator, no invention:**

1. `PositionFromID(anBoard, posId)` -- gnubg decodes the board.
2. `MatchFromID(anDice, &fTurn, &fResigned, &fDoubled, &fMove, &fCubeOwner,
   &fCrawford, &nMatchTo, anScore, &nCube, &fJacoby, &gs, matchId)` -- gnubg
   decodes the surrounding state.
3. Copy those gnubg-produced values into a **local** `matchstate`. This is
   assembly of gnubg's own outputs into gnubg's own struct -- not invention.
   The facade must not compute, infer, or default any of these fields itself.
4. `GetMatchStateCubeInfo(&ci, &msLocal)` -- gnubg derives the cubeinfo.
   **Never** hand-build a `cubeinfo` via `SetCubeInfo`; per Q2 a private
   cubeinfo in the facade is by default a bug.
5. `FindnSaveBestMoves(..., &ci, &esAnalysisChequer.ec, aamfAnalysis)` -- the
   same named analysis instances the tutor already uses.

Proposed verbs (names indicative):

- `gnubg_mobile_position_from_ids(posId, matchId, out_board[50], out_state[])`
  -- decode and validate; return a clean error for a malformed ID rather than
  evaluating something wrong.
- `gnubg_mobile_analyze_position(...)` -- local matchstate ->
  `GetMatchStateCubeInfo` -> `FindnSaveBestMoves`, returning the ranked list in
  the shape the tutor panel already renders.
- `gnubg_mobile_current_position_ids(out_posId, out_matchId)` -- the reverse,
  via `PositionID()` and `MatchIDFromMatchState()`. Cheap, and immediately
  useful: "copy this position" out of a live game.

**Validation belongs to gnubg too.** `anChequers[]` (`eval.h:157`) exists for
chequer-count checking, as `SetBoard` used it via `CorrectNumberOfChequers`.
`SwapSides` is declared (`eval.h:390`) and available for frame correction.
Open item: `CorrectNumberOfChequers` is **not** declared in any vendored header
-- confirm it links before relying on it, and if it does not, use gnubg's
exposed equivalent (or `anChequers[ms.bgv]` with gnubg's own check) rather than
writing a chequer count in Kotlin or in the facade.

**Frame convention:** reuse the existing `facade_unpack_board` /
`facade_pack_board` (`gnubg_mobile.c:348`) so the pasted position enters the
same player-on-roll frame the rest of the port already speaks.

## The rule still governs

Nothing here relaxes `CLAUDE.md`. Position entry decodes an ID with gnubg's
decoder. Evaluation calls gnubg. Review replays gnubg's own move records through
gnubg's own routines and displays what gnubg returns. Kotlin draws boards, holds
transient UI state, and calls JNI. If a verdict about a position appears on
screen, a named gnubg routine produced it.

The Risk 3 finding sharpens this. The rule is usually stated as *do not compute
what gnubg computes*. Position entry adds a second edge: **do not silently supply
what the user did not state.** An unstated score or cube position is an invented
input, and an invented input produces an invented answer no less surely than
invented code does. Where the context is unknown, the screen asks -- it does not
assume.
