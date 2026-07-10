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
