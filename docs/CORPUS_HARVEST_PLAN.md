# CORPUS_HARVEST_PLAN.md -- LLM-drafted harvest of the phrase corpus

Status: PLAN (2026-07-12). Extends CORPUS_PLAN.md. Governs the workflow that
turns the Insight-layer aspiration in COMPANION.md into a shipping static
corpus, using LLMs offline as first-draft engines and gnubg as the sole
authority for what a position means.

Maintainer decision this session: Option A. No model bundled in the APK; no
network at play time; ship a hand-verified static JSON. Credit Yair
Weinberger's yairwein/backgammon-teacher (MIT) throughout, where its schema
shape, threshold table, taxonomy, and prompt structure shaped ours.

## 0. Maintainer decision on the gnubg manual (2026-07-12)

Supersedes the GFDL firewall of Part 1 / §7e below for the gnubg manual
specifically. The maintainer's reading: the GFDL states for text essentially
what the GPL states for code, and the claimed GFDL/GPL incompatibility has
been offered in vague terms only, without specificity, case law, or rms
correspondence. The gnubg manual (GFDL) WILL be used as source material for
the insight corpus. A mail to Richard has been sent; the maintainer stands by
this decision by name and commits to immediately pulling all GFDL-derived
content from the project should the answer contradict it. Nothing in Phase A
or B touches manual text, so no work is gated on the reply.

## 1. What is taken and what is not from yairwein/backgammon-teacher

Verified this session against the actual repo (LICENSE, src/lib/*). MIT
license, GPLv3-compatible one-way in the direction we need.

  TAKEN, credited:
    - the race / board / threat presentation taxonomy from
      llm/prompt.ts SimpleExplanation;
    - the notable-delta threshold shape from features/extract.ts
      getNotableThreshold (as a starting silence gate);
    - the prompt scaffolding rule "Only reference facts provided in the
      data. Never invent strategic facts." -- our constitution in their
      words.

  NOT TAKEN:
    - their extractFeatures pipeline. They compute features in TypeScript
      from their own board type; we do not, because gnubg computes them
      already and would disagree at edges (prime, anchor, trapped,
      wastage). Sole-authority rule applied to state as to logic.
    - their runtime LLM path. They call Anthropic/OpenAI at play time;
      we do not, because the APK ships offline.
    - their FeatureVector as concrete field names. Their names carry
      heuristics gnubg does not compute the same way (see §2).

  NOT AVAILABLE TO TAKE:
    - a phrase library. Their prose is generated at request time; there
      is no static corpus in the repo, so "just use theirs" is a
      misreading of what they ship.

## 2. Features come from gnubg, in gnubg's own vocabulary

CORPUS_PLAN.md Part 5 anticipated a Phase A features verb. The verb
already exists (verified this session):

  - engine-core/eval.h:553-618 : enum { I_OFF1..I_BACKRESCAPES, MORE_INPUTS }
    -- gnubg's own named neural-net input indices, each with a source
    comment: "Position of the back anchor", "Number of rolls that hit at
    least one checker", "How many rolls let the back checker escape",
    and so on.
  - engine-core/eval.h:620 : extern CalculateHalfInputs -- fills afInput[I_*].
  - engine-core/eval.h:373 : extern PipCount(TanBoard, unsigned int[2]).
  - engine-core/eval.c:1641 : ClassifyPosition(TanBoard, bgv) -> positionclass.
  - jni-bridge/src/gnubg_mobile.c:473 : int gnubg_mobile_position_features
    (const int board[50], float out[]) -- returns 2*MORE_INPUTS floats:
    out[0..MORE_INPUTS-1] for side 0, out[MORE_INPUTS..] for side 1.
  - jni-bridge/src/native-lib.c:635 : Engine_positionFeatures JNI wrapper.
  - gnubg-app/.../Engine.kt:141 : external fun positionFeatures(IntArray):
    FloatArray. End-to-end wired.

Adopting backgammon-teacher's playerLongestPrime, playerWastage, etc. as
field names would put us back in the position of naming a concept and
picking a formula for it -- exactly what CLAUDE.md forbids. The rule is
applied here as:

  - The corpus SIGNATURE is written in gnubg's I_* names (or PipCount, or
    positionclass). Signals gnubg does not compute do not appear in
    signatures. If a pattern the maintainer wants to teach has no gnubg
    signal, the phrase waits.
  - The PRESENTATION to the user borrows backgammon-teacher's category
    words (race, board, threat) -- shape of the OUTPUT, not shape of the
    signal. Each corpus entry is tagged with one category; the UI shows
    at most one line per category, at most two lines total, or none.

## 3. Phase A verbs -- what exists, what is missing

Verified this session by grep + reading:

  positionFeatures : DONE. `gnubg_mobile_position_features(board[50], out[])`
    returns 2*MORE_INPUTS floats -- side 0 in the first half, side 1 in the
    second. Raw normalised CalculateHalfInputs output, both sides. No
    interpretation, no denormalisation. This is the input the matcher scores
    corpus signatures against.

  positionClass : DONE (corrected 2026-07-12; this plan previously said
    MISSING, unverified). gnubg_mobile_classify(board[50]) -> int wraps
    ClassifyPosition (jni-bridge/src/gnubg_mobile.c:1186), JNI
    Engine_classifyPosition (native-lib.c:781), Kotlin external
    classifyPosition (Engine.kt:145). Note: passes VARIATION_STANDARD, not
    ms.bgv -- acceptable for the corpus (standard backgammon only), recorded
    here so nobody "fixes" it blind.

  pipCount : DONE (same correction). gnubg_mobile_pip_count(board[50],
    out[2]) wraps PipCount (gnubg_mobile.c:607), JNI Engine_pipCount
    (native-lib.c:735), Kotlin external pipCount (Engine.kt:137).

  All three Phase A verbs therefore exist end-to-end; there is no Phase A
  code to write. The MISSING claims above were asserted without the Q0
  grep this contract requires -- the correction is the lesson.

Convention for the caller: the C facade's existing positionFeatures returns
side 0 first, side 1 second, regardless of ms.fMove -- callers that need
"me / opp" reorient client-side using ms.fMove read from get_match_state.
The two new sibling verbs will follow the same "player-0-first" packing.

Order matters. One-fact-per-verb keeps each verb's Q0/Q1 checkpoint local
and matches the existing get_board / get_match_state / get_cube_info style.
Do NOT expand positionFeatures to fold pip counts and class into it.

## 4. Workflow

Sequenced, each stage a separable commit:

  A. Phase A verbs. positionFeatures done; add positionClass and pipCount
     siblings. Verified against a hand-computed baseline (opening position
     pips 167/167, position class CONTACT). Nothing user-visible.

  B. Signature-driven phrase list. `docs/CORPUS_ENTRIES_DRAFT.md`,
     maintainer-owned, assistant proposes. ~40-80 entries, each with:
       * stable id (e.g. "prime.break.5", "anchor.surrender.mid");
       * one-line teachable principle;
       * target signature in I_* terms with expected direction of delta;
       * race / board / threat category;
       * which gnubg skill bands this phrase can accompany.
     No prose phrases yet -- this is the shelf where phrases will go.

  C. Position corpus. Signature-first, not random:
       C.1  Parametric construction. For each entry in B, build ~10-30
            positions instantiating the signature; verify each with a
            Phase A roundtrip that the intended signal is present.
       C.2  gnubg self-play supplement. Headless run from a seed set;
            filter for positions where the target delta appears between
            played and best after gnubg analysis.
       C.3  gnubg reference matches shipping upstream. Positions are
            facts, not copyrightable; we read board states only, never
            quote surrounding prose.
     Excluded: positions extracted from copyrighted books or lesson
     sites. Committed to `tools/harvest/positions/*.json`, each entry
     carrying its provenance (parametric / self-play / gnubg-refmatch).

  D. Harvest run. Offline Python script `tools/harvest/harvest.py`.
     Header carries backgammon-teacher's MIT notice for the ported
     prompt structure, then this repo's GPLv3+ header. For each
     position, the script:
       1. Loads the position into gnubg headless via a small Python
          driver; captures the coach-verdict inputs (played board,
          best board) plus Phase A features on each.
       2. Builds a system prompt in the shape of buildSystemPrompt
          (ported and credited), and a user prompt whose "features"
          section is gnubg's numbers, named in gnubg's vocabulary
          (I_BACK_ANCHOR delta, not "anchors"). Rule row unchanged:
          "Only reference facts provided in the data. Never invent
          strategic facts."
       3. Calls BOTH Claude AND GPT with identical prompts. Exact
          model strings recorded in the run metadata for
          reproducibility of the workflow, not of the output.
       4. Saves raw JSON per position per model to
          tools/harvest/output/raw/<position_id>.<model>.json --
          .gitignored.
     No API keys in git; env-var driven. Nothing here runs on-device.
     The run is not reproducible bit-for-bit and does not need to be;
     the shipping artifact is Phase F.

  E. Curation. `tools/harvest/output/curated/<signature>.md`, one file
     per signature, produced by maintainer + assistant together. Each:
       * the target signature and its I_* terms;
       * 2-3 best drafts across models, quoted with model attribution;
       * the maintainer's final Tier-A phrase, edited into his voice;
       * a diff-note stating what was cut and why;
       * or a rejection log entry if no phrase survived -- "no standard
         pattern, just equity" is a valid outcome, not a failure.
     Curation is committed. Raw model output is not.
     Per-entry rejection criteria:
       * verbatim overlap with any known passage from Robertie /
         Magriel / Woolsey / Trice (author-side spot-check plus a
         substring search against a small locally-held list of quoted
         excerpts kept OUT of the repo);
       * restating gnubg's numbers instead of naming the concept
         ("loses 0.081" is not a phrase; "gives back the 5-prime" is);
       * signal mismatch (LLM misread the position; gnubg's delta does
         not support the claim);
       * over 25 words per phrase (COMPANION.md's "short teachable
         phrase" -- the alternatives list is capped at four lines, so a
         full sentence is available, not a stub);
       * counsellor-voice ("You should consider..."); a phrase asserts,
         it does not counsel.

  F. Bake. `tools/harvest/bake.py` reads curated markdown, emits
     `gnubg-app/app/src/main/assets/insights_v0.json` per the §5 schema.
     Rejects any entry with tier != "authored". The JSON is the shipping
     artifact -- checked in, CHANGELOG-tracked, patch-level bumpable
     without touching code.

  G. Matcher. `gnubg-app/.../coach/InsightMatcher.kt`.
     At coach verdict:
       * request Phase A features for both played and best boards;
       * compute delta per named input;
       * score each corpus entry against the delta (magnitude x
         specificity, weighted per signature term);
       * pick up to two independent entries clearing threshold, or none;
       * emit the phrase(s), tagged race / board / threat.
     The matcher does NO position interpretation. It sorts a JSON table
     against a delta vector the engine handed it. No board[] read.

  H. UI. The coach panel's "Why" area is already reserved (WhyStub in
     coach/CoachScreen.kt around line 525) and the move-alternative list
     is capped at 4 lines, leaving genuine screen estate for a full
     sentence per selected phrase rather than a stub. Extend WhyStub to
     render one line per selected phrase, prefixed by its category chip
     (Race / Board / Threat). Debug-only "why shown" tail (naming the
     dominant delta term) behind an Expert-tab toggle, off by default.

     Later stage: a per-entry `manual_ref` optional field carrying a URL
     to the relevant section of gnubg's online manual (GFDL, freely
     hostable) so the user can drill in. The URL is a LINK, never
     embedded prose -- GFDL/GPL incompatibility (CORPUS_PLAN Part 1) is
     unchanged. A link is not incorporation of the linked text; this is
     the same reason the app can point at gnu.org for its own manual
     from the About screen. Deferred until the corpus has stabilised.

## 5. Corpus JSON schema (asset, ships as-is)

```json
{
  "version": 1,
  "corpus_license": "GPL-3.0-or-later",
  "generated": "2026-MM-DD",
  "authors": [
    "clavierhaus <gnubg@clavierhaus.at>",
    "GNU Backgammon for Android contributors"
  ],
  "acknowledgements": {
    "yairwein/backgammon-teacher": {
      "license": "MIT",
      "copyright": "Copyright (c) 2024-2026 Yair Weinberger",
      "for": "presentation taxonomy (race/board/threat), notable-delta threshold shape, harvest prompt scaffolding and rules"
    }
  },
  "draft_assistance_note": "Entries were first-drafted with the assistance of large language models (Claude, GPT). Each entry is human-verified and edited into original expression before inclusion. No prose from copyrighted sources is included.",
  "entries": [
    {
      "id": "prime.break.5",
      "principle": "A 5-prime is worth holding.",
      "phrase_flag": "You broke a 5-prime for a play that gains little.",
      "phrase_praise": null,
      "category": "board",
      "signature": {
        "I_BACKESCAPES": { "side": "opp", "direction": "up",   "min_abs_delta": 0.05, "weight": 0.6 },
        "I_BACKBONE":    { "side": "me",  "direction": "down", "min_abs_delta": 0.10, "weight": 0.4 }
      },
      "severity_hint": ["doubtful", "bad"],
      "author_notes": "Signal choice is a GUESS from reading enum comments, not a measurement. First pilot step is to identify the I_* that actually respond to a 5-prime break by running positionFeatures on paired constructed positions (see §9.4).",
      "tier": "authored"
    }
  ]
}
```

Notes:
  - "side" names whose input we read: "me" = player-on-roll (fMove), "opp" = other.
  - "direction" is the sign of (best - played) for that input. Interpretation
    of "worse" is gnubg's: the best move goes the other way; we do not judge sign.
  - "min_abs_delta" is the silence gate on trivial deltas.
  - "weight" is contribution to the entry's match score.
  - Multiple terms let one signature say "prime broken AND back checker
    exposed" as one phrase, not two.

## 6. Attribution mechanics (per commit)

  - Repo root NOTICE: append a paragraph naming yairwein/backgammon-teacher
    (MIT), the specific pieces adopted, the exact copyright line, and a
    pointer to tools/harvest/prompt_template.py which carries the MIT
    header verbatim.
  - tools/harvest/prompt_template.py: MIT header from backgammon-teacher's
    LICENSE at the top; then GPLv3+ notice for our additions below the
    MIT block; then the ported scaffolding.
  - insights_v0.json: acknowledgements.yairwein/backgammon-teacher per §5,
    plus draft_assistance_note naming the LLMs used.
  - Coach UI: an "About the coach's language" line in the Expert tab
    reads from acknowledgements at load time -- presented, not buried.
  - CHANGELOG entry for the corpus release names the exact model strings
    used, for reproducibility of the workflow (not of the outputs).

## 7. GPL-3.0-or-later compliance -- end-to-end audit

  a. The APK. All shipping code (Kotlin, C, JNI) GPLv3+, unchanged.
     The matcher and any sibling verbs are GPLv3+ additions in existing
     GPLv3+ files/dirs.
  b. The shipping asset. insights_v0.json declares corpus_license
     "GPL-3.0-or-later" in its header. It is a data file that is
     Corresponding Source under GPLv3 sec. 1.
  c. Yair Weinberger's contribution. MIT permits inclusion in a GPLv3+
     work; the combined work is GPLv3+, MIT-origin files inside
     tools/harvest/ retain the MIT notice; nothing MIT-origin ships in
     the APK anyway -- the touch point is offline tooling.
  d. LLM outputs. Not automatically covered by any license; treated as
     first-draft text. Phase E converts them into original expression
     that we license GPLv3+. Nothing shipped verbatim from any LLM.
  e. Source-material firewall. The GFDL/GPL barrier is at the level of
     TEXT INCORPORATION into a single work, not at the level of ideas
     or facts (Debian: "you can't legally extract text from a GFDL'ed
     manual and put it into integrated help strings in a GPL'ed
     program"). Facts and standard terminology are not copyrightable
     and are used freely (Tier B). CC BY-SA (Wikipedia) is also not
     GPL-compatible for text mixing. All-rights-reserved book passages:
     STUDIED, never COPIED, never paraphrased closely enough to be
     derivative (CORPUS_PLAN Part 1). Enforced per-entry by the Phase E
     rejection criteria. LINKS to GFDL/CC-BY-SA sources are permitted;
     a link is not incorporation.
  f. LLM provider terms. Anthropic and OpenAI outputs are permitted
     to be used and redistributed by their user in the ordinary case.
     This does not confer copyright on us and we do not need it to.
     The editing step is the operative license basis. Nothing in the
     APK invokes either provider at any time.
  g. Positions. Board positions are facts, not copyrightable. Parametric
     + gnubg-generated + gnubg-refmatch sources have no licensing
     question; book-scraped positions are excluded.
  h. Runtime independence. If Anthropic and OpenAI vanished tomorrow,
     the APK would be unchanged. If yairwein/backgammon-teacher's repo
     vanished, the APK would be unchanged. Neither is a runtime dep.

  This feature adds no proprietary runtime, no network call at play
  time, no bundled non-free content, and no dependency the user cannot
  rebuild from Corresponding Source.

## 8. Rejected alternatives

  - Bundle a small LLM in the APK. Adds ~0.5-2 GB; non-deterministic
    outputs cannot be re-verified against gnubg at runtime; current
    small-model weights are not DFSG-free. COMPANION.md already rules
    this out; kept here so the option is not re-litigated on impulse.
  - Ship LLM output verbatim. Fails Tier-A. Restatement risk against
    Tier-C training data; no per-entry verification; not our voice.
  - Adopt backgammon-teacher's FeatureVector field names literally.
    Reintroduces heuristic definitions gnubg does not share; port-
    checkpoint Q4 fails.
  - Runtime call to an online LLM even as a "flavor layer". Network
    dependency at play time defeats the offline invariant; user data
    (their moves, their positions) would leave the device.

## 9. Next actions (no code yet -- maintainer sign-off)

  1. Maintainer reviews and edits this doc.
  2. Maintainer + assistant iterate the phrase list in
     `docs/CORPUS_ENTRIES_DRAFT.md` (Phase B). First pass 10-15 entries,
     enough to prove the workflow before scaling to 40-80.
  3. STRUCK (2026-07-12): both sibling verbs already existed end-to-end;
     see the §3 correction. Q0 caught it.
  4. Pilot signal-discovery: CORRECTED 2026-07-12 -- desktop gnubg 1.07
     has no `show inputs`; the assumption above was never verified. The
     pilot instead runs tools/pilot/inputs_harness (build.sh), a host
     harness over THIS repo's eval.c -- same code the APK runs -- printing
     PipCount, positionclass and both sides' named I_* inputs for a board
     in `set board simple` order. Paired positions are diffed; results
     land in the Pilot log of CORPUS_ENTRIES_DRAFT.md. First two pairs
     measured; two entries confirmed with corrected directions.
  5. Only then: pilot harvest of the first ~10 signatures against a
     small position set, curated end-to-end, to prove Phase E's
     rejection criteria are workable before running at full scale.

Documents in this series:
  - COMPANION.md            -- why the insight layer exists.
  - CORPUS_PLAN.md          -- source licensing, tiers, and the plan this extends.
  - CORPUS_HARVEST_PLAN.md  -- this file: the LLM-drafted workflow.
  - CORPUS_ENTRIES_DRAFT.md -- to be created in Phase B: the phrase list.
