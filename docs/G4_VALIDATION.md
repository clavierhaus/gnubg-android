# G4 — Verifying the match analysis against desktop GNU Backgammon

The app's match statistics are produced by gnubg's own analysis code
(AnalyzeGame walk, AddStatcontext, getMWCFromError) running on the phone.
G4 is the gate that proves it: the same match, analysed by an independent
desktop installation of GNU Backgammon, must produce the same numbers,
field for field. Not close -- identical at displayed precision. The
evaluation is deterministic, the nets and the analysis settings are the
same on both sides, so there is nothing for a tolerance to absorb.

Anyone can run this. That is the point.

## Preconditions (two minutes)

1. Same neural nets. gnubg's shipped nets have not changed in years, so
   any current desktop gnubg qualifies -- but verify rather than assume:

       sha256sum /usr/share/gnubg/gnubg.weights
       sha256sum <app source>/gnubg-app/app/src/main/assets/gnubg.weights

   The two hashes must match. If they do not, point the desktop at the
   app's weights file (gnubg -d <dir>) and re-check.

2. Same match-equity table. The app's default is Kazaross-XG2; desktop
   gnubg's built-in default is different. On the desktop: Settings ->
   Options -> Match equity table -> Kazaross-XG2. The luck-adjusted
   results are MET-dependent; a mismatch here shows up as small
   divergences in exactly those fields.

3. Desktop analysis settings at gnubg's defaults (2-ply chequer and cube,
   pruning on, analysis move filters). The app runs gnubg's own
   esAnalysisChequer / esAnalysisCube / aamfAnalysis contexts -- the same
   defaults -- so an unmodified desktop configuration is already correct.

## Procedure

1. Play a match in the app to completion.
2. Open the match statistics and record every displayed number
   (a screenshot of the details view suffices).
3. Save match. The .sgf lands in your chosen CBG folder.
4. Copy the .sgf to the desktop machine.
5. Desktop gnubg: File -> Open the .sgf, then Analyse -> Analyse match,
   then Analyse -> Match statistics.
6. Compare, field for field, per player.

## Field mapping

The app reads gnubg's summed statcontext directly. Desktop's statistics
window shows the same quantities under these rows:

| App (statcontext source)            | Desktop statistics row          |
|-------------------------------------|---------------------------------|
| Moves analysed / unforced           | Chequerplay: total / unforced   |
| Chequer error total (EMG)           | Error total EMG (chequerplay)   |
| Close cube / total cube decisions   | Cube: close / total             |
| Cube error total (EMG)              | Error total EMG (cube)          |
| Luck total (EMG)                    | Luck: total                     |
| Combined per-decision rate          | Overall: error rate per decision|
| Skill marker counts                 | Marked moves (doubtful/bad/...) |
| Luck marker counts                  | Marked rolls                    |
| Actual result / luck-adjusted       | Actual / luck-adjusted result   |

The app never prints a bare "PR": gnubg's per-decision denominator is not
XG's, so the rate is always labelled as gnubg's own.

## Acceptance

Every field identical at displayed precision, both players. A single
divergent field is a defect: either the port's aggregation, the
preconditions (nets or MET), or the mapping above -- in that order of
suspicion. The gate stays failed until the divergence is explained and
fixed, never widened into a tolerance.
