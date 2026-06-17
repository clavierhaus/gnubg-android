# GNU Backgammon for Android

A work-in-progress Android port of **GNU Backgammon**, built around one strict rule:

> **GNUbg is the authority.**  
> The Android layer may draw the board, route taps, and present convenience interactions, but game legality, move generation, cube decisions, pip counts, and match state must come from the GNU Backgammon engine.

This repository is interesting if you care about bringing mature desktop C software to Android without replacing the real engine with a simplified rewrite.

## Current milestone: 0.8.9

Version 0.8.9 is a playable development milestone. It combines the earlier JNI/engine work with a much more usable Android board:

- full match flow through the embedded GNUbg engine;
- human roll, move, commit, undo, and engine reply loop;
- source-point tapping for single submoves;
- destination-point convenience tapping for the common “move two checkers here” case;
- per-die undo instead of whole-turn undo;
- dice greying that reflects the actual remaining dice;
- explicit dice swap by tapping the dice area;
- doubling cube tap path wired through GNUbg cube evaluation and native match-state mutation;
- start screen with match length and provisional opponent-strength selector;
- board-relative layout work for phones and tablets with different aspect ratios.

This is not yet a polished Play Store application. It is a serious playable prototype and an engineering record of how the genuine GNUbg engine is being made usable on Android.

## Why this is not “just another backgammon app”

Many mobile backgammon projects reimplement the rules and use an engine only for hints or evaluations. This project intentionally avoids that route. The long-term goal is a real Android GNU Backgammon port, not a look-alike UI over partial game logic.

That design has practical consequences:

- **Move legality** comes from GNUbg move generation / submove application.
- **Cube decisions** come from GNUbg cube evaluation.
- **Pip counts** are computed by GNUbg.
- **Match state** is read from the native engine state.
- Kotlin-side code must remain UI and interaction glue, not a parallel backgammon rules engine.

## Gameplay status

The Android UI currently supports normal play against the GNUbg engine. Recent convenience features focus on making touch play less clumsy without inventing rules:

- tap a checker/source point to apply one legal die move;
- tap the dice area to swap die order deliberately;
- tap Undo to undo only the last die move;
- tap a destination point when two legal checkers can land there unambiguously;
- tap Commit to submit the complete move to GNUbg.

The board shows scores, dice, used dice, the doubling cube, pip counts, checkers on points and bar, and action buttons.

## Architecture overview

```
Android UI (Kotlin / Jetpack Compose)
        |
        | JNI calls
        v
JNI bridge (C)
        |
        | controlled access to match state and engine functions
        v
GNU Backgammon engine core
```

Important rule: the bridge exists to expose GNUbg behaviour to Android, not to replace it.

## Repository layout

The exact tree is still evolving, but the important areas are:

- `gnubg-app/` — Android application.
- `jni-bridge/` — native bridge and Android-facing C entry points.
- `engine-core/` or upstream/imported GNUbg sources — portable GNUbg engine code used by the bridge.
- `doc/` / `docs/` — milestone documentation and technical notes.

## Build notes

The current development build is an Android debug build targeting `arm64-v8a`. Native rebuilds are only needed when bridge or engine code changes; most UI work rebuilds only the app.

See [`docs/BUILD.md`](docs/BUILD.md) for the local development commands used during this milestone.

## Current limitations

The project is intentionally honest about unfinished areas:

- real difficulty/strength selection still needs to be wired into GNUbg evaluation settings;
- cube pass/drop and beaver UI paths are not complete;
- SGF import/export UI is not yet built;
- analysis and tutor surfaces are not yet built;
- visual polish and accessibility still need a dedicated pass;
- broader device testing is still required.

See [`docs/KNOWN-LIMITATIONS.md`](docs/KNOWN-LIMITATIONS.md).

## For developers

The most useful code to inspect first is the boundary between UI intent and engine authority:

- the Kotlin `GameViewModel` interaction paths;
- the JNI declarations in `Engine.kt`;
- the C bridge functions that read and mutate GNUbg match state;
- the board drawing and hit detection in `Board.kt`.

The interesting part is not that an Android board can be drawn. The interesting part is keeping touch convenience features while refusing to create a second, divergent backgammon engine in Kotlin.

## Documentation

- [`docs/CHANGELOG-0.8.9.md`](docs/CHANGELOG-0.8.9.md)
- [`docs/TECHNICAL-NOTES.md`](docs/TECHNICAL-NOTES.md)
- [`docs/KNOWN-LIMITATIONS.md`](docs/KNOWN-LIMITATIONS.md)
- [`docs/BUILD.md`](docs/BUILD.md)

The longer LaTeX master document remains the detailed engineering record.
