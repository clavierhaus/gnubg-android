# Tutor Mode Architecture

## Purpose

Tutor Mode is a separate learning surface for GNU Backgammon Android.

It shares infrastructure with Regular Play where that is safe, but it
does not share Regular Play semantics.

The purpose of this document is to prevent accidental re-coupling.

## The Core Boundary

Regular Play is for playing backgammon normally.

Tutor Mode is for learning.

These are not the same product mode.

They may share:

- board rendering;
- board state data structures;
- visual components;
- engine/facade APIs;
- neutral analysis models;
- preferences.

They must not share:

- mode flow;
- user interruption semantics;
- Try Again behaviour;
- Coach Card timing;
- guided lesson behaviour;
- tutor explanation generation.

## Layer Ownership

### Android / Compose Layer

Examples:

- `tutorui/TutorModeScreen.kt`
- `play/GameLayout.kt`
- `play/TutorCoachCard.kt`

Allowed responsibilities:

- render state;
- collect state;
- dispatch user actions;
- handle local screen layout;
- handle Android navigation and lifecycle.

Forbidden responsibilities:

- determine whether a move is good or bad;
- decide the main tutor reason;
- compute shots;
- compute point-making meaning;
- interpret cube decisions;
- implement Try Again semantics;
- call GNUbg commands directly from a lesson component.

### Neutral Tutor Layer

Examples:

- `tutor/TutorSession.kt`
- `tutor/TutorModels.kt`
- `tutor/TutorBoardPreview.kt`

Allowed responsibilities:

- Tutor session state;
- Tutor session transitions;
- lesson state;
- neutral Tutor UI state;
- tutor-specific board action meaning;
- future feature deltas and hint selection.

This is the correct home for Tutor behaviour that is not Android-specific.

### Shared Board Layer

Examples:

- `play/Board.kt`
- `play/BoardActions.kt`

Allowed responsibilities:

- render a board;
- render checkers, dice, cube, point numbers;
- translate pointer input into abstract board actions;
- render externally supplied highlights.

Forbidden responsibilities:

- depending on `GameViewModel`;
- knowing whether it is in Play or Tutor;
- calling Play or Tutor methods directly;
- owning backgammon rules.

### Regular Play Adapter

Example:

- `PlayBoardActions` in `GameLayout.kt`

Responsibility:

Map abstract board actions to Regular Play ViewModel actions.

This adapter is intentionally Play-specific.

### Tutor Board Adapter

Example:

- `tutorui/TutorBoardActions.kt`

Responsibility:

Map abstract board actions to Tutor session transitions.

This adapter is intentionally Tutor-specific.

## Current Tutor Flow

At 0.9.1, Tutor Mode is still a prototype surface.

Current flow:

1. User enters Tutor from the hub.
2. Tutor session shows a static board preview.
3. User may tap points.
4. Tutor session highlights the selected point.
5. Tutor session shows neutral point-region lesson text.
6. User may show a static Coach Card prototype.
7. Regular Play remains unaffected.

## Current Board Action Flow

### Regular Play

`BackgammonBoard`
→ `BoardActions`
→ `PlayBoardActions`
→ `GameViewModel`

### Tutor Mode

`BackgammonBoard`
→ `BoardActions`
→ `TutorBoardActions`
→ `TutorSessionController`

The shared board does not know which path is active.

## State and Immutability

Tutor state should remain explicit and copy-based.

Prefer:

- `data class TutorSessionState`
- immutable lists;
- neutral value objects;
- small controller functions returning new state.

Avoid:

- mutable singleton lesson state;
- hidden UI-side decisions;
- Android-only lesson logic;
- state changes buried in Compose event handlers.

## Coach Cards

`TutorCoachCard` is reusable UI.

It does not decide whether a card should appear.

It receives a `TutorHint` and renders it.

The decision to show a card belongs to Tutor session logic or future
neutral tutor decision logic.

## Try Again

Try Again must not be implemented as an Android UI trick.

Future implementation must define a neutral restore contract.

Acceptable locations:

- neutral Tutor/session layer;
- mobile game facade;
- native/mobile bridge, if the restore must be GNUbg-backed.

Forbidden location:

- Compose-only state manipulation that pretends to restore a real game.

## GNUbg Evaluation

Real move evaluation should enter Tutor Mode through a narrow mobile
facade API.

The first API should be small and factual.

It should return facts such as:

- user move;
- best move;
- equity loss;
- severity;
- top alternatives if needed.

It should not return human prose.

Human-readable tutor explanations must be generated from grounded facts
in the neutral Tutor layer.

## Documentation Rule

Whenever a new Tutor capability is added, update at least one of:

- this architecture document;
- the implementation plan;
- milestone/release notes;
- relevant KDoc in source files.

Do not allow the architecture to become implicit again.
