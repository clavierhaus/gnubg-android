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

## Traps in this port (each cost a real bug; verified against the source)

### jni-bridge headers shadow engine-core headers

`target_include_directories` lists `jni-bridge/` **before** `engine-core/`, and
`jni-bridge/` contains deliberately empty stubs for seven engine headers:
`analysis.h`, `config.h`, `drawboard.h`, `format.h`, `matchid.h`, `render.h`,
`renderprefs.h`. They exist so GTK-dependent translation units compile.

Consequence: a plain `#include "matchid.h"` from facade code resolves to the
**stub**, defines the guard, declares nothing, and the call falls through to an
implicit declaration. Reach the real header explicitly:

    #include "../../engine-core/matchid.h"

Do NOT reorder the include path to "fix" this; it would unshadow the other six.
`positionid.h` is not shadowed, which is why it appears to work.

### SetGNUbgID's 0 does not mean the position was set

`SetGNUbgID` (set.c) discards `SetBoard`'s return value at both call sites
(set.c:4781, set.c:4873). `SetBoard` refuses unless `ms.gs == GAME_PLAYING`, and
by then `SetMatchID` (play.c:4205) has already run `FreeMatch()`,
`InitBoard(ms.anBoard, ...)` -- resetting the board to the opening position --
and set `ms.gs` from the Match ID.

So an ID captured after a game ended silently yields the STARTING position with
the final score, reported as success; and a bare Position ID with no game in
progress silently changes nothing. The correct test is gnubg's own precondition:
after the call, if `ms.gs != GAME_PLAYING`, no position was installed. This is
why a GNU BG ID is the wrong artifact to save at the end of a game.

Call `SetGNUbgID`, never `CommandSetGNUbgID`: the wrapper answers the
"player on roll is on top -- swap?" question through `GetInputYN`, which in this
port (android-app.c:854) always returns TRUE, so it would swap silently. gnubg
returns 2 to hand that decision back; the UI must ask.

### CommandSaveMatch tokenizes its path

`CommandSaveMatch` (sgf.c:2365) begins with `NextToken(&sz)`, which splits on
whitespace. **A path containing a space is silently truncated.** It also refuses
when `plGame` is NULL.

### FACADE_FILE_OP always reports success

`FACADE_FILE_OP` returns 1 unconditionally, and the `Command*` functions it wraps
return `void`. So `Engine.saveMatch()`'s Boolean means "the call was made", not
"the file was written". Verify by checking the file exists and is non-empty.

### PositionFromXG inverts PositionFromID's convention

`PositionFromID` returns 1 for a valid position (it ends with `CheckPosition`).
`PositionFromXG` returns **0 on success** and 1 on error, and does not validate.
Do not write a decoder that tries one and falls back to the other: `PositionFromID`
base64-decodes anything handed to it, so an XG string can decode to a legal-looking
but wrong board. `SetGNUbgID` already discriminates the dialects properly.

## JNI bridge role

The bridge should expose a narrow, auditable set of GNUbg operations to Kotlin:

- match state reads;
- dice and turn state;
- move generation and submove application;
- move formatting / matching;
- cube decision and cube state;
- pip count and evaluation entry points.

The bridge is not a separate engine.
