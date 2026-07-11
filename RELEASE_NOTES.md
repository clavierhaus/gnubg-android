## GNU Backgammon for Android 0.11.2

Field-report fixes to the 0.11.0 release. (0.11.1 was a re-tag of identical code
during release setup; 0.11.2 is the first with these fixes.)

### Fixed
- **The review verdict could be cut off** the bottom of the screen — the move
  info and the verdict together overflowed past the navigation buttons on shorter
  phones. The info block is now bounded above the buttons, so the verdict is
  always on screen.
- **No obvious way out of Analyse.** After analysing a position there was no
  visible exit; you had to close the app. A Home button is now pinned to the
  Analyse pane in every state (paste, editor, result).
- **The engine's opening looked frozen.** When gnubg won the opening roll at a
  slow level, the board sat static for several seconds with no sign it was
  working. It now shows a thinking state and the opening roll as it lands, like
  every other engine turn.
- **The "Show equity" setting did nothing.** It now gates the equity line in the
  analysis panel. (Showing match-winning-chance instead of equity is still to
  come — it needs gnubg's own conversion, not an app-side one.)

