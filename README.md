# GNU Backgammon by clavierhaus.at

Android front-end and functional port of GNU Backgammon.

GNU Backgammon remains authoritative for game logic, cube logic, match/session state, analysis, tutor behaviour, rollout, command semantics, and engine configuration. Android provides the modern touch UI, navigation, presentation, persistence, and command translation layer.

## Current milestone

Current internal milestone: **V0.8.14**

V0.8.14 introduced the grouped five-tab Settings surface:

- Game
- Board
- Engine
- Analysis
- Expert

The restricted GNUbg command bridge exists, but live Settings command dispatch is deliberately quarantined. Settings rows currently update Android-side configuration state unless a lifecycle-safe GNUbg application path has been verified.

## Repository layout

- `gnubg-app/` — Android app module
- `jni-bridge/` — JNI/native bridge layer
- `engine-core/` — GNUbg-derived engine core integration
- `upstream-source/` — upstream GNU Backgammon source
- `docs/` — canonical project documentation

## Documentation

Start here:

- `docs/README.md` — documentation index
- `docs/BUILD.md` — build notes
- `docs/KNOWN-LIMITATIONS.md` — known limitations
- `docs/TECHNICAL-NOTES.md` — technical notes
- `docs/SETTINGS-UX-BLUEPRINT.md` — Settings UX blueprint
- `docs/STATUS_V0.8.10.md` — V0.8.10 milestone note
- `docs/STATUS_V0.8.11.md` — V0.8.11 milestone note
- `docs/SETTINGS_GNUBG_MAPPING_V0.8.13_DRAFT.md` — archived V0.8.13 Settings mapping draft

## Build

Typical debug build:

    cd gnubg-app
    ./gradlew assembleDebug

Typical install/restart sequence:

    cd gnubg-app
    ./gradlew assembleDebug
    adb install -r app/build/outputs/apk/debug/app-debug.apk
    adb shell am force-stop com.clavierhaus.gnubg
    adb shell am start -n com.clavierhaus.gnubg/.MainActivity

Do not use `adb uninstall` unless explicitly necessary.
