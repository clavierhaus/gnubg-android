## GNU Backgammon for Android 0.21.4

### Changed
- **Settings tidied for the first F-Droid release.** Removed all the unfinished
  placeholder options that were showing as greyed-out "Later" rows -- they were
  development notes, not user settings. What remains is only what actually
  works.
- **The GPL v3 license now has its own tab**, showing the complete license
  text. This app is proudly free software, and the license it's released under
  deserves the visibility.
- **Release builds are signed** with the project's own release key (previously
  the GitHub download was a debug build). This is also the foundation for
  reproducible builds on F-Droid.

**Verifying this download:** the release attaches `app-release.apk.sha256`.
Run `sha256sum -c app-release.apk.sha256` in the download directory (macOS:
`shasum -a 256 -c`); it prints `OK` when the APK is intact. Release tags are
signed -- `git tag -v v0.21.4`.

Note: if you previously installed a build from GitHub, you may need to
uninstall it first before installing this signed release (Android does not
allow updating across different signing keys). Future updates will then apply
normally.
