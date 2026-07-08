# Roadmap: Analysis Feature Parity (target: XG Mobile)

## Why this target

XG Mobile is the acknowledged standard for serious backgammon analysis on
mobile ("if you want to track your PR and understand your mistakes, nothing
else on mobile comes close" -- multiple 2026 reviews). It is also **decaying**:
buggy, not actively developed, and increasingly incompatible with modern
Android devices. A gnubg-based Android app that reaches parity has a real
opening, because the reference engine that rates XG's own strength *is gnubg*.

The strategic point for this roadmap: **almost none of this is new analysis
work.** gnubg already computes every number XG shows. The engine exposes
`GetRating` (Performance Rating), the full `statcontext` error machinery
(`updateStatcontext` / `AddStatcontext`), `FindnSaveBestMoves` (ranked move
lists with full probability vectors), and `PositionID` / `PositionFromID`
(position entry). The work is **exposing** gnubg through the facade and
**surfacing** it in the UI -- not reimplementing anything. That keeps every
item below inside the one rule.


## XG Mobile feature set, mapped to our state

Legend: [have] shipped | [engine] gnubg computes it, needs exposing | [ui] needs UI only

| XG Mobile feature                                  | Our state |
|----------------------------------------------------|-----------|
| Play vs strong neural-net AI                        | [have] |
| Tutor mode: flag mistakes as you play               | [have] (blunder level) |
| Played-move win/gammon/bg breakdown + equity        | [have] (Analysis Tier 1) |
| Ranked candidate list (best->worst, played marked)  | [engine] FindnSaveBestMoves already ranks |
| Per-candidate win/gammon stats ("Details")          | [engine] arEvalMove per move |
| Tap a candidate to preview it on the board ("Show")  | [ui] have the boards, need preview wiring |
| Performance Rating (PR) / error rate                 | [engine] GetRating + statcontext |
| Post-game / match statistics summary                 | [engine] statcontext accumulation |
| Position entry ("input any position")                | [engine] PositionFromID / PositionID |
| Variable ply (eval at 0/1/2/3/4)                     | [engine] evalcontext plies; fixed 2-ply now |
| Rollouts (Monte Carlo)                               | [engine] RolloutGeneral; heavy on phone |
| Luck rating per roll/game                            | [engine] LuckFirst / statcontext |


## Phased plan

### Phase A -- The analysis pane (move-level parity)
The heart of XG's analysis: not one verdict but the ranked field.

- **A1. Ranked candidate list.** Facade verb over `analyze_replay`'s already-
  ranked `ml.amMoves[]`: return top-N moves as (notation via `FormatMove`,
  cubeful equity, cubeless equity, equity delta vs best). Played move flagged.
  UI: a list, best at top, played marked, equity + mP diff each. (This is the
  single most-requested "analysis" element in reviews.)
- **A2. Per-candidate probabilities ("Details").** Extend A1 rows with each
  move's `arEvalMove` (Win/Wg/Wbg/Lg/Lbg). UI: expandable per row.
- **A3. Tap-to-preview ("Show").** Tapping a candidate shows that move on the
  board (we already reconstruct boards; wire candidate -> board preview,
  read-only, with a way back to the played position).

Outcome: the move analysis pane matches XG's, on the platform XG is leaving.

### Phase B -- Performance Rating (the headline metric)
PR is *the* number serious players track; gnubg computes it natively.

- **B1. Per-move error accumulation.** After each analysed move, feed the
  equity error into a `statcontext` via gnubg's own `updateStatcontext`
  (no hand-rolled accumulation).
- **B2. PR / error-rate display.** Surface `GetRating` + error-rate-per-move
  (EPM) for the current game/session. UI: a PR readout, gnubg's named rating
  (Beginner .. Supergrandmaster).
- **B3. Post-game summary.** The chequer-play stats gnubg's match statistics
  already produce (total/unforced moves, moves marked doubtful/bad/very-bad,
  error rate). UI: an end-of-game panel.

Outcome: "track your PR and understand your mistakes" -- the XG headline --
delivered from the engine that defines PR.

### Phase C -- Position entry & depth
Turn it into a study tool, not just a game reviewer.

- **C1. Position entry via Position ID.** `PositionFromID` to load any gnubg
  Position ID (and `PositionID` to share the current one). This is the "input
  any position, get the strongest move" workflow and the natural home for the
  opening-move study people use XG for.
- **C2. Variable ply.** Expose 0/1/2/3/4-ply selection (`evalcontext`) for the
  analysis pane, so a user can trade speed for depth. Gated on analysis
  performance (see Phase D).

### Phase D -- Performance (enables C2 and rollouts)
- **D1. Multi-core analysis.** Parallelise `ScoreMoves` (design in
  `MULTICORE_ANALYSIS.md`) so deeper ply and eventually rollouts are usable on
  a phone. Out-of-tree, upstream-able.
- **D2. Rollouts (stretch).** `RolloutGeneral` for a selected move/position.
  Heavy; only sensible after D1, and likely a "single position" affordance
  rather than full-match rollouts.


## Sequencing rationale

- **A before B:** the candidate list is the foundation PR analysis reads from,
  and it's the most-cited missing element in mobile analysis today.
- **B before C:** PR is the headline competitive feature; ship it once the
  per-move analysis it summarises exists.
- **D underpins C2/D2:** depth and rollouts are only usable once analysis is
  parallelised; until then, fixed 2-ply is the right default.

## UI parity and differentiation

Engine parity is necessary but not sufficient: XG Mobile is the analysis
standard, yet its **interface is its weakest dimension** -- and that is our
opening. Reviews consistently praise XG's engine and tutor while criticising its
UI. We should match what XG does well and beat what it does badly, and our
existing `ARCHITECTURE.md` settings-vs-actions principle already points the way.

### What XG does well (match these)

- **Forgiving, natural checker movement.** Reviewers single out that XG moves
  checkers smoothly "without penalizing you for a typo" and offers multiple
  intuitive ways to move/roll/double -- valuable on small screens. Our tap/drag
  handling (with fPartial=0 landing guidance) is already heading here; keep the
  input forgiving and never punish a mis-tap.
- **Fast position checking.** Quick start-up and instant position evaluation is
  why players carry XG to tournaments. Our analysis must feel instant for the
  common case (decoupled analysis already helps; multi-core will finish it).
- **The move-list "eye" / arrow visualization.** XG's most-liked analysis touch:
  select a move in the list and it draws the move on the board with arrows. This
  is Phase A3 (tap-to-preview); do it cleanly.
- **Full outcome probabilities per move.** Praised explicitly ("how does every
  move change my chances to win, gammon, backgammon, and same for opponent").
  This is Phase A1/A2.

### What XG does badly (beat these)

- **The "infuriating toolbar."** XG's single most-criticised UI element -- ugly,
  and with auto-hide "infuriatingly hard to open." A reviewer's direct plea:
  "can we get rid of the toolbar -- other games let you do the config away from
  the game screen." **Our answer already exists as policy:** board interactions
  stay on the board surface; configuration lives in dedicated settings screens,
  never a board-overlay toolbar. We are positioned to win here by design.
- **Config on the game screen.** Directly disliked; our settings-vs-actions
  boundary (config screens are configuration-only, actions live on the board or
  hub) is exactly what XG's users are asking for. This is a differentiator we
  get "for free" if we hold the line.
- **Painful position setup.** XG's "Setup a Position" requires dragging checkers
  off and back -- ugly enough that reviewers say it needs its own tutorial.
  Phase C1 (Position ID paste/entry via `PositionFromID`) sidesteps XG's worst
  workflow entirely: paste an ID, done.
- **Dated, bordered, glitchy rendering.** XG has rendering glitches, large
  borders, and recent "invisible dice"/lag bugs. A clean Compose-native board
  with our theme system is a straightforward win on polish.

### The settings surface as an asset

Our settings approach is not just neutral here -- it is a competitive advantage.
XG bolts configuration onto the board via a hated toolbar; we keep the board
clean and put configuration in proper screens. The same discipline that keeps us
faithful to gnubg (clear layer boundaries) keeps the UI uncluttered. As analysis
features land (candidate list, PR, position entry), each gets a proper surface --
an analysis pane, a PR readout, a position-entry screen -- rather than being
crammed into a board overlay. Holding the settings-vs-actions line as the feature
set grows is how we reach XG's analysis depth without inheriting XG's UI
problems.

### UI sequencing

UI work rides with the feature phases, not as a separate track:
- Phase A ships the analysis pane (list + details + tap-to-preview) as a clean
  surface off the board.
- Phase B ships the PR readout and post-game summary as their own panels.
- Phase C ships position entry as a dedicated screen (ID paste first, visual
  editing later if wanted) -- deliberately not XG's drag-off-and-back model.

## Explicitly deferred (post-parity differentiators)

- **Cube/match analysis, MWC, Market Window, MET, Temperature Map** -- the
  cube/match-play surface; deferred with the rest of match-play support.
- **The "why" explanation** -- reviews show even XG lacks a real "why this move"
  explanation. gnubg's position features (`position_features` verb already
  exists: shots, pip loss, containment, timing, mobility) could ground a
  genuine explanation. This is the natural charter of the future **Tutorial
  mode** (distinct from Analysis mode) and a real chance to exceed XG, not just
  match it. Post-parity.
