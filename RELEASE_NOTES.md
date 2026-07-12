## GNU Backgammon for Android 0.20.0

A milestone: **"Train with the Coach"** is now a complete, self-contained
fourth mode. You play a full game or match against gnubg and, after each of
your moves, gnubg judges it — showing whether it was best, what was better, and
by how much, all in gnubg's own values. Every number on screen is the engine's;
the app only renders it.

### Added
- **Train with the Coach**, the fourth hub mode. A setup screen chooses
  opponent strength (the same seven levels as tournament play) and match length
  before the board. One point keeps the cube dead — chequer play only; longer
  matches put the cube in play, and the Coach judges those decisions too.
- **Per-move verdicts.** After each move the Coach shows one of three honest
  tiers — the best move affirmed, a fine-but-not-best move with what beat it,
  or a flagged move with gnubg's severity and the equity it cost — anchored to
  the move by name. The verdict is computed before gnubg replies, so the
  position under discussion never shifts.
- **A before/after move explorer.** Each candidate (your move, then gnubg's
  better ones in rank order) is a two-state toggle: first tap shows the
  decision point — the position before the move, dice in full colour, no
  arrows, identical for every candidate; second tap shows the position that
  move produces, with green arrows to the destinations it occupies. Your move
  is marked P in red; the alternatives are numbered by rank.
- **Cube coaching** (match length > 1). When you double, take, or drop, the
  Coach judges the decision against gnubg's — correct or reasonable, or flagged
  with the equity cost and which decision it was — using only gnubg's own cube
  evaluation and skill bands.
- **On-board "GNU's turn."** The continuation button sits on the board's left
  half, mirroring the player's Roll on the right, aligned to the dice row. While
  you are studying an alternative it reads **Back** and returns you to the live
  position first, so continuing is always a clear two step: back to the game,
  then GNU's turn.
- **The match score** flanks the Coach title — GNU on the left, You on the
  right — in a match longer than one point.
- **A breathing cube.** When you offer a double, the cube softly pulses to show
  the offer is made but GNU has not answered yet; it settles to its new value
  once you continue and GNU responds.

### Changed
- The hub background now fills the screen rather than sitting letterboxed,
  which also removes the downscaling that pixelated the piano's fine lines.
- Hub entries lead with their verb — **Play**, **Train**, **Analyse**,
  **Review** — in the signature orange.
- The settings gear is now one size on every screen.

### Removed
- The unused **Profile** stub, whose every stated purpose was already served by
  Settings and the per-mode setup screens.

### Fixed
- A regression where cube coaching intercepted the ordinary roll and masked the
  chequer verdict — the ordinary roll is untouched again, and cube coaching
  triggers only on cube actions you initiate.
- The doubling window is now announced: at the start of your turn the Coach says
  you may roll or tap the cube to double, so the one moment a double is possible
  is no longer easy to miss.
- The cube verdict now reads in the present tense, about the decision you are
  about to commit, rather than as a completed, already-accepted transaction.

### Documentation
- A **Quick Start** guide covering all four modes and the non-obvious gestures —
  tap a destination to make a point, long-press to preview a checker's landings,
  the two-tap before/after explorer, and the held-cube two step.
