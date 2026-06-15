package com.clavierhaus.gnubg

object Engine {
    init {
        System.loadLibrary("gnubg-engine")
    }

    // Initialisation
    external fun initialise(weightsPath: String): Boolean

    // Match state management
    external fun newGame(): IntArray
    external fun rollDice(): IntArray
    external fun getLegalMoves(board: IntArray, die0: Int, die1: Int, fPartial: Int = 0): IntArray
    external fun applyMoveString(moveStr: String): IntArray
    external fun formatMove(board: IntArray, move: IntArray): String

    // Match state queries
    external fun getMatchBoard(): IntArray
    external fun getMatchDice(): IntArray
    external fun getMatchTurn(): Int
    external fun getMatchStatus(): Int
    external fun getMatchWinner(): Int
    external fun getLastEngineDice(): IntArray

    // Board utilities
    external fun swapBoard(board: IntArray): IntArray
    external fun applySubMove(board: IntArray, iSrc: Int, nRoll: Int): IntArray
    external fun findMove(oldBoard: IntArray, curBoard: IntArray, die0: Int, die1: Int): String
    external fun pipCount(board: IntArray): IntArray

    // Analysis
    external fun evaluatePosition(board: IntArray): FloatArray?
    external fun classifyPosition(board: IntArray): Int
    external fun cubeDecision(board: IntArray, cubeValue: Int, cubeOwner: Int,
                               matchTo: Int, score0: Int, score1: Int,
                               crawford: Int): IntArray?
    external fun rollout(board: IntArray, trials: Int): FloatArray?

    // SGF
    external fun loadSGF(path: String): Boolean
    external fun saveSGF(path: String): Boolean
}
