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

## The rule still governs

Nothing here relaxes `CLAUDE.md`. Position entry decodes an ID with gnubg's
decoder. Evaluation calls gnubg. Review replays gnubg's own move records through
gnubg's own routines and displays what gnubg returns. Kotlin draws boards, holds
transient UI state, and calls JNI. If a verdict about a position appears on
screen, a named gnubg routine produced it.
