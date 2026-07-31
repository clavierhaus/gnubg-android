# The Career and the Stats Panel — architecture and reasoning

**This document lives on the plus branch only.** It is the maintainer's
architecture note for the career/stats arc: what the stats panel is, what the
career ledger will be, why they are separate, and the reasoning behind the
refactoring that begins here. It sits above two existing documents and does
not restate them:

- `docs/POSITION_YOUR_PERSONAL_STATS.md` — the philosophy (why "PR" is dead,
  what a standard is, why gnubg's numbers are the honest ground).
- `docs/PERSONAL_STATS_DESIGN.md` — the stage-4 laws L1–L6 governing the
  match-report screen.

Those two remain authoritative for what they cover. This note connects them
to the career ledger and records the reasoning for the first substantial
feature work after the API-36 toolchain migration.

Status date: 2026-07-30.

---

## 1. One authority, stated once

gnubg is the sole authority for every number and every piece of notation this
arc produces. This is not a slogan; it is a rule with consequences that recur
throughout the document:

- A displayed statistic is gnubg's own computed value, read at source, never
  re-derived or rescaled in Kotlin (L1, L6).
- A displayed move is gnubg's own `FormatMove` output — the same string the
  desktop move list writes, hits and `*` and all — never a reformatting of
  our own.
- A rating word is gnubg's own `aszRating`, selected by gnubg's own
  thresholds (`analysis.c:66-93`, cited in the screen source).
- Where gnubg is silent, we are silent. The correct-or-silent guarantee is
  this rule applied to the coach; the stats panel is the same rule applied to
  numbers.

Every design choice below defers to this. When a choice looks like taste
(notation, label format, which figure leads), the tie is broken by "what does
gnubg itself say," not by preference.

## 2. Two things, deliberately separate: the panel and the career

The arc has two distinct artifacts. Keeping them separate is a decision, not
an accident of sequencing.

**The stats panel** (exists today, `PersonalStatsScreen.kt`) is the analysis
of the match *just finished*. It is ephemeral by design: it covers one match,
resets when the next begins, and makes no claim about the player over time. A
single match's rating is mostly variance (L3) — the panel says so on its own
face.

**The career** (stage 5, not yet built) is the accumulated record across
matches: the silent collector, the signed chain, the thing that answers "how
am I doing" rather than "how did that match go." It is where a rating becomes
meaningful (it settles over ~20 matches) and where the tamper-evident ledger
lives.

Why separate:

- **Different honesty claims.** The panel shows a number and immediately
  qualifies it as noisy. The career shows a number that has earned its
  stability. Merging them would let one match's variance masquerade as a
  career figure — the exact hidden-normalization sin the position paper
  criticizes.
- **Different provenance rules.** The career records *Play* matches only.
  Coached matches are excluded by construction: Undo lets a player retry into
  better numbers, and per-move verdicts change behaviour even without Undo.
  "The gym does not go on your record." The panel, by contrast, will happily
  analyse a coached match — it is a mirror of what just happened, not a
  record of who you are.
- **Different surfaces.** The panel is reached from the match-over screen (the
  one moment the just-finished record still exists and the player cares). The
  career is reached from the hub — and a hub entry only becomes honest once a
  ledger exists to show on days without a fresh match.

The panel is the near-term work. The career is the larger second-release
centrepiece. This document scopes both so the panel is built in a way the
career can extend, not a way the career must undo.

## 3. The panel refactoring (the work beginning now)

Two changes, taken together because they touch the same file and the same
reasoning.

### 3a. Landscape three-column layout

The app is landscape-only (`screenOrientation="sensorLandscape"`; the sensor
only flips 180°, never to portrait). There is therefore no narrow layout to
preserve and no breakpoint to choose — the screen is always wide.

The stage-4 screen was composed as a single vertical scroll (one
`LazyColumn`, sections stacked: games → costliest moves → rating → result →
details). On an always-wide surface this ran past two full screens with the
right half empty. The information was all correct and all present (L1–L6 held)
— it was simply laid out for a portrait phone on a landscape tablet.

The refactoring replaces the single column with three, using the guaranteed
width:

1. **Scorecard** — Rating (You / GNU), Result (actual · luck-adjusted), the
   settle-over-~20-matches caveat. The headline. This is the
   compartmentalisation the maintainer called for after the stage-4 smoke
   test: aggregates lead.
2. **Details** — the You / GNU per-decision breakdowns (per-decision mEMG,
   unforced/close-cube counts, chequer/cube error, luck, skill histogram),
   plus the how-to-verify line (L5). Placing You and GNU where they can be
   read against each other is more useful than the old stacked cards: you
   read your chequer error under the opponent's at a glance.
3. **Costliest moves** — the per-move receipts (L3's hero), in their own
   independently-scrolling list, occupying the space that was empty before.

No content is removed; L1–L6 are unchanged. Only placement changes. The
ordering ruling stands: this layout ships on numbers already checked against
authoritative gnubg — never as a way to dress up unverified output.

### 3b. Move labels: gnubg's own notation, not an array index

The stage-4 error rows read `Game 1 · record 22`. `record 22` is the raw
walk index (`recIdx`) from the error facade — internal plumbing that means
nothing to a player. It is the Review-jump seam (G5) showing through before
the jump exists.

The fix defers to authority (rule 1): the row shows gnubg's own move string,
exactly as the desktop move list writes it — dice and play, including hits
and the `*` marker, produced by gnubg's `FormatMove`. Not our formatting of
the move; gnubg's.

The engineering, re-derived against the engine sources in this tree:

- `FormatMove(sz, anBoard, anMove)` (`drawboard.h:32`) needs the board *as it
  stood before the move*. The move record (`pmr`) carries `anMove` and
  `anDice` but not the pre-move board.
- gnubg's own way to reproduce a mid-match board is to replay from the game's
  start applying each record — `ApplyMoveRecord` (`backgammon.h:490`). The
  error-row facade already walks every record of every game; the addition is
  a running board per game, seeded at game start, advanced by
  `ApplyMoveRecord` after each record, with `FormatMove` called against the
  board *before* the record is applied.
- This is a new facade function (`gnubg_mobile_match_error_moves`), parallel
  to `gnubg_mobile_match_errors`, not an extension of it. The numeric
  function packs a fixed `int out[104]` with a G3 self-consistency check that
  is already G4-validated; move strings are variable-length text that do not
  fit that contract. A separate string-returning call leaves the proven
  numeric path untouched and returns strings parallel-indexed to the error
  rows (row *i*'s number pairs with move-string *i*).
- `MatchError` gains the dice and the move text; the ViewModel decoder reads
  the parallel call; `ErrorRow` renders `Game 1 · 63: 21/12` (gnubg's string)
  in place of `record 22`.

The label is a step toward Review-jump (G5): once a row names a real move, the
tap target that opens that move in Review has something meaningful to open.

## 4. The career ledger (stage 5 — scoped, not built)

Recorded here so the panel work above is built compatibly. Full mechanism
detail lives in the maintainer's project notes; the reasoning is:

- **Silent collector.** Every finished *Play* match (never coached) is
  analysed silently at match end and appended to the career. No button — the
  collection is a consequence of finishing an honest match. The asset-gate
  pattern keeps this inert in FOSS: a Plus-only marker asset is checked; FOSS
  finds none and does nothing.
- **Plaintext, signed, chained.** The store stays plain gnubg-format `.sgf`
  in the user's own folder (the data-continuity law: nothing locked to an
  installation, FOSS↔Plus lossless). A sidecar index per match holds the
  file hash, the analysis numbers, and the previous entry's hash — a chain
  that makes any later editing, deletion, or reordering visible. Each entry
  is signed with a user-owned, exportable key (device-generated, passphrase-
  wrapped or QR-transferable), so a new device plus the folder plus the key
  restores the career intact and verifiable.
- **Honest limits, stated.** The chain is tamper-*evident*, not unforgeable:
  a rooted owner can drive the signing path. It proves the record as kept, not
  that the keeper never had the means to forge. This is stated in the design,
  never oversold.
- **Verification is everyone's.** Collection is Plus; the chain verifier is
  FOSS (an open script or FOSS-side tool). "We do not ask to be believed, we
  ask to be checked" requires that the check not itself be paywalled.
- **Why it matters strategically.** A tamper-evident career record is the
  first auditable player record in backgammon — a trustless path to a
  federation-grade rating that depends on no central server, consistent with
  the no-network law.

The panel is where a single match's numbers are shown and qualified. The
career is where matches accumulate into something a rating can honestly
describe. The panel refactoring must not assume ephemerality in a way that
blocks the collector (it does not: the collector is a separate match-end path,
not a reuse of the panel's cache).

## 5. Sequence

1. **Panel refactoring** (§3): the facade move-string function (C, re-derived
   and syntax-gated), the MatchError/decoder threading, the three-column
   layout. Kotlin gate, parity audit, device look.
2. **Review-jump (G5)** once rows name real moves — the tap semantics derived
   from Review's own stepping.
3. **Career ledger (§4)** — the stage-5 centrepiece, its own build.

Everything in §3 ships only on numbers checked against authoritative gnubg.
Everything in §4 waits behind §3 and its own design pass. Nothing here relaxes
L1–L6 or the correct-or-silent guarantee; this document extends them to the
career and records the reasoning for the panel work that carries them there.
