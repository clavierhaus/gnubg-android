# Verbose Coaching Mode: Complete Design

This is the authoritative design document for CBG's verbose coaching mode --
the layer that answers *why* a move was an error, not just *that* it was. It
records the goal, every approach that was tried and discarded (with the
reason each failed), and the design that now ships. It is written for a
developer who has never seen the system and needs to understand not just how
it works but why it is shaped the way it is, so that no discarded path is
re-attempted from scratch.

Companion documents: `INSIGHT_JOURNEY_AND_ARCHITECTURE.md` (a shorter journey
narrative and the runtime map), `DELTA_NARRATOR_PLAYBOOK.md` (the binding law
for the deterministic tier), `CORPUS_HARVEST_PLAN.md` (the corpus pipeline in
full detail). Where any of those disagree with this document on design intent,
this document is canonical; where they carry more operational detail, they
govern the detail.

The shipped natural-language *assets* (the baked corpus and rule blobs) are
part of the Plus edition. The *design, reasoning, and architecture* recorded
here are open, because open development is the only home of the project's
thinking. This document therefore lives in FOSS.


## 1. The problem, stated precisely

Every strong backgammon engine -- gnubg, XG, the commercial bots -- tells you
*what* the best move is and *how much* equity a chosen move gave up. None tell
you *why* in words a learner can carry to the next position. The market gap is
real and repeatedly voiced by players: "they tell me the best move, but not
why it is better."

The naive framing of the gap is that it is a presentation problem -- that the
understanding exists in the numbers and merely needs human language wrapped
around it. This framing is wrong, and getting it right is the whole design.

There are two regimes:

- **Blunders and clear errors.** A large equity gap usually corresponds to a
  real, statable structural cause: a point left open, a blot exposed for
  nothing, a prime broken, an anchor abandoned. Here a "why" exists and can be
  said truthfully.

- **Close decisions.** At the 0.00x equity scale, the difference between two
  plays is a tiny statistical edge integrated over millions of rollouts. No
  verbal principle *generates* that difference. A titled human asked "why this
  over that?" would mostly answer "it feels right, and the bot agrees" -- a
  post-hoc rationalization, not a derivation. Demanding a "why" for these is
  demanding the software fabricate a story.

The design therefore commits to an **epistemic honesty threshold**: explain
where a real structural reason exists; stay silent where only a statistical
whisper does. Silence is not a failure mode. It is the system refusing to
lie, and it is the single property that separates a trustworthy coach from a
plausible one.


## 2. Discarded approaches, with reasons

Each of these was seriously considered or partially built. Each is recorded
here so it is not re-attempted without confronting the reason it failed.

### 2.1 A runtime LLM (the backgammon-teacher path)

The reference implementation for this whole space is Yair Wainberger's
`yairwein/backgammon-teacher` (MIT-licensed; the author was corresponded with
in July 2026 and agreed to free use with improvements contributed back and a
conversation before any store launch). Its architecture: gnubg CLI -> a
TypeScript feature extractor -> a delta-grounded prompt to a server-side LLM
(Claude/GPT/Gemini) with a race/board/threat triage schema, an "only
reference facts provided in the data" constitution, and a confidence
self-rating.

What was taken from it, with credit in our source: the race/board/threat
presentation taxonomy; the no-invention rule; the notable-threshold shape;
the plain-language register.

Why the runtime LLM itself was rejected:
- **Per-move network calls** break the offline, no-account, no-tracking
  premise that the whole app is built on.
- **Non-deterministic output** cannot be verified, versioned, or held to a
  bake discipline.
- **Residual hallucination with no human gate.** This is the decisive one.
  Our own *offline* harvest ran essentially the same prompt fence -- the same
  "only reference provided facts" constitution -- and still produced a draft
  that narrated "a retreat that doesn't exist in either board." Fluent
  composition is not correct attribution. An LLM will generate a confident,
  well-formed reason for a 0.002 difference every time, and it will be fiction
  every time, because there was no signal to explain. The very scale at which
  players most want a "why" is the scale at which an LLM most reliably
  invents one.

The LLM survives in the pipeline in exactly one place: as an *offline*
drafting aid for candidate phrasing, whose every output is subject to
maintainer curation before it can be baked. It never runs at inference time
and is never an authority on position meaning.

### 2.2 A second feature extractor as a cross-check

It was briefly attractive to keep backgammon-teacher's feature extractor as a
"cross-check" against gnubg -- a second opinion. This was retracted under the
two-tier doctrine (section 3): a cross-check *is* a second authority. Two
authorities that can disagree require a resolution rule, and any resolution
rule that can overrule gnubg violates the doctrine, while any that cannot is
not a cross-check. The extractor survives only as an offline QA instrument
for scenario *framing* (does this constructed test position exhibit the shape
we intended?), never for position *meaning* at runtime.

### 2.3 A database for the phrase material

A relational or key-value store for the natural-language phrases was rejected
as machinery without benefit at this scale. The phrase material is a few
hundred short strings, versioned and adopted as a unit. A baked, tiered JSON
blob under the same discipline as everything else is simpler, diffable,
reproducible, and sufficient. This decision is revisited only if the phrase
count grows by an order of magnitude.

### 2.4 Cataloguing the race and bearoff family

Multiple attempts were made to author corpus entries for pure-race decisions
(stack vs distribute, bearoff gaps, crossover waste) and for the midpoint's
bridge value. Every one died on *measured silence*: the gnubg static inputs
that the whole system reads simply do not carry the distinction. I_MOBILITY
came back flat across the plays that differ; I_FREEPIP tracks pip count, not
distribution; I_BACKBONE ran contrary to the intended reading. The race and
bearoff family is therefore left uncatalogued -- not by oversight but by
honest necessity, because the signal is not present in the vocabulary the
system is allowed to speak from. This is the epistemic honesty threshold
enforced at the level of what can be authored at all.


## 3. The two-tier doctrine (the governing principle)

Everything the system does is subordinate to one rule:

**gnubg is tier 1: the sole, final authority for every game decision, every
legal move, every equity, every cube evaluation. Nothing overrules a gnubg
decision, and nothing runs beside it as a parallel authority. There is no
alternative to a gnubg decision.**

The verbose "why" is **tier 2**: it operates only on the *residue* that falls
through gnubg's decision matrix -- the numbers-without-words, the uncovered
corner cases. It is built on proven models (backgammon-teacher is the
reference), it is subordinate by construction, and it never contradicts tier
1. Where neither tier has anything true to say, the system is silent.

This is why there is no cross-check, why the LLM never judges at runtime, and
why whole families of positions are left unexplained: each is an application
of the doctrine.


## 4. The solution: three layers over one honesty threshold

The shipped design is three layers, consulted in order, all gated by the
honesty threshold. The verdict itself (rank, equity cost, the fixed five-
candidate list) is FOSS and always shown; the explanatory layers below are
the Plus content.

### 4.1 Layer 1 -- the corpus tier (authored narratives)

Hand-authored entries (24 at time of writing), each carrying: a one-line
principle; a SIGNATURE expressed in gnubg's own I_* input vocabulary (deltas
between the played board and the best board, with direction, magnitude, and
band gates); a category (race / board / threat); the skill bands it may speak
on; and two maintainer-adopted phrases (one for the flagged case, one for
praise).

The runtime matcher does no interpretation whatsoever: it computes the delta
vector, tests it against the baked signatures and gates, and emits at most one
line per category and at most two lines total. All meaning was decided at
authoring time by a human; the matcher only recognizes.

Three laws the authoring batches proved, now permanent:

- **Statics carry no history.** "Broke the five-prime" and "never made it"
  are the same board pair to gnubg's inputs; so are "abandoned the anchor"
  and "failed to make it." Where the distinction is historical, one entry
  owns the pattern and the other wording is not written.
- **Statics under-see the future.** The race/bearoff silence of section 2.4.
- **Narrative aliasing is inherent.** Different game stories share delta
  shapes. A declined-double hit was once narrated as "you left blots under
  fire" by two same-category entries stacking. The fixes are structural: the
  *envelope law* (only the strongest entry per category speaks, then the two-
  line cap) and *context gates* (the blot entries require the opponent not on
  the bar; the hit family owns bar-side positions). Aliasing is never
  "solved"; each new alias found in the field becomes a measured amendment.

### 4.2 Layer 2 -- the narrator tier (the deterministic floor)

Governed by `DELTA_NARRATOR_PLAYBOOK.md`, written in full before its first
line of code at maintainer order. Seven laws: gnubg sole authority;
deterministic and offline; subordinate to the corpus; maintainer-authored
rule table; measured never guessed; plain words with no raw numbers; loud
failures. Strict phase order: rule table + proto -> maintainer adoption ->
bake + Kotlin -> device verification.

Eleven rules (after amendment 1) each map a single gnubg delta to one fixed
sentence. They are alias-proof by construction: a sentence claims only what
its one delta literally says. The narrator speaks only when a flagged move
matches no corpus signature -- it is the floor beneath the corpus, ensuring a
flagged move is not left wordless when a single honest thing can still be
said. Amendment 1 was field-driven: a too-safe move whose only notable delta
ran the "wrong" way (the best play *accepts* a shot) met a table that only
knew best-is-safer directions; the gap was measured and one rule adopted,
moving the bake assertion from 10 to 11 deliberately.

### 4.3 Layer 3 -- amendment 2: comparative rules over the candidate set

The narrator's original rules are *pairwise* -- played board vs best board.
Amendment 2 adds a new claim type: **quorum predicates evaluated over the
fixed top-five candidate list** the verdict already packs (K = COACH_K = 5).
These are claims quantified over the closed, on-screen set:

- "Every play but yours keeps the anchor."
- "Yours is the only candidate that breaks contact."
- "All five top plays hit -- the question is only where."

The grounding argument is what makes this worth doing: a pairwise claim asks
the player to trust the narrator's reading of two positions, but a quorum
claim quantified over the *visible* rows is **checkable by the player against
the screen**. If the narrator says "yours alone leaves a blot," the player
confirms it by looking at the five rows. This is the strongest grounding the
narrator can offer -- the claim is falsifiable by the reader in the moment.

Three law extensions govern it:
- **L1 extended:** a comparative sentence must be derivable from the candidate
  boards' gnubg values alone, and the quantifier must be *literally* true over
  the packed set -- "every", "the only", "all five", never "most", never
  "usually". A quorum claim is exact or it is not made.
- **L4 extended (the phrase bank):** comparative natural-language material
  lives in a seeded, limited, maintainer-adopted store -- a baked JSON blob,
  versioned and tiered exactly like the rules. The composer may only assemble
  sentences from adopted bank entries; no free text in code.
- **L5 applied (measured on real position sets):** quorum thresholds and
  triggers cannot be validated on bench pairs, because a bench pair carries
  only two boards and a quorum claim needs the whole candidate set. They must
  be measured over a corpus of real decision positions with their full
  candidate lists (section 5). No quorum rule ships before its evidence is
  reviewed and the rule is adopted.
- **Adoption by reference-authority agreement (not maintainer judgment).** A
  drafted quorum rule is adopted if and only if its predicate is CONFIRMED by
  a higher-authority gnubg evaluation across the corpus, above a threshold
  fixed IN ADVANCE. The authority is gnubg itself at a reference strength
  materially stronger than the verdict's (the verdict runs 0-ply; the
  authority runs a fixed higher ply or short rollout) -- the same engine, a
  deeper truth, GPL-clean by construction and reproducible via the
  deterministic-noise mechanism (section 5.2). For each drafted predicate the
  harness measures how often the predicate as evaluated on the verdict's
  candidate set AGREES with the same predicate on the authority's evaluation;
  a rule earns adoption only if agreement clears a PRE-REGISTERED threshold,
  recorded in this document before results are seen so it cannot be tuned to
  bless a favoured rule. Each adopted rule ships with a citable adoption
  record (corpus version, authority setting, measured agreement rate) checked
  into the tooling, so anyone can re-run the measurement and verify the rule
  met its bar. The maintainer sets the authority setting and the thresholds
  ONCE, in advance (a policy decision), and may veto on grounds outside the
  metric, but cannot hand-pick rules the evidence does not support -- personal
  bias is designed out. Where published reference sources apply (opening
  rollouts, standard cube positions, match-equity tables), the adoption record
  may additionally CITE them as corroboration -- referenced, never
  incorporated, so no copyrighted text is baked and GPL stays clean.

Phase order, primed to distinguish it from the narrator's own phases: A'
(instrumentation + drafts) -> B' (maintainer adoption) -> C' (bake + Kotlin
composer) -> D' (device verification).


## 5. The reproducible evidence corpus (amendment 2's instrument)

Amendment 2's rules are measured, not guessed (L5). The measurement needs a
body of real decision positions, each with gnubg's full ranked candidate set,
over which candidate-set statistics can be computed. The first instrument for
this was on-device QUORUMPROBE logging during live play. That instrument is
sound but its *source* is not ideal: it depends on one player's move choices,
which oversample the blunder-types that player personally makes and never
visit the quorum situations they would have to misplay to trigger. It is also
not reproducible, and reproducibility matters here for two reasons: sound
methodology, and GPL corresponding-source. If a generated position corpus
shapes the shipped rules, that corpus's generation must be re-derivable from
source, or the "corresponding source" for the baked rules is incomplete.

The design is therefore a reproducible, headless, two-stage harness that needs
no play session and no device. It is Plus tooling (it produces Plus rule
evidence) built entirely on FOSS engine primitives.

### 5.1 The primitives it stands on

All three are existing gnubg / engine-facade surface, verified present:
- `setGnubgId(id)` installs any position from a GNU BG ID / XGID -- the same
  path pasted analysis uses. Arbitrary positions, fed headlessly.
- `hintMoves(maxN, outEquity, outMoves)` returns gnubg's own ranked candidate
  list for the loaded position (via `FindnSaveBestMoves`): up to maxN
  candidates, each as an anMove (four src/dst pairs) plus its equity.
- `applyMoveToBoard(board, anMove)` reconstructs each candidate's resulting
  board, from which the same subject features the QUORUMPROBE logs are
  extracted.

The loop is: for each position ID -> setGnubgId -> hintMoves -> per candidate
applyMoveToBoard -> extract features -> compute quorum statistics. No play,
fully deterministic, hundreds of positions in seconds on the desktop.

### 5.2 Reproducible noise, and why gnubg already provides it

Clean gnubg self-play only visits well-played positions and glides through the
contact-heavy positions where quorum claims are most useful. Noise is wanted,
to reach human-blunder-adjacent positions. But noise must be reproducible --
methodologically and for GPL.

gnubg's evaluation noise is already designed for this. `evalcontext` carries
`rNoise` (the standard deviation) and `fDeterministic`. When `fDeterministic`
is set, `Noise()` (eval.c) derives the perturbation as a pure function of the
board: it MD5-hashes the board bytes and derives the noise value from the
digest. There is no RNG state, no wall clock, no hidden seed -- **the same
position always receives the same noise.** This is the mechanism the strength
levels already use (Beginner rNoise = 0.060, casual 0.050, intermediate
0.040, advanced 0.015, expert 0.0).

Consequently a noise-injected self-play run at a chosen strength is fully
deterministic and reproducible *from the board positions alone*. Anyone can
re-run it and obtain byte-identical positions. This satisfies both the
methodological and the GPL requirement without inventing any randomness
source of our own.

### 5.3 The two-stage pipeline

- **Stage 1 -- generation (one-time, dumps a static, checked-in list).** A
  gnubg self-play run at a fixed starting position and a chosen strength level
  (which sets rNoise with fDeterministic = TRUE) produces a deterministic
  position stream. Every decision position's GNU BG ID is dumped to a
  versioned file under the narrator tooling. Because the noise is a pure
  function of position, the run is reproducible; because the ID list is
  checked in, it is stable and inspectable even if engine behaviour ever
  shifts. Breadth across game phases (opening / middlegame / bearoff-contact)
  is obtained from a small, recorded set of starting positions and strengths,
  all fixed and disclosed.
- **Stage 2 -- measurement (deterministic, re-runnable).** The harness loops
  the checked-in ID list through the primitives of 5.1, emitting the same
  candidate-set statistics the on-device QUORUMPROBE emits, but over the whole
  corpus at once. The output is a data file the maintainer reads to see which
  quorum predicates actually hold, and how often -- the evidence B' adoption
  is gated on.

Every step is re-derivable from source: fixed starting positions + recorded
strengths + the deterministic-noise engine -> the checked-in ID list -> the
measurement output. This is the GPL-clean, reproducible-noise corpus the
methodology requires.


## 6. Runtime architecture (one judged move)

When the player commits a move in coaching mode:
1. gnubg evaluates and ranks; the verdict packs the played move's rank, the
   equity cost, and the fixed top-five candidate list (FOSS; always shown).
2. If the move is flagged (skill != none), the explanatory layers are
   consulted, corpus first: the matcher scores gnubg's own feature deltas
   between the played and best boards against the baked signatures.
3. If a corpus entry fires, its phrase is shown (envelope law: strongest per
   category, two lines max) and the narrator stays silent.
4. If no corpus entry fires, the narrator tier speaks if any single-delta rule
   (or, under amendment 2, any quorum predicate over the candidate set) is
   literally true.
5. If neither has anything true to say, the panel is silent.

Silence at step 5 is correct behaviour, not a gap.


## 7. The FOSS / Plus boundary

FOSS ships: the engine, the coach verdict (rank, equity cost, classification),
the fixed five-candidate list with the before/after board toggle, and all of
the *design and reasoning* -- including this document. A learner on the free
edition sees what gnubg judged and by how much, and can compare candidates on
the board.

Plus ships: the authored corpus asset, the narrator rule and phrase-bank
assets, and the composer that assembles them -- the natural-language "why".
The boundary is content, not capability: the free edition is a complete,
honest analysis tool; the paid edition adds the words.


## 8. Status and open items (2026-07-20)

- Corpus tier: 24 entries, device-verified, live in Plus.
- Narrator tier: 11 rules, phases A-D complete and device-verified. It speaks
  on an uncatalogued flagged move, stays silent when the corpus fires, and
  shows nothing on an unflagged move -- all confirmed on device.
- Amendment 2: phase A' complete (QUORUMPROBE instrument live and, since the
  stale-native-build fix, producing clean data; rules and phrase bank drafted
  at tier proposed). Blocked at B' (maintainer adoption) pending the
  reproducible evidence corpus of section 5, which supersedes the biased
  play-session method. C' (bake + composer) and D' (device verification) not
  started.
- The reproducible harness of section 5 is designed (this document) and not
  yet built.
