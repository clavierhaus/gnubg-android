# Curated: bearoff.shot.left
Signature (measured; tools/harvest/signatures.py): opp I_P1 down (min 0.04), played gives 11-13/36 hitting rolls, best none; me rearmost home-side (I_BACK_CHEQUER <= 0.40 both)
Drafts: claude-fable-5 in-session, run 2026-07-19, one draft per verified pair
(4 pairs), written against signature deltas plus the full-vector context
section (deltas_all).
Status of the final phrase: **PROPOSED** -- awaiting maintainer adoption.

## Source cross-check (2026-07-19, assistant-side per maintainer instruction)
Principle verified against public sources; vocabulary aligned to standard use
(Robertie/Gammon Press lessons 9, 15, 29; Deluxe Backgammon and other guides
via web search). No verbatim or near-verbatim overlap with any retrieved
excerpt; spot-check by distinctive-substring search, not a full-book scan.
gnubg's certification of the position pair is untouched by this check
(two-tier doctrine): sources verify the WORDS, never the position.

Canon note: Bearing in against an anchor: standard canon is to clear and fill without leaving a shot (Gammon Press lesson 9: most of the trailing side's wins come from a left shot).

## Proposed final phrases (tier: proposed)
- phrase_flag: The race was already won; the blot you left in front of their anchor is the one roll that loses it.
- phrase_praise: Filled safely behind the anchor -- they hold their point and never get a shot.

## Reasons (from the data)
- opp hitting rolls fall from 11-13/36 to none (opp.I_P1 -0.306..-0.361)
- pip counts 67-68 vs 103: the race is decided unless the shot lands
- context: opp.I_PIPLOSS collapses -- the hit was their whole remaining equity

## Diff-note / rejections
Single composed draft per entry this run (identical grounding across the
entry's pairs made per-pair variants redundant); no rejections to record.
Confidence: high (deltas state the pattern plainly).
