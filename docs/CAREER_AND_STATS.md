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
to the career ledger and records why this body of code exists at all.

The founding rule it rests on is not restated here; it lives at the top of
`CLAUDE.md` — *"gnubg is the SOLE authority for all game logic AND all
analysis. Port it, never reinvent it."* Everything below is that rule applied
to numbers, notation, and the record. This document is the *motivation*: the
rule is the ground, this is the reason we build on it.

Status date: 2026-07-30.

---

## 1. Why this code exists

### The origin

The analytical heart of this app is not ours and was never meant to be. It is
gnubg — GNU Backgammon, engine version 1.08.003, vendored whole into
`engine-core/` and driven through a thin facade. gnubg is one of the two or
three strongest backgammon engines in existence, it is free software, and its
analysis has been the quiet reference standard for two decades: when people
want to know whether a move was right, they ask gnubg. CBG does not add a
brain to the phone. It carries gnubg's existing brain, unaltered, onto the
phone — and then does the honest work of showing what that brain says without
distorting it.

That is the whole origin. Every screen in this arc — the coach, the stats
panel, the career to come — is a window onto gnubg's own output. The
engineering is in the carrying and the showing, never in the deciding.

### The mobile predecessor: XG, and what it did and didn't do

A strong mobile analyzer already exists, and has since 2014: XG Mobile, the
phone edition of eXtreme Gammon. It must be credited honestly, because
pretending otherwise would be its own kind of dishonesty. XG is a
fourth-generation neural net in the line TD-Gammon → Jellyfish → Snowie → XG;
its mobile champion level plays near world-class (PR ~0.55, a hair below the
desktop 3-ply); it is endorsed by the US Backgammon Federation; and for years
it was, deservedly, the study tool serious players carried. CBG's claim is
therefore **not** "the first strong analyzer on mobile," nor "the strongest."
Those claims are taken, and taking them falsely would forfeit the one thing
this project is built to have: credibility.

What XG Mobile did was bring real strength to the pocket. What it did not do —
and this is the whole opening — is make its authority *checkable*, and it has
lately stopped even keeping its distribution intact. XG's numbers are trusted
because XG is endorsed and because XG is strong; the trust rests on reputation
and authority. You cannot open XG, take its verdict on your match, and
independently confirm it against the same engine anywhere else, because the
engine is closed and the "PR" scale is computed by an unpublished filter. And
as of 2026 the incumbent is visibly decaying on the platform: the Android
build has fallen out of Google Play (users sideload an APK), and the app
creeps forward in cosmetic point releases while the desktop parent has shipped
nothing of substance in years. The strong mobile analyzer is still there — but
neglected, closed, and unverifiable.

### The goal: authority you can check, not authority you must trust

CBG's goal is a mobile backgammon experience whose authority rests on
**verifiability** — and this is the spine of the entire project, the one point
that, if every other were stripped away, would still be the reason this code
exists.

State it plainly, because stated plainly it ends the argument:

> Every number CBG shows you is gnubg's own number, and you can prove it.
> Save the match, open the same file in desktop GNU Backgammon — the free,
> open engine anyone can download — and the figures are identical, because
> they were never anything but gnubg's to begin with. We do not ask you to
> believe us. We ask you to check us.

This is a categorically different basis for trust than XG's. XG says: *trust
this number because the program is strong and the federation endorses it.* CBG
says: *don't trust the number — verify it, here is exactly how, and the tool
to verify it with is free.* One asks for faith backed by authority. The other
removes the need for faith entirely.

The power of this framing is that **it makes discussion redundant.** There is
no debate to be had about whether CBG's analysis is "as good as" XG's, or
biased, or tuned, or wrong — because CBG's analysis is not CBG's. It is
gnubg's, reproducible by anyone, on an open engine, in under a minute. A
disagreement about a CBG number is not an argument with CBG; it is an argument
with gnubg, settled by running gnubg. The project deliberately owns no ground
on which such a fight could happen. Every design law downstream — no unlabeled
numbers (L1), never the bare "PR" token (L6), the correct-or-silent coach, the
verify-line on the stats screen (L5) — exists to keep that redundancy total:
to ensure there is never a CBG-specific claim standing between the user and
gnubg's checkable truth.

Verifiability first. Everything else — honesty of silence, the auditable
career record, active stewardship on a platform the incumbent has abandoned —
follows from it and reinforces it, but this is the spine.

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
- **Why it matters — the spine, extended across time.** The panel makes a
  single match's numbers verifiable. The career makes the *whole record*
  verifiable: not "trust that this player's rating is real" but "check the
  chain — every match is a plain gnubg file you can re-analyse, and the
  signatures prove the sequence was not edited after the fact." It is the
  first auditable player record in backgammon: a rating whose provenance can
  be checked move by move, match by match, against the same open engine,
  depending on no central server and no one's endorsement. XG can tell a
  federation a player's PR; it cannot let the federation *verify* it. CBG's
  career is verifiability applied to a career instead of a single match —
  the same argument-ending move, one level up.

The panel is where a single match's numbers are shown and qualified. The
career is where matches accumulate into something a rating can honestly
describe — and, because the spine holds, something anyone can check rather
than take on trust. The panel refactoring must not assume ephemerality in a
way that blocks the collector (it does not: the collector is a separate
match-end path, not a reuse of the panel's cache).

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
