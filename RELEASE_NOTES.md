# GNU Backgammon for Android 0.21.1

This release prepares GNU Backgammon for independent source builds and submission to the main F-Droid repository.

The application retains the complete Play, Train, Analyse and Review functionality introduced in previous releases.

## Distribution changes

- Added Fastlane-compatible application metadata.
- Added upstream application icon and screenshots for F-Droid.
- Removed the release-build fallback to the Android debug signing key.
- Clean source checkouts now produce an unsigned release APK suitable for signing by F-Droid or another distributor.
- Updated release documentation for third-party source builds.

## Requirements

- Android 12 or newer
- 64-bit ARM device
