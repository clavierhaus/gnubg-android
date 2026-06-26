# Phase 3 Tutor Analysis -- gnubg-native approach

## Principle
gnubg is the only authority for game logic, including move analysis. The tutor
does NOT reinvent move matching, evaluation, or skill classification. It calls
gnubg's own AnalyzeMove() and reads the results gnubg stores.

## Status
- HEAD baseline: clean (Phase 1 + Phase 2 committed).
- Abandoned approach (do not revive): a hand-rolled gnubg_mobile_tutor_analyze
  that searched the move list by PositionKey and re-evaluated each candidate.
  It returned raw.size=0 because the custom loop corrupts the move list and
  duplicates work gnubg already does. WIP commit 4d81ea9 holds that dead code;
  revert to clean HEAD before implementing the correct approach.

## The correct architecture: gnubg's AnalyzeMove

gnubg's move analysis is AnalyzeMove(). The async wrapper asyncAnalyzeMove() is
already vendored in the facade (engine-core) and calls:

    AnalyzeMove(pmd->pmr, pmd->pms, plGame, NULL,
                pmd->pesChequer, pmd->pesCube, pmd->aamf, NULL, NULL)

After AnalyzeMove runs on a move record, gnubg's own move_skill() reads quality:

    move_i = &pmr->ml.amMoves[pmr->n.iMove];   // the move that was PLAYED
    move_0 = &pmr->ml.amMoves[0];              // the BEST move (list sorted)
    skill  = Skill(move_i->rScore - move_0->rScore);

Therefore:
  - played equity = pmr->ml.amMoves[pmr->n.iMove].rScore
  - best   equity = pmr->ml.amMoves[0].rScore
  - equity loss   = move_0->rScore - move_i->rScore   (>= 0)
  - pmr->n.iMove is the index of the played move -- gnubg STORES it; no search.

## moveData struct (engine-core, confirmed)
    typedef struct {
        moverecord *pmr;
        matchstate *pms;
        const evalsetup *pesChequer;
        evalsetup *pesCube;
        movefilter(*aamf)[MAX_FILTER_PLIES];
    } moveData;

## Analysis setup globals (already vendored)
    evalsetup  esAnalysisChequer = EVALSETUP_2PLY;
    evalsetup  esAnalysisCube    = EVALSETUP_2PLY;
    movefilter aamfAnalysis[MAX_FILTER_PLIES][MAX_FILTER_PLIES] = MOVEFILTER_NORMAL;

## New facade function

    int gnubg_mobile_tutor_analyze(int out[52])

  1. Get the LAST move record from plGame (the move just played by applyMoveString).
     - plGame is the current game's move list (listOLD*).
     - The last MOVE_NORMAL record is the human's move.
  2. Build moveData md = { pmr=last_record, pms=&ms,
        pesChequer=&esAnalysisChequer, pesCube=&esAnalysisCube, aamf=aamfAnalysis }.
  3. Call asyncAnalyzeMove(&md)  (or AnalyzeMove directly).
  4. Read:
       out[0] = floatbits( pmr->ml.amMoves[pmr->n.iMove].rScore )   // played
       out[1] = floatbits( pmr->ml.amMoves[0].rScore )              // best
  5. Best-move board: ApplyMove(copy of pre-move board, amMoves[0].anMove),
     SwapSides, pack into out[2..51].

IMPORTANT ordering: tutor_analyze runs AFTER applyMoveString (the move record
must exist in plGame). This is the OPPOSITE of the abandoned hook ordering.

## GameViewModel.confirm() changes
  - Move the tutor call to AFTER Engine.applyMoveString(moveStr).
  - tutorAnalyze takes NO board arguments -- it reads plGame's last record.
  - Signature: Engine.tutorAnalyze(): IntArray   (52 ints, or empty on failure).

## Open item to resolve first
Confirm how the facade reaches plGame's last moverecord. Search engine-core for
plGame, plLastMove, AddMoveRecord. gnubg's AddMoveRecord appends to plGame; the
last element is the move just played.

## Files to change (against clean HEAD)
  - jni-bridge/src/gnubg_mobile.c     : implement tutor_analyze using AnalyzeMove
  - jni-bridge/include/gnubg_mobile.h : signature (no board args)
  - jni-bridge/src/native-lib.c       : tutorAnalyze JNI (no board args)
  - gnubg-app/.../Engine.kt           : external fun tutorAnalyze(): IntArray
  - gnubg-app/.../GameViewModel.kt    : hook AFTER applyMoveString, no board args

## Verification
  ./build_and_deploy.sh ; adb logcat | grep -E "gnubg-tutor|gnubg-vm"
  Each human move -> gnubg-tutor line with real equity loss + feature deltas.
  A known blunder must show level=BLUNDER with matching deltas.

## DO NOT
  - Do not hand-roll move matching by PositionKey. gnubg stores iMove.
  - Do not re-evaluate moves in a custom loop. AnalyzeMove fills amMoves[].rScore.
  - Do not reinvent skill classification. gnubg's Skill()/move_skill() is authoritative.
