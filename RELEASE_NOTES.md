## GNU Backgammon for Android 0.11.0

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

