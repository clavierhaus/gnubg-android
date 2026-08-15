# The Career and the Stats Panel (CBG Pro) — architecture and reasoning

**This documents CBG Pro features. The document itself is public by
principle: no CBG documentation ever resides behind the paywall. What it
describes is part of the paid edition; the description is everyone's.** It is the maintainer's
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
`engine-core/` and driven through a thin facade. gnubg is one of the
strongest backgammon engines in existence, it is free software, and its
analysis has been a reference standard for two decades: when people want to
know whether a move was right, one of the things they do is ask gnubg. CBG
does not add a brain to the phone. It carries gnubg's existing brain,
unaltered, onto the phone — and then does the honest work of showing what
that brain says without distorting it.

That is the whole origin. Every screen in this arc — the coach, the stats
panel, the career to come — is a window onto gnubg's own output. The
engineering is in the carrying and the showing, never in the deciding.

### The company it keeps

CBG is not the first serious backgammon analyzer to reach a phone, and it does
not pretend to be. eXtreme Gammon's mobile edition has been available since
2014; it is strong, it is respected, and it is endorsed by the US Backgammon
Federation. Serious players have carried it for years, and rightly. We
acknowledge its standing without reservation — a document that had to
diminish what came before in order to justify itself would be admitting it had
no reason of its own.

CBG has a reason of its own, and it is not "better." It is *different*, in one
specific way that the rest of this document builds on. So this note does not
compare, rank, or argue against anything. It states what CBG does, why it does
it that way, and why it invites people to see for themselves.

### What CBG does, and why: verifiability

CBG's organizing principle — the spine of the whole project, the one thing
that if everything else were stripped away would still be the reason this code
exists — is **verifiability**.

Stated plainly:

> Every number CBG shows you is gnubg's own number, and you can prove it.
> Save the match, open the same file in desktop GNU Backgammon — the free,
> open engine anyone can download — and the figures are the same, because they
> were never anything but gnubg's to begin with. We do not ask you to believe
> us. We ask you to check us.

Why we build this way: trust that is *asked for* can always be doubted, and
doubt invites argument. Trust that can be *checked* needs neither. When the
tool to verify a CBG number is free, open, and produces that number in under a
minute, there is nothing left to debate — not because we have won an argument,
but because we have declined to have one. A question about a CBG number is
answered the same way every time: run gnubg and look. The project deliberately
holds no position that could be disputed, because it claims nothing of its own
about the numbers — the numbers are gnubg's, and gnubg is checkable by anyone.

This is why the invitation to compare is genuine and unworried. We are not
asking anyone to take our word over someone else's; we are handing them the
means to need no one's word at all. Every design law downstream — no unlabeled
numbers (L1), never the bare "PR" token (L6), the correct-or-silent coach, the
verify-line on the stats screen (L5) — exists to keep that true: to ensure
there is never a CBG-specific claim standing between a person and gnubg's
checkable output. Verifiability first. Everything else the project values —
the honesty of silence, the auditable career record, keeping the software
alive and correctly distributed — follows from it and serves it.

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
- **The cryptographic stack and its full reasoning are public.** They live in
  the FOSS repository — `docs/CRYPTOGRAPHY.md` — deliberately: the decision
  method, the candidates and verdicts (Keystore disqualified as key home,
  Tink declined, platform JCA chosen), the exact algorithms and encodings,
  and the point-by-point GPLv3 defense. The spec for checking the record
  cannot live behind the record's paywall.
- **Verification is everyone's.** Collection is Plus; the chain verifier is
  FOSS (an open script or FOSS-side tool). "We do not ask to be believed, we
  ask to be checked" requires that the check not itself be paywalled.
- **Why it matters — the spine, extended across time.** The panel makes a
  single match's numbers verifiable. The career makes the *whole record*
  verifiable: not "trust that this player's rating is real" but "check the
  chain — every match is a plain gnubg file you can re-analyse, and the
  signatures prove the sequence was not edited after the fact." It is an
  auditable player record: a rating whose provenance can be checked move by
  move, match by match, against the same open engine, depending on no central
  server and no one's endorsement. The career is verifiability applied to a
  career instead of a single match — the same principle, one level up.

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
