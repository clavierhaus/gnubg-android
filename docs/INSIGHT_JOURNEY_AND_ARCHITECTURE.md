# The Coach Insight System: Journey, Discards, and Architecture

> The canonical, complete design of verbose coaching mode is
> `VERBOSE_COACHING_DESIGN.md`. This document is the shorter journey
> narrative and runtime map; where the two differ on design intent, the
> design document is authoritative.

Audience: developers who were not in the room. This document records how the
coaching "Why" layer came to be, which ideas were tried and discarded (and
why), and how the current system fits together. The binding law lives in
docs/DELTA_NARRATOR_PLAYBOOK.md and the two-tier doctrine in
docs/CORPUS_HARVEST_PLAN.md §0-bis; this document explains them.

## 1. The problem

gnubg tells a player THAT a move was bad (skill bands, equity loss) and
WHICH move was best. It never says WHY in words. The insight system exists
to add the why -- under two hard constraints chosen at the start and never
relaxed: everything runs offline and deterministic on the device, and gnubg
remains the sole authority on what a position means.

## 2. The two-tier doctrine

Tier 1 is gnubg: evaluations, equities, move rankings, skill bands, feature
inputs, position classes. Final. Nothing overrules it, reweights it, or runs
beside it as a second authority. Tier 2 is everything that turns tier-1
numbers into words, and it is subordinate by construction: it may phrase
what gnubg computed and nothing else. Where neither tier can speak, the
system stays silent -- silence is a feature, not a failure.

## 3. What was taken from backgammon-teacher, and what was rejected

yairwein/backgammon-teacher (MIT; correspondence with the author, 2026-07:
use freely, give improvements back, talk before any store launch) solved
the same problem with a runtime LLM. We took, with credit in the source:
the race/board/threat presentation taxonomy; the no-invention constitution
rule ("Only reference facts provided in the data"); the notable-threshold
shape; the plain-language register.

Rejected, with reasons on record:
- HIS RUNTIME LLM PATH: per-move network calls, non-deterministic output,
  and residual hallucination with no human gate. Our own offline harvest
  ran essentially his prompt fence and still produced a draft narrating "a
  retreat that doesn't exist in either board" -- caught only by maintainer
  curation. Fluent composition is not correct attribution.
- HIS FEATURE EXTRACTOR AS A SIGNAL SOURCE: a second authority beside
  gnubg. Briefly considered as a "cross-check" and retracted under the
  doctrine: a cross-check IS a second authority. (It survives only as an
  offline QA instrument for scenario FRAMING, never for position meaning.)
- A DATABASE for phrases: machinery without benefit at this size; the
  phrase bank is a versioned, tiered JSON blob under the same bake
  discipline as everything else.

## 4. The corpus tier (premium narratives)

Hand-authored entries -- currently 24 -- each with: a one-line principle,
a SIGNATURE in gnubg's own I_* input vocabulary (deltas between the played
and best boards, with direction, magnitude and band gates), a category, the
skill bands it may speak on, and two maintainer-adopted phrases (flag and
praise). Pipeline: parametric board-pair generators -> verification through
the pilot harness (built from this repo's own eval.c) -> signature
measurement and amendment -> in-session drafting against the credited
template -> maintainer curation ("take them all" is an adoption act) ->
bake (refuses any phrase not tier-authored) -> asset.

The matcher (runtime) does no interpretation: delta vector against baked
signatures, gates, at most one line per category, at most two lines total.

What the batches taught, kept as law:
- STATICS CARRY NO HISTORY. "Broke the five-prime" and "never made it" are
  the same position pair to gnubg's inputs; so are "abandoned the anchor"
  and "failed to make it"; so is the point-lost board crunch and the
  never-closed board. Where the distinction is history, one entry owns the
  pattern and the other wording waits.
- STATICS UNDER-SEE THE FUTURE. Pure-race efficiency (stack vs distribute,
  bearoff gaps, crossover waste) and the midpoint's bridge value live in
  future rolls; every attempted entry died on measured silence (I_MOBILITY
  flat, I_FREEPIP tracking pips not distribution, I_BACKBONE contrary).
  The race/bearoff family is therefore uncatalogued by honest necessity.
- NARRATIVE ALIASING IS INHERENT. Different game stories share delta
  shapes. A declined double hit was once narrated as "you left blots under
  fire" by two same-category entries stacking (field, 2026-07-20 00:09).
  Fixes: the envelope law (strongest entry per category, then the two-line
  cap) and context gates (e.g. the blot entries' story requires the
  opponent NOT on the bar; the hit family owns bar-side positions). The
  confusion matrix proves separation only on constructed pairs; the field
  keeps finding aliases, and each one becomes a measured amendment.

## 5. The narrator tier (the deterministic floor)

DELTA_NARRATOR_PLAYBOOK.md governs it; written before the first line of
code, at maintainer order, with seven laws (gnubg sole authority;
deterministic offline; subordinate to the corpus; maintainer-authored rule
table; measured never guessed; plain words, no raw numbers; loud failures)
and a strict phase order (rule table + proto -> adoption -> bake + Kotlin
-> device verification).

Eleven rules (after amendment 1) map single gnubg deltas to fixed
sentences: alias-proof by construction, because each sentence claims only
what its delta literally says. The narrator speaks only when a flagged move
matches no corpus signature. Amendment 1 was field-driven: a too-safe move
whose only notable delta ran the "wrong" way (the best play ACCEPTS a shot)
met a table that only knew best-is-safer directions; the gap was measured,
the rule adopted, the bake assertion moved 10 -> 11 deliberately.

Amendment 2 (in progress, phase A'): comparative rules quantified over the
FIXED top-five candidate list the verdict packs -- claims like "every
better play here makes the point; yours alone leaves it open", checkable by
the player against the rows on screen. Natural-language material is seeded
in a LIMITED maintainer-adopted phrase bank (subjects with better/worse verb
pairs, quantifier frames); the composer can say nothing outside the bank.
Evidence comes from the field (QUORUMPROBE) because bench pairs carry only
two boards.

## 6. Runtime architecture (one judged move)

  gnubg engine (tier 1)
    -> coach_verdict_pre packs ONE array: rank, equities, skill, played and
       best anMove, the pre-board, top COACH_K=5 candidate rows with evals,
       and the dice. Single source of truth; the UI re-derives everything
       from it (boards via gnubg's own ApplyMove; no parallel capture).
    -> CoachGlance (Kotlin decode)
    -> the panel: verdict header; the FIXED five-row candidate list (played
       marked P in place, or appended when it ranks outside); board try-on
       per row.
    -> the Why area, flagged moves only:
         InsightMatcher (corpus, authored asset)   -- speaks first
         DeltaNarrator (rules asset)               -- only if the matcher
                                                      found nothing
       Both: <= 1 line per category, <= 2 lines; identical envelope, so the
       player cannot tell tiers apart. Logcat can (gnubg-insight: fired= vs
       narrated=), and every decision logs its reasons, including the
       first-failing gate per near-miss.

## 7. The FOSS / Plus boundary

Public FOSS carries: the engine, the app, the coach (verdicts, candidate
list, gameplay), ALL pipeline tooling (tools/harvest, tools/narrator,
tools/pilot) and all documentation including this file. The Plus overlay --
enumerated exhaustively in plus-overlay.paths, enforced by
tools/plus/check_foss_parity.sh, which alarms on any drift outside the
manifest -- carries the Why layer's implementation and content: the two
baked assets (insights, narrator rules), InsightMatcher.kt, DeltaNarrator.kt,
plus branding and edition identity. Architecture is public; authored
content and its consumers are the product. Every public commit is mirrored
and merged to the private repo immediately (sync law), and the parity audit
runs after every sync.

## 8. Instruments and conventions

- tools/pilot/inputs_harness: the measuring instrument for gnubg inputs;
  its frame (mover = "me") is THE frame; all signatures were measured on it.
- tools/harvest/matcher_proto.py and tools/narrator/narrator_proto.py:
  offline mirrors of the Kotlin; divergence is a defect; the confusion run
  (currently 108 pairs / 108 top-1 / 0 foreign / 0 silent) gates every
  signature change.
- THE LOG RULE (maintainer order, 2026-07-20): causes are named from logs,
  never ahead of them. When a defect is reported and log data exists or can
  be captured, analysis starts from the captured lines; theories are
  written down only as questions the next capture must answer. Tooling for
  sifting logs is part of the system (tools/insight_log.py), not an
  afterthought.
- Field probes, named and disciplined: a probe is added BEFORE a frame or
  attribution theory is acted on (the one time this order was violated --
  the side-flip misdiagnosis of 2026-07-19 -- both halves were reverted the
  same day, and SIDEPROBE's data settled the question; the probe was then
  retired). Currently armed: APPLYPROBE (an unexplained zero-delta glance,
  2026-07-19 23:58, reproduction pending) and QUORUMPROBE (amendment-2
  evidence collection).
- BUILD COHERENCE: the app is Kotlin over a native facade; a fix that spans
  both MUST ship in one deploy. The class of bug this prevents is proven:
  a facade change went into the device's native libs, its revert shipped
  apk-only, and reverted Kotlin read un-reverted C for hours -- every
  feature delta zeroed exactly when only the mover's half differed between
  boards (the opponent's half being what the swapped read reported). The
  deploy script now refuses --apk-only when jni-bridge/ or engine-core/
  changed since the last native build (.native_build_stamp;
  --force-apk-only overrides deliberately). Corollary evidence law: field
  measurements captured under a stale native build are poisoned -- the
  amendment-2 quorum evidence was restarted from the rebuilt binary.
- Bake discipline everywhere: tiered artifacts, bake refuses "proposed",
  registration precedes baking, counts asserted before commit (a silent
  ENTRY_META skip once shipped a commit whose message claimed more entries
  than its asset carried; the history was rewritten and the lesson encoded).

## 9. Open items (as of 2026-07-20, early morning)

- Amendment 2 phase A': QUORUMPROBE field data -> quorum rule drafts ->
  maintainer adoption.
- (resolved 2026-07-20 ~05:50) The zero-delta ghost WAS the stale native
  build above; root-caused from APPLYPROBE fingerprints + QUORUMPROBE's
  identical-across-candidates signature + a sandbox reproduction that
  exonerated the committed C; fixed by rebuilding native; verified live by
  threat.risk.accepted firing on a quiet doubtful.
- Corpus batch 5 phrases (home.point.made.missed, opening.builder.wasted):
  drafted, curated, tier proposed -- awaiting maintainer adoption.
- Waiting list (may never close, by doctrine): race/bearoff efficiency,
  the history-wordings ("broke it"), the midpoint bridge, enter-from-bar
  anchor making.
