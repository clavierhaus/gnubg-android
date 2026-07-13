## GNU Backgammon for Android 0.21.2

A small maintenance release.

### Added
- **Report a problem.** A new tab in Settings (second, after Tournament)
  with a single button: Generate bug report copies a paste-ready diagnostic
  to the clipboard -- app version, device, Android version, and the exact
  game position as a GNU Backgammon ID plus the match state. Paste it into a
  GitHub issue so a problem can actually be reproduced. Nothing is sent
  automatically; the report is only what you paste, and the diagnostic
  scratch is cleared on entry so nothing lingers between sessions.

### Fixed
- The end-of-match message no longer clips to "You win the" -- it now shows
  "You win the match!" in full (and likewise the Gammon and Backgammon win
  messages, which the same one-line limit had been truncating).

**Verifying this download:** each release attaches `app-debug.apk.sha256`.
Run `sha256sum -c app-debug.apk.sha256` in the download directory (macOS:
`shasum -a 256 -c`); it prints `OK` when the APK is intact. Release tags are
signed -- `git tag -v v0.21.2`.
