# GNU Backgammon for Android — Status (V0.9.1)

**Authoritative current-state document.** Everything else defers to this.
For deep engineering history see MASTER_V0.9.md; for forward plans see ROADMAP.md.

## What V0.9.1 is

A consolidation checkpoint. It reconciles two development streams that the
codebase already unifies but the docs never did:

- the **app-shell line** (V0.8.x): Home Hub start screen, five-tab Settings,
  native-backed cube interaction;
- the **engine-port line** (V0.9, Phases 10-13): platform-neutral facade,
  de-Androidification, and gnubg-native tutor analysis.

V0.9 continued the engine work on top of the V0.8.x shell. The only V0.8.x
stream not carried forward is the deep BMPCC-inspired Settings UX vision,
which remains design intent (see SETTINGS-UX-BLUEPRINT.md), not built.

build versionName: 0.9.1 · last engine milestone: Phase 13 (tutor analysis)

## The three features that define current work

All current feature work traces to one piece of user feedback, which named the
three things missing from every Android backgammon app:

1. **Set up an arbitrary position and have gnubg evaluate it.** Reported as the
   reason people still use XG Mobile. A competitor's developer has stated he
   will not build it. -- **BUILT** (Analyse Position).
2. **Save the match file afterwards**, to review on a larger screen or catalogue.
   -- **BUILT.** "Save match" in the in-game panel writes gnubg's native `.sgf`
   through `CommandSaveMatch` and hands it to the Storage Access Framework, so
   the user chooses the destination (Downloads, Drive, anywhere).
3. **Step through a match afterwards, or during play.** -- **BUILT, first form**
   (Review Match). Open a saved `.sgf` and walk it move by move and game by game,
   on gnubg's own board. gnubg navigates its game record with `CommandNext` and
   `CommandPrevious`; the screen keeps no cursor of its own. No per-move verdict
   yet -- see Known gaps.

All three are now built. The design is recorded in `ARCHITECTURE_ANALYSE_MODE.md`.
Nothing outside these three should be started before they are finished.

## Recent additions (post-0.9.1, July 2026)

Work toward a first public release. All landed on the working branch:

- **Match Equity Table selection.** Eight canonical gnubg METs (Kazaross-XG2
  default, Rockwell-Kazaross, Woolsey, Jacobs & Trice, Snowie, GNUbg-11, MEC26,
  Zadeh) bundled as assets and loaded via a new `gnubg_mobile_set_met` verb
  wrapping gnubg's own `InitMatchEquity`. Selectable in the Tournament tab.
- **Full UI theming.** The whole interface (rails, panels, buttons, tabs, text,
  match-setup, settings) now follows the selected theme via a `LocalBoardPalette`
  CompositionLocal, not just the board. Ocean / Classic / Forest.
- **Settings persistence.** All settings survive an app restart (previously only
  board theme did), via a single settings-object DataStore round-trip.
- **Tournament settings tab.** Renamed from "Game"; cube on/off wired through the
  safe `CommandSetCubeUse`; dead placeholder rows removed; honest subtitles.
  Settings now opens on this tab, not the license/Expert tab.
- **Crash fix.** Restored a `getLegalMoves` JNI binding accidentally removed by a
  July-4 refactor; surfaced on the first clean rebuild since. See PROVENANCE.

## What works now

- **Live play** against the gnubg engine: full move/dice/undo/confirm,
  gnubg authoritative for all legality and match state.
- **Home Hub**: Play Tournament Match -> Analyse Position -> Options, with
  Profile in the corner. "Live Game Analysis" is gone as a hub entry: it was a
  way of playing occupying a slot in a menu of places. The tutor is now a
  match-setup option backed by the persisted `settings.tutorMode`, which until
  July 2026 was a switch no screen read.
- **Settings**: five grouped tabs (Tournament, Board, Engine, Analysis, Expert).
- **Cube (V0.9.x audit ongoing)**: human cube decisions route through
  gnubg's `GetMatchStateCubeInfo` against the live `ms`, and the Kotlin
  `cubedecision` enum mapping mirrors `engine-core/eval.h` (21 values).
  Cube evaluation depth (cubeful eval, V1) and engine-side proactive
  cube (V4 strength wiring) remain open audit items -- see
  `MASTER_V0.9.md` Phase 11.1.
- **Engine port**: platform-neutral facade; native-lib reaches the engine
  directly in only two intentional places (board-changed callback, runCommand).
- **Tutor analysis** (Phase 13): after each human move, gnubg's own routines
  score the move and report blunder level and equity loss. Surfaced in
  TutorAnalysisPanel when tutor mode is on. It runs at fixed 2-ply through the
  named instance `esAnalysisChequer.ec`, independent of opponent strength
  (32a7c91). It is honestly a **chequer-play** tutor: the tutored game is a
  single game, where the cube is out of play by gnubg's own rule
  (`gnubg_can_double`, play.c:156), so there are no cube decisions to comment on.
- **Save match**: writes the whole match so far as gnubg's native `.sgf` via
  `CommandSaveMatch`, then copies it to a user-chosen destination through the
  Storage Access Framework. Opens in gnubg desktop and Backgammon Studio. This
  is feature [2]. Success is verified (the file exists and is non-empty), because
  `FACADE_FILE_OP` reports success unconditionally.
- **Review Match**: open a saved `.sgf` through the Storage Access Framework and
  step through it. Navigation is gnubg's own `CommandNext` / `CommandPrevious`
  over its game record; the board is read-only (`viewModel = null`). This is
  feature [3].
- **Analyse Position**: paste a GNU BG ID or an XGID; gnubg installs it via its
  own `SetGNUbgID` and ranks the chequer plays with `FindnSaveBestMoves`. The
  match context (length, score, cube and owner, Crawford, who is on roll) is
  displayed every time, because a bare Position ID inherits whatever context the
  matchstate already held. This is feature [1] of the three requested.

## Scaffolded but not feature-complete

- Learn and Profile exist as scaffolds. `AppMode.LEARN` is not reachable from
  the hub. Analyse Position and Review Match are no longer scaffolds; both are
  built, and Review Match took the third hub slot when it existed, not before.
- Settings tabs render and bind, but several rows are local-only pending a
  lifecycle-safe gnubg command path (see ARCHITECTURE.md, command bridge).

## Known gaps

- Cube pass/drop after a human double, and beaver handling, are incomplete.
- Resignations offered BY GNU are now handled (accept / play on). GNU offers them
  itself once the position is lost; before this, gnubg refused every subsequent
  roll and the game could not finish.
- The full tutor vision (CoachCard, arrows, Try-Again loop) is not built.
  TutorAnalysisPanel is the current, minimal surface.
- **Review Match [3] is a first version.** It navigates and displays; it does not
  yet show the verdict. `hint_moves` (gnubg's ranking of the alternatives) and
  `analyze_played_move` (the equity the player gave up) both exist and are used by
  the tutor; the screen does not call them yet. No move list, no jumping to
  marked moves.
- **Review Match discards a game in progress.** `Engine.loadMatch` replaces the
  engine's match. The screen warns in a caption; it should be a confirmation.
- The hub column uses `Modifier.offset(x = 64.dp)`, which CLAUDE.md forbids for
  interactive elements: the drawing moves, the layout slot does not, so the tap
  targets sit 64dp left of their labels. Pre-existing.
- In tutor mode the whole `PlayLifecyclePanel` is replaced by
  `TutorAnalysisPanel`, so Resign, New game, New match, Home and Save match are
  all unreachable while the tutor is on. Pre-existing; not addressed by the save
  work.
- Release signing, Play Store readiness, broad device QA not done.

### Corrections to earlier editions of this document

- It claimed the tutor evaluates at 1-ply via `fac_ec_default`. That symbol no
  longer exists; the tutor has run at 2-ply since 32a7c91.
- It claimed the tutor has no UI surface, two paragraphs after stating that it is
  surfaced in TutorAnalysisPanel. The panel exists.
- It claimed the opponent-strength selector is "not wired to gnubg eval
  settings". It is: `Engine.setEngineStrength` is applied at engine init and on
  every change (GameViewModel.kt:49, 1042).

## Authority invariant (unchanged, load-bearing)

gnubg is the sole authority for rules, legality, cube logic, scoring,
analysis, and tutor semantics. Kotlin owns shell, rendering, touch, and
presentation only. It must never reimplement game logic.

## Document map

- STATUS.md (this file) -- current truth.
- MASTER_V0.9.md -- deep engineering reference and build history.
- ARCHITECTURE.md / TECHNICAL-NOTES.md -- ownership boundaries and invariants.
- PHASE3_TUTOR_ANALYSIS.md -- tutor analysis internals (canonical).
- ARCHITECTURE_ANALYSE_MODE.md -- design for the three requested features, and
  the verified risk analysis behind Analyse Position. Records its own corrections.
- ROADMAP.md -- forward plan.
- SETTINGS-UX-BLUEPRINT.md -- aspirational Settings design (partially realised).
- gnubg_mobile_tutor_mission_statement.{tex,pdf} -- product philosophy/vision.
- history/ -- frozen snapshots (V0.8.x status, changelog, settings-mapping draft,
  superseded known-limitations).
