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
    external fun applyMoveString(moveStr: String): IntArray
    external fun formatMove(board: IntArray, move: IntArray): String

    // Match state queries
    external fun getMatchBoard(): IntArray
    external fun getMatchDice(): IntArray
    external fun getMatchTurn(): Int
    external fun getMatchStatus(): Int
    external fun getMatchWinner(): Int
    external fun getMatchScore(): IntArray
    external fun getMatchLength(): Int  // [humanScore, engineScore, matchLength]
    external fun getGameResult(): IntArray  // [fWinner, nPoints]
    external fun getMatchCubeInfo(): IntArray  // [fDoubled, fCubeOwner, nCube]
    external fun getCubeDebugState(): IntArray  // [gs, fTurn, fMove, dice0, dice1, fDoubled, fCubeOwner, nCube, fCrawford, fCubeUse, score0, score1, matchTo]
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
    external fun pipCount(board: IntArray): IntArray

    // Analysis
    external fun evaluatePosition(board: IntArray): FloatArray?
    external fun classifyPosition(board: IntArray): Int
    external fun cubeDecision(board: IntArray, cubeValue: Int, cubeOwner: Int, fMove: Int,
                               matchTo: Int, score0: Int, score1: Int,
                               crawford: Int): IntArray?
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
