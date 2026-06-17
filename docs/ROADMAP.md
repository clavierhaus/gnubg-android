# Roadmap

## Completed milestones

### V0.8.10 — Home Hub scaffold

Introduced the mode-based Home Hub shell.

Top-level areas:

- Play
- Learn
- Analyse
- Options
- Profile

Play remained the existing live game. Learn, Analyse, and Profile were placeholders. Options wrapped the Settings surface.

Historical note: `docs/STATUS_V0.8.10.md`.

### V0.8.11 — GNUbg lifecycle bridge groundwork

Exposed broader GNUbg lifecycle and command groundwork through JNI, Kotlin `Engine.kt`, and `GameViewModel`.

Prepared commands included:

- new game
- new match
- new session
- end game
- resign
- next
- accept
- reject
- decline
- agree
- redouble
- load/save game
- load/save match
- load/save position

Historical note: `docs/STATUS_V0.8.11.md`.

### V0.8.12 — Play lifecycle controls and long-press move hints

Added first visible Play lifecycle controls using GNUbg-backed bridge groundwork:

- Resign
- New game
- New match
- Home as Android shell navigation

Also added Android-native long-press checker move hints. Legal landing destinations are highlighted without changing GNUbg move authority.

### V0.8.13a — Restricted GNUbg settings command bridge

Added a restricted settings command bridge through JNI/Kotlin.

The bridge can call selected GNUbg command interpreter prefixes, but it is not exposed as an arbitrary command shell.

### V0.8.13b — GNUbg settings command grammar fixes

Audited command grammar and corrected known mappings:

- `set automatic doubles N` is valid
- `set jacoby on/off` is valid grammar
- `set beavers` is numeric, not on/off
- `set output mwc on/off` is valid
- `set analysis threshold doubtful/bad/verybad VALUE` is structurally valid

### V0.8.13c — Quarantine live Settings command dispatch

Smoke testing showed live Settings dispatch to GNUbg commands is unsafe from the current UI timing/state.

Crawford and Jacoby were returned to local-only behaviour. Settings became stable again.

### V0.8.14 — Grouped five-tab Settings surface

Rewrote Settings into a grouped configuration surface:

- Game
- Board
- Engine
- Analysis
- Expert

The Expert tab now houses future GNUbg bridge diagnostics and lifecycle-sensitive controls as disabled placeholders.

## Next candidate milestones

### V0.8.15 candidate A — Persist expanded Settings values

Persist more Android-side Settings values, not only board theme.

Candidates:

- match length
- Crawford local preference
- Jacoby local preference
- automatic doubles local value
- beavers local preference
- point numbers
- pip count
- difficulty
- tutor/hint local preferences
- analysis output preferences
- thresholds

### V0.8.15 candidate B — Lifecycle-safe GNUbg settings application path

Design and implement a safe path for applying GNUbg-backed settings.

Requirements:

- no direct live dispatch from arbitrary Settings recomposition/toggle moments
- apply at known lifecycle boundaries
- capture command result/output
- report failures without crashing
- keep risky match/player commands out until sequencing is verified

## Later work

- Expert diagnostics surface
- Read-only `show ...` command output capture
- Analysis mode
- Learn/Tutor mode
- Profile/defaults
- Library/import/export flows
- Friend evaluation APK feedback triage
