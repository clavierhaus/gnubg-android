# Architecture for Contributors

This document explains how gnubg-android is put together and how to add to it
without breaking its central rule. It complements the higher-level policy notes
in `ARCHITECTURE.md` (which covers the GNUbg-authoritative philosophy and the
settings-vs-actions boundary) and the divergence record in `../PROVENANCE.md`.
If you read only one thing first, read the "One rule" section below.


## The one rule

**GNU Backgammon is the sole authority for all game logic AND all analysis.**
This is a port, not a reimplementation. The vendored C engine in `engine-core/`
already implements every rule, move, cube decision, score, and analysis. A
contributor's job is to expose and wire that logic -- never to re-implement
backgammon semantics in Kotlin or in new C.

"Game logic" is not the only forbidden territory. gnubg also computes, inside
its own evaluation, everything a position or move *means*: shot counts, primes,
blots, anchors, board strength, position class, race vs contact, blunder
severity. Re-deriving any of that in Kotlin is reinvention, even when no rule is
being decided. The full statement lives in `CLAUDE.md` at the repo root.

The one sanctioned exception is **drawing**: Kotlin may loop over the board to
place checker sprites and to hit-test taps. Those loops compute pixel positions,
not board facts.


## The four layers

A user action travels through four layers, each thin:

```
  Compose UI  (play/, options/, hub/, ...)     Kotlin -- rendering + gestures
      |
  GameViewModel / Engine.kt                    Kotlin -- state + external decls
      |   JNI boundary
  native-lib.c                                 C -- JNI marshalling only
      |
  gnubg_mobile.c  (the "facade", ~47 verbs)    C -- thin wrappers over gnubg
      |
  engine-core/*.c  (vendored GNU Backgammon)   C -- the authority
```

1. **`engine-core/`** -- vendored gnubg (27 `.c` files), copied near-verbatim.
   Divergences are limited to a handful of documented "seams" (see
   `PROVENANCE.md`): visibility wrappers that expose an internal function or
   gate one static flag. Logic is never changed here.

2. **`jni-bridge/src/gnubg_mobile.c`** -- the facade. ~47 verbs
   (`gnubg_mobile_*`), each a thin wrapper: `command_*` verbs call gnubg's
   `Command*` functions; `get_*` verbs read match state; a few
   (`tutor_analyze`, `analyze_played_move`) run gnubg's own analysis and hand
   back the raw numbers. If you find yourself writing a *loop that reasons about
   the board* in this file, stop -- find the gnubg function that already does it.

3. **`jni-bridge/src/native-lib.c`** -- JNI marshalling only. Converts Kotlin
   arrays/strings to C and back. No logic.

4. **Kotlin** (`gnubg-app/app/src/main/kotlin/com/clavierhaus/gnubg/`):
   - `Engine.kt` -- the `external fun` declarations that bind to the JNI layer.
     This is the Kotlin face of the facade.
   - `engine/GameViewModel.kt` -- holds UI state (`BoardState`) and turns user
     actions into `Engine` calls. (Large; see "Known rough edges".)
   - `engine/BoardState.kt` -- the immutable state the UI renders.
   - `play/` -- the board Composable, gesture handling, rendering.
   - `options/`, `hub/`, `analyse/`, `profile/`, `learn/`, `tutor/` -- other
     screens and helpers.


## Data flow: one real path

A human confirming a checker move traces the whole stack:

```
GameViewModel.confirm()
  -> Engine.applyMoveString(moveStr)          // Engine.kt external
     -> Java_..._applyMoveString              // native-lib.c (JNI)
        -> gnubg_mobile_command_move(move)    // gnubg_mobile.c (facade)
           -> CommandMove(move)               // engine-core (gnubg itself)
```

gnubg's `CommandMove` validates and applies the move, advances match state, and
(via the facade's drain of the turn loop) lets the engine take its turn. The
Kotlin side then reads the new state back through `get_*` verbs and rebuilds
`BoardState`. At no point does Kotlin decide whether the move was legal -- gnubg
does, and an illegal or non-maximal move comes back rejected.

The reverse direction (engine -> UI) is always: facade `get_*` verb reads
gnubg's match state -> JNI marshals it -> `GameViewModel.readMatchState()`
assembles a fresh `BoardState` -> Compose recomposes.


## How to add a feature the right way

1. **Find the gnubg function first.** Before writing any logic that decides,
   generates, evaluates, scores, ranks, or describes a position or move, locate
   the gnubg function that already does it (grep `engine-core/`). If it exists,
   your job is to expose it, not to reproduce it.

2. **Add a thin facade verb** in `gnubg_mobile.c` that calls that function and
   returns the raw result. Hold `gnubg_lock` around engine calls. Keep it a
   wrapper -- no board reasoning.

3. **Bind it** through `native-lib.c` (JNI) and `Engine.kt` (external fun).

4. **Consume it** in `GameViewModel` / the relevant Composable. Any per-pixel
   drawing or tap hit-testing is fine here; anything that computes a *board
   fact* is not.

5. **Record divergences.** If you had to add or change anything in
   `engine-core/` (a new seam), document it in `PROVENANCE.md` with the exact
   code and rationale. The modification record must always match the code.


## Known rough edges (honest state)

These are documented so contributors aren't surprised; several are tracked in
`CODE_AUDIT_2026-07.md`.

- **`GameViewModel.kt` is large** (~1000 lines) and mixes board interaction,
  turn flow, command passthroughs, settings setters, and persistence. Extracting
  the mechanical groups (settings, commands, persistence) is planned; the
  interaction core (tap/drag/confirm) is the delicate part to leave alone.
- **The live-settings command bridge is quarantined.** A restricted GNUbg
  command bridge exists, but firing `set ...` commands from the live Settings UI
  can disturb match/session state, so Settings rows are not yet reconnected to
  it. Command application must become lifecycle-safe first. (See
  `ARCHITECTURE.md`.)
- **Analysis is single-threaded.** The 2-ply tutor/analysis runs on one engine
  thread; it is decoupled from the turn so the UI never blocks, but the compute
  itself is not yet parallelised. A design for multi-core analysis (and an
  upstream contribution path) is in `MULTICORE_ANALYSIS.md`.


## Where to look next

- `CLAUDE.md` -- the full statement of the one rule.
- `PROVENANCE.md` -- every divergence from upstream gnubg, with rationale.
- `ARCHITECTURE.md` -- philosophy, settings-vs-actions, themes, doc policy.
- `CODE_AUDIT_2026-07.md` -- current known reinventions/dead code/tangles.
- `TUTOR_VISION.md` -- where the tutor/analysis work is heading.
- `MULTICORE_ANALYSIS.md` -- the analysis-parallelisation design.
