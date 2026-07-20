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

## Explicitly deferred (post-parity differentiators)

- **Cube/match analysis, MWC, Market Window, MET, Temperature Map** -- the
  cube/match-play surface; deferred with the rest of match-play support.
- **The "why" explanation** -- reviews show even XG lacks a real "why this move"
  explanation. gnubg's position features (`position_features` verb already
  exists: shots, pip loss, containment, timing, mobility) could ground a
  genuine explanation. This is the natural charter of the future **Tutorial
  mode** (distinct from Analysis mode) and a real chance to exceed XG, not just
  match it. Post-parity.
