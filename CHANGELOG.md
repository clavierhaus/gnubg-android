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

## [0.10.1] — 2026-07-11

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

## [0.10.0] — 2026-07-11

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

[Unreleased]: https://github.com/clavierhaus/gnubg-android/compare/v0.10.1...HEAD
[0.10.1]: https://github.com/clavierhaus/gnubg-android/compare/v0.10.0...v0.10.1
[0.10.0]: https://github.com/clavierhaus/gnubg-android/compare/v0.9.1-preview...v0.10.0
[0.9.1-preview]: https://github.com/clavierhaus/gnubg-android/releases/tag/v0.9.1-preview
