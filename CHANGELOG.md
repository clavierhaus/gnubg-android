# Changelog

All notable changes to GNU Backgammon for Android are recorded here.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project uses [Semantic Versioning](https://semver.org/) adapted for a
pre-1.0 app: while the major version is 0, the **minor** number rises for new
features and the **patch** number for fixes.

One rule underlies every entry: GNU Backgammon is the sole authority for game
logic and analysis. "Fixed" almost always means the app stopped disagreeing with
the engine, or stopped hiding what the engine already knew.

## [Unreleased]

_Nothing yet._

## [0.11.4] — 2026-07-11

### Fixed
- **The cube row in the position editor was cut off** the bottom of the screen on
  taller phones, and couldn't be reached (issue #1). The editor now fits: "On
  roll" and "Dice" share one row, the redundant instruction line is gone, and the
  spacing is tighter — measured to clear the buttons with room to spare across the
  supported aspect-ratio range.

### Added
- **A "Start pos" button** in the position editor fills the standard opening
  position, so you don't have to place all 30 checkers by hand (issue #1).

## [0.11.3] — 2026-07-11

### Fixed
- **You can now leave a match that isn't finished.** Mid-match, "Home" ends the
  current match (after confirming) and returns to the home screen — where you can
  start a new match with different strength and length. Before, leaving mid-match
  dropped you back into the same game with no way to change parameters short of
  playing it out.

## [0.11.2] — 2026-07-11

Field-report fixes to the 0.11.0 release. (0.11.1 was a re-tag of identical code
during release setup; 0.11.2 is the first with these fixes.)

### Fixed
- **The review verdict could be cut off** the bottom of the screen — the move
  info and the verdict together overflowed past the navigation buttons on shorter
  phones. The info block is now bounded above the buttons, so the verdict is
  always on screen.
- **No obvious way out of Analyse.** After analysing a position there was no
  visible exit; you had to close the app. A Home button is now pinned to the
  Analyse pane in every state (paste, editor, result).
- **The engine's opening looked frozen.** When gnubg won the opening roll at a
  slow level, the board sat static for several seconds with no sign it was
  working. It now shows a thinking state and the opening roll as it lands, like
  every other engine turn.
- **The "Show equity" setting did nothing.** It now gates the equity line in the
  analysis panel. (Showing match-winning-chance instead of equity is still to
  come — it needs gnubg's own conversion, not an app-side one.)

## [0.11.0] — 2026-07-11

**The first real release** — the successor to the 0.9.1 preview. It makes the app
a comprehensive backgammon companion: play at gnubg strength, set up and analyse
any position, save matches, and review them. (Versions 0.10.0 and 0.10.1 were
internal steps and never shipped an APK; everything in them is included here.)

### Added
- **Set up any position and analyse it.** A board editor: tap points and the bar
  to place checkers, tap the bear-off tray to clear, then set dice, cube, score,
  match length, and who is on roll. Dice set → gnubg's ranked chequer plays; no
  dice → gnubg's cube decision (double/take/drop with equities). The GNU BG ID is
  shown and copyable, and IDs/XGIDs paste in.
- **Save the match to `.sgf`** at any point, through the Android file picker;
  opens in desktop gnubg.
- **Review a saved match**, stepping game by game and move by move on gnubg's own
  board — with **gnubg's verdict on every move**: what was played, what gnubg
  preferred, the equity difference, the rank among all legal plays, and gnubg's
  own classification (doubtful / bad / very bad) when the move deserves one.
- **Seven playing levels.** The original four (0-ply with descending noise) plus
  **Expert** (0-ply, no noise), **World class** (2-ply) and **Grandmaster**
  (3-ply), exposing gnubg's real strength.
- A settings gear on every screen, over a single settings overlay; consistent
  "Home" and "New match" throughout.
- **The engine's roll is visible while it thinks.** gnubg rolls before it
  searches; the board now shows those dice grayed the moment they land (with
  "Rolled 5-3. Thinking..." in the panel), so you can start reading the position
  during the wait -- exactly as desktop gnubg behaves.

### Fixed
- **Saved SGF names were swapped** — the human was labelled "gnubg", the engine
  "user". The port's player 0 is the human; the names now match ("You" / "GNU
  Backgammon").
- **The strongest level was not strong.** The old "Advanced" is a
  0-ply-with-noise preset and occasionally played a poor move (a 24/16 on an
  opening 5-3 was reported). The per-player move filter was also never
  initialised, which silently broke multi-ply evaluation; fixing it is what makes
  the new 2-ply and 3-ply levels correct.
- **Start Match could vanish** on short landscape phones, squeezed to zero height
  by a weighted layout. It is now pinned.
- **The Analyse screen could hide its own output** — the ranked plays, the
  editor's Analyse button, and long labels fell off short panes. Regions are
  pinned or bounded now; labels never wrap.
- **A fresh clone could not build** — engine headers the Android build compiles
  (`sound.h`, `export.h`, `movefilters.inc`, `boarddim.h`, `progress.h`,
  `openurl.h`) were hidden by `.gitignore`. All tracked now, guarded by
  `tools/check_buildable_clone.sh`.
- The release build is signed, so its APK installs.
- Engine-fidelity fixes: answer the resignation GNU offers (a won game could not
  be finished); read each die from gnubg's move list rather than guessing; repair
  `EVALSETUP_2PLY`/`GetEvalMoveFilter` (25 build warnings to zero); tap and
  highlight along gnubg's own legal-move list.

### Notes
- **Thinking time.** A Grandmaster (3-ply) move takes about 7-9 seconds on a
  current phone, a 2-ply move about 2. This is the honest single-core cost of a
  strong search: gnubg already prunes and runs its neural-net evaluation with ARM
  NEON SIMD, so any app at this strength on this hardware pays the same. It is not
  a defect. See `docs/THREADING.md` for why the move cannot be threaded (gnubg
  parallelises rollouts and analysis, not a single live search) and the
  conditions under which multi-core support arrives for those. The per-move
  review verdict runs at gnubg's 2-ply analysis setting, so each step is quick.

## [0.10.1] — 2026-07-11 (internal, never shipped)

A follow-up to 0.10.0 with the fixes from the first field reports. Everything in
0.10.0 that had not yet reached a device is folded in here.

### Added
- **Three stronger playing levels** — Expert (0-ply, no noise), World class
  (2-ply), Grandmaster (3-ply) — exposing gnubg's real strength. The four
  original levels are 0-ply with noise; the strongest of those still let an
  occasional weak move through.

### Fixed
- **Saved SGF names were swapped**: the human was written as "gnubg", the engine
  as "user". The port's player 0 is the human; the names now match.
- **The strongest level was not strong.** "Advanced" is a 0-ply-with-noise preset
  and occasionally played a poor move (a 24/16 on an opening 53 was reported).
  Underneath, the per-player move filter was never initialised, which silently
  broke multi-ply evaluation; fixing it is what makes the new levels correct.
- **Start Match could vanish** on short landscape phones, squeezed to zero height
  by a weighted layout. It is now pinned.
- **The Analyse screen could hide its own output** — the ranked plays, the
  editor's Analyse button, and long labels fell off short panes. Regions are now
  pinned or bounded; labels never wrap.
- **A fresh clone could not build**: engine headers the Android build compiles
  (`sound.h`, `export.h`, `movefilters.inc`, `boarddim.h`, `progress.h`,
  `openurl.h`) were hidden by `.gitignore`. All tracked now, with
  `tools/check_buildable_clone.sh` as a guard.
- The release build is signed, so its APK installs.

## [0.10.0] — 2026-07-11 (internal, never shipped)

The comprehensive-companion release: the three features players open other apps
for, plus the fixes from the first days of field reports.

### Added
- **Set up any position and analyse it.** A board editor: tap points and the bar
  to place checkers, tap the bear-off tray to clear, then set dice, cube, score,
  match length, and who is on roll. Dice set → gnubg's ranked chequer plays; no
  dice → gnubg's cube decision (double/take/drop with equities), gnubg's own
  edit-mode convention. The GNU BG ID is shown and copyable.
- **Paste a position.** A GNU BG ID or XGID from anywhere installs and evaluates
  through the same engine path a hand-built position takes.
- **Save the match to `.sgf`** at any point, through the Android file picker;
  opens in desktop gnubg.
- **Review a saved match**, stepping game by game and move by move on gnubg's own
  board (gnubg's `CommandNext`/`CommandPrevious`).
- **Three stronger playing levels** — Expert (0-ply, no noise), World class
  (2-ply), Grandmaster (3-ply) — exposing gnubg's real strength beyond the four
  noise-based levels.
- A settings gear on every screen, over a single settings overlay that returns to
  wherever it was opened.

### Fixed
- **Saved SGF names were swapped** — the human was labelled "gnubg" and the engine
  "user". The port's player 0 is the human; the default names now match.
- **The strongest level was not strong.** All four levels were 0-ply with noise,
  so "Advanced" occasionally played a poor move (a 24/16 on an opening 53 was
  reported). Root cause ran deeper: the per-player move filter was never
  initialised, which silently broke any multi-ply evaluation. Both fixed; the new
  levels depend on it.
- **The Start Match button vanished** on short landscape phones — it was squeezed
  to zero height by a weighted layout. It is now pinned so it cannot be.
- **The Analyse screen could hide its own output** — results, the editor's Analyse
  button, and long labels fell off the bottom on short panes, because nothing
  scrolls and two regions were unbounded. Controls are now pinned and results
  bounded; labels never wrap.
- **A fresh clone could not build.** Several engine headers the Android build
  compiles (`sound.h`, `export.h`, `movefilters.inc`, `boarddim.h`, `progress.h`,
  `openurl.h`) were hidden by `.gitignore`. All are tracked now, and a check
  (`tools/check_buildable_clone.sh`) fails the build if it ever recurs.
- Consistent navigation: "Home" leaves to the hub and "New match" restarts with
  the same parameters, on every screen and in both play modes.
- The release build is now signed, so its APK installs.
- Numerous engine-fidelity fixes: answer the resignation GNU offers (a won game
  could not be finished); read each die from gnubg's move list rather than
  guessing; repair `EVALSETUP_2PLY` and `GetEvalMoveFilter` (25 build warnings to
  zero); tap and highlight along gnubg's own legal-move list.

## [0.9.1-preview] — 2026-07-09

First public preview.

### Added
- Full matches against the gnubg engine, four strength levels.
- The doubling cube, decided by gnubg (offer, take, drop, redouble, resign).
- Tournament rules: Crawford, Jacoby, automatic doubles, beavers.
- A choice of match equity table (Kazaross-XG2, Woolsey, and others).
- Live tutor: gnubg's own equity evaluation as you play.
- A native touch board with three themes; persistent settings.

[Unreleased]: https://github.com/clavierhaus/gnubg-android/compare/v0.11.4...HEAD
[0.11.4]: https://github.com/clavierhaus/gnubg-android/compare/v0.11.3...v0.11.4
[0.11.3]: https://github.com/clavierhaus/gnubg-android/compare/v0.11.2...v0.11.3
[0.11.2]: https://github.com/clavierhaus/gnubg-android/compare/v0.11.0...v0.11.2
[0.11.0]: https://github.com/clavierhaus/gnubg-android/compare/v0.9.1-preview...v0.11.0
[0.10.0]: https://github.com/clavierhaus/gnubg-android/compare/v0.9.1-preview...v0.10.0
[0.9.1-preview]: https://github.com/clavierhaus/gnubg-android/releases/tag/v0.9.1-preview
