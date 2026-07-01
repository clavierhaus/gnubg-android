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
    external fun setAutoDoubles(n: Int)
    external fun setBeavers(n: Int)

    /**
     * Return ranked move candidates with cubeless 1-ply equity.
     *
     * Returns a flat IntArray with layout:
     *   [0]         = n  (number of candidates)
     *   [1 + i*9]   = anMove[0..7] for candidate i  (8 ints, same encoding as getLegalMoves)
     *   [1 + i*9+8] = Float.fromBits(equity_i)       (cubeless equity, higher = better)
     *
     * candidates[0] is always the engine's best move.
     * nMax is capped at 20 in the native layer.
     */
    external fun getCandidates(board: IntArray, die0: Int, die1: Int, nMax: Int = 10): IntArray

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
    external fun engineCubeResponse(take: Boolean)
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

    /** gnubg own move classifier (analysis.c Skill). equityDelta = played - best
     *  (<= 0). Returns gnubg skilltype ordinal: 0=VERYBAD 1=BAD 2=DOUBTFUL 3=NONE. */
    external fun skill(equityDelta: Float): Int
    external fun pipCount(board: IntArray): IntArray

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

    // Restricted GNUbg command bridge for translated Android settings.
    external fun runCommand(command: String): Boolean
}
