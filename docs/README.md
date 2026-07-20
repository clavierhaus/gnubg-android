# Documentation index

The GNU Backgammon Android port. This index is the map; read the three
starting points first, treat the rest as reference, and ignore `history/`
unless you're tracing why something is the way it is.

## Start here (in order)

1. **`ARCHITECTURE_FOR_CONTRIBUTORS.md`** — the four-layer stack (gnubg engine
   → C facade → JNI/Kotlin bridge → Compose UI), a real tap-to-command data
   flow, and how to add a feature. The single best entry point.
2. **`QUICKSTART.md`** — the user's view: the four modes and the non-obvious
   board gestures (the destination-tap stack move, the multi-hop long-press
   highlight, the coach two-tap compare). Read it to know what the app
   actually does before changing how it does it.
3. **`ARCHITECTURE.md`** — the one rule everything obeys: gnubg is the sole
   authority for game logic; Kotlin owns only shell and presentation. The
   command-bridge policy lives here.

## Reference (read when the task touches them)

- `TECHNICAL-NOTES.md` — interaction model and invariants: sub-move testing,
  undo snapshots, the cube path, the terminal-state (GAME_OVER) latch.
- `THREADING.md` / `MULTICORE_ANALYSIS.md` — the single engine thread, the
  projection contract, parallel analysis.
- `VERBOSE_COACHING_DESIGN.md` — the canonical, complete design of verbose
  coaching mode: the epistemic-honesty threshold, every discarded approach
  with its reason, the three explanatory layers, amendment 2's comparative
  rules, and the reproducible evidence-corpus method. Read this first before
  touching coach mode.
- `INSIGHT_JOURNEY_AND_ARCHITECTURE.md` — the shorter journey narrative and
  the runtime map; a companion to the design doc above.
- `INPUT_DICTIONARY.md` — the measured meaning and side of each gnubg neural
  input (I_*). The reference for any signature or delta work.
- `ARCHITECTURE_ANALYSE_MODE.md` — the Analyse-mode design.
- `SETTINGS-UX-BLUEPRINT.md` — settings surface and the gnubg option mapping.

## Coach content pipeline (public tooling; the baked assets are Plus)

- `CORPUS_HARVEST_PLAN.md` — the corpus pipeline and its two-tier doctrine.
- `DELTA_NARRATOR_PLAYBOOK.md` — the binding law for the deterministic
  fallback narrator.
- `CORPUS_ENTRIES_DRAFT.md`, `INSIGHT_COVERAGE.md` — the entries and what they
  cover. `COACH.md`, `COMPANION.md` — coach-mode notes.

## Release & distribution

- `FDROID_SUBMISSION.md` — the F-Droid recipe and reproducible-build notes.
- `RELEASE_SIGNING.md` — keystore and signing policy.
- The user-facing changelog is `../CHANGELOG.md`; per-release F-Droid notes
  live in `../fastlane/metadata/android/en-US/changelogs/`.

## history/

Superseded plans, completed visions, and point-in-time status/handover
snapshots. Kept for provenance (this is a GPL project); not part of the
current picture. Nothing here is authoritative — if it disagrees with the
docs above, the docs above win.
