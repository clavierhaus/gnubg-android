# Documentation index

This directory contains the canonical project documentation for the GNU Backgammon Android port.

## Current documents

- `ROADMAP.md` — milestone history and next candidate work
- `ARCHITECTURE.md` — architecture, ownership boundaries, and command-bridge policy
- `BUILD.md` — build notes
- `KNOWN-LIMITATIONS.md` — known limitations
- `TECHNICAL-NOTES.md` — technical notes
- `SETTINGS-UX-BLUEPRINT.md` — Settings UX blueprint and earlier planning
- `STATUS_V0.8.10.md` — Home Hub scaffold milestone
- `STATUS_V0.8.11.md` — GNUbg lifecycle bridge milestone
- `SETTINGS_GNUBG_MAPPING_V0.8.13_DRAFT.md` — archived Settings/GNUbg command mapping draft

## Current source of truth

Current internal milestone: **V0.8.14**

Settings now uses five grouped tabs:

- Game
- Board
- Engine
- Analysis
- Expert

The restricted GNUbg command bridge exists, but live Settings command dispatch is quarantined until a lifecycle-safe application path is implemented.

## Documentation rule

Project-wide documentation belongs here at repository root under `docs/`.

The Android module may keep a tiny pointer README, but it should not have its own project-level `docs/` directory.

## Product and design

- [GNU Backgammon Mobile Tutor: Mission Statement and Product Philosophy](gnubg_mobile_tutor_mission_statement.tex)
