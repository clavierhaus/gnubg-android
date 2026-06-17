# Architecture

## Core principle

GNU Backgammon is authoritative.

Android must not become an independent backgammon implementation. It should present a modern touch-native UI while delegating game semantics to GNUbg wherever GNUbg already has mature logic.

GNUbg owns:

- game rules
- legal moves
- cube logic
- match and session state
- scoring
- analysis
- tutor semantics
- rollout
- command grammar
- engine/player/evaluation configuration

Android owns:

- app shell and navigation
- presentation
- board rendering
- touch interaction
- Android-only display preferences
- settings surfaces
- persistence of Android preferences
- translation of safe UI actions into GNUbg-backed calls

## Settings vs actions

Settings screens are configuration-only.

Actions do not belong in Settings. Examples that must remain on the Home Hub or active Play/board surface:

- New game
- New match
- Resign
- End game
- Save/load game/session/position, unless later placed in an explicit Library or Analyse flow
- Return Home
- Roll
- Double
- Move
- Undo
- Confirm

Board interactions stay on the board surface.

The lower-left Play rail is reserved for genuine GNUbg-backed lifecycle controls, not duplicated board actions.

## Command bridge status

A restricted GNUbg command bridge exists. It was added to expose selected `set ...` and `show ...` command prefixes through JNI and Kotlin.

The bridge is intentionally not a public arbitrary shell.

Live Settings dispatch through this bridge is currently quarantined. Smoke testing showed that firing GNUbg settings commands directly from the live Settings UI can crash or disturb match/session state. The bridge remains valuable, but command application must become lifecycle-safe before Settings rows are reconnected.

## Board themes

Board themes are Android-only presentation palettes.

Classic, Ocean, Forest, and System use the same board coordinate system, point geometry, checker positions, bar/bearoff geometry, hit testing, and canvas scaling. Theme selection must not alter legal move logic or relative positioning.

## Documentation policy

The canonical documentation lives at repository root under `docs/`.

The app module may have a small pointer README, but project-wide documentation should not live under `gnubg-app/docs/`, because the project spans the Android app, JNI bridge, engine core, and upstream source.
