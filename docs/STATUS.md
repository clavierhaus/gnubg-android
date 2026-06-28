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

## What works now

- **Live play** against the gnubg engine: full move/dice/undo/confirm,
  gnubg authoritative for all legality and match state.
- **Home Hub** start screen routing to Play / Learn / Analyse / Options / Profile.
- **Settings**: five grouped tabs (Game, Board, Engine, Analysis, Expert).
- **Cube**: human double via native gnubg cube decision on the real match board.
- **Engine port**: platform-neutral facade; native-lib reaches the engine
  directly in only two intentional places (board-changed callback, runCommand).
- **Tutor analysis** (Phase 13): after each human move, gnubg's own routines
  score the move and report blunder level, equity loss, and feature deltas.
  **Currently log-only** -- emitted to logcat, not yet surfaced in any UI.

## Scaffolded but not feature-complete

- Learn, Analyse, Profile mode screens exist as scaffolds, not finished modes.
- Settings tabs render and bind, but several rows are local-only pending a
  lifecycle-safe gnubg command path (see ARCHITECTURE.md, command bridge).

## Known gaps

- Cube pass/drop after a human double, and beaver handling, are incomplete.
- Tutor output has no UI surface (CoachCard, arrows, Try-Again loop) -- the
  mission-statement vision is not yet built.
- Tutor evaluates at 1-ply (the 2-ply/prune path returns inf in this build;
  see PHASE3_TUTOR_ANALYSIS.md).
- SGF/Library import-export UI not present.
- Opponent-strength selector is UI/state plumbing, not wired to gnubg eval settings.
- Release signing, Play Store readiness, broad device QA not done.

## Authority invariant (unchanged, load-bearing)

gnubg is the sole authority for rules, legality, cube logic, scoring,
analysis, and tutor semantics. Kotlin owns shell, rendering, touch, and
presentation only. It must never reimplement game logic.

## Document map

- STATUS.md (this file) -- current truth.
- MASTER_V0.9.md -- deep engineering reference and build history.
- ARCHITECTURE.md / TECHNICAL-NOTES.md -- ownership boundaries and invariants.
- PHASE3_TUTOR_ANALYSIS.md -- tutor analysis internals (canonical).
- ROADMAP.md -- forward plan.
- SETTINGS-UX-BLUEPRINT.md -- aspirational Settings design (partially realised).
- gnubg_mobile_tutor_mission_statement.{tex,pdf} -- product philosophy/vision.
- history/ -- frozen snapshots (V0.8.x status, changelog, settings-mapping draft,
  superseded known-limitations).
