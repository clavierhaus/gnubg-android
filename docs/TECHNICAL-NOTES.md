# Technical Notes

## Core invariant

GNUbg is the only authority.

The Android layer must not become a parallel backgammon implementation. Kotlin code may:

- draw the board;
- calculate hitboxes in board-relative coordinates;
- keep transient UI state such as displayed dice order;
- store undo snapshots for uncommitted submoves;
- call JNI functions.

Kotlin code should not invent game legality, cube legality, score logic, pip counts, or match progression.

## Interaction model

The human turn is modelled as uncommitted UI state on top of the current GNUbg match board.

Single submoves are tested through the native bridge. A legal submove returns a new board representation. The move is not committed to GNUbg match history until the full move is confirmed.

### Source tap

A source-point tap tries the first remaining die, then the second if necessary. The resulting board is accepted only if GNUbg returns a legal submove result.

### Dice swap

Dice order is a UI preference for which die to try first. Swapping dice changes play order but not the actual roll.

### Per-die undo

Before each accepted submove, the UI stores a `MoveSnapshot` containing:

- board;
- remaining dice;
- legal moves;
- blocked dice;
- pip counts.

Undo restores the last snapshot. This is UI-local because uncommitted `ApplySubMove` operations do not mutate the native match state.

### Destination-stack tap

A destination tap is a convenience gesture. It searches for two legal GNUbg submoves that both land on the tapped destination point. It applies the gesture only if exactly one unique resulting board/remaining-dice state exists.

This keeps the convenience behaviour deterministic and avoids guessing user intent.

## Cube path

The human cube path must not call GNUbg `CommandDouble()` from the embedded Android flow.

The safe path is:

1. read native cube debug state;
2. verify the legal cube window;
3. call cube decision on `Engine.getMatchBoard()`, not on a transformed UI board;
4. read the `cubedecision` enum from the native result;
5. for take-like decisions, call the native `applyHumanDoubleTake()` path;
6. update UI state from the native return.

Current pass/drop and beaver paths are intentionally not presented as complete.

## Layout discipline

Board drawing and hitboxes should be expressed in board-relative units. Avoid hard-coded screen fractions or magic vertical offsets. The same layout must survive different aspect ratios, including phones and tablets.

## JNI bridge role

The bridge should expose a narrow, auditable set of GNUbg operations to Kotlin:

- match state reads;
- dice and turn state;
- move generation and submove application;
- move formatting / matching;
- cube decision and cube state;
- pip count and evaluation entry points.

The bridge is not a separate engine.
