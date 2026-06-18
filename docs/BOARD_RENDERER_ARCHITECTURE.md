# Board Renderer Architecture

## Purpose

The board renderer is shared infrastructure.

It must remain independent from Regular Play and Tutor Mode semantics.

## Main Components

### `BackgammonBoard`

Location:

`gnubg-app/app/src/main/kotlin/com/clavierhaus/gnubg/play/Board.kt`

Responsibilities:

- draw the board;
- draw checkers;
- draw dice and cube UI affordances;
- draw point highlights;
- translate pointer input into abstract board actions.

It receives:

- `GameSettings`;
- `BoardState`;
- optional `BoardActions`;
- optional externally highlighted points.

It does not receive `GameViewModel`.

### `BoardActions`

Location:

`gnubg-app/app/src/main/kotlin/com/clavierhaus/gnubg/play/BoardActions.kt`

This interface is the boundary between board input and mode-specific
behaviour.

It currently exposes:

- `offerDouble()`;
- `rollDice()`;
- `swapDice()`;
- `undo()`;
- `confirm()`;
- `tapSource(point)`.

The names still reflect the existing board affordances. Future cleanup
may split Regular Play actions from Tutor lesson actions if needed.

## Regular Play Usage

Regular Play supplies `PlayBoardActions`.

This adapter delegates to `GameViewModel`.

That is the only place where the shared board renderer should connect to
Regular Play actions.

## Tutor Usage

Tutor Mode supplies `TutorBoardActions`.

At 0.9.1, most actions are no-ops and `tapSource(point)` selects a point
inside the neutral Tutor session.

This prevents Tutor Mode from inheriting Regular Play move semantics.

## Highlighting

The board renderer supports external highlighted points.

This is currently used by Tutor Mode to highlight the selected lesson
point.

Regular Play still keeps its temporary long-press landing hints.

The two highlight sources are combined by the renderer.

## Rules for Future Work

Do not add `GameViewModel` back to `Board.kt`.

Do not add Tutor session logic to `Board.kt`.

Do not add GNUbg command logic to `Board.kt`.

If a new board interaction is needed, add an abstract action or a more
specific neutral input event and let the active mode decide what it means.
