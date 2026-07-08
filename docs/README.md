# Documentation index

Canonical project documentation for the GNU Backgammon Android port.

## Authoritative current state

The single source of truth for where the project is now is **`STATUS.md`**
(current version V0.9.1). Read it first. Everything else is reference,
history, or forward-looking design and defers to it.

## Living documents

- `STATUS.md` -- authoritative current state (V0.9.1). Start here.
- `MASTER_V0.9.md` -- deep engineering reference and full build history.
- `ARCHITECTURE.md` -- ownership boundaries (gnubg authoritative; Kotlin owns
  shell/presentation) and command-bridge policy.
- `TECHNICAL-NOTES.md` -- interaction model and invariants (submove testing,
  undo snapshots, cube path).
- `PHASE3_TUTOR_ANALYSIS.md` -- tutor analysis internals (the chequer tutor;
  canonical reference for the tutor pattern).
- `ROADMAP.md` -- milestone arc (V0.8.x app-shell line, V0.9 engine-port line,
  V0.9.1 consolidation) and forward plan.

## Forward-looking / design

- `SETTINGS-UX-BLUEPRINT.md` -- aspirational Settings UX design. Partially
  realised (the five-tab skeleton exists); the deep fine-tuning/exposure model
  is not built.
- `gnubg_mobile_tutor_mission_statement.tex` / `.pdf` -- product philosophy and
  vision for the mobile tutor (not an engineering spec).

## History (frozen snapshots, superseded by STATUS.md)

Under `history/`:

- `STATUS_V0.8.10.md` -- Home Hub scaffold milestone.
- `STATUS_V0.8.11.md` -- GNUbg lifecycle bridge milestone.
- `CHANGELOG-0.8.9.md` -- 0.8.9 changelog.
- `SETTINGS_GNUBG_MAPPING_V0.8.13_DRAFT.md` -- archived Settings/GNUbg command
  mapping draft.
- `KNOWN-LIMITATIONS.md` -- superseded by the "Known gaps" section of STATUS.md.

## Build

- Build instructions are in the top-level [`README.md`](../README.md#building).
  For local development, `./build_and_deploy.sh` at the repository root is a
  convenience wrapper around the Gradle build + install + launch.

## Documentation rule

Project-wide documentation lives here at repository root under `docs/`. The
Android module may keep a tiny pointer README, but should not have its own
project-level `docs/` directory, because the project spans the Android app, the
JNI bridge, the engine core, and the upstream source.

## Building documentation

From the repository root:

    make docs

To build only the mobile tutor mission statement PDF:

    make tutor-mission-pdf

The LaTeX source remains the editable source of truth for the mission
statement; the generated PDF is the public-facing rendered artifact.
