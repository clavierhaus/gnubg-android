# Settings backlog — rows removed from the UI 2026-07-14

Every "Later" placeholder row was removed from the Settings screen before the
first F-Droid release: a wall of greyed promises reads as unfinished. Nothing
is lost — each row is recorded here verbatim and returns to the UI only when
actually wired. The Expert tab was removed whole; a License tab (About block +
the full GPL v3 text from the bundled COPYING asset) took its place.

## Board — Interaction (section removed empty)
- Destination-stack tap helper — Android-only candidate
- Dice swap gesture — Currently available on the board surface
- Board orientation — Future display preference

## Board — Board information
- Move landing hints — Currently always available by long-press

## Board — Accessibility / display (section removed empty)
- Larger point numbers — Android-only candidate
- High contrast checkers — Android-only candidate
- Animation speed — Android-only unless safely mapped

## Engine — Evaluation behaviour (section removed empty)
- Evaluation depth — GNUbg pendant: set evaluation plies ...
- Move filter — GNUbg pendant: set evaluation movefilter ...
- Cube decision strength — GNUbg pendant: set player ... cubedecision ...
- Plies / search depth — Will be lifecycle-safe before activation

## Engine — Rollout (section removed empty)
- Rollout trials — GNUbg pendant: set rollout trials ...
- Variance reduction / JSD — GNUbg pendant: set rollout varredn/jsd ...
- Deterministic test mode — GNUbg evaluation pendant to be audited
- Rollout seed — GNUbg pendant: set rollout seed ...

## Analysis — Tutor
- Warn before bad move — GNUbg pendant: set warning / set tutor skill
- Explain move choice — Future analysis output surface

## Analysis — Output
- Show cube action — Future analysis output control
- Show best move — Future analysis output control
- Show alternatives — Future analysis output control

## Expert tab (removed whole; the command-bridge cockpit returns as a feature)
GNUbg command bridge: Restricted command bridge (implemented, not fired from
live Settings); Lifecycle-safe dispatch; Capture GNUbg output.
Command grammar / diagnostics: Show command allowlist; Show current GNUbg
settings; Dry-run settings command; Command result log.
Advanced engine configuration: Raw evaluation plies; Player chequerplay
settings; Player cube-decision settings; Rollout seed / RNG; Deterministic
evaluation.
Experimental / unsafe: Direct set command execution (intentionally disabled);
Match/session command timing; Player setup timing.

Prerequisite for most Engine/Expert rows: the lifecycle-safe command bridge
(the "Settings as analysis cockpit" plan, discussed 2026-07-13).
