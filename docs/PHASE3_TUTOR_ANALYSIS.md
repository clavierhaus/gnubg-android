# Phase 3 Tutor Analysis -- IMPLEMENTED

## Principle
gnubg is the sole authority for game logic, including move analysis. The tutor
uses gnubg's own routines (FixMatchState, ApplyMoveRecord, FindnSaveBestMoves)
to reconstruct state and score moves. Nothing is reinvented.

## Status: COMPLETE and verified on device (Pixel 8 Pro)
A deliberate blunder produces level=HUGE_BLUNDER with a real equity loss; a
best-equals-played move produces level=NONE loss=0.0000 with zero feature
deltas. Real gnubg equities end to end.

## What it does

Single facade function `gnubg_mobile_tutor_analyze(const int old_board[50], int out[52])`,
called from GameViewModel.confirm() AFTER applyMoveString. Steps:

1. **Locate the human's move record.** The last record in plGame may be the
   engine's reply, so walk backward from plGame->plPrev to the last MOVE_NORMAL
   whose player is human (ap[fPlayer].pt == PLAYER_HUMAN; this app sets
   ap[0] = human).

2. **Reconstruct the pre-move matchstate** by replaying the game exactly as
   gnubg's AnalyzeGame does:
   - AnalyzeMove on the first record (MOVE_GAMEINFO) initialises msAnalyse.
   - For each subsequent record up to (not including) the human's move:
     FixMatchState(&msAnalyse, pmr); if pmr->fPlayer != msAnalyse.fMove then
     SwapSides(msAnalyse.anBoard) and set msAnalyse.fMove; ApplyMoveRecord(...).
   - This leaves msAnalyse as the state BEFORE the human's move.

3. **Score all legal moves** with gnubg's FindnSaveBestMoves (fAnalyse=TRUE) --
   the same call AnalyzeMove uses internally (play.c ~65823). The move list
   comes back sorted best-first with rScore filled.

4. **Find the played move** by position key (pre-move board + pmr->n.anMove,
   then PositionKey), matched against ml.amMoves[j].key.

5. **Read equities** as gnubg's move_skill() does:
   - played = ml.amMoves[iPlayed].rScore
   - best   = ml.amMoves[0].rScore
   - out[0]/out[1] = float bits of played/best

6. **Best-move board** for feature comparison: old_board + ApplyMove(best anMove),
   NO SwapSides -- same frame as the caller's played board (state.board, built
   by applySubMove which also does not swap).

## Critical findings (the bugs that were fixed)

- **Wrong record.** plGame->plPrev->p is the ENGINE's reply, not the human's
  move. Fixed by walking back to the last human MOVE_NORMAL.

- **Wrong matchstate.** AnalyzeMove/FindnSaveBestMoves rebuild everything from
  pms->anBoard. The global ms has advanced past the move; pms->anBoard MUST be
  the pre-move board. Fixed by reconstructing msAnalyse via the AnalyzeGame
  replay (FixMatchState + ApplyMoveRecord).

- **inf rScore from the 2-ply/prune evalcontext.** esAnalysisChequer.ec
  (EVALSETUP_2PLY, fUsePrune=TRUE) returns inf in this build. The facade's
  proven getCandidates path uses fac_ec_default (1-ply) and scores correctly.
  FindnSaveBestMoves is therefore called with fac_ec_default (chequer evalcontext)
  and a cubeinfo built via GetMatchStateCubeInfo(&ci, &msAnalyse) -- the
  gnubg-native pre-move cubeinfo. NOTE: this means the tutor currently
  evaluates at 1-ply. Revisiting 2-ply/prune is a future improvement (see
  Handover, and MASTER_V0.9.md Phase 11.1 for the V1 audit item that will
  replace fac_ec_default with gnubg's named context).
- **Cubeinfo source (corrected V0.9.x).** The original implementation
  passed fac_ci_default (a money-play frozen cubeinfo) to FindnSaveBestMoves.
  That was a port-rule violation: from game 2 onward the tutor's analysis
  ran with the wrong score / cube ownership / Crawford state. The facade
  now passes the cubeinfo built via GetMatchStateCubeInfo against the
  pre-move matchstate (msAnalyse). See MASTER_V0.9.md Phase 11.1.

- **Feature-delta frame mismatch.** An extra SwapSides on the best board flipped
  pip sign (pipDifference 8->-8 when played==best). Fixed by building the best
  board in the played board's frame (no swap).

## Files
- jni-bridge/include/gnubg_mobile.h    : gnubg_mobile_tutor_analyze decl
- jni-bridge/src/gnubg_mobile.c        : implementation
- jni-bridge/src/native-lib.c          : JNI tutorAnalyze(oldBoard) -> IntArray[52]
- gnubg-app/.../Engine.kt               : external fun tutorAnalyze(oldBoard): IntArray
- gnubg-app/.../engine/GameViewModel.kt : hook after applyMoveString (log-only)

## Output contract (IntArray[52])
- [0]     = Float.fromBits(played equity)
- [1]     = Float.fromBits(best equity)
- [2..51] = best-move board, player-on-roll frame (same as state.board)
- empty array => no analyzable human move (dance / not found)

## Verification
./build_and_deploy.sh ; adb logcat | grep -E "gnubg-tutor|gnubg-vm"
- blunder  -> level=BLUNDER/HUGE_BLUNDER loss>0 with sensible deltas
- best move-> level=NONE loss=0.0000, no notable deltas

## Handover -- open items / next phases

1. **Evaluation depth.** Tutor runs at 1-ply (fac_ec_default) because the 2-ply
   /prune path (esAnalysisChequer) returns inf in this build. Investigate why
   prune nets / 2-ply yield inf in FindnSaveBestMoves here; the get_candidates
   1-ply path is the only proven-scoring route. Raising to 2-ply would match
   desktop gnubg analysis strength.

2. **Build artifact hygiene (do next).** libgnubg-engine.so is tracked and keeps
   dirtying the tree. Run: git rm --cached
   gnubg-app/app/src/main/jniLibs/arm64-v8a/libgnubg-engine.so and add it to
   .gitignore so it never blocks a clean-tree check again.

3. **Cube decisions.** Only chequer play is analysed. Cube-decision tutoring
   (esAnalysisCube / the cube path in AnalyzeMove) is not yet wired.

4. **Phase 4+: coaching layer.** The tutor now emits level + equity loss +
   feature deltas to logcat. Next: PhraseLibrary (static GPL-3 coaching assets),
   CoachCard UI, then tutorial/position-review modes. See MASTER 7.6.

5. **Play Protect.** The device flags the app verdict=HARMFUL (logcat cna/bvv).
   Awareness only; can interfere with install/run, unrelated to tutor logic.

## DO NOT
- Do not hand-roll move matching or evaluation. gnubg's FindnSaveBestMoves /
  FixMatchState / ApplyMoveRecord / PositionKey are the authorities.
- Do not pass the global ms to the analysis -- it has advanced past the move.
- Do not add SwapSides to the best board -- it must match state.board's frame.
