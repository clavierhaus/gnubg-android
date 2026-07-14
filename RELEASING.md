# Releasing

## One-time: create a signing key

An unsigned APK will not install. The release build needs a keystore.

    keytool -genkey -v -keystore ~/gnubg-release.jks \
        -keyalg RSA -keysize 2048 -validity 10000 -alias gnubg

Then create `keystore.properties` in `gnubg-app/` (the gradle root, gitignored) from
`keystore.properties.example`, pointing `storeFile` at that `.jks` and filling in
the passwords. Keep both the `.jks` and `keystore.properties` out of git and
backed up somewhere safe: lose the key and you can never update the app under the
same signature.

If `keystore.properties` is absent, the release build still works but falls back
to the debug signing key -- fine for a throwaway test, wrong for a published APK,
because every future update must be signed with the SAME key or it will not
install over the old one.

## Each release

Two steps: prepare the version in a commit, then run `./release.sh`.

**1. Prepare (a normal commit).** Bump `versionCode` (must increase) and
`versionName` in `gnubg-app/app/build.gradle.kts`; roll `CHANGELOG.md` (rename
`[Unreleased]` to the new version, open a fresh `[Unreleased]`, fix the compare
links); regenerate `RELEASE_NOTES.md` from that changelog section. Commit and push
all of it to `main`.

    # regenerate RELEASE_NOTES.md from the new changelog section, e.g.:
    awk '/^## \[0.11.4\]/{f=1} f&&/^## \[0.11.3\]/{exit} f' CHANGELOG.md > RELEASE_NOTES.md
    # then set its first line to the release title:
    #   ## GNU Backgammon for Android 0.11.4

**2. Release (one command).**

    ./release.sh

`release.sh` does the whole thing and refuses to proceed if anything is off:

- reads the version from `build.gradle.kts` (the single source of truth);
- checks the working tree is clean, you are on `main`, and in sync with origin;
- checks the tag `vX.Y.Z` does **not** already exist (never silently moves a
  release tag -- moving tags is what left stale APKs on releases before);
- runs the buildable-clone check;
- verifies `gh` is authenticated;
- builds the debug APK (signed with the debug key, so it installs -- this is
  what the preview releases ship; `assembleRelease` produces an *unsigned* APK
  that will not install unless a keystore is configured, so the debug APK is the
  correct artifact for these sideloaded builds);
- tags `vX.Y.Z`, pushes it, and creates the GitHub release with the APK attached
  and `RELEASE_NOTES.md` as the body.

Useful flags: `--dry-run` (verify and build, but do not tag/push/publish),
`--no-build` (reuse an APK you already built), `--prerelease` (mark it a
pre-release).

To try a build without a device attached, `release.sh` builds the APK directly;
to also install and test on a device first, run `./build_and_deploy.sh` before it.

### On signed release builds

The steps above ship the **debug-signed** APK, which is what these sideloaded
preview releases have always used. If you later want a proper release-signed APK
(for the Play Store, or a stable published signature), configure a keystore as
below and build `assembleRelease`; then point `release.sh` at that artifact. The
one rule that never changes: every update must be signed with the SAME key as the
one before it, or it will not install over the old version.

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

## Release signing and third-party distribution

When `gnubg-app/keystore.properties` is present, `assembleRelease` produces a release signed with the Clavierhaus release key.

When `gnubg-app/keystore.properties` is absent, `assembleRelease` intentionally produces an unsigned release APK. This is the expected configuration for clean-clone verification and third-party distributors such as F-Droid, which build the application from source and apply their own signing key.

The normal Android debug build remains debug-signed by the Android build tools and is unaffected by this release configuration.
