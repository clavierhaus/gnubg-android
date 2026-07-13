## GNU Backgammon for Android 0.20.1

A cosmetic follow-up to 0.20.0. Two small user-visible changes, no engine
changes.

### Added
- **Publisher mark on the hub.** "clavierhaus.at" now reads in the top-right
  corner, symmetric with the settings gear on the top-left, in the same DejaVu
  Serif as the title and menu entries. "vie" is set in GNU orange — the Vienna
  pun the company name carries — the rest in the hub's off-white.

### Fixed
- **Coach move-list chip labels are now visible.** The "P" identifier on the
  player's move row and "1", "2", "3" on gnubg's better alternatives drew as
  coloured pills with no letters on every device since the coach shipped. The
  strings were in the source, but the 30dp chip slot squeezed the compact
  button's inner text constraint (30 − 19 − 19 dp of padding) to zero width, so
  the glyph measured to zero and never rasterised. Replaced with a fixed-size
  26dp circular identifier chip whose size makes that failure mode
  unrepresentable; matches the visual grammar of the score-tag badge.

**Verifying this download:** each release attaches `app-debug.apk.sha256`. After downloading both, run `sha256sum -c app-debug.apk.sha256` in the download directory (macOS: `shasum -a 256 -c`); it prints `OK` when the APK is intact.
