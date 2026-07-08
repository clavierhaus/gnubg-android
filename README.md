# GNU Backgammon for Android

A faithful Android port of [GNU Backgammon](https://www.gnu.org/software/gnubg/) — the free, world-class backgammon engine. This is not a re-implementation or a stripped-down evaluator: it runs the actual gnubg engine underneath a modern touch interface, so the game logic, cube decisions, match handling, and analysis you get are gnubg's own.

The goal is a genuinely good, free, open, offline backgammon companion for Android — the kind of app a regular player keeps on their phone because the opponent is strong, the board is clean, and the analysis is real.

> **Status: pre-release (V0.9.1).** The core game is fully playable and stable. Analysis and online features are actively being built out — see [Roadmap](#roadmap).

## What it does today

- **Play full matches against the gnubg engine.** Match play to any length, with four strength levels (Beginner, Casual, Intermediate, Advanced). Every move the engine makes is gnubg's own decision.
- **Doubling cube.** Full cube play — offer, take, drop, redouble — decided by gnubg, not by app-side heuristics.
- **Tournament rules.** Crawford, Jacoby, automatic doubles, beavers, and cube on/off, plus a choice of **match equity table** (Kazaross-XG2, Woolsey, Jacobs & Trice, Snowie, and others — the canonical gnubg tables) for match-play cube decisions.
- **Live analysis / tutor.** See gnubg's own equity evaluation of positions as you play — the same numbers gnubg's desktop tutor produces.
- **Themed board and UI.** Three hand-tuned themes (Ocean, Classic, Forest) plus a System option that follows Material You dynamic colour. The whole interface themes together, not just the board.
- **Your settings stick.** Preferences persist across restarts.

Built for Android 12+ (minSdk 31), landscape, with a native touch board (tap or drag to move).

## Design principle: gnubg is the authority

The single rule this project is built around: **GNU Backgammon is the sole authority for all game logic and analysis.** The Android layer draws the board, translates taps into gnubg commands, and presents gnubg's output. It does not invent, re-rank, or second-guess a single backgammon decision. Where the port must diverge from upstream for the mobile context, every divergence is recorded in [`PROVENANCE.md`](PROVENANCE.md).

This is what makes the app trustworthy: the strength you play against and the analysis you learn from are gnubg's, verifiable against the upstream source in this repository.

## Building

Requirements:

- Android SDK with the NDK installed (the native engine is compiled with CMake via the NDK)
- JDK 17
- A device or emulator running Android 12+ (arm64-v8a)

A standard debug build compiles the engine and the app together:

    cd gnubg-app
    ./gradlew assembleDebug
    adb install -r app/build/outputs/apk/debug/app-debug.apk

The native engine (`engine-core/` + `jni-bridge/`) is built as part of the Gradle build through CMake — there is no separate engine compilation step for a normal build. A full clean of the native artifacts is only needed when engine or bridge code changes; ordinary Kotlin/UI work rebuilds incrementally.

After installing, a clean restart is:

    adb shell am force-stop com.clavierhaus.gnubg
    adb shell am start -n com.clavierhaus.gnubg/.MainActivity

## Repository layout

- `gnubg-app/` — the Android app (Kotlin, Jetpack Compose)
- `jni-bridge/` — the JNI/native bridge: a small facade over the engine plus the JNI bindings
- `engine-core/` — the GNU Backgammon engine core, compiled for Android
- `upstream-source/` — reference upstream GNU Backgammon source
- `docs/` — project documentation

## Documentation

- [`docs/STATUS.md`](docs/STATUS.md) — authoritative current-state document
- [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) — architecture and design philosophy
- [`docs/ARCHITECTURE_FOR_CONTRIBUTORS.md`](docs/ARCHITECTURE_FOR_CONTRIBUTORS.md) — the four-layer stack, real data-flow, and how to add a feature
- [`docs/ROADMAP.md`](docs/ROADMAP.md) — forward plans
- [`PROVENANCE.md`](PROVENANCE.md) — record of every divergence from upstream gnubg
- [`CLAUDE.md`](CLAUDE.md) — the one rule (gnubg is authoritative) in full

## Roadmap

The port is faithful and the core is solid; the frontier is analysis depth and online play:

- **Deeper analysis** — a ranked candidate-move list with per-move equities, a Performance Rating (the metric serious players track), and position entry.
- **Online play** — a modern client for [FIBS](http://www.fibs.com/), the long-running free backgammon server, with local gnubg analysis alongside.
- **Multi-core evaluation** for faster analysis and rollouts.

See [`docs/ROADMAP_ANALYSIS_PARITY.md`](docs/ROADMAP_ANALYSIS_PARITY.md) for the detailed analysis plan.

## Contributing

Contributions are welcome. The most useful starting point is [`docs/ARCHITECTURE_FOR_CONTRIBUTORS.md`](docs/ARCHITECTURE_FOR_CONTRIBUTORS.md), which traces exactly how a tap becomes a gnubg command and back. Please keep the guiding rule in mind: game and analysis logic belongs in gnubg, not in the app layer.

## License

This program is a modified derivative of **GNU Backgammon** and is licensed under the **GNU General Public License, version 3 or (at your option) any later version (GPL-3.0-or-later)**.

- Full license text: [`COPYING`](COPYING)
- Attribution, copyright holders, and modification notice: [`NOTICE`](NOTICE)
- Record of every divergence from upstream: [`PROVENANCE.md`](PROVENANCE.md)

GNU Backgammon is Copyright (C) the Free Software Foundation, Inc. and the GNU Backgammon AUTHORS; the per-file copyright notices in `engine-core/` retain the authoritative attribution. The Android front end, JNI bridge, and port integration are Copyright (C) 2025-2026 clavierhaus <gnubg@clavierhaus.at>.

This program comes with ABSOLUTELY NO WARRANTY. This is free software, and you are welcome to redistribute it under the conditions of the GPL. You have the right to the complete corresponding source code.
