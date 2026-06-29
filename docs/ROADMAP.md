# Roadmap

Current version: V0.9.1 (consolidation). Authoritative current-state document:
`docs/STATUS.md`. Deep engineering history: `docs/MASTER_V0.9.md`. This roadmap
records the milestone arc and what is next; it is not the source of truth for
current state.

## Completed milestones

### App-shell line (V0.8.x)

The V0.8.x series built the Android application shell on top of the engine
bridge. Frozen per-version snapshots live in `docs/history/`.

- **V0.8.10 -- Home Hub scaffold.** Mode-based shell: Play / Learn / Analyse /
  Options / Profile. Play was the existing live game; Learn, Analyse, Profile
  were placeholders; Options wrapped Settings. (`docs/history/STATUS_V0.8.10.md`)
- **V0.8.11 -- GNUbg lifecycle bridge groundwork.** Exposed broader GNUbg
  lifecycle/command surface through JNI, `Engine.kt`, and `GameViewModel`
  (new game/match/session, end game, resign, accept/reject/agree, redouble,
  load/save game/match/position). (`docs/history/STATUS_V0.8.11.md`)
- **V0.8.12 -- Play lifecycle controls + long-press move hints.** First visible
  Play lifecycle controls (Resign, New game, New match, Home navigation) plus
  Android-native long-press checker hints highlighting legal landings, without
  changing GNUbg move authority.
- **V0.8.13a/b/c -- Restricted settings command bridge.** Added a restricted
  GNUbg command bridge (selected `set ...`/`show ...` prefixes, not an arbitrary
  shell); audited command grammar; then quarantined live Settings dispatch after
  smoke testing showed it unsafe from live UI timing. Crawford/Jacoby returned to
  local-only.
- **V0.8.14 -- Grouped five-tab Settings.** Settings rewritten into grouped tabs
  (Game, Board, Engine, Analysis, Expert); Expert tab reserved for future
  diagnostics and lifecycle-sensitive controls.

### Engine-port line (V0.9, Phases 10-13)

The V0.9 line continued on top of the V0.8.x shell, porting the gnubg engine
behind a platform-neutral facade. Detailed narrative in `docs/MASTER_V0.9.md`.

- **Phase 10 -- Facade and de-Androidification.** Three-tier split: Kotlin UI ->
  Engine.kt/JNI -> native-lib.c (marshalling only) -> platform-neutral C facade
  (android-app.c + gnubg_mobile.*) -> engine-core (vendored gnubg, the
  authority). Boards cross as flat int[50]; facade self-locks.
- **Phase 11 -- Cube routed through the engine.** Live cube decisions driven by
  gnubg's own cube path; the `ms` hand-poke retired.
- **Phase 12 -- Repository completeness and provenance.** Build-complete from a
  clean clone; `PROVENANCE.md` records every engine-core divergence.
- **Tier B/C -- engine-symbol reduction.** `initialise` extracted into
  `gnubg_mobile_initialise`; board/dice readers routed through facade getters.
  native-lib.c now reaches the engine directly in only two intentional places.
- **Phase 13 -- Tutor analysis (chequer play).** After each human move, gnubg's
  own routines (FixMatchState + ApplyMoveRecord replay, FindnSaveBestMoves) score
  the move and report blunder level, equity loss, and feature deltas. LOG-ONLY
  (tag gnubg-tutor); no UI surface. Currently 1-ply (fac_ec_default); the
  2-ply/prune path returns inf in this build. (`docs/PHASE3_TUTOR_ANALYSIS.md`)

### V0.9.1 -- consolidation (current)

- Reconciled the two lines into a single documented state (`docs/STATUS.md`);
  archived V0.8.x snapshots to `docs/history/`.
- Restored the repository to a clean, build-verified history; tracked the
  previously-untracked `engine-core/glib-ext.{c,h}` (miscategorised by
  `.gitignore`) so the native build is reproducible from a clean clone.
- **Engine strength wired to gnubg presets.** `gnubg_mobile_set_engine_strength`
  copies a gnubg `aecSettings` preset into the engine's chequer eval context.
  The selector now uses gnubg's four real named levels
  (Beginner / Casual play / Intermediate / Advanced = SETTINGS_* 0..3); the
  invented Expert/Master are gone. Applied at init and on selection.
- **Long-press landing hints upgraded.** `landingPointsForSource` now enumerates
  all reachable landings via gnubg partial move generation
  (getLegalMoves fPartial=1) + BFS over the sub-move chain, covering direct,
  double, and combined-move destinations.
- **Tournament-testing defaults.** Match length default 3; Settings opens on the
  Expert tab.

## Next

### In-progress: tournament UI polish

- **Cube/resign feedback.** Brief toast on cube and resign actions (who
  doubled/resigned, who took/passed). Deferred from the current tournament-UI
  branch.
- **Bar-dance Continue.** When a checker on the bar cannot re-enter, show the
  roll grayed and hand the turn to the engine via an explicit Continue button.
  Requires a GamePhase.HUMAN_DANCED state; rollDice currently leaves the player
  stuck in HUMAN_MOVING with no legal moves (a real dead-end bug).

### Tutor: from log to coach

- **2-ply / prune evaluation.** Investigate why the 2-ply/prune path returns inf
  here so tutor strength matches desktop gnubg; 1-ply (fac_ec_default) is the
  only proven-scoring route today.
- **Cube-decision analysis.** Extend the tutor beyond chequer play to the cube
  path (esAnalysisCube).
- **Coaching UI (mission-statement vision).** PhraseLibrary (static GPL-3
  coaching assets), CoachCard, best-move arrows, Try-Again loop, progressive
  disclosure. See `docs/gnubg_mobile_tutor_mission_statement.pdf`.

### Settings: make the surface real

- **Persist expanded Settings values** (beyond board theme): match length,
  Crawford/Jacoby/automatic-doubles/beavers local preferences, point numbers,
  pip count, difficulty, tutor/hint, analysis output, thresholds.
- **Lifecycle-safe GNUbg settings application path.** Apply GNUbg-backed
  settings only at known lifecycle boundaries (never from live recomposition);
  capture command result/output; fail without crashing; keep risky match/player
  commands out until sequencing is verified.
- **Expert diagnostics surface.** Read-only `show ...` command output capture;
  native engine-state inspection.

### Gameplay completeness

- **Cube pass/drop after a human double**, and **beaver UI/path** (engine
  computes beaver decisions; no UI yet -- they currently collapse to take).
- **Fix blockedDiceFor** (computes s0/s1 but returns an always-empty set).
- **SGF / Library import-export UI.**

### Platform reach

- **iOS adapter.** Engine.swift mirroring Engine.kt over the existing facade;
  GameViewModel -> ObservableObject; SwiftUI board. No facade or engine changes
  required -- the point of the architecture.
- **Callback vtable** (gnubg_mobile_set_host_callbacks) for full iOS-readiness.
- **Rename android-app.c -> mobile-app.c** (already platform-neutral; last
  Android-ism in the C layer).

### Release engineering

- Release signing, Play Store readiness, broad device QA.
- Build-script doc reconciliation (the two MASTER 7.6 sections still describe the
  retired build.sh/run.sh; the real entry point is build_and_deploy.sh).
- Untrack libgnubg-engine.so (git rm --cached + .gitignore) so it stops dirtying
  the tree.
