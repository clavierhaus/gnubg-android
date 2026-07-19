# Curated: hit.loose.homeboard
Signature (measured; tools/harvest/signatures.py): opp on the bar on BOTH sides (I_BACK_CHEQUER 0.95-1.0 gate); opp I_P1 down (min 0.05, played >= 0.08); opp I_ENTER up.
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

Canon note: Robertie, lessons 15/29: a home-board loose hit needs a key point or a required tempo; structure is what makes hits tolerable -- the praise variant is exactly the hit-and-cover case.

## Proposed final phrases (tier: proposed)
- phrase_flag: A loose hit in your own board with no cover -- they enter shooting, and the return shot undoes the hit.
- phrase_praise: Hit and covered -- the point is made, and they enter against a stronger board with nothing to shoot at.

## Reasons (from the data)
- return-shot rolls fall from 17/36 to none (opp.I_P1 -0.472)
- their entry gets costlier against the covered point (opp.I_ENTER +0.156, I_ENTER2 +0.194)
- opp.I_PIPLOSS context: the uncovered hit hands most of its gain back

## Diff-note / rejections
Single composed draft per entry this run (identical grounding across the
entry's pairs made per-pair variants redundant); no rejections to record.
Confidence: high (deltas state the pattern plainly).
