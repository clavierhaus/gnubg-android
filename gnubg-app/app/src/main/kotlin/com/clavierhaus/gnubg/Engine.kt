package com.clavierhaus.gnubg

object Engine {
    init {
        System.loadLibrary("gnubg-engine")
    }

    // Initialisation
    external fun initialise(weightsPath: String): Boolean

    // Match state management
    external fun newGame(matchLength: Int): IntArray
    external fun nextGame(): IntArray
    external fun rollDice(): IntArray

    // GNUbg command-surface groundwork
    external fun commandNewGame()
    external fun commandNewMatch(matchLength: Int)
    external fun commandNewSession(games: Int)
    external fun commandEndGame()
    external fun commandResign(value: String)
    external fun commandNext(argument: String = "")

    // Step backwards through the game record. PORT: CommandPrevious.
    // Same argument grammar as commandNext: "" for one record, or
    // "game" / "roll" / "rolled" / "marked", or a count.
    external fun commandPrevious(argument: String = "")
    external fun commandAccept()
    external fun commandReject()
    external fun commandDecline()
    external fun commandAgree()
    external fun commandRedouble()
    external fun getLegalMoves(board: IntArray, die0: Int, die1: Int, fPartial: Int = 0): IntArray

    /** Set engine chequer-play strength to a gnubg preset.
     *  idx: 0=Beginner, 1=Casual play, 2=Intermediate, 3=Advanced. */
    external fun setEngineStrength(idx: Int)
    external fun setAutoCrawford(on: Boolean)
    external fun setJacoby(on: Boolean)
    external fun setCubeUse(on: Boolean)
    /** Load a match equity table by file path (empty = built-in Zadeh default). */
    external fun setMet(path: String)
    external fun setAutoDoubles(n: Int)
    external fun setBeavers(n: Int)



    external fun applyMoveString(moveStr: String): IntArray    external fun formatMove(board: IntArray, move: IntArray): String

    // Match state queries
    external fun getMatchBoard(): IntArray
    external fun getMatchBoardHuman(): IntArray  // stable human-frame board; use for display
    external fun getMatchDice(): IntArray
    external fun getMatchTurn(): Int
    external fun getMatchStatus(): Int
    external fun getMatchWinner(): Int
    external fun getMatchScore(): IntArray
    external fun getMatchLength(): Int  // [humanScore, engineScore, matchLength]
    external fun getGameResult(): IntArray  // [fWinner, nPoints]
    external fun getMatchCubeInfo(): IntArray  // [fDoubled, fCubeOwner, nCube]
    external fun getCubeDebugState(): IntArray  // [gs, fTurn, fMove, dice0, dice1, fDoubled, fCubeOwner, nCube, fCrawford, fCubeUse, score0, score1, matchTo]
    external fun canDouble(): Boolean
    external fun commandDouble()
    external fun commandTake()
    external fun commandDrop()

    // Resignation offered BY GNU: 0 none, 1 normal, 2 gammon, 3 backgammon.
    // GNU offers it itself when the position is lost (play.c:1335). gnubg then
    // refuses every roll until the human answers.
    // Answered with the EXISTING commandAgree() / commandDecline() above.
    external fun getResignation(): Int
    external fun getLastEngineDice(): IntArray
    external fun getMoveRecordDice(): IntArray

    // Board utilities
    external fun swapBoard(board: IntArray): IntArray
    external fun applySubMove(board: IntArray, iSrc: Int, nRoll: Int): IntArray
    external fun findMove(oldBoard: IntArray, curBoard: IntArray, die0: Int, die1: Int): String

    /**
     * Tutor analysis via gnubg's AnalyzeMove. Call AFTER applyMoveString.
     * oldBoard = pre-move board. Returns IntArray[52]:
     *   [0] = Float.fromBits(played equity), [1] = Float.fromBits(best equity),
     *   [2..51] = best-move board. Empty array if no analyzable move.
     */
    external fun tutorAnalyze(oldBoard: IntArray): IntArray

    /** Analysis-mode detail: [Win, Wg, Wbg, Lg, Lbg, eqCubeful, eqCubeless]. */
    external fun analyzePlayedMove(oldBoard: IntArray): FloatArray

    /** gnubg's verdict on the move at the review cursor (plLastMove).
     *  Empty array when the cursor is not a chequer move. Layout:
     *  [0]=rank(0-based) [1]=movesConsidered [2]=playedEq(bits) [3]=bestEq(bits)
     *  [4]=skill(0 very bad,1 bad,2 doubtful,3 none)
     *  [5..12]=played anMove  [13..20]=best anMove
     *  [21..70]=pre-move board (mover frame), for formatMove. */
    external fun reviewVerdict(): IntArray

    /** Coach M0 (docs/COACH_MODE_PLAN.md): gnubg's verdict on the last human
     *  chequer move of the live game, widened for every disclosure level over
     *  ONE evaluation. Empty when there is no human move to judge. Layout:
     *  [0]=rank [1]=movesConsidered [2]=playedEq(bits) [3]=bestEq(bits)
     *  [4]=skill(0 very bad,1 bad,2 doubtful,3 none)
     *  [5..12]=played anMove [13..20]=best anMove
     *  [21..70]=pre-move board (mover frame, for formatMove)
     *  [71..77]=played arEvalMove (win,winG,winBG,loseG,loseBG,eq,cubefulEq bits)
     *  [78..84]=best arEvalMove
     *  [85]=K candidates, then K rows of 16: anMove[8], equity(bits),
     *  arEvalMove[7](bits), ranked from best. */
    /** Coach verdict computed BEFORE the move is applied -- same 166-int layout
     *  as coachVerdict, but board-based: pre-move board + dice + the chosen
     *  board, judged against the live pre-move match state. Empty on failure. */
    external fun coachVerdictPre(oldBoard: IntArray, d0: Int, d1: Int, newBoard: IntArray): IntArray

    /** Coach cube verdict (M4): judges the human's cube action against gnubg's
     *  decision. action = 0 no-double, 1 doubled, 2 took, 3 dropped. Returns a
     *  10-int array (layout at gnubg_mobile_coach_cube_verdict) of gnubg's own
     *  values only -- cd enum, arDouble equities, Skill() band. Empty on error. */
    external fun coachCubeVerdict(board: IntArray, action: Int): IntArray

    /** gnubg's ApplyMove on a board: the resulting position of one of gnubg's
     *  own anMove[8] candidates, for the Coach explorer. Empty on failure. */
    external fun applyMoveToBoard(board: IntArray, anMove: IntArray): IntArray

    external fun coachVerdict(): IntArray

    /** Lock-free live-dice channel: [seq, die0, die1]. The seq advances the
     *  moment gnubg rolls (play.c DiceRolled -> port hook), which for the
     *  engine is BEFORE its move search -- so the UI can show what the engine
     *  is thinking about. Snapshot seq before the turn; dice are fresh only
     *  once seq has moved. */
    external fun peekLiveDice(): IntArray

    /** gnubg own move classifier (analysis.c Skill). equityDelta = played - best
     *  (<= 0). Returns gnubg skilltype ordinal: 0=VERYBAD 1=BAD 2=DOUBTFUL 3=NONE. */
    external fun skill(equityDelta: Float): Int
    external fun pipCount(board: IntArray): IntArray

    /** gnubg own position-feature inputs (eval.c CalculateHalfInputs), both sides.
     *  FloatArray[2*MORE_INPUTS]: player0 block then player1 block, raw normalised. */
    external fun positionFeatures(board: IntArray): FloatArray

    // Analysis
    external fun evaluatePosition(board: IntArray): FloatArray?
    external fun classifyPosition(board: IntArray): Int
    external fun cubeDecision(board: IntArray): IntArray?
    external fun rollout(board: IntArray, trials: Int): FloatArray?

    // Files / SGF
    external fun loadGame(path: String): Boolean
    external fun saveGame(path: String): Boolean
    external fun loadMatch(path: String): Boolean
    external fun saveMatch(path: String): Boolean
    external fun loadPosition(path: String): Boolean
    external fun savePosition(path: String): Boolean

    external fun loadSGF(path: String): Boolean
    external fun saveSGF(path: String): Boolean

    // PR feature: gnubg's own analysis over the whole match record, summed
    // with AddStatcontext, rates from getMWCFromError -- no arithmetic of
    // ours near the errors. 40-int layout documented at
    // gnubg_mobile_match_stats (floats as IEEE bits). [15..16] carry the
    // combined per-decision EMG rate; PR as the pros quote it is that rate
    // times 500 (a display convention, applied only at the UI). Heavy:
    // 2-ply over the full record -- call from a background dispatcher.
    external fun matchStats(): IntArray

    // Position entry (Analyse Position). Wraps gnubg's SetGNUbgID: accepts a
    // GNU BG ID ("PositionID:MatchID") or an XGID. Returns gnubg's own code:
    // 0 installed, 1 no valid IDs found, 2 installed but the player on roll is
    // on top -- ask the user, then call swapPlayers() only if they agree.
    external fun setGnubgId(id: String): Int

    // Encode an edited position + context as "PositionID:MatchID", with gnubg's
    // own encoders. Install the result via setGnubgId -- the same path a pasted
    // ID takes, with the same validation. Dice 0,0 = not rolled = cube decision.
    // cubeOwner: -1 centred, 0 human, 1 engine. matchTo 0 = money game.
    external fun idsFromState(board: IntArray, d0: Int, d1: Int, turn: Int,
                              scoreH: Int, scoreE: Int, matchTo: Int,
                              cube: Int, cubeOwner: Int, crawford: Int): String?

    // gnubg's own words for cubeDecision()'s decision value (index 18).
    external fun cubeRecommendation(cd: Int): String?
    external fun swapPlayers(): Int

    // gnubg's renderings of the current state: [0] Position ID, [1] Match ID.
    external fun currentIds(): Array<String>?

    // One consistent snapshot of gnubg's matchstate, taken under a single lock:
    // [0] gs, [1] fTurn, [2] fMove, [3] dice0, [4] dice1, [5] fDoubled,
    // [6] fCubeOwner, [7] nCube, [8] fCrawford, [9] fCubeUse, [10] score0,
    // [11] score1, [12] nMatchTo.
    external fun getMatchState(): IntArray

    // Ranked chequer-play candidates for the position currently loaded, best
    // first, as gnubg orders them. outEquity holds maxN floats, outMoves maxN*8
    // ints (gnubg anMove: four src/dst pairs). Returns the count written, 0 when
    // the position has no dice, -1 on error.
    external fun hintMoves(maxN: Int, outEquity: FloatArray, outMoves: IntArray): Int

    // Restricted GNUbg command bridge for translated Android settings.
    external fun runCommand(command: String): Boolean
}
