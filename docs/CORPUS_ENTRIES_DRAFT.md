# CORPUS_ENTRIES_DRAFT.md — the signature-driven phrase list (Phase B)

Status: first pass APPROVED as written (maintainer, 2026-07-12) -- the 12
entries stand unchanged. Next: the §9.4 pilot measures each signature.
Governed by CORPUS_HARVEST_PLAN.md. 12 entries — enough to prove the workflow
(plan §9.2) before scaling to 40–80.

Rules this file lives under:
- Signature vocabulary is gnubg's ONLY: the `I_*` input names of
  engine-core/eval.h:553–618, `PipCount`, `positionclass`. No field a
  gnubg verb does not compute appears here.
- **Every signature below is a GUESS from reading the enum's source
  comments** (plan §9.4). The pilot — paired positions through
  `positionFeatures`, diffing the `I_*` by hand — confirms or replaces each
  before any phrase is authored. Entries whose inputs carry **no source
  comment** (`I_TIMING`, `I_BACKBONE`, `I_BACKG`, `I_BACKG1`, `I_CONTAIN`,
  `I_ACONTAIN`, `I_MOMENT2`, `I_FREEPIP`, `I_BACKRESCAPES`) are marked
  UNCOMMENTED and must not reach Phase D until the pilot has established
  what they respond to.
- `side`: "me" = player on roll (fMove), "opp" = the other. `direction` is
  the sign of (best − played) for that input — gnubg's better move moves the
  input that way. Encodings (is a further-back anchor a higher or lower
  value?) are themselves pilot questions; directions below assume nothing
  beyond the comment text and say so where it matters.
- Categories: race / board / threat (backgammon-teacher taxonomy, MIT,
  credited per plan §6). Skill bands: gnubg's own — doubtful / bad / very bad.
- No prose phrases here. This is the shelf phrases go on in Phase E.

---

## board

**prime.break.5** — A 5-prime is worth holding.
signature: opp I_BACKESCAPES up · me I_BACKBONE down (UNCOMMENTED)
bands: doubtful, bad
note: the plan §5 example, kept verbatim as the workflow's reference entry.

**prime.contain.lost** — Don't let the trapped checker out for free.
signature: opp I_BACKESCAPES up · me I_CONTAIN down (UNCOMMENTED)
bands: doubtful, bad, very bad
note: the general containment sibling of prime.break.5; pilot must separate
the two or merge them.

**anchor.surrender.back** — A back anchor is shelter; don't leave it without a reason.
signature: me I_BACK_ANCHOR delta (sign = pilot question) · opp I_P1 up
bands: bad, very bad
note: "Position of the back anchor" — whether surrender reads as the value
rising, falling, or vanishing is exactly what the pilot measures first.

**anchor.advance.golden** — The 20-point anchor is worth fighting for.
signature: me I_FORWARD_ANCHOR delta (sign = pilot question)
bands: doubtful
note: praise-side candidate — fires when the BEST move makes the forward
anchor and the played move declined it.

**board.close.entry** — Every home point you make keeps the hit man out longer.
signature: opp I_ENTER2 down · opp I_ENTER down
bands: doubtful, bad
note: "Probability of entering from the bar" — cleanest-commented signal in
the enum; good pilot starter.

**stack.mobility.dead** — Stacked checkers are pips doing nothing.
signature: me I_MOBILITY down · me I_MOMENT2 delta (UNCOMMENTED)
bands: doubtful
note: I_MOBILITY carries a comment ("contribution of mobility"); I_MOMENT2
("second moment about centre of mass") may proxy distribution — pilot decides
whether one input suffices.

**backgame.timing** — A backgame lives or dies on timing.
signature: me I_TIMING delta (UNCOMMENTED) · me I_BACKG delta (UNCOMMENTED)
bands: bad, very bad
note: BOTH inputs uncommented; parked until the pilot names them. Included so
the shelf shows where backgame coaching would sit.

## threat

**blot.shot.given** — Count the shots before you leave the blot.
signature: opp I_P1 up · opp I_PIPLOSS up
bands: doubtful, bad, very bad
note: "Number of rolls that hit at least one checker" / "average pips lost
from hits" — which side's half-inputs carry MY exposure is a pilot question
(CalculateHalfInputs is per-side).

**blot.double.given** — One blot is a risk; two in range is a plan for disaster.
signature: opp I_P2 up
bands: bad, very bad
note: the I_P2 ("hit at least two") sibling of blot.shot.given.

**hit.declined** — When the hit is right, take it — pips on the bar are pips won.
signature: opp I_ENTER up when played vs best (best puts opp on the bar) ·
me I_PIPLOSS delta (side/sign = pilot question)
bands: doubtful, bad
note: hitting shows as the OPPONENT suddenly having entry inputs at all;
weakest-specified entry in this pass, pilot may replace the signature wholesale.

## race

**race.break.ahead** — Ahead in the race, break contact and run.
signature: PipCount me < opp (precondition) · me I_BREAK_CONTACT delta
(sign = pilot question)
bands: doubtful, bad
note: first entry using the PipCount verb as a signature precondition —
proves the §2 "or PipCount" clause in practice.

**race.escape.window** — Escape rolls are a resource; don't spend the window doing nothing.
signature: me I_BACKESCAPES down · me I_BACK_CHEQUER delta (sign = pilot question)
bands: doubtful, bad
note: "How many rolls let the back checker escape" — the decline-to-run
pattern; overlaps race.break.ahead at the edges, pilot separates them.

---

Pilot order (per plan §9.4): board.close.entry and blot.shot.given first —
their inputs carry the clearest source comments — then the anchor pair to
settle position-value encodings, then the UNCOMMENTED set or their removal.
