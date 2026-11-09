## GNU Backgammon for Android 1.0.4

### Added
- Two new board themes, Monte Carlo and Chocolate, in Settings under Board.
  Their field, frame, points, checkers, dice and cube come from the reference
  images in issue #19.
- Analyse Position: tap a ranked move to see gnubg's own win, gammon and
  backgammon chances for it and its arrow on the board. Tap it again to clear
  (issue #16).
- Analyse Position: when the ranked moves do not fit, the list pages, with its
  controls in the header row. Nothing scrolls (issue #18).
- Analyse Position: the last analysed ID stays in the box after leaving the
  screen or closing the app, ready to analyse again. It is not installed by
  itself, because installing an ID replaces the match the engine holds
  (issue #18).

### Fixed
- Analyse Position: an XGID is now drawn the way XG draws it, with XG's bottom
  player at the bottom in white. gnubg calls that player 1, and this app draws
  player 1 on top, so the position came out upside down with the colours
  exchanged. gnubg's analysis was always for the correct player (issue #14).
- Analyse Position: Swap, offered when an XGID has the top player on roll, now
  turns the board round. It used to exchange the scores and leave the board
  as it was.
- Review Match opens at the first position of the first game instead of the
  last position of the last one, and each position shows the roll about to be
  played, using gnubg's own "next rolled" and "previous rolled" (issue #21).

**Verifying this download:** the release attaches `app-release.apk.sha256`.
Run `sha256sum -c app-release.apk.sha256` in the download directory (macOS:
`shasum -a 256 -c`); it prints `OK` when the APK is intact. Release tags are
signed -- `git tag -v v1.0.4`.
