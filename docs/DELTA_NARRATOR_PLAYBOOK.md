# DELTA_NARRATOR_PLAYBOOK.md -- the fallback tier's binding law

Status: BINDING (2026-07-19, maintainer order). Governs the design,
implementation, amendment, and shipping of the DeltaNarrator -- the
deterministic sentence layer that speaks when a flagged move matches no
corpus signature. This playbook may not be violated; changes to it require
an explicit maintainer decision recorded in this file.

Position in the two-tier doctrine (CORPUS_HARVEST_PLAN.md §0-bis): the
narrator IS tier 2 speaking. gnubg remains tier 1 and final; the narrator
phrases what gnubg's numbers already said, and nothing else.

## 1. The seven laws

L1  GNUBG SOLE AUTHORITY. Every sentence must be derivable from a named
    gnubg value: an I_* input delta (best - played), a pip count, or a
    position class. If a sentence cannot cite its number, it does not ship.
    (Ported constitution rule, credited: yairwein/backgammon-teacher,
    "Only reference facts provided in the data. Never invent strategic
    facts." MIT, notice in tools/harvest/prompt_template.py.)

L2  DETERMINISTIC AND OFFLINE. The narrator is a fixed rule table applied
    by fixed code. No model, no network, no randomness at play time. The
    same position pair always yields the same words.

L3  SUBORDINATE TO THE CORPUS. Order of speech on a flagged move:
    (1) corpus signatures (authored, premium) -- if any fire, the narrator
    stays silent; (2) the narrator -- only when zero signatures fire.
    Unflagged moves (skill band "none") get neither in v1; extending the
    narrator to unflagged or praise contexts is a playbook amendment.

L4  THE RULE TABLE IS MAINTAINER-AUTHORED. Rules live in a versioned,
    tiered artifact (narrator_rules_v0.json) produced by the same
    discipline as the corpus: assistant proposes with measured evidence,
    maintainer adopts, bake refuses tier "proposed", registration precedes
    baking, the entry count is asserted before commit. Storage is cheap;
    unverified rules are not.

L5  MEASURED, NEVER GUESSED. Every rule's trigger (input, side, direction,
    notable threshold) is set from harness measurement on verified pairs
    -- the same instrument (tools/pilot/inputs_harness) and the same frame
    as the corpus. Frame changes to any instrument require a SIDEPROBE-
    style on-device measurement BEFORE the change, never after (lesson:
    the 2026-07-19 side-flip misdiagnosis, reverted same day).

L6  PLAIN WORDS, NO RAW NUMBERS. Sentences use direction words ("more",
    "fewer", "stronger"), never feature values; the equity cost already
    appears in the verdict header. Race / board / threat triage shapes the
    output (credited b-t taxonomy): at most one sentence per category, at
    most two sentences total -- identical envelope to the corpus, so the
    player cannot tell tiers apart. Logging tells them apart (gnubg-insight:
    "fired=" for corpus, "narrated=" for the fallback).

L7  LOUD FAILURES, INSTRUMENTED ALWAYS. The narrator logs every decision
    (rules consulted, rules fired, first-failing gate per near-miss). A
    rule that cannot explain itself in logcat does not ship. Silent skips
    are defects (lesson: the bake ENTRY_META silent skip).

## 2. Implementation phases (strict order; no phase begins before the
## previous one is adopted)

A. RULE TABLE + OFFLINE PROTO. narrator_proto.py mirrors the future Kotlin
   exactly; the rule table drafted with per-rule measured evidence from the
   110-pair verified corpus positions AND counter-evidence (positions where
   the rule must stay silent). Delivered to the maintainer as a review
   table: input, side, threshold, sentence template, evidence, tier
   "proposed".
B. MAINTAINER ADOPTION. Wording edits and strikes exactly as corpus
   curation; adopted rules flip to "authored".
C. BAKE + KOTLIN. narrator_rules bake (refuses "proposed"); DeltaNarrator.kt
   (Plus overlay, beside InsightMatcher.kt) consuming the baked asset;
   matcher-first ordering enforced at the single call site; compile gate;
   proto/Kotlin cross-check on identical pairs before any device deploy.
D. DEVICE VERIFICATION. Live filtered logcat during real play; the narrator
   must demonstrate (1) speech on a flagged uncatalogued move, (2) silence
   when a corpus entry fires, (3) silence on unflagged moves. Only then is
   the feature "done".

## 3. Amendment law

Field misfires are amended by the phase-A process in miniature: reproduce,
measure on the harness, adjust the rule, re-run proto + cross-check,
maintainer re-adopts changed wording. Hot-patching thresholds from theory
is forbidden.
