# GNU Backgammon for Android

A faithful Android port of [GNU Backgammon](https://www.gnu.org/software/gnubg/) — the free, world-class backgammon engine. This is not a re-implementation or a stripped-down evaluator: the actual gnubg engine runs underneath a modern touch interface, so the strength you play against, the cube decisions, the match handling, and the analysis you learn from are gnubg's own — verifiable against the upstream source in this repository.

The aim is **the** comprehensive backgammon companion for Android: an app a serious player keeps on their phone because the opponent is world-class, the board is clean and precise on every screen, positions can be set up and analysed on the spot, and matches can be reviewed move by move — all offline, all free, all open source.

> **Status: pre-release (V0.9.x).** The core game is fully playable and stable, and the three companion features below are built and working. Online play and deeper analysis reporting are the active frontier — see [Roadmap](#roadmap).

## What it does

### Play

- **Full matches against the gnubg engine.** Match play to any length, four strength levels (Beginner, Casual, Intermediate, Advanced). Every move the engine makes is gnubg's own decision — no app-side heuristics.
- **The doubling cube, in full.** Offer, take, drop, redouble, and resign, all decided by gnubg. When the engine is losing it resigns; when it is winning it doubles, and it is gnubg that judges your take.
- **Tournament rules.** Crawford, Jacoby, automatic doubles, beavers, and cube on/off, plus a choice of **match equity table** (Kazaross-XG2, Woolsey, Jacobs & Trice, Snowie, and the other canonical gnubg tables) for match-play cube decisions.
- **Live tutor.** See gnubg's own equity evaluation of your positions as you play — the same numbers the desktop tutor produces.

### The three things a companion app is for

These are the features players reach for other apps to get. Each runs entirely on gnubg's own engine, and each is built and working:

- **Set up any position and analyse it.** Open the position editor, tap points and the bar to place checkers, tap the bear-off tray to clear the board, then set the dice, cube, score, match length, and who is on roll. With dice set you get gnubg's ranked chequer plays; with **no dice** you get gnubg's cube decision — double / take / drop with the equities behind it, exactly as gnubg's desktop edit mode treats a no-dice position. The encoded GNU BG ID is shown and can be copied out. This is what most people open XG Mobile for; here it is free, and it works on current Android.
- **Paste a position from anywhere.** Have a GNU BG ID or an XGID from a forum, a book, or another app? Paste it and gnubg installs and evaluates it — the same engine path a hand-built position takes.
- **Save the match as a file.** Write the whole match to a standard `.sgf` at any point, through the Android file picker — to review later on a bigger screen, to catalogue, or to open in desktop gnubg.
- **Review a match move by move.** Open a saved `.sgf` and step through it, game by game and move by move, on gnubg's own board. Navigation is gnubg's own game-record walk, not a re-derivation.

### Interface

- **One board, every device.** The board is drawn from a single geometry computed once from the screen size, so a tap lands exactly where the eye says it will — verified across aspect ratios from tablet (16:10) to tall phone (20:9), which sit on opposite sides of the point where the scaling flips sign. Nothing scrolls; what does not fit is made to fit.
- **Consistent navigation.** A settings gear sits top-left on every screen. "Home" always returns to the hub; "New match" always restarts with the same parameters. One word for each action, everywhere.
- **Themed throughout.** Three hand-tuned themes (Ocean, Classic, Forest) plus a System option that follows Material You dynamic colour. The whole interface themes together, and the hub carries the app's own serif identity.
- **Your settings stick.** Preferences persist across restarts.

Built for Android 12+ (minSdk 31), landscape, with a native touch board (tap or drag to move).

## Design principle: gnubg is the authority

The single rule this project is built around: **GNU Backgammon is the sole authority for all game logic and analysis.** The Android layer draws the board, translates taps into gnubg commands, and presents gnubg's output. It does not invent, re-rank, or second-guess a single backgammon decision — not a move ranking, not a cube verdict, not a legal-move list, not a resignation. A position you build in the editor is encoded by gnubg's own encoders and installed through the same validated path a pasted ID takes; a cube verdict is gnubg's own `GetCubeRecommendation`, not a Kotlin mapping of it.

Where the port must diverge from upstream for the mobile context, every divergence is recorded in [`PROVENANCE.md`](PROVENANCE.md).

This is what makes the app trustworthy: the strength you play against and the analysis you learn from are gnubg's, checkable against the upstream source shipped in this repository.

## Building

Requirements:

- Android SDK with the NDK installed (the native engine is compiled with CMake via the NDK)
- JDK 17
- A device or emulator running Android 12+ (arm64-v8a)

A standard debug build compiles the engine and the app together:

    cd gnubg-app
    ./gradlew assembleDebug
    adb install -r app/build/outputs/apk/debug/app-debug.apk

The native engine (`engine-core/` + `jni-bridge/`) is built as part of the Gradle build through CMake — there is no separate engine compilation step. The repository builds from a clean clone with no submodule initialisation: every source the native build needs is tracked here.

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
- [`docs/ARCHITECTURE_ANALYSE_MODE.md`](docs/ARCHITECTURE_ANALYSE_MODE.md) — design and outcome of position entry, match save, and review
- [`docs/TECHNICAL-NOTES.md`](docs/TECHNICAL-NOTES.md) — the traps this port hit, each verified against the engine source
- [`docs/ROADMAP.md`](docs/ROADMAP.md) — forward plans

## Roadmap

The three companion features are built; the core is solid. The frontier now:

- **Analysis reporting** — a per-move verdict inside match review (gnubg's ranking of the alternatives against the move actually played), a jump-to-blunder move list, and a Performance Rating for a whole match.
- **Review while playing** — stepping back through the live game record in place, not only from a saved file.
- **Online play** — a modern client for [FIBS](http://www.fibs.com/), the long-running free backgammon server, with local gnubg analysis alongside.
- **Multi-core evaluation** for faster analysis and rollouts.

See [`docs/ROADMAP_ANALYSIS_PARITY.md`](docs/ROADMAP_ANALYSIS_PARITY.md) for the detailed analysis plan.

## Comparison

Two apps cover this ground for most players today. Where this port stands, by their own published feature lists and documentation:

- **XG Mobile** has a position editor — the feature it is chiefly used for — but its editor is widely described as fiddly, and the app has become hard to install on current Android. This port offers position entry that starts from an empty board (the gnubg way: place what you have, rather than clear a full board first), is free, and targets Android 12+.
- **Backgammon NJ** displays a position's GNU BG ID, top moves, and cube decision during play, and its paid analysis package steps through matches — but it has no position editor at all. This port sets up positions *and* reads pasted GNU BG IDs, so a BGNJ user's IDs drop straight in.

Neither is a criticism of strong apps; the point is that this one is free, open, offline, and covers position setup, match save, and match review together, on gnubg's own engine.

## Contributing

Contributions are welcome. The most useful starting point is [`docs/ARCHITECTURE_FOR_CONTRIBUTORS.md`](docs/ARCHITECTURE_FOR_CONTRIBUTORS.md), which traces exactly how a tap becomes a gnubg command and back. Please keep the guiding rule in mind: game and analysis logic belongs in gnubg, not in the app layer. If a piece of Kotlin computes, ranks, or classifies a backgammon decision, it is in the wrong place.

## License

This program is a modified derivative of **GNU Backgammon** and is licensed under the **GNU General Public License, version 3 or (at your option) any later version (GPL-3.0-or-later)**.

- Full license text: [`COPYING`](COPYING)
- Attribution, copyright holders, and modification notice: [`NOTICE`](NOTICE)
- Record of every divergence from upstream: [`PROVENANCE.md`](PROVENANCE.md)

GNU Backgammon is Copyright (C) the Free Software Foundation, Inc. and the GNU Backgammon AUTHORS; the per-file copyright notices in `engine-core/` retain the authoritative attribution. The Android front end, JNI bridge, and port integration are Copyright (C) 2025-2026 clavierhaus <gnubg@clavierhaus.at>.

This program comes with ABSOLUTELY NO WARRANTY. This is free software, and you are welcome to redistribute it under the conditions of the GPL. You have the right to the complete corresponding source code.
