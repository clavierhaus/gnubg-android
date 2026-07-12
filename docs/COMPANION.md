# The Companion: natural-language coaching, fully local

Status: ANALYSIS + PLAN (2026-07-12). Source studied: yairwein/backgammon-teacher
(MIT, SvelteKit client/server, gnubg CLI adapter, cloud LLMs). Its design
principles are our constitution independently rediscovered:

    1. Engine truth -- GNU Backgammon is the authority on move strength
    2. Structured interpretation -- feature extraction provides interpretable data
    3. Natural language teaching -- LLM explains using features, NEVER invents facts

That convergence is the strongest possible validation of the coach vision, and
their pipeline gives us a concrete checklist. Where we differ BY DESIGN: they
ship a server and call Anthropic/OpenAI; we are an offline GPL app on a phone
-- everything below is local, or it does not ship.

## Element-by-element sift

| backgammon-teacher element      | gnubg-android status                          |
|---------------------------------|-----------------------------------------------|
| gnubg as sole analysis engine   | HAVE -- in-process, deeper than their CLI parse|
| blunder threshold (equity)      | HAVE -- gnubg's own Skill() thresholds         |
| pause to review alternatives    | HAVE, better -- COACH_REVIEW hold, P/1..3      |
|                                 | before/after toggles on the common ground      |
| interactive board, legal hints  | HAVE                                           |
| position feature extraction     | ADOPT -- this is M2, their feature list is the |
|                                 | starting schema (below)                        |
| LLM explanation layer           | ADOPT TRANSFORMED -- local model, phase C      |
| client/server, database, cloud  | REJECT -- offline, single APK + optional local |
|                                 | model download                                 |

## Phase A (= M2): the features verb -- gnubg speaks in structure

`gnubg_mobile_position_features(board[50], out[])`: a fixed schema computed by
gnubg's OWN functions (CalculateHalfInputs, eval.c:61/613, exports the neural
inputs; ClassifyPosition, eval.h:413, names the position class). Starting
schema, informed by their list: pip counts + race lead (HAVE: pipCount),
position class (race/contact/crashed...), blots and hit exposure, prime length
and location, anchors, home-board points made, checkers back, checkers on the
bar. The verb runs on BOTH boards of a verdict (played result, best result):
the coach's raw material is the FEATURE DELTA between them.

Rule: the app never computes a feature itself. If gnubg's inputs do not
express a concept, the concept waits.

## Phase B (= M2 delivery, no LLM required): the deterministic reason line

Dominant-delta -> one sentence from a fixed template table keyed to the
lexicon (M3). "Best keeps a 5-prime; your move breaks it." Deterministic,
offline, tiny, shippable soon -- and it IS the grounding harness the LLM will
later be constrained by. Their project needed the LLM for this; we do not.

## Phase C (new milestone M6): the local companion

A small instruct model on-device verbalizes -- prompt = verdict numbers +
feature deltas + lexicon entries; hard constraint mirrored from their
principle 3: the model may only restate provided facts, in coaching tone, as
a SHORT LESSON. No position reasoning by the model, ever; gnubg reasons, the
model speaks.

Open decisions (maintainer): runtime (llama.cpp JNI vs MediaPipe LLM
inference); model + LICENSE (Qwen small = Apache-2.0 fits the FOSS stance;
Gemma/Llama licenses do not sit cleanly beside GPLv3+); distribution
(optional download, never in the APK); expertise corpus for lesson flavor --
the gnubg MANUAL is GPL and expert-written, a cleaner grounding source than
Wikipedia's thin backgammon coverage; Wikipedia can supplement terminology.

## Order of work

Phase A after 0.12.0 ships (try-again sandbox first, per COACH_MODE_PLAN).
Phase B immediately on A. Phase C prototyped only when A+B are field-proven:
the companion's voice is worthless until the facts it speaks are.
