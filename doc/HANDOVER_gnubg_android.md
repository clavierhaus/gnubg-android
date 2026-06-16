---

## 3. Core Architecture Principle

**gnubg is the only authority. No game logic exists in Kotlin.**

The Android layer does exactly three things:
1. Call gnubg functions (`CommandRoll`, `CommandMove`, `ApplySubMove`, etc.)
2. Read `ms` (global `matchstate`) after each call
3. Render what the engine reports

Any logic not traceable to a gnubg source function is a liability. This rule was violated repeatedly early in development and caused cascading bugs every time.

---

## 4. Key gnubg Source Findings

These took significant time to discover. Do not re-derive them.

### TurnDone / NextTurn split
- `CommandMove` calls `TurnDone()` which sets `fNextTurn=TRUE` but does NOT advance `ms.fTurn`
- `NextTurn(TRUE)` advances `ms.fTurn` and calls `ComputerTurn` if it's the engine's turn
- On Android there is no GTK idle loop — `NextTurn(TRUE)` must be called explicitly
- **In `applyMoveString` JNI:** call `NextTurn(TRUE)` after `CommandMove`
- **In `rollDice` JNI:** call `while(fNextTurn) NextTurn(TRUE)` after `CommandRoll` to handle no-legal-moves

### ms.anDice is cleared by TurnDone
- `ms.anDice` is valid only between `CommandRoll` and `TurnDone`
- After engine moves, `ms.anDice = [0,0]`
- **Never read `ms.anDice` after an engine turn**
- For display: use `getMoveRecordDice()` which reads `plLastMove->p->anDice` — persists in the move record

### fAutoRoll must be FALSE
- Defined in `android-app.c` line 494: `int fAutoRoll = FALSE;`
- If TRUE, `NextTurn` auto-rolls the human's dice, bypassing the Roll button
- Was TRUE by default in the original `android-app.c` — this caused hours of debugging

### ApplySubMove does not touch ms
- `ApplySubMove(TanBoard, iSrc, nRoll, fCheckLegal)` operates purely on the passed board array
- It never modifies `ms` — safe for incremental display during move selection
- It does NOT check whether all checkers are in the home board (no bear-off legality check)
- Legal move validation must come from `GenerateMoves` — check that `src` appears in move list

### findMove via position key
- After human places checkers via `ApplySubMove`, find the complete move for `CommandMove` by:
  1. `GenerateMoves(oldBoard, d0, d1, FALSE)` — generates all maximal moves
  2. `PositionKey(currentBoard)` — hash of current board
  3. Find move where `EqualKeys(ml.amMoves[i].key, curKey) && cMoves==cMaxMoves && cPips==cMaxPips`
  4. `FormatMove` → `CommandMove`
- This mirrors `update_move()` + `Confirm(bd)` in `gtkboard.c`

### Board encoding
- `anBoard[1]` = moving player's checkers (always)
- `anBoard[0]` = opponent
- When `ms.fTurn=1` (engine moving), board is swapped for display via `SwapSides`
- `anBoard[1][24]` = moving player's checkers on bar
- Human = `board[25..49]` in our flat 50-element array, Engine = `board[0..24]`

### CommandRoll no-legal-moves
- When `GenerateMoves` returns 0 inside `CommandRoll`, gnubg:
  1. Creates `MOVE_NORMAL` with empty move
  2. Calls `PlayMove` which sets `ms.fTurn = !fPlayer`
  3. Calls `TurnDone` → `fNextTurn=TRUE`
- Our `while(fNextTurn) NextTurn(TRUE)` in `rollDice` JNI handles this correctly
- Reset `fNextTurn=FALSE` before `CommandRoll` is NOT needed (and caused bugs)

### Cube in 1-point match
- `CommandDouble` does nothing in a 1-point match (correct backgammon rules)
- Cube only functions in 3+ point matches
- `ms.fCubeOwner` stays -1 and `ms.nCube` stays 1 in a 1-point match

### Gammon/backgammon
- `getGameResult()` reads `plGame->plNext->p->g` (game info record)
- `pmgi->nPoints = nCube * GameStatus()` where `GameStatus()` returns 1/2/3
- `pmgi->fWinner` = 0 (human) or 1 (engine)

---

## 5. JNI API Summary

All in `native-lib.c`. Full documentation in `MASTER_V0.8.7.tex`.

| Kotlin call | gnubg mapping |
|-------------|---------------|
| `initialise(weightsPath)` | `EvalInitialise` + setup |
| `newGame(matchLength)` | `CommandNewMatch(n)` + `CommandNewGame` |
| `rollDice()` | `CommandRoll` + `while(fNextTurn) NextTurn(TRUE)` |
| `applyMoveString(str)` | `CommandMove` + `NextTurn(TRUE)` |
| `applySubMove(board, src, die)` | `ApplySubMove` — no ms side effects |
| `findMove(old, cur, d0, d1)` | `GenerateMoves` + position key match |
| `getLegalMoves(board, d0, d1, partial)` | `GenerateMoves` |
| `getMoveRecordDice()` | `plLastMove->p->anDice` |
| `getMatchCubeInfo()` | `ms.fDoubled, ms.fCubeOwner, ms.nCube` |
| `getMatchScore()` | `ms.anScore[0/1], ms.nMatchTo` |
| `getGameResult()` | `plGame->plNext->p->g.fWinner/nPoints` |
| `commandDouble/Take/Drop()` | `CommandDouble/Take/Drop` + `while(fNextTurn)` |

---

## 6. Coordinate System

**Critical for tap detection correctness across devices.**

The board uses a virtual coordinate system (`TOT_W=102, TOT_H=82`) with:
- `ux(u) = u * canvasW / TOT_W` (pixels)
- `uy(u) = u * canvasH / TOT_H` (pixels)

`canvasW`/`canvasH` are captured via `onSizeChanged` on the Canvas composable and shared with the `pointerInput` handler on the wrapping Box. **Do not use `size.width/height` from `pointerInput` directly** — on tablets the Box size and Canvas size differ due to layout weight allocation.

The Xiaomi Pad 6 delivers 3–4 simultaneous touch events per physical tap. This is hardware behaviour, not a bug. Phase guards in `GameViewModel` (`ENGINE_THINKING`, `AtomicBoolean`) prevent duplicate engine calls.

---

## 7. Current State (v0.8.7)

### Working
- Full game loop: roll → move → commit → engine responds → repeat
- Bar re-entry (tap bar checker)
- Bear-off including partial forced moves
- No legal moves auto-pass
- Engine dice display (from move record, not `ms.anDice`)
- Roll button on board canvas
- Undo / Commit with gnubg move validation
- Swap dice
- Max 5 checkers per point with overflow count
- Bearoff tray (15 checker capacity)
- Winner detection (correct player)
- Gammon/backgammon detection and display
- Blocked die graying
- Score display in left panel
- Cube tap to offer double (works in 3+ point matches)
- Accept/Drop for engine cube offer (coded, needs multi-point test)
- Checker proportions relative to point width
- Tap detection correct on both Pixel 8 Pro and Xiaomi Pad 6

### Not working / not implemented
- Match length selector UI (settings wired, no UI button)
- Engine cube offer flow (CUBE_OFFERED phase) — needs testing
- New game within match (resets score — should preserve)
- Cube visual: value and ownership position
- Resign (game or match)
- 16KB page alignment for Android 16
- SGF save/load UI
- Analysis / tutor mode
- Player names

---

## 8. Next Steps (v0.8.8)

In priority order:

1. **Match length selector** — add 1/3/5/7 toggle in left panel or game-over screen. `viewModel.setMatchLength(n)` exists. `Engine.newGame(n)` is wired.

2. **New game within match** — `CommandNewGame(NULL)` without `CommandNewMatch` preserves score. Currently `startNewGame()` calls both, resetting score.

3. **Cube visual** — `cubeValue` from `gameState.cubeValue`, cube position: `cubeOwner=1` → top of bar, `cubeOwner=0` → bottom, `-1` → centre.

4. **Engine cube offer** — when engine calls `CommandDouble` during its turn, `ms.fDoubled=1` and `ms.fTurn=0`. Detect in `rollDice` after `while(fNextTurn)`: if `ms.fDoubled=1`, set `CUBE_OFFERED` phase. Accept/Drop buttons already in `GameLayout`.

5. **Resign** — `CommandResign(NULL)` in gnubg. Add confirmation dialog. Show in left panel during `HUMAN_MOVING` or `WAITING_FOR_ROLL`.

6. **16KB page alignment** — add `-Wl,-z,max-page-size=16384` to `CMakeLists.txt` linker flags.

---

## 9. Key Files to Read First

Before touching anything:
1. `doc/MASTER_V0.8.7.tex` — full project history and architecture
2. `jni-bridge/src/native-lib.c` — all engine↔Kotlin interface
3. `jni-bridge/src/android-app.c` — gnubg global variable definitions
4. `gnubg-app/.../engine/GameViewModel.kt` — game state machine
5. `gnubg-app/.../ui/Board.kt` — board rendering and tap detection
6. `engine-core/play.c` — gnubg game logic (read before modifying any flow)

**When in doubt: read the gnubg source first. The answer is always there.**

---

*GNU Backgammon Android Port — Developer Handover — clavierhaus.at — June 2026*
EOF