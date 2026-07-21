# CBG Strategy: The FOSS / Plus Boundary and the Forked Roadmap

**This document lives on the plus branch only.** It is the maintainer's
strategy notebook -- boundary decisions, value-proposition reasoning, and the
forked roadmap. It is deliberately candid in a way a public document should
not be. Nothing here is a commitment; everything here is a decision aid.

Status date: 2026-07-20. Written while the project is listed on F-Droid but
not yet meaningfully public. That window matters: **items can still move
between the FOSS and Plus realms without anyone perceiving a taking-away.**
Once there is a real user base, moving a feature from FOSS to Plus reads as
enclosure and costs trust; moving one from Plus to FOSS is always safe. So
every boundary call made now should err toward Plus, because the cheap
direction later is downward (Plus -> FOSS), never upward.


## 1. The boundary principle

One sentence, tested against every feature below:

> **FOSS is the complete, honest game. Plus is understanding, accumulation,
> and acceleration.**

Unpacked:

- **FOSS is never crippled.** The free edition is a full backgammon: complete
  match play (Crawford, Jacoby, cube, any length), a world-class engine, the
  per-move verdict (rank, equity cost, the five candidates with before/after
  boards), position paste-and-evaluate, and move-by-move review. A casual
  player -- someone who wants to play a proper game against a strong,
  honest opponent and occasionally see what the engine thought -- should
  never hit a wall, never see a nag, and never feel the app is withholding
  the game from them. The F-Droid promise ("the first standalone backgammon
  app on F-Droid") is kept in full.

- **Plus sells three things, none of which is "the game":**
  1. **Understanding** -- the words. The corpus and narrator that say *why*
     a move was an error. The verdict (a number) is free; the explanation
     (a sentence) is paid. This is already the shipped boundary and it has
     proven clean: the number is gnubg's, the words are ours.
  2. **Accumulation** -- the numbers over time. Anything that aggregates
     across moves, games, matches, sessions: Performance Rating, error
     lists, progress tracking. The per-move truth is free; the ledger of
     your improvement is paid. This is the natural home of the roadmap's
     biggest item (section 4).
  3. **Acceleration** -- workflow for the ambitious. The board editor
     ("Set up" a free position), play-the-suggestion, deeper rollout
     control, anything that saves a serious student time. Convenience for
     people who study, not features the game needs.

- The test for any future feature: *does a casual player feel its absence
  during a normal evening of play?* If yes, it is FOSS. If only an improver
  on a mission feels it, it is Plus.

Why this split is defensible to the outside world: it maps exactly onto the
two audiences, it never paywalls gnubg (whose GPL engine is the commons we
build on -- selling *access to gnubg's strength* would be both indefensible
and, for the coaching content, a community-relations disaster), and it sells
only what we authored ourselves: the words, the aggregation logic, the
workflow. The engine's truth stays free because it isn't ours to sell.


## 2. The two players (value proposition, stated plainly)

**The casual player** plays a match after dinner. They want: a beautiful
board, honest dice, a strong opponent, proper match rules, and -- sometimes --
"what would the bot have done?" FOSS gives them all of it, forever, no
account, no ads, no network. They are not a conversion target; they are the
reputation. Their five-star review of the free app is worth more than their
five euros.

**The ambitious player** is on a mission to reach a PR under some number.
They want to know *why* a move was wrong (Coach's words), *how bad* they
played today versus last month (PR and error accumulation), and they want to
*drill* (editor, play-the-suggestion, rollouts on demand). Each of those is
a Plus feature or a Plus roadmap item. The upgrade moment is when the free
verdict makes them curious and the app can honestly say: the number you just
saw is free forever; the understanding and the ledger are the paid tier.

The bridge between the two -- and this is a **hard requirement, not a
feature** -- is data continuity:

> **Everything the FOSS edition accumulates must import into Plus,
> completely and losslessly.** Match records, saved games, settings,
> anything the free app ever writes. An ambitious player who upgrades
> brings their whole history with them; upgrading must never mean starting
> over. Concretely: both editions read and write the same formats (gnubg's
> own .sgf match records wherever possible -- the engine's native format is
> the interop guarantee), and Plus ships an explicit "Import from CBG
> (free)" flow on first launch. This must be designed BEFORE the free
> edition starts persisting anything of value, so we never create a legacy
> format that traps data on the free side.

The inverse direction (Plus -> FOSS export) should also work, on principle:
we are not a lock-in shop, and the GPL ethos of the project's foundation
argues for data freedom in both directions. Locking data in would poison the
exact goodwill the FOSS edition exists to earn.


## 3. Feature inventory (surveyed 2026-07-20, code-grounded)

From a full read of the screens, engine surface, and docs -- not memory.

| Feature | State | Realm today |
|---|---|---|
| Play: full match (Crawford/Jacoby/cube/1-25) | complete | FOSS |
| Coach: per-move verdict, 5 candidates, before/after | complete | FOSS |
| Coach: the "why" (corpus 24 + narrator 11) | complete, device-verified | Plus |
| Analyse: paste GNU BG / XG ID, evaluate | complete | FOSS |
| Analyse: "Set up" board editor | complete | Plus (fenced) |
| Analyse: rollout (fixed 144 trials) | complete but shallow | FOSS |
| Review: move-by-move navigator + per-move verdict | complete as navigator | FOSS |
| Learn mode | 35-line stub, not in hub | neither (dead door) |
| Coach: play-the-suggestion / undo | not built | scoped Plus |
| Post-match PR + error aggregation | not built; all raw data exists | unassigned -> Plus |
| Rollout depth controls (trials/plies/variance) | not built | unassigned |

Notes on the inventory:
- Review's docstring is explicit that it "counts nothing, decides nothing" --
  it is a navigator by design. That is exactly the right FOSS shape: the
  per-move truth, free. The counting is the Plus layer on top.
- The quorum/comparative coaching tier ran to an honest negative result
  (VERBOSE_COACHING_DESIGN.md sec 5.5) and is parked, not planned. It does
  not appear on this roadmap.
- Learn is an empty room with a name on the door. Either it gets a real
  pedagogical concept someday or the door should not be shown. It is not a
  boundary question until it exists.


## 4. The forked roadmap, ranked

Ranking is value-over-effort for the *upgrade story* specifically -- what
makes the ambitious player shell out while the casual player loses nothing.

**1. Post-match PR + error review -- the Plus flagship gap.**
The defining serious-study feature of the market (XG's core loop). Play a
match, get your Performance Rating and a ranked list of your errors, each
jumping into Review at that move. All per-move numbers already exist (the
Review verdict path computes equity loss and skill class); this is
aggregation, not engine work. Realm: **Plus** -- it is accumulation, the
casual player never misses it, and it is the single strongest answer to
"why would I pay?". FOSS keeps the per-move verdict in Review untouched, so
the free edition still answers "what did the engine think of this move" --
just not "what is my rating".
Dependency: the data-continuity requirement (section 2) must be settled
first, because PR history is exactly the data an upgrader will want to keep.

**2. Coach play-the-suggestion / undo -- the scoped convenience.**
Small, already scoped, rounds out the coach as a *training* loop (see the
mistake, undo, play the right move, feel the difference). Realm: **Plus**
(acceleration). No casual-player impact.

**3. Rollout depth controls -- small, honest depth.**
Expose trials/plies and show the variance so a rollout is a real instrument
rather than a fixed button. Realm: **split** -- the basic 144-trial rollout
stays FOSS (it is "what would the bot do", part of the honest game); the
depth controls and variance readout go Plus (acceleration for students).
This is the one item where the boundary runs *through* a feature, and the
split is defensible by the section-1 test on each half.

**4. Learn mode -- only with a concept.**
Not before someone (us) knows what it teaches and how. The corpus-vocabulary
work (VERBOSE_COACHING_DESIGN.md sec 5.6 -- the measured I_* map) might one
day ground a Learn concept in measured pedagogy rather than listicle tips.
Until then: keep the stub out of the hub, which it already is.

Explicitly NOT on the roadmap: anything network (multiplayer, accounts,
cloud sync, leaderboards). The standalone/offline identity is the moat --
every competitor listed in the 2026 app roundups leads with online play and
treats offline as the afterthought. We are the inverse, on purpose.


## 5. Boundary ledger (running record of realm decisions)

So future-us can see what moved where, when, and why -- and so the "still
private" window's flexibility is used deliberately, not accidentally.

- 2026-07 -- Coach verdict + candidates: FOSS. The engine's truth is free.
- 2026-07 -- Coach words (corpus + narrator): Plus. Our authorship.
- 2026-07 -- Board editor: Plus, entry-fenced (code compiles in FOSS,
  unreachable). Precedent for the fence pattern.
- 2026-07 -- Basic rollout: FOSS (shipped inside Analyse before the
  boundary was articulated; RETAIN in FOSS -- clawing it back would be the
  bad direction, and it passes as "the honest game").
- 2026-07 -- PR/aggregation (unbuilt): assigned Plus by this document.
- 2026-07 -- Rollout controls (unbuilt): assigned split by this document.

Rule going forward: every realm decision gets a ledger line with a date and
a one-line reason, at the moment it is made.
