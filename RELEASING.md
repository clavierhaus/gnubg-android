# Releasing

## One-time: create a signing key

An unsigned APK will not install. The release build needs a keystore.

    keytool -genkey -v -keystore ~/gnubg-release.jks \
        -keyalg RSA -keysize 2048 -validity 10000 -alias gnubg

Then create `keystore.properties` in the repo root (it is gitignored) from
`keystore.properties.example`, pointing `storeFile` at that `.jks` and filling in
the passwords. Keep both the `.jks` and `keystore.properties` out of git and
backed up somewhere safe: lose the key and you can never update the app under the
same signature.

If `keystore.properties` is absent, the release build still works but falls back
to the debug signing key -- fine for a throwaway test, wrong for a published APK,
because every future update must be signed with the SAME key or it will not
install over the old one.

## Each release

1. Bump `versionCode` (must increase) and `versionName` in
   `gnubg-app/app/build.gradle.kts`, commit, and move/create the tag.

2. Build the signed release APK:

       cd gnubg-app
       ./gradlew clean assembleRelease
       ls -la app/build/outputs/apk/release/
       # -> app-release.apk   (NOT app-release-unsigned.apk)

   If you see `app-release-unsigned.apk`, `keystore.properties` was not found --
   fix that before publishing.

3. Test the built APK on a device before publishing:

       adb install -r app/build/outputs/apk/release/app-release.apk

4. Publish, attaching the APK and using RELEASE_NOTES.md as the body:

       cd ..
       gh release create v0.10.0 \
         gnubg-app/app/build/outputs/apk/release/app-release.apk \
         --title "0.10.0 position setup, match save, match review" \
         --notes-file RELEASE_NOTES.md

## Versioning and the changelog (from 0.10.0 on)

Version numbers are `0.MINOR.PATCH` while the app is pre-1.0:

- **MINOR** rises for a release that adds a feature (0.10 → 0.11).
- **PATCH** rises for a release that only fixes things (0.10.0 → 0.10.1).
- Tags are `vMAJOR.MINOR.PATCH`, e.g. `v0.10.0`. The GitHub release title is the
  version plus a short phrase: `0.10.0 — position setup, match save, review`.

`CHANGELOG.md` is the source of truth for what changed, and it is updated in the
SAME commit as the change, not reconstructed at release time:

- Add every user-visible change under `## [Unreleased]`, in `Added` / `Changed` /
  `Fixed` / `Removed` groups, phrased for a player, not a developer.
- At release, rename `[Unreleased]` to the new version with today's date, open a
  fresh empty `[Unreleased]`, and update the compare links at the foot.
- The GitHub release body is that version's changelog section, verbatim.

Commit messages stay in the `type(scope): summary` form already in use
(`feat(analyse): …`, `fix(setup): …`); the changelog is the human-facing digest
of them, not a duplicate.

## Before every release: the buildable-clone check

    ./tools/check_buildable_clone.sh

It fails if any file the Android build compiles or includes is gitignored -- the
mistake that made fresh clones unbuildable more than once. A release must not go
out while it fails.
