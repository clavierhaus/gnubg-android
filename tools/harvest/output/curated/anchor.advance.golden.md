# Curated: anchor.advance.golden
Signature (measured; tools/harvest/signatures.py): me I_FORWARD_ANCHOR up (min 0.30), played in (0,0.4], best in [0.5,1.0]
Drafts: claude-sonnet-4-6, run 2026-07-12, one draft per verified pair.
Status of the final phrase: **PROPOSED** -- the maintainer edits it into his
voice; tier becomes "authored" only then. Verbatim-overlap spot-check against
Robertie/Magriel/Woolsey/Trice: pending (author-side, per plan).

## Best drafts (quoted, model-attributed)
- [a20f1] "Advancing to the golden point gives you a strong anchor deep in enemy territory -- staying back on the 24 wastes that opportunity."
- [a20f1 (praise)] "Well played -- claiming the golden point plants a deep anchor that makes you very hard to attack."

## Proposed final phrases (maintainer to edit; tier: proposed)
- phrase_flag: The golden point was there for the taking, and the anchor stayed buried on the 24.
- phrase_praise: The golden anchor is made -- the best point in their board is yours.

## Diff-note / rejections
SIGNAL MISMATCH rejections: a19f1/a19f2/a21f1/a21f2/a20f2 narrate
'retreating off the golden point' -- no retreat exists in either board; the
played position simply never made the point. The deltas exposed the
hallucination (criterion 3). Only the a20f1 draft read the pair correctly.

## Amendment (2026-07-13)
best_in narrowed [0.50,1.0] -> [0.80,0.87]: the wide range fired the GOLDEN
phrase on 21/22-point advances (wrong words). Those advances now belong to
anchor.advance.mid; the 19-advance (their 6-point, value 1.0) is
deliberately uncovered rather than mislabeled. Generator pairs 6 -> 2 (the
20-point fillers), both verified.
