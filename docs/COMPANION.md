# The Insight Layer: a phrase that explains the move

Status: ANALYSIS + PLAN (2026-07-12, reframed per maintainer). NOT voice, NOT a
chatbot, NOT a talking companion. The deliverable is ONE OR TWO PHRASES of
written guidance matched to the move actually played -- teaching the PATTERN
behind gnubg's verdict in backgammon's own conceptual vocabulary.

## Who it is for

Not primarily the rookie and not the expert -- the large MIDDLE: players already
decent who want to significantly improve their grasp of patterns and standards.
For them "-0.081, 4th of 17" is legible but inert; "you broke your 5-prime for a
play that gains nothing" is the lesson. The number says THAT the move was worse;
the phrase says WHY, in terms they can carry to the next game.

## What it is NOT

- Not speech/audio -- text only.
- Not a language model writing prose (that is a later, optional flavor layer at
  most; the product must be whole and shippable without it).
- Not gnubg's numbers restated -- those already show. The phrase adds the
  CONCEPT the numbers embody.

## The two hard problems (maintainer-identified, and they ARE the whole problem)

### (a) The phrase corpus -- authored, versioned, curated

A table of backgammon PRINCIPLES, each a short teachable phrase keyed to a
detectable positional pattern. Authored content, GPL-compatible (gnubg manual +
standard published theory), never generated. The craft is in the collection:
coverage of the standard pattern vocabulary, phrases true across the skill
range, each falsifiable against a concrete feature signature. Domains to cover:
prime integrity and length, anchor holding vs. timing, builder economy and
flexibility, blot exposure weighed against board strength, race vs. hold
commitment, timing and crunch, home-board priority, bar-point and golden-point
value, back-checker escape. This is the biggest and most valuable single body
of work in the whole feature; it is DATA, editable without touching code.

### (b) Analysis of possible moves -- already gnubg's, to be made structural

gnubg has ranked and scored every legal move (the coach verdict already carries
the top-K with equities and neural eval). Phase A adds the STRUCTURE: a features
verb (gnubg's own CalculateHalfInputs / ClassifyPosition) run on the played
result AND the best result, so each candidate is described not just by equity
but by a vector of named positional facts.

### (c) Matching phrase to gameplay -- the FEATURE DELTA selects

The match is deterministic and gnubg-grounded: the delta between the played
result's feature vector and the best result's names what changed -- prime length
-1, a blot added in the home board, an anchor surrendered. The corpus entry
whose signature best fits the dominant delta is the phrase shown. gnubg computed
the difference; the app only looks up which authored principle that difference
instantiates. No invention, offline, fully explainable ("shown because: prime
5->4 while best held it").

Matching design questions to settle in build:
  - dominant-delta vs. weighted top-2 (a move can violate two principles);
  - thresholds per feature so trivial deltas stay silent (P2 no-noise);
  - tie-break order when several signatures fit;
  - a phrase must be allowed to say "nothing standard -- just equity"; honesty
    over a forced lesson.

## Order of work (unchanged in sequence, reframed in aim)

0.12.0 (try-again sandbox) ships first. Then Phase A features verb; then the
corpus (a) begun in parallel since it is data; then the matcher (c) as a
deterministic reason line. Optional prose polish by a local model is a distant
maybe, explicitly after the deterministic phrase is proven -- and it too may
only restate the matched principle, never reason about the position.
