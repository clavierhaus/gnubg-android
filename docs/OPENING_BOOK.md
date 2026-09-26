# OPENING_BOOK.md — the BackgammonDB opening book in CBG Pro

Status: DESIGN (2026-09-26), maintainer to approve. Governs how the
BackgammonDB opening book (github.com/chalkstreamdev/bgdb-opening-book;
data CC-BY-4.0, generator GPL-3.0; 4,230,443 gnubg 1.08.003 3-ply entries
over 351 match-score contexts) reaches CBG Pro users, what it adds that
the live engine cannot, and what we give back.

Binding constraints, from CLAUDE.md: gnubg is the sole authority (the
book IS gnubg's numbers, cached — the same standing as gnubg's own
bearoff databases); no network at any time; the FOSS app is the master;
correct-or-silent; and THE FIDELITY GATE: the book is gnubg-as-packaged
(x86, fast-math), trusted at the measured distro epsilon, never at zero.

## 1. What the book adds that the phone cannot compute

The phone evaluates at 2-ply in seconds. The book answers at 3-ply in
microseconds, for the first three decisions of a game, **at every match
score to 25-away**. The score dimension is the real gift: nobody plays
the opening the same way at 2-away/2-away, at Crawford, and for money,
and no other app shows a player how the best play moves with the score.
The book has that table computed already. The features below are built
on that, and on nothing the app would have to invent.

## 2. Features, in delivery order

### 2.1 Opening Trainer (fills the LEARN scaffold, hub slot)

Roll, play, and get gnubg's 3-ply verdict instantly — at the score the
trainer set. Modes:

- **Random score.** The trainer draws a score context (match length,
  both scores, Crawford where legal, or money ± Jacoby) and an opening
  roll; the player plays; the book's top-8 list appears with equities
  and the player's play marked. Scores are drawn from a printed seed
  so a session can be replayed.
- **Fixed score.** The player picks a context (e.g. 2-away/2-away) and
  drills every opening roll at that score.
- **Reply drill** (depth-2 data, opening + every reply to 15-away): the
  trainer plays the opening for the *opponent* and asks for the reply.
  This is the book's second unique layer: the phone's engine could do
  this at 2-ply, the book does it at 3-ply and instantly.

Display is the existing board, `viewModel == null`, dice on the on-roll
side and numbers by the player on roll (issue #13 semantics). The verdict
line names its source: **"book · gnubg 1.08.003 · 3-ply"**, never bare.

### 2.2 Score-sensitivity view

For one opening roll, a compact table: rows = score contexts (money,
DMP-ish, 2a2a, 3a3a, Crawford, post-Crawford …), cells = the book's best
play and the equity gap to the play the user chose (or the money best).
Pure rendering of gnubg data: the app computes nothing; it sorts and
formats. This is the single screen the book makes possible and no
engine-only app has. Correct-or-silent: a context the shipped subset
lacks shows a dash, never an interpolation.

### 2.3 Coach and Analyse: the book as the 3-ply oracle for the opening

- **Coach**: for the first three decisions, when the position is in the
  book (canonical XGID hit), the verdict comes from the book, instantly,
  labelled *book · 3-ply*; the live 2-ply verdict is shown beneath it or
  not at all (maintainer's design). From the fourth decision on, the
  live engine as today.
- **Analyse**: a book hit adds a *Book (3-ply)* row set beside the live
  list, and a *Verify* button that runs our own engine at 3-ply on the
  same position — the two lists side by side, with the difference. That
  is the fidelity gate made visible to the user, in the app's own words.

### 2.4 Personal opening statistics (Career)

The career layer already records played games. Matching the first three
decisions of each game to the book yields, per roll and per score class,
the player's average equity loss in the opening — gnubg's numbers,
subtracted (book best − book played). "Your 4-3 openings cost 0.031 on
average; at Crawford you play 24/21 13/9 where the book wants 13/9 13/10."
No interpretation, no phrase library: numbers and the book's own plays.

## 3. Delivery: a data package on F-Droid, no network

- **Package** `com.clavierhaus.gnubg.book`, a second app on F-Droid,
  signed with the release key, containing the book subset as a
  read-only SQLite in assets plus a manifest. No activity beyond an
  About page; no code paths into the engine.
- **Access** from CBG Pro through `createPackageContext` on the book
  package (read its assets; no permission involved), gated by a
  signature check (same signer as CBG Pro) so a stranger's APK with the
  same name is refused. CBG Pro never touches the network; the user
  installs the book from F-Droid like any app and CBG Pro lights the
  features up on next start. Without the book: the hub entry says what
  to install, once; nothing else changes.
- **Subset**: depth 1 (all 351 contexts, opening rolls), depth 2 (every
  reply, ≤ 15-away), depth 3 for the small contexts (≤ 5-away and money)
  — measured before fixing: per-entry ~408 bytes, so the target is tens
  of MB, not a gigabyte. Assembled by `tools/book_subset/` from the
  release's per-context JSONL files, verified against the release's
  SHA256SUMS, into a git repository that the F-Droid recipe pulls as a
  `srclib` at a pinned commit. The subset repository records the
  upstream release tag, its SHA256SUMS, and the exact extraction command.
- **Reproducible**: no native code, no gradle surprise; two worktrees
  build the same bytes as for the main app (verify_reproducible.sh
  runs unchanged against the book project).
- **Attribution** (CC-BY-4.0, as the data licence asks): "BackgammonDB
  opening book, generated with GNU Backgammon 1.08.003" on the book
  package's About page and in CBG Pro's acknowledgements, with the
  repository URL. The GPL-3 generator code is not shipped; nothing of it
  is needed on the phone (the reader is two Python files we reimplement
  in Kotlin as a 40-line SQLite lookup plus the canonicalisation rule
  from book-format.md).

## 4. Agreement, stated to the user

The book is gnubg 1.08.003 as packaged on x86 with `-ffast-math`
(PROVENANCE.md, "Arithmetic"). CBG Pro's engine is the exact branch of the
same source. Measured 2026-09-26 on thirty opening positions: 150 of 187
plays identical to six decimals; the rest within 3e-3 in cubeful equity.
Before every release of the book package, `tools/book_conformance` runs
on a seeded random sample of the shipped subset and writes its summary
into the package manifest; CBG Pro shows it on the book's About page:
"agreement with this engine: N plays sampled, max difference x". The
number is shown, never rounded to a slogan. A book whose sample fails the
recorded epsilon is not released.

Canonicalisation: the book keys on a canonical XGID (away-away collapsed,
turn standardised, flags masked; book-format.md). The Kotlin lookup
implements exactly that rule and is tested against `canonical.py`'s
fixtures copied from the upstream repository's tests (GPL-3 test data
used as test data, in tools/, not shipped).

## 5. What we give back

- The conformance harness and the finding behind it: the book's numbers
  are gnubg's; the divergence to an exact build is one compiler flag in
  gnubg's own sigmoid. A suggestion for `reader.serves()`: record the
  arithmetic regime (fast-math on/off, SIMD family) in `meta` beside
  `engineVersion`, and let consumers gate on agreement with a sample
  rather than on the version string alone — the MET alone moves every
  equity by 0.010 under the same version string.
- A heads-up on gnubg master: the movefilter semantics changed in 2026
  (upstream 73d644d7a6, "Improve movefilters"); with the worker's exact
  override commands, `hint` on master evaluates only to 1-ply at settings
  2 and 3 unless every intermediate level is set explicitly. The book as
  ground on 1.08.003 is unaffected; a regrind on a newer gnubg would be.
- A phone-sized subset specification (depth 1 + 2, per-context files) and
  a mobile consumer with users, which is what a book is for.
- Positions and equities for the opening are exactly the "facts, not
  prose" source CORPUS_HARVEST_PLAN.md C.1 wants; the harvest's opening
  entries can cite the book instead of running the engine.

## 6. Phases

1. `tools/book_subset/`: extraction + verification + size table
   (measure, then fix the subset). Kotlin lookup + canonicalisation with
   the upstream fixtures. Book package skeleton, signature check, About
   page with attribution and the agreement line. F-Droid recipe (srclib).
2. Opening Trainer, random-score and fixed-score modes, on the existing
   board. Hub entry. Correct-or-silent for missing contexts.
3. Coach/Analyse book rows with the Verify button.
4. Reply drill; score-sensitivity view.
5. Career opening statistics.

Each phase is gated by the fidelity gate for the engine and by the
book-conformance sample for the data. Nothing ships that disagrees with
gnubg beyond the recorded, displayed epsilon.
