# Insight coverage map

What the coach can and cannot teach, grounded in what gnubg actually
computes -- not in what feels coverable. Two facts set the boundary:

1. The matcher scores **signatures** written in gnubg's contact inputs
   (`CalculateHalfInputs`, the I_* enum). It reads those via
   `positionFeatures`, which ONLY ever calls `CalculateHalfInputs`.
2. gnubg evaluates a **pure race** with a DIFFERENT, unnamed input set
   (`CalculateRaceInputs`, eval.c:1294: per-point occupancy + men-off +
   crossovers). No I_* names, no mobility/wastage/distribution feature.

Therefore the signature mechanism covers CONTACT positions only. A race
mistake has no nameable feature to match; it lives in gnubg's equity
delta, not in a signature. This is a mechanism boundary, not a gap to be
filled with more entries.

## Covered now (10 entries, all measured)

| entry | pattern | class |
|---|---|---|
| prime.break.5 | broke a 5-prime cheaply | contact |
| prime.contain.lost | let the trapped checker out | contact |
| board.close.entry | left the board open vs a bar checker | contact |
| blot.shot.given | left a blot under fire | contact |
| blot.double.given | left two blots in range | contact |
| anchor.surrender.back | abandoned the back anchor | contact |
| anchor.advance.golden | declined the golden anchor | contact |
| race.break.ahead | stayed in contact with the lead | contact->race |
| race.escape.window | didn't run the back checker | contact |
| hit.declined | passed up a hit | contact |

Note the two "race" entries are contact-boundary entries: they fire while
contact still exists (the decision to break it / run through it), not in a
pure race.

## Not coverable by signatures (mechanism boundary)

- **Race wastage** (bury a checker, waste pips, wrong crossover). MEASURED
  DEAD 2026-07-12: at equal pips, the buried vs efficient position moves
  only contact inputs (I_MOBILITY/I_CONTAIN/I_TIMING) that are computed
  against a blockade that no longer exists -- noise, not signal. gnubg
  reasons over CalculateRaceInputs (occupancy/men-off/crossovers), which
  positionFeatures does not expose. To flag race errors, compare gnubg's
  EQUITY delta between played and best -- a separate "equity-gap" coach
  mechanism, deferred and scoped below.
- **Bear-off errors** (fill/spread, wrong-side gaps). Same story: bearoff
  is its own position class and database, no contact I_* signal.
- **Cube errors** (double/take/drop, too-good). Already handled OUTSIDE the
  signature layer -- the Coach's cube branch shows gnubg's own cube verdict
  (docs/ARCHITECTURE_ANALYSE_MODE + the coach cube plan). No corpus needed.

## Coverable by signatures, just not authored yet (the real backlog)

These DO have contact-input signals; they are candidate entries 11+ and
need the same pilot->signature->positions->phrase loop as the ten:

- **timing.hold.crunch** -- holding-game timing collapse (I_TIMING is
  characterized; needs a pair).
- **backgame.timing** -- already drafted in CORPUS_ENTRIES_DRAFT, signature
  source-grounded (I_BACKG + I_TIMING), pair deferred. Nearest to ready.
- **blitz.point.missed** -- declined to make a home-board point during an
  attack (I_ENTER2 / opponent-on-bar family, related to close.entry).
- **prime.extend** -- had a prime and failed to extend/prime the last gap.
- **duplication / diversification** -- number-duplication when leaving
  shots (may be too fine for a single I_* signal; pilot decides).
- **anchor.advance.mid** -- advanced-anchor jumps other than the golden
  point (I_FORWARD_ANCHOR at other zone values).

## Verdict

The ten are a coherent slice: the crisp, nameable contact mistakes. The
backlog above (~6) is the honest remainder that the CURRENT mechanism can
reach. Race, bear-off, and (already-handled) cube sit OUTSIDE it: race and
bear-off would each need an equity-gap coach, a separate feature -- worth
building, but not a corpus entry and not a signature.
