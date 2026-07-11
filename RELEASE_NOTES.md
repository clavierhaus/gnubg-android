## GNU Backgammon for Android 0.10.1

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

