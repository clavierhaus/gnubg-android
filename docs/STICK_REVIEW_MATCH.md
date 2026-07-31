# Stick's review, matched against CBG — the core-audience punch-list

**This document lives on the plus branch only.** It matches the 2026-07-31
BGonline review by Stick (owner of bgonline.org, reviewing Backgammon Sage
Pro) against the *verified current state* of CBG — FOSS and Plus. Stick is
treated here as the representative of the core audience: the demanding,
serious player whose endorsement is the multiplier the project is aiming for.

Two framing facts:

- Stick posted the CBG announcement on BGonline on the maintainer's behalf
  (the maintainer has not yet resolved BGonline login/posting). Stick has not
  yet seen the full scope of Plus — reasonably, since it is not public and
  FOSS/Plus are deliberately distinct. The goal of this punch-list is to move
  CBG Plus toward what a player like Stick wants, staying true to the
  project's principles, and earn him as a genuine believer.
- The review is of **Backgammon Sage Pro** by Mark Higgins — a *new*,
  cloud-based, open-source, subscription bot with a world-class engine,
  endorsed by named players. This is a distinct new competitor. It is not XG
  (which the project's notes already track). SP being open-source means
  "verify the code" is not on its own a CBG differentiator; CBG's sharper
  claim stands — verify each *number* against the independent, established
  desktop gnubg, which SP's cloud engine does not offer.

Every "current state" line below was read from the tree on 2026-07-30, not
from memory. Marks: ★ already done · ◐ planned · ○ genuine gap / decision.

---

## What Stick complained about in SP that CBG already does right

These are SP annoyances that are **already solved** in CBG. They are the
strongest part of the story: the things the arbiter found missing are built.

- **★ Tap the doubling cube to double.** Stick's first complaint: SP won't let
  you click the cube. CBG already hit-tests the cube (`Board.kt` cubeRect →
  `offerDouble()`); tapping the cube doubles. Done.
- **★ Dice appear instantly, no wiggle.** Stick: SP's dice "dance for a second
  before settling"; he wants them "just there, done" as in XG. CBG's dice are
  drawn static — there is no roll animation to sit through. Already exactly
  what he asks for.
- **★ Tap the dice to change play order.** Stick: SP forces a precise
  right-click on the dice. CBG hit-tests the dice (`Board.kt` swapDiceRect →
  `swapDice()`); tapping them swaps order. Done — and simpler than what he
  wanted.
- **★ No cloud, no network dependence.** Stick's autoplay-abandons-you,
  slow-cloud-rollout, and "feels like a browser" complaints are all cloud
  artifacts. CBG's nothing-network-ever law makes every one of them
  impossible by construction. The engine runs on the device; nothing waits on
  a server.
- **★ Honest coaching, or none.** Stick's longest and angriest paragraph: SP's
  AI explanations "ramble, talking out of its ass," and would do a beginner
  "far more harm than good" — because it is harder to unlearn something learnt
  wrong. This is the correct-or-silent guarantee, named by the arbiter as a
  design principle. CBG never fabricates a reason; silence is the credential.
  This is the single most important confirmation in the review: the thing
  Stick says matters most is the thing CBG is built around.
- **★ One-time purchase, not subscription.** Stick calls SP's subscription
  "an enormous knock against the program," praising XG/Snowie/BGBlitz's
  outright purchase. CBG's paid-app direction matches. *Decision flagged:*
  the project's notes still carry a "monthly subscription for updates" idea —
  Stick is direct evidence against exactly that half. Worth revisiting the
  pricing model in light of the core audience's stated aversion.
- **★ Saves matches; surfaces and reviews your errors.** Stick praises SP for
  saving games and letting you review your blunders. CBG already saves matches
  (SAF, plain `.sgf`, the user's own folder) and surfaces costliest moves in
  Your Personal Stats. Built — and the career ledger (below) extends it.

## What Stick praised in SP that maps to CBG's roadmap (planned)

- **◐ Accumulate a record; categorize errors; quiz yourself on your
  mistakes.** Stick praises SP for this. CBG's stage-5 signed career ledger is
  precisely this — plus verifiability SP cannot offer. His praise independently
  confirms the whole career direction. **New desirable on top:** a *quiz /
  self-review mode* over your own recorded mistakes — Stick names it as a
  feature he values.
- **◐ Readable analysis text.** Stick's angriest UX complaint: SP's analysis
  text is "small af… like the 1980s… a lot of your backgammon audience is old
  enough to be on the bad end of eyesight." The three-column landscape stats
  layout (just shipped) is the start of the answer — legibility as a
  first-class concern. Carry the same discipline (size, contrast) into every
  numeric surface.
- **◐ Set-up / position editor.** Already a planned Plus capability
  ("Analyse Position with the set-up editor"). Stick wants a board-clear
  shortcut while editing — fold that detail into the editor when built.
- **◐ Position + analysis export.** CBG can currently *paste in* a GNU/XG
  position ID (Analyse mode), and the bug-report path copies a position out —
  but there is no clean "export this position WITH its gnubg analysis in a
  BGonline-postable form." Stick misses XG's export-to-BGO (HTML/text/forum).
  For CBG this is the highest-leverage item on the list: a shared position
  carrying its gnubg analysis is verifiability made viral — every posted
  position is a checkable advertisement for the engine, on Stick's own forum.
  Promote from gap to planned.

## Genuine gaps / decisions (Stick wants it; CBG has no plan yet)

- **○ One-swoop undo of a full multi-part move.** The one place Stick's SP
  complaint *matches* a CBG behaviour: CBG's undo is per-sub-move
  (`GameViewModel.undo()` drops one snapshot: "restored one submove"). Stick
  finds undoing a played double one-step-at-a-time (four taps) an annoyance;
  XG undoes the whole move at once. **Decision:** should *Play-mode* undo
  collapse the whole move in one action while *coach-mode* stays granular (the
  per-sub-move step is deliberate there — enactment is the pedagogy)? The
  mechanism exists (moveHistory is a stack); the question is whether Play mode
  offers a "undo whole move" that pops to the last commit boundary.
- **○ Hint that shows analysis and can then play the best move.** CBG's coach
  shows analysis; "then play it for me" was deliberately not built (enactment
  is the pedagogy). For *Play* mode specifically — not coach mode — a
  hint-then-apply may be worth it: it is what Stick reaches for reflexively.
  Keep it out of coach mode, where making/feeling/retrying the mistake is the
  point.
- **○ Autoplay the no-contact bear-off.** Stick wants the race auto-finished.
  No autoplay path found in the tree. A convenience item, network-free by
  nature (local engine plays it out).
- **○ More / better board themes; 3D boards.** CBG currently has three
  palettes (OCEAN, CLASSIC, FOREST). Stick found SP's three "hideous" and
  wants more, plus 3D. Three tasteful palettes may already beat his bar;
  "more and better" and "3D" are open.
- **○ Clock option.** Stick praises SP's customizable match clock for
  practicing timed play. CBG has no game clock. A self-contained feature,
  network-free.
- **○ Heat map.** Stick likes SP's (a blend of XG's and gnubg's). gnubg has
  its own; worth investigating as an analysis visualization.
- **○ Variants (Nackgammon, etc.).** Stick asks why not more variants. gnubg
  supports several natively; a question of surfacing them.

## What the match reveals

The thesis items — honest coaching, offline, the record, one-time price — are
the ones Stick independently validates as what matters, and every one is
already built or planned. Most of his concrete SP annoyances (cube-click,
instant dice, dice-order tap, no cloud) are *already solved* in CBG; he simply
has not seen Plus yet. The genuine remaining gaps are a short, well-understood
list of interaction conveniences: full-move undo, play-mode hint, bear-off
autoplay, more themes, a clock, a heat map, variants — plus the one on-thesis
item, **export a position with its gnubg analysis**, which is both something
Stick wants and a direct expression of verifiability.

CBG has the hard, differentiating things right. The distance to a Stick-grade
daily driver is polish, not foundations. The path to making him a believer is
to show him Plus (he reviewed against SP without knowing Plus exists), close
the convenience gaps in his own idiom, and never compromise the two things
that are the whole point: honest coaching and checkable numbers.
