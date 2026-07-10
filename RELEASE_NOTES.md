## GNU Backgammon for Android 0.10.0 — the comprehensive-companion release

This release completes the three features the app set out to provide, each running on GNU Backgammon's own engine — not an app-side re-implementation:

- **Set up any position and analyse it.** Tap points and the bar to place checkers, tap the bear-off tray to clear the board, then set dice, cube, score, match length, and who is on roll. Dice set → gnubg's ranked chequer plays. No dice → gnubg's cube decision (double / take / drop with equities), exactly as gnubg's desktop edit mode treats a no-dice position. The GNU BG ID is shown and copyable. You can also paste a GNU BG ID or XGID from anywhere.
- **Save the match to a file.** The whole match written to a standard `.sgf` at any point, via the Android file picker — opens in desktop gnubg.
- **Review a match move by move.** Open a saved `.sgf` and step through it, game by game and move by move, on gnubg's own board.

Also in 0.10.0:

- One-geometry board: a tap lands exactly where it is drawn, verified across aspect ratios from tablet to tall phone.
- Settings gear on every screen; consistent Home / New match everywhere.
- The repository now builds from a clean clone with no submodule setup.

Built for Android 12+ (arm64-v8a). GPL-3.0-or-later.
