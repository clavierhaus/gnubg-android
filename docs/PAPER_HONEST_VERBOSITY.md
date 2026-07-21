# The Boundary to Hallucination: Foundations of Honest Verbosity in Machine Backgammon Coaching

*Draft — CBG / gnubg-android project. Companion document to
VERBOSE_COACHING_DESIGN.md, which holds the full engineering record; this
paper states the argument. All measurements referenced here are reproducible
from the tools in `tools/narrator/` of the source tree.*

## Abstract

Backgammon was among the first domains where machine play surpassed human
play, and it has remained for a quarter century a domain where machines
cannot say *why*. Neural evaluators produce equities and rankings; players
ask for reasons; and every system that has tried to bridge that gap has
faced the same choice — stay mute, or start inventing. This paper treats the
gap as a measurement problem rather than a generation problem. We define the
boundary to hallucination for an evaluator-grounded coach: a statement is
licensed if and only if its truth is established by the evaluator's own
output or by a measurable property of the position; everything else —
however plausible, however fluent — is confabulation. We describe a coaching
architecture bounded by that definition, an instrument for measuring where
honest explanation runs out (the *honesty curve*), and a null-baseline
method that tests whether a candidate explanatory rule has validity a
structure-free predicate cannot fake. We report that our own first
candidate tier *failed* this test — a negative result we regard as the
method working as designed, and as the strongest available evidence that
the instrument measures what it claims to. Verbosity, on this account, is
not a stylistic register but an evidential budget: a coach may spend
exactly as many words as it has measured facts, and the measured curve of
explanatory yield against error severity is the calibration of that budget.

## 1. The problem, twenty-five years old

Since neural evaluation reached expert strength in backgammon in the 1990s
and open, world-class analysis became universally available around 2000,
the situation of the learning player has been paradoxical. The best move in
almost any position can be had for free, instantly, with a numerical
account of exactly how much every alternative costs. What cannot be had at
any price is a trustworthy sentence explaining *what the mistake was*.

The evaluator knows the answer in the only sense that matters
competitively: its output ranks the moves. But its knowledge is stored as
network weights over engineered input features, not as propositions. The
number `-0.071` attached to an opening play is true, precise, and mute. A
generation of players has been told, in effect: study the rollout until
understanding precipitates. Some do. Most stop asking.

The engineering problem is therefore not to make an engine talk — anything
can be made to talk — but to make only *licensed* talk possible. That
reframing is the subject of this paper.

## 2. State of the art, critically assessed

We group existing approaches by the strategy they take toward the gap, and
assess each against two questions: does it explain, and is it honest?

**Numbers with adjectives (desktop analyzers).** GNU Backgammon and
eXtreme Gammon, the field's reference tools, annotate moves with skill
labels ("doubtful", "bad", "very bad") derived by thresholding the same
equity delta they display. This is the honest end of the field: every
statement is licensed by the evaluator, and nothing is claimed beyond it.
It is also the mute end: a label is a severity, not a reason. The player
learns *that* they erred and *how much*, never *what the error was*. As
explanation, twenty-five years of this amounts to a well-calibrated shrug.

**Pre-commit interruption (tutor modes).** eXtreme Gammon's tutor —
echoed by GNU Backgammon's own desktop tutor — verifies each move before
it is committed and interrupts when the intended play falls below a
threshold, offering reconsideration or a hint. Whatever its pedagogical
merits (the vendor's documentation calls the reconsider-and-retry loop a
good exercise), two criticisms stand. First, it still explains nothing:
the interruption points at a better move without a word about why the
chosen one fails. Second, the interruption itself outsources vigilance —
when the machine warns before every error, the absence of a warning
quietly becomes the player's judgment, and the habit of pre-commitment
scrutiny atrophies. The player is protected from mistakes rather than
taught by them.

**The promise economy (consumer apps).** In the mobile and web tier,
"AI coach" appears prominently in marketing and roadmaps — in at least one
prominent case as a feature "coming very soon" over an extended period —
while the shipped functionality remains hint buttons and error flags. We
note this pattern not to disparage particular vendors but because it
defines the trust environment any honest system enters: the phrase "AI
coach" has been spent in advance of the capability, and a system that
means it must now demonstrate the difference rather than assert it.

**Fluent generation (LLM wrapping).** The contemporary temptation is to
hand the position, the engine's ranking, and a prompt to a large language
model, which will return confident, idiomatic, pedagogically shaped
commentary. The failure mode is structural, not incidental: the language
model's account of *why* a move is wrong is not derived from the
evaluator's decision basis, because nothing ties the generated proposition
to a measured property of the position. The output is a plausible essay
about a position, not an explanation of this one. In a domain with an
authoritative ground truth standing right there, fluent confabulation is
strictly worse than silence: it spends the authority of the true number on
sentences with no such warrant, and the learner cannot tell which words
are load-bearing. We regard unconstrained generation over engine output as
the canonical hallucination architecture, and the field's current default.

**Hand-authored pattern commentary.** The oldest approach — expert-written
commentary attached to recognized position classes — is honest where it
fires and silent elsewhere, which is the correct shape. Its historical
weaknesses are coverage (catalogues are small and expensive), staleness,
and above all *unvalidated attachment*: the rule that decides whether a
catalogued comment applies to the present position is typically authored
by intuition and never measured. A true sentence attached by an unreliable
trigger is a hallucination with extra steps.

The assessment, compressed: the honest systems do not explain, the
explaining systems are not honest, and the market has been promising the
combination without building it.

## 3. Defining the boundary

We now say precisely what we mean by hallucination, since the term is
doing the work. Fix an evaluator E (here: GNU Backgammon's neural
evaluation over its engineered input features) as the authority. For a
coaching statement S about a position and a played move:

- S is **licensed** if its truth is established by (a) E's own output —
  the ranking, the equities, the candidate list, verbatim or arithmetically
  restated; or (b) a *measured predicate* of the position — a property
  computable from the board that has been shown, by measurement against E,
  to track what E's evaluation rewards; or (c) a literal comparative fact
  between two concrete boards ("the played move leaves two blots where the
  best move leaves one").
- S is a **hallucination** if it asserts an explanatory or causal claim
  whose truth is not so established — *regardless of whether the claim is
  plausible, conventional, or even true*. Unverifiable truth spends trust
  it cannot back.

Two consequences are worth drawing out. First, the boundary is about
*license*, not about style: a single unlicensed clause inside otherwise
verbatim output crosses it. Second, silence is always licensed. A coach
that says nothing where it has nothing measured is not failing; it is
holding the boundary. This inverts the usual product instinct, in which
silence reads as missing functionality. We hold that in explanation, the
missing functionality of the field is precisely the silence.

## 4. An architecture bounded by the definition

The shipped coach is three layers over one honesty threshold, each layer's
license stated:

**The verdict tier** (license: E's output, clause a). Every judged move
shows its rank among the legal plays, its equity cost, and the top
candidates with before/after board views. This tier is the entire factual
substrate; it can never hallucinate because it never composes.

**The corpus tier** (license: clause b, with the honesty threshold).
A small catalogue of position-class commentary (currently 24 entries),
each attached not by intuition but by explicit measurable signatures over
E's own input features, verified present in the position before a word is
shown. Where the signature does not fire, the entry stays silent.

**The pairwise narrator floor** (license: clause c). Where no catalogue
entry applies but the move is flagged, a narrator states literal
differences between the played and best resulting boards, from a fixed
rule set (currently 11 rules), each rule emitting only what is true of the
two boards by construction. This is deliberately the *floor*: comparative
fact without causal claim.

Nothing else speaks. In particular, there is no generative layer, and the
architecture has no place where one could be added without crossing the
section-3 boundary — which is the point of stating the boundary first and
building second.

## 5. Measuring where explanation runs out

An architecture that only speaks when licensed raises the empirical
question: how often is that? If honest coverage is negligible, honesty is
an excuse. This required an instrument.

**The honesty curve.** Using the engine itself as the data source — an
opening bank of 45 lines derived from E's own top replies to the 15
opening rolls, avoiding any external data of unclear provenance — we
generate candidate decision sets by deterministic self-play and measure,
per error-severity band, the fraction of flagged decisions on which a
given explanatory tier has something licensed to say. Yield plotted
against severity is the honesty curve. Its reading is the verbosity
calibration of section 7: where the curve falls to the floor, the words
must stop.

**The candidate tier and the null baseline.** Above the narrator floor we
drafted a candidate tier of quorum predicates — structural conditions
(chequer on the bar, blot in the opponent's home board, and three others)
intended to *isolate* which candidate move the error concerns, licensing a
stronger explanatory sentence. Early measurement looked encouraging: the
yield of the leading predicates rose monotonically with severity
(3.8% to 9.6% on the first curve; on the full bank, on-bar events rose
from 14.1% to 30.2% across severity bands). Before adoption we required a
control: *null predicates* — structure-free conditions with no possible
explanatory content (parity of occupied odd points; whether the 13-point
is occupied) — run through the identical pipeline. Whatever score a null
can achieve is the score's noise floor.

The nulls were fatal, twice. A first agreement metric (does the predicate
still fire on the higher-ply candidate set?) scored the parity null at
91.2% — *between* the two real predicates — exposing the metric as
measuring candidate-set stability under depth, not explanation. The
corrected metric — equity alignment: is the predicate-isolated candidate
systematically best or worst at the authority ply? — scored the real
predicates at 8.4% and 3.8%, and the parity null at 10.9%. The null beat
both.

**The negative result.** None of the five drafted predicates shows
explanatory validity a structure-free null cannot fake. The rising yield
was real but meant only that hard positions contain more on-bar events,
not that the on-bar move is the error. The candidate tier was therefore
not adopted; the shipped coach's upper tier remains the corpus, and its
floor the pairwise narrator. The design document records the correction
over its own earlier optimism, dated.

We present this negative result as the paper's central evidence, for a
reason worth stating plainly: a measurement method that cannot return "no"
is advertising. This one returned "no" to its own authors' candidate, over
their initial positive reading, on the strength of a control they almost
did not build. That is what the boundary to hallucination looks like as an
engineering practice rather than a slogan.

**Measuring the vocabulary itself.** The same alignment machinery, pointed
at E's own input features instead of our predicates, yields the empirical
map the corpus tier had been missing: which of the evaluator's inputs
actually carry explanatory weight. Over 5,822 candidate sets, the deltas
of pip-loss-under-hitting and the primary point count dominate and grow
with the stakes; a middle band (entry, mobility, free-pip, containment
features) sharpens two- to threefold as decisions get serious; and a
tail of inputs barely moves between best and worst play at all. Two
findings matter beyond the ranking. First, the corpus's signatures had
been authored — by the project's own admission in its harvest notes — as
a guess from reading feature names; they can now be authored against
measurement. Second, the map corrected a standing belief of ours:
features we had recorded as inert had been tested only inside narrow race
positions and are in fact among the most stakes-responsive across the
full decision space. The instrument audits its own project's folklore,
which is the property we wanted.

## 6. The verbosity foundation

We can now state the paper's account of verbosity. In coaching systems,
verbosity is conventionally a register — terse versus chatty, a UX
preference. Under the section-3 boundary it becomes an *evidential
budget*: each additional sentence must be purchased with an additional
licensed fact, and the honesty curve prices the purchase. A coach's
verbosity ceiling at a given severity is the measured yield of its
licensed tiers at that severity — not a design choice, a measurement. The
practical corollary is that verbosity claims are falsifiable: a system
that speaks more than its curve supports is, by that fact, hallucinating
somewhere.

This account also explains why the negative result cost the product
nothing. The candidate tier was gated behind adoption, adoption was gated
behind the measurement, and the measurement said no; the shipped coach
never spoke words it later had to retract. The posture generalizes: claim
only what ships, publish the negatives, and let the curve — not the
roadmap — set the promise.

## 7. Limitations and open questions

The corpus is small (24 entries) and its signatures, though verifiable,
were authored before the vocabulary map existed; re-grounding them on the
measured inputs is open work. The narrator's 11 rules cover the
comparative floor but their coverage has been measured only over the
opening bank, which — though engine-derived and license-clean — samples
early-game structure disproportionately. The central open question is
whether explanatory predicates *constructed* to isolate the
equity-relevant candidate (rather than correlated structural features) can
clear the null baseline, or whether pairwise comparative narration is the
honest ceiling for machine explanation over an opaque evaluator. The
instrument to answer this exists and is part of the source tree; the
answer will be accepted either way.

## 8. Conclusion

The gap between machine strength and machine explanation in backgammon is
not closed by better generation but by better measurement. Define the
license, build only licensed tiers, measure where their words run out,
and control every claimed signal against a null that cannot possibly
explain. On that method, our own best candidate for richer explanation
failed, and the shipped system says less than we hoped and everything it
can back. We commend the trade to the field: in a domain with ground
truth, the coach that does not hallucinate is not the one with the best
words but the one whose silence is exact.
