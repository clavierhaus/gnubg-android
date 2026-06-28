# Changelog — 0.8.9

This milestone consolidates the playable Android GNU Backgammon prototype after the 0.8.7 documentation baseline.

## Headline

0.8.9 is a usability and correctness milestone: the game is playable with a more natural touch interface, while preserving the core rule that GNUbg remains the only authority for game legality and match state.

## Added

### Start screen

- Added an improvised match setup screen.
- Added visible match-length selection.
- Added three visible opponent-strength choices:
  - Beginner
  - Advanced
  - Master

The strength selector is currently UI/state plumbing. It should not be presented as complete engine-strength control until GNUbg evaluation settings are wired accordingly.

### Dice UI

- Replaced transparent used-dice rendering with solid muted colours.
- Kept dice positions stable after a move.
- Used dice now grey out in the position where they were thrown, unless the player deliberately swaps the dice first.
- Dice swap remains explicit by tapping the dice area.

### Per-die undo

- Undo now restores the previous submove snapshot.
- Undo no longer resets the whole human turn by default.
- Dice colour/state follows the restored `remainingDice`.

### Unplayable remaining dice

- If GNUbg says no legal continuation exists after a submove, remaining impossible dice are consumed/greyed.
- This covers cases such as doubles where only one, two, or three of four dice can legally be played.

### Destination-stack convenience tap

- Tapping an empty/opponent destination point can move two legal checkers to that point at once.
- The shortcut only applies when the resulting two-submove action is unambiguous.
- The move is still built from GNUbg-validated submoves.
- One Undo reverses the combined convenience action.

### Cube interaction

- Cube tap path restored and wired through native GNUbg state.
- Human double uses GNUbg cube decision evaluation on the real match board.
- `CommandDouble()` is deliberately not used for the human double execution path.
- Accepted/taken human doubles are applied by direct native match-state mutation guarded by the same legal cube window.

### Board and layout

- Continued board-relative layout discipline.
- Avoided absolute screen-position constants for visual fixes.
- Continued testing focus on both phone and tablet aspect ratios.

## Changed

- UI convenience logic now treats Kotlin as touch-routing/state-display glue.
- Game legality continues to be checked through GNUbg calls.
- Dice drawing is now based on stable displayed dice plus a remaining-dice multiset, rather than by moving active dice to the front after each move.

## Fixed

- Used dice no longer become high-brightness HDR white.
- Used die no longer swaps position with the remaining movable die after a submove.
- Single Undo no longer undoes all two or four dice.
- Cube display updates after accepted human double.
- Several dice edge cases around partial doubles and blocked remaining dice were corrected.

## Still incomplete

- Beaver UI/path.
- Engine pass/drop path after human double.
- Real difficulty-to-evaluation-setting wiring.
- SGF UI.
- Tutor/analysis UI.
- Final visual design.
- Broad device QA.
