# INPUT_DICTIONARY.md — gnubg's contact inputs, researched

Status: 2026-07-12. Every entry verified against eval.c THIS session; pilot
measurements from docs/CORPUS_ENTRIES_DRAFT.md where they exist. This file
exists because the eval.h enum comments are NOT reliable — several are
wrong, vague, or absent — and the corpus workflow (signatures, and the
Phase D prompts that name gnubg's numbers) must not inherit those errors.
Line numbers reference engine-core/eval.c.

Frame conventions: each side's half-inputs are computed by
CalculateHalfInputs(anBoard, anBoardOpp, afInput) in the OWNER's frame
(index 0 = own ace point, 23 = opponent's ace, 24 = own bar). "me/opp"
below matches the pilot harness columns. THE single most important
correction: threat and containment inputs live on the ACTING side's half —
my exposure appears in the opponent's I_P1; my prime appears in MY
I_CONTAIN.

| input | eval.h comment | formula (eval.c) | verified meaning | verdict on comment |
|---|---|---|---|---|
| I_OFF1..3 | "number of checkers off" | menOffNonCrashed, 3-slot saturating ramp: 0–2 fills slot1 (/3), 3–5 slot2, 6–8 slot3 | checkers borne off, ramp-encoded | ACCURATE (encoding undocumented) |
| I_BREAK_CONTACT | "Position of the block furthest back." | :764 — np = Σ over MY checkers BEHIND their rearmost of (dist past it)·count, /167 | my pip-mass still engaged behind their rearmost checker; falls as I run past them; JUMPS when they sit on the bar (their rearmost = bar ⇒ my whole army counts — measured 0.198→1.102 on a hit) | **WRONG** — comment describes nothing in this formula |
| I_BACK_CHEQUER | "Position of the checker furthest back." | :830 — rearmost index /24 | as stated; 1.0 = on the bar | ACCURATE |
| I_BACK_ANCHOR | "Position of the back anchor." | :840 — rearmost ≥2-point index /24, searched down from back chequer | as stated, BUT silently falls back to ANY stack (a 13-point stack reads 0.500) — unreliable alone as a surrender signal | ACCURATE but trap undocumented |
| I_FORWARD_ANCHOR | "Position of the forward anchor." | :845–861 — most ADVANCED anchor in indices 18..back-anchor: (24−j)/6 ∈ (0,1]; outfield fallback (24−j)/6 > 1; 2.0 = none | home-zone anchor quality; golden point = 0.833 exactly; crossing above 1.0 = anchor left the zone (clean surrender threshold) | ACCURATE, zone semantics undocumented |
| I_PIPLOSS | "Average number of pips opponent loses from hits." | :1120 — Σ pips hit over the 21 rolls ×2, /432, enumerating THIS side's hitting rolls | expected pips the HITTER (this side) takes off the victim; the victim's exposure appears HERE, on the hitter's half | ACCURATE once the side is stated |
| I_P1 / I_P2 | "rolls that hit at least one/two checkers" | :1122 — count/36 from the same roll enumeration | as stated; hitter's half (measured: victim's blot → opp I_P1 0→0.389; two blots → I_P2 0→0.111 = 4/36) | ACCURATE, side undocumented |
| I_BACKESCAPES | "How many rolls let the back checker escape." | :1126 — Escapes(MY board, their back checker)/36 | rolls THEIR back checker escapes through MY wall — a CONTAINMENT input on the container's half, not the runner's | MISLEADING — reads as the owner's own escapes; it is the reverse |
| I_BACKRESCAPES | (none) | :1128 — Escapes1 twin of the above | same, alternate escape table (anEscapes1); moved in lockstep in every pilot | previously uncommented, now defined |
| I_ACONTAIN(2) | (none) | :1134 — worst-case Escapes over indices 15..their-back-checker; (36−min)/36 (+ square) | my containment over the zone their runner actually occupies | previously uncommented, now defined |
| I_CONTAIN(2) | (none) | :1148 — same over the full 15..23 (bar-restart when they have no checker back) | my general worst-case containment; 5-prime held 0.944 → dissolved 0.333 | previously uncommented, now defined |
| I_MOBILITY | "For all checkers, contribution of mobility." | :1155 — Σ_{i≥6} (i−5)·count·Escapes(THEIR board, i) /3600 | pip-weighted freedom of my outfield mass through their blockade; a 9-stack on a free point RAISES it — it is NOT a stacking penalty | MISLEADING as a name; formula says throughput, not distribution |
| I_MOMENT2 | "Second moment about centre of mass." | :1185 — mean index n of all checkers, then mean squared deviation of checkers BEHIND n only | one-sided (rearward) second moment about the mean | HALF-RIGHT — the one-sidedness is the useful part and is undocumented |
| I_ENTER | "Enter from the bar." | :1189–1217 — expected pip-LOSS while entering vs their home board (doubles lose 4·pt etc.), /(36·49/6); 0 unless on the bar | bar-entry pip-loss burden; nonzero only ON the bar | VAGUE — it is a loss expectation, not an event |
| I_ENTER2 | "Probability of entering from the bar." | :1227 — n = their made home points; (36−(6−n)²)/36 | CLOSURE ramp of the entry board: 2 pts = 0.556, 3 = 0.750, 6 = 1.0 — RISES as entering gets harder | **WRONG** — as a probability it is inverted; it is board closure |
| I_TIMING | (none) | :816 — rear mass (index-weighted) plus home spares beyond 2, minus borrowed gaps; /100 | the classic timing reserve — pips available before the position crunches; anchor-surrender 0.52→0.92, dissolution 0.32→0.72 | previously uncommented, now defined |
| I_BACKBONE | (none) | :1257 — consecutive rear points pair-weighted by gap (≤6 pips: weights 11..2); 1 − w/(tot·11) | connectivity of my rear points/anchors — LOW when they chain, HIGH when isolated; anchor→split blots 0.864→0.333... (inverted: 1−w/…, chained anchors give LOW w? measured says chained = HIGH value 0.864, split = 0.333 — value falls as structure fragments) | previously uncommented, now defined by measurement + formula |
| I_BACKG | (none) | :1272 — ≥2 anchors in THEIR home (indices 18..23): (rear checkers − 3)/4 | two-plus-anchor backgame mass — the backgame indicator | previously uncommented, now defined |
| I_BACKG1 | (none) | same block — exactly 1 such anchor: rear checkers /8 | single-anchor holding-game mass | previously uncommented, now defined |
| I_FREEPIP | (none) | :~770 — Σ (i+1)·count for my checkers IN FRONT of their rearmost, /100 | pip-mass already free of contact | previously uncommented; name accurate |

Method note. The dictionary was forced by three observed comment/behaviour
mismatches (BREAK_CONTACT, MOBILITY, ENTER2) and completed for all inputs
so no future signature or harvest prompt trusts an unverified comment.
Where measurement and formula disagree with a comment, the FORMULA and the
MEASUREMENT win, in that order of authority.
