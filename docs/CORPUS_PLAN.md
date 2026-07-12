# The Phrase Corpus: sources, licensing, structure, retrieval

Status: RESEARCH + ARCHITECTURE (2026-07-12). No code. Governs how the insight
layer's phrases (docs/COMPANION.md) are sourced, licensed, structured, matched.

## Part 1 -- Source licensing (VERIFIED, not assumed)

The governing legal fact, confirmed against FSF and Debian primary sources:

  THE GFDL IS GPL-INCOMPATIBLE IN BOTH DIRECTIONS. Text from a GFDL document
  cannot be incorporated into a GPL program (FSF license-list; Debian position
  statement: "you can't legally extract text from a GFDL'ed manual and put it
  into integrated help strings in a GPL'ed program"). GPLv3 is our license.

Consequences for the obvious sources:

  - gnubg MANUAL / "All About GNU Backgammon" (Silver/Anthon): GFDL 1.3, no
    invariant/cover texts. Authoritative, expert, freely available -- and
    UNUSABLE as copied text in our GPLv3 app. Its rules section is additionally
    third-party (Tom Keith / Backgammon Galore, "by permission"), a separate
    restriction. USE: reference for our OWN authoring and for feature/threshold
    calibration (facts and rules are not copyrightable -- only their
    expression); COPY: never.
  - Wikipedia backgammon articles: CC BY-SA. Copyleft but NOT GPL-compatible
    for verbatim text; also thin on strategy. USE: terminology cross-check;
    COPY: never into the app.
  - Backgammon Galore (Tom Keith): all-rights-reserved website. USE: study;
    COPY: never.
  - Published strategy books (Robertie, Magriel's "Backgammon", Woolsey,
    Trice): all-rights-reserved. USE: study the STANDARDS they codify; COPY:
    never. Concepts and standard terminology (prime, anchor, blitz, golden
    point) are facts of the game, not anyone's copyrighted expression.

Classification of what we could source, by (quality x authority x GPL-availability):

  TIER A -- authored by us, GPLv3+, grounded in study of the above.
    Quality: as high as we make it. Authority: derives from correctness against
    gnubg + standard theory. Availability: total; it is ours. This is the ONLY
    tier that ships. Every other tier is INPUT TO OUR HEADS, never to the repo.

  TIER B -- facts and standard terminology (not copyrightable): pip count,
    prime, anchor, blitz, priming game, holding game, golden point, the pattern
    NAMES. Freely usable because facts and short standard terms carry no
    copyright. These are the vocabulary our Tier-A phrases are written in.

  TIER C -- GFDL/CC-BY-SA/all-rights-reserved expression (manual prose, book
    passages, Wikipedia text): STUDY ONLY. Never copied, never paraphrased so
    closely as to be a derivative. A firewall, not a source.

Conclusion: the corpus is ORIGINAL GPLv3+ CONTENT WE AUTHOR, informed by study
of Tier C and built from Tier B vocabulary. This is not a compromise forced by
licensing -- it is also the only way each phrase can be pinned to a gnubg
feature signature, which no external prose is written to do.

## Part 2 -- What one corpus entry must contain

Each entry is a PRINCIPLE with a machine-checkable trigger and an authored
phrase. Draft schema (data, versioned, human-editable):

  id            stable key, e.g. "prime.break.5"
  principle     the teachable idea, one clause ("a 5-prime is worth holding")
  phrase_flag   shown when the player VIOLATED it ("You broke your 5-prime for
                little gain.")  <= ~12 words, plain, skill-neutral
  phrase_praise optional, shown when the player UPHELD a non-obvious instance
  signature     the feature-delta pattern that triggers it (Part 3): named
                features + directions + thresholds
  severity_hint which gnubg skill bands this can accompany (doubtful..very bad)
  refs          NON-shipping author notes: where the standard is documented
                (manual section, book+page) for our verification -- comments,
                not content
  tier          provenance self-audit: must read "authored"

## Part 3 -- The feature substrate (depends on Phase A verb)

gnubg_mobile_position_features(board[50], out[]) -> a fixed vector from gnubg's
OWN CalculateHalfInputs / ClassifyPosition. Candidate named features (final set
pinned when the verb is read against eval.c, per contract):
  position class; pip count + race lead; home-board points made; blots and
  direct/indirect shot count; longest prime + its location; anchors held;
  checkers on the bar; checkers back; builders in the zone; crunch/timing
  proxy. Computed for the played result AND the best result; the DELTA is the
  matcher's input.

## Part 4 -- Retrieval & processing (the matcher)

Deterministic, offline, explainable:
  1. verdict yields played-result board + best-result board (HAVE).
  2. features verb -> two vectors (Phase A).
  3. delta = best - played, per feature.
  4. score each corpus entry: does the delta match its signature, and how
     dominantly (magnitude x specificity)?
  5. select: highest-scoring entry above its threshold; allow a second if
     clearly independent; allow NONE ("no standard pattern -- just equity").
  6. render the phrase, with an optional "why shown" line naming the delta.

Data format: the corpus ships as a versioned asset (JSON/TOML in the repo,
loaded at runtime), CHANGELOG-tracked like code; a bump adds/edits phrases
without a binary change. The matcher is pure Kotlin over gnubg's numbers -- it
classifies nothing itself, it only looks up which authored principle the
engine-computed delta instantiates.

## Immediate next actions (no code yet)

  A. Confirm the Tier-A-only decision with the maintainer (licensing is now
     established; this is a strategy sign-off).
  B. Draft 8-12 seed entries by hand for the highest-frequency patterns
     (prime break, blot in home board, anchor surrender, builder waste,
     race/hold mismatch) to pressure-test the schema before the verb exists.
  C. Then Phase A features verb; then wire the matcher; then expand the corpus.
