# Design: the Analyse destination (position entry, match save, match review)

> **STATUS (2026-07-10).** All three features named below are now built.
> [1] Analyse Position and [2] Save match are complete. [3] Review Match exists in
> a first form: it opens a saved `.sgf` and steps through it, but does not yet show
> gnubg's per-move verdict. Sections below are the design as decided; where they
> speak of what "will" be built, read them as the record of a decision, not as a
> description of missing code. Deviations are recorded at the end.

Status: implemented. Written as a proposal before implementation, to fix the architecture before
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

## Decided structure

    Play Tournament Match   -- a match against gnubg. Tutor is a match-setup
                               option here, not a separate destination.
    Analyse Position        -- enter a position, gnubg evaluates it.      [1]
    Review Match            -- open a saved match, step through it,
                               gnubg's verdict on each move.              [3]
    Options
    Profile

Position entry takes the **second** slot, directly after Play. The menu is a
positioning statement: two peers, play and analyse -- not a game with an
analysis afterthought. Feedback was unambiguous that position entry is the
reason people still open XG Mobile, and that a competitor's developer has
declined to build it. It does not belong behind a submenu.

**Saving a match [2] is not a menu point.** It is an *action* -- offered at the
end of a game and from within match review -- that produces the artifacts
`Review Match` consumes.

This makes the three features one coherent story rather than three bolted-on
additions:

> Play a match -> save it -> open it in Review Match -> step through gnubg's
> verdict on every move. And, independently: paste any position -> get gnubg's
> verdict.

`Analyse Position` and `Review Match` are kept as **separate modes** rather than
one Analyse destination with sub-entries. They share a look (a board plus
gnubg's verdict) but not an interaction model: position entry is stateless
(paste, evaluate, done); match review is sequential (load, step, step). A
hub-within-a-hub would also bury the feature that most justifies the app,
one tap deeper than it deserves.

### What happens to "Live Game Analysis"

It is removed as a hub entry. Two findings justify this, and the second is a bug.

**It is not a destination.** `AppMode.LIVE_ANALYSIS` is `GameLayout` with
`tutorMode = true`. It is a way of playing occupying a slot in a menu of places.

**`tutorMode` currently does four things at once:**

| Line (`GameLayout.kt`) | Effect |
|---|---|
| 56 | forces a **1-point match** (`startMatch(if (tutorMode) 1 else settings.matchLength)`) |
| 186 | shows `TutorAnalysisPanel` -- the tutor itself |
| 447 | changes the screen title |
| 480 | hides the match-length selector (because length is forced) |

Line 186 is the tutor. Line 56 is a **deliberate design decision** -- the tutored
game is cubeless, and the single game is the only cubeless format (see below).
Lines 447 and 480 follow from 56. The problem is not what these lines do; it is
that a single boolean parameter, hard-coded per hub entry, performs all of it
invisibly -- while the persisted setting that ought to control it is ignored.

**The bug: `settings.tutorMode` does nothing.** There is a persisted Tutor mode
switch in Settings > Analysis (`SettingsScreen.kt:320`), with a ViewModel setter
and DataStore round-trip (`PreferencesManager.kt:66,89`). **No screen reads it.**
`GameLayout` reads only the `tutorMode` *parameter*, hard-coded `false` for
`AppMode.PLAY` and `true` for `AppMode.LIVE_ANALYSIS`. A user can enable Tutor
mode, have it survive a restart, and see no effect. Its subtitle -- "Stored
locally until GNUbg timing is safe" -- is accidentally accurate.

Consequently, "demote the tutor to a toggle" is not a demotion. It is **making
the existing toggle work**, and choosing where the tutored-game path lives.

**Resolution:**

- The tutor becomes a real option at **match setup**, alongside match length and
  difficulty -- where a user would look for it, and where the other match
  parameters already are. It reads and writes `settings.tutorMode`, so the
  Settings switch finally means something.
- The **single tutored game** remains exactly that. See "Why the tutor implies a
  single game" below: this is not incidental scaffolding to be generalised away.
  What changes is only that the implication is **stated** rather than performed
  silently at `GameLayout.kt:56`. Choosing the chequer-play tutor should say, on
  screen, that it is a single game and that the cube is not in play.
- The stale subtitle is corrected.

This frees the second slot without growing the hub, and is more honest about what
the feature is.

### Why the tutor implies a single game

The original design excluded cube decisions from the tutor deliberately. Cube
tutoring needs a different interaction model -- a verdict at *decision points*
(double? take? drop?), not after each chequer move -- so rather than half-build
it, the tutored game was made cubeless.

The lever chosen was a 1-point match, and that is the correct lever, not a blunt
proxy for one. **The single game is the only coherent cubeless format.** A
cubeless 7-point match is not a 7-point match; it is seven single games with a
scoreboard. The cube is what makes match play *match play* -- what makes
score-dependent equity mean anything, what distinguishes 4-away/2-away from
2-away/4-away, what turns a sequence of games into a strategic arc. Remove it and
the match length is an arbitrary repetition count, not a structure.

gnubg agrees, structurally. `gnubg_can_double` (`play.c:156`) gates on:

    if (ms.nMatchTo && ms.nCube >= (ms.nMatchTo - ms.anScore[ms.fTurn])) return 0;

At `nMatchTo = 1`, score 0-0: `nCube` (1) `>=` `1 - 0` -- the cube is already
unavailable. You cannot double past the match. The single game is cubeless by the
rules, not by app convention.

**Correction to an earlier draft of this document**, recorded rather than quietly
deleted: it claimed the forced 1-point match had "no engine reason" and proposed
`fCubeUse = off` at any length as a more precise expression of the same intent.
That was wrong twice. It had a *game* reason, which is stronger than an engine
reason. And cube-off-at-any-length expresses a superset of the intent, most of
which is meaningless -- being more general is not the same as being more correct.
gnubg *can* run a cubeless 7-pointer; that does not make it a thing. The error
was substituting "can the engine do it?" for "does it mean anything?"

When cube tutoring is eventually built, it attaches to real matches, where it
belongs. Until then the tutor is honestly a **chequer-play** tutor and should be
labelled as one -- per the rule, do not imply an analysis that is not being
provided.

### Cost of the change

Removing a visible hub entry after a public preview is a real change, not a
free one. It should be stated in the next release notes rather than disappearing
silently -- "tutor moved from its own menu entry to a match-setup option; the
analysis features moved to the new Analyse Position and Review Match modes"
reads as intent. The audience is small and the release is a preview, so the cost
is low and the architectural gain is permanent.

`LEARN` remains unreachable; it is a separate question. Note that `LEARN` and
`ANALYSE` sitting wired-but-unreachable is exactly the failure this document
exists to avoid repeating: **a slot is not reserved for a feature that does not
exist yet.** `Review Match` enters the hub when it is built, not before.

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

**CORRECTION.** An earlier draft of this document said "do not route position
entry through `CommandSetBoard`", on the grounds that `SetBoard` calls
`ParsePosition`, which was "defined in no vendored file" and "would fail to
link". **That was false, and it was the load-bearing premise for a parallel
design that should never have been drawn up.**

- `ParsePosition` **is** defined: `jni-bridge/src/android-app.c:1061`, a verbatim
  port of `gnubg.c:885`, part of the GTK-shell replacement.
- `CorrectNumberOfChequers` **is** defined: `set.c:584`, used internally by
  `SetBoard`. It was never the port's concern.
- `set.c` is compiled. `SetBoard` links and works today.

The `GAME_PLAYING` requirement is real but is **not** a blocker, and gnubg
already answers it -- see `SetGNUbgID` below.

`PositionFromID` avoids both. **It also validates, by itself, using gnubg's own
checker.** Its final statement is:

    return CheckPosition((ConstTanBoard) anBoard);

`CheckPosition` (`positionid.c:286`, compiled) rejects more than fifteen chequers
for either player, both players occupying the same point, and both players on the
bar against closed boards. It returns 1 for a legal position and 0 with
`errno = EINVAL` otherwise. A malformed or illegal ID therefore fails at the
decoder, and no chequer count is ever written by this port.

**Bonus: gnubg also decodes XG position IDs.** `PositionFromXG(TanBoard, const
char *pos)` (`positionid.c:247`, declared `positionid.h:30`) is vendored and
compiled. Given that this feature exists to serve people leaving XG Mobile, the
screen should accept an XG ID as readily as a gnubg one.

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

**Outcome (2026-07-10).** Confirmed, and cheaper than this section expected. The
PORT CHECKPOINT found that `CommandNext` was already wrapped as
`gnubg_mobile_command_next`, that `load_match` and `save_match` were already
wired, and that `hint_moves` and `analyze_played_move` -- both already used by the
tutor -- supply the verdict. Exactly one engine verb was missing, `CommandPrevious`
(41b00e9). The rest was the UI, as predicted.

Two things this section did not anticipate:

- `Engine.loadMatch` replaces the engine's match, so opening a file discards a
  game in progress. Reviewing "while playing" therefore cannot be done by loading
  a file; it would have to navigate the record of the live match in place. The
  first version does not attempt it.
- `CommandLoadMatch` tokenizes its path exactly as `CommandSaveMatch` does, so the
  cache file the SAF document is copied into must carry no whitespace.

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

1. **[1] Analyse Position: facade verbs, then screen, then hub entry.** The hub
   entry lands in the same change as a working screen -- never before it. A
   reachable placeholder is the same error as an unreachable feature, facing the
   other way. Cheapest of the three, uses analysis that already exists, and fills
   the one gap a competitor has publicly refused to fill.
2. **[2] Save match as `.sgf`.** Near-free; produces the artifacts step 3 needs.
   `.mat` deferred pending its own scoping. Not a menu point -- an action.
3. **[3] Review Match.** The largest build. By then serialisation exists, and
   (per `ROADMAP_ANALYSIS_PARITY.md`) so does the ranked candidate-move list it
   wants to display. It takes the third hub slot when it works, not before.

(An earlier draft of this list began "build the Analyse destination shell" -- a
leftover from the rejected single-destination design. Analyse Position and Review
Match are separate modes; there is no shared shell to build.)

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

**Consequence:** the tutor's chain is safe and correct as written, and must not be
disturbed.

**But this risk was mis-framed.** It was posed as "must position entry avoid the
global `ms`?", and answered by designing a private matchstate. gnubg's model is
that `ms` *is* the position under analysis -- you set it, then you analyse it.
`SetGNUbgID` sets it. The real question was never "how do I avoid the global",
but "what does the user expect to happen to a game in progress when they paste a
position?" That is a UI question -- warn, or discard, or keep a game aside -- and
it is answered on the screen, not by cloning gnubg's state model.

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

**Restated for the `SetGNUbgID` design.** The hazard is not in the facade -- it
is in the feature. `SetGNUbgID` sets the match context only when the pasted ID
carries one:

    if (matchid) SetMatchID(matchid);
    if (posid)   SetBoard(posid);

Paste a bare Position ID and the board changes while `ms` keeps whatever score,
cube, match length and Crawford flag it already held. gnubg will then evaluate
correctly -- for a context the user never stated.

**The mitigation is gnubg's own, and the port should copy it.**
`CommandSetGNUbgID` ends with `ShowBoard()`, which prints the position *together
with* the score and cube. The desktop shows you the context you have just
inherited. So must this screen: after setting an ID, read the resulting match
context back out and display it. Match length, score, cube value and owner,
Crawford, Jacoby, and who is on roll -- visible, every time, not on request.

The screen must never quietly default. "Money play" is a legitimate answer; an
unstated assumption of it is not. Note also that `SetGNUbgID` returns `2` when
the position has the player on roll on top; gnubg's callers offer a swap. The
port must surface that choice rather than silently swapping or silently not.

## The design for [1]: use gnubg's own entry point

**`SetGNUbgID(char *sz)`** -- declared `backgammon.h:519`, defined in `set.c`,
compiled. This *is* the feature, already written:

    switch (SetXGID(sz)) { case 0: return 0; case 2: return 2; default: ; }
    /* otherwise: split base64 tokens, discriminate by LENGTH */
    ...  strlen(out) == L_MATCHID   -> matchid
    ...  strlen(out) == L_POSITIONID -> posid
    if (matchid) SetMatchID(matchid);
    if (posid)   SetBoard(posid);

Read what it gives us:

- It accepts an **XGID** (via `SetXGID`) *or* a **GNU BG ID**. The dialects are
  discriminated properly -- XGID first, then base64 tokens keyed on their known
  lengths. Not by trial-decoding and hoping.
- It sets the **match context before the board**, which is exactly why the
  `GAME_PLAYING` requirement in `SetBoard` is not a blocker: `SetMatchID` sets
  `ms.gs` from the Match ID's gamestate field, and `SetBoard` then passes its own
  check. The ordering is deliberate and it is gnubg's.
- Return codes: `0` set, `1` no valid IDs found, `2` the position has the player
  on roll on top (gnubg's own callers respond by offering `CommandSwapPlayers`).

Call `SetGNUbgID`, not `CommandSetGNUbgID` / `CommandSetXGID`: the Command
wrappers call `GetInputYN` and `ShowBoard`, which are terminal affordances. The
non-Command function is the seam gnubg deliberately exposes, and it hands the
swap decision back to the caller as return code `2`.

**The state is the position.** gnubg's model is that the global `ms` holds the
position under analysis; you set it, then you analyse it. That is how the desktop
works: set the board, then `hint`. The port does not need -- and must not build --
a private matchstate universe alongside it.

Verbs, thin:

- `gnubg_mobile_set_gnubg_id(const char *id)` -- lock, `SetGNUbgID` on a mutable
  copy, unlock. Return gnubg's code unchanged.
- Reading the result back uses the existing `gnubg_mobile_get_board` and the
  existing match-state readers. Evaluation uses the existing evaluation verbs.
  Nothing new is required for either.
- `gnubg_mobile_current_gnubg_id(...)` -- the reverse, via `PositionID()` and
  `MatchIDFromMatchState()`, for "copy this position" out of a live game.

**Correction, recorded.** An earlier draft of this section proposed decoding with
`PositionFromID` + `MatchFromID`, hand-assembling a **local** `matchstate` from
the loose out-params, deriving a cubeinfo from it, and evaluating against that --
explicitly to avoid touching the global `ms`. Every step of that is a
reimplementation of `SetGNUbgID` plus gnubg's own state model, and it was drawn
up to route around a link failure that does not exist. It also invented a
bespoke return-code taxonomy and a field ordering for a struct gnubg already
defines. The rule is not "avoid globals"; the rule is *port, never reinvent*.

**Struck.** An earlier draft proposed trying `PositionFromID` and, on failure,
`PositionFromXG` -- and defended it as "dispatch, not interpretation". That was a
rationalisation, and the code would have been dangerous: `PositionFromID`
base64-decodes whatever it is handed, so an XG string can decode into a board
that passes `CheckPosition` and looks entirely legal while being the wrong
position. A silently wrong answer produced by invented code. gnubg discriminates
the dialects properly and already does it -- see `SetGNUbgID`.

**Validation belongs to gnubg too, and the open item is closed.**
`CorrectNumberOfChequers` is indeed not declared in any vendored header -- and it
is not needed. `PositionFromID` returns `CheckPosition(...)` (`positionid.c:286`),
gnubg's own legality check, so a bad ID is rejected by the decoder itself. The
facade must propagate that return value, never substitute a count of its own.
`SwapSides` is declared (`eval.h:390`) and available for frame correction.

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
