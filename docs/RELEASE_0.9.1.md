# GNU Backgammon Android 0.9.1 Milestone

## Status

0.9.1 is an architectural milestone.

It is not yet a Tutor Mode feature release. It establishes the boundaries,
state models, documentation, and shared UI infrastructure needed to build
Tutor Mode without contaminating regular Play Mode.

## Main Result

Regular Play and Tutor Mode are now explicitly separate modes.

Regular Play remains the normal GNU Backgammon game surface. Tutor Mode
is a distinct learning flow with its own entry point, session state, and
board-action path.

The two modes may share rendering and neutral models, but they must not
share mode-specific behaviour accidentally.

## Implemented in This Milestone

### Documentation

- Mobile Tutor mission statement added in LaTeX.
- PDF build target added through the root Makefile.
- Tutor Mode implementation plan added.
- De-Androidified architecture rule added.
- Explicit Play/Tutor mode-boundary rule added.

### Home Hub

- Refreshed photographic hub background.
- Larger, more readable hub typography.
- Separate `Tutor` hub entry added.

### Tutor Settings

A persisted Tutor settings subsection now exists.

It includes:

- Tutor Mode preset.
- Feedback threshold.
- Try Again preference.
- Board annotation mode.
- Equity detail preference.
- Cube Tutor preference.
- Rollout access preference.

These settings are local and inert for now. They do not change gameplay.

### Tutor Mode Foundation

Tutor Mode now has:

- a separate app route;
- its own screen;
- a neutral session controller;
- a neutral session state;
- a Coach Card prototype;
- a read-only board preview;
- Tutor-specific board actions;
- point selection and neutral lesson text.

### Shared Board Infrastructure

The board renderer no longer depends directly on `GameViewModel`.

Instead, board input is routed through a neutral `BoardActions` interface.
Regular Play supplies a Play adapter. Tutor Mode supplies a Tutor adapter.

This is the key shared-board milestone.

### Regular Play

Regular Play remains intentionally clean:

- no Tutor Card button;
- no Coach Card overlay;
- no Try Again behaviour;
- no Tutor-specific lesson flow;
- no Play-to-Tutor semantic coupling.

Manual testing confirmed normal Play behaviour after the refactors.

## Current Package Responsibilities

### `com.clavierhaus.gnubg.play`

Owns the shared board renderer and regular Play UI.

Important rule:

The board renderer may draw shared board state and dispatch abstract board
actions. It must not know about Tutor Mode or `GameViewModel`.

### `com.clavierhaus.gnubg.tutor`

Owns neutral Tutor models, session state, session controller, and prototype
lesson data.

Important rule:

Tutor reasoning belongs here or in a lower neutral/native layer, not in
Compose UI.

### `com.clavierhaus.gnubg.tutorui`

Owns the Android/Compose Tutor surface.

Important rule:

This package renders Tutor state and dispatches Tutor user actions. It
must not become the Tutor reasoning layer.

### `com.clavierhaus.gnubg.engine`

Currently owns regular game ViewModel orchestration and existing engine
bridge usage.

Important rule:

New Tutor logic must not be hidden here merely because the current Play
flow lives here. Use neutral Tutor/session layers or native/mobile facade
APIs as appropriate.

## Non-Goals of 0.9.1

0.9.1 does not implement:

- GNUbg move evaluation in Tutor Mode;
- live move judging;
- real best-move display;
- Try Again restoration;
- shot-count analysis;
- point-making analysis;
- cube tutoring;
- rollout UI;
- post-game review;
- curated lessons.

## Verified Behaviour

Manual testing covered:

- entering Tutor Mode from hub;
- Coach Card prototype display and dismissal;
- Tutor board preview;
- Tutor point selection and clearing;
- point-region lesson text;
- entering Play from hub;
- starting a 3-point match;
- rolling;
- moving;
- undo before commit;
- committing a move;
- unchanged Play behaviour after board-action extraction.

## Architectural Commitments

The following are now project rules:

1. Regular Play and Tutor Mode are separate user-facing modes.
2. Tutor Mode is not Regular Play with overlays.
3. Shared board rendering is allowed.
4. Shared neutral state models are allowed.
5. Shared engine/facade APIs are allowed.
6. Tutor-specific behaviour must stay in Tutor Mode.
7. Backgammon/tutor reasoning must not live in Compose UI.
8. Android code handles UI, persistence, lifecycle, and dispatch.
9. Native/mobile facade code handles GNUbg-facing facts and commands.
10. Neutral Tutor/game layers handle Tutor session semantics.

## Next Milestone Direction

The next milestone should make Tutor Mode data-driven before adding real
GNUbg evaluation.

Recommended next steps:

1. Introduce a neutral lesson concept model.
2. Move selected-point lesson text into lesson data.
3. Define a Tutor board lesson contract.
4. Add a first static board lesson.
5. Only then wire GNUbg evaluation through a narrow mobile facade API.
