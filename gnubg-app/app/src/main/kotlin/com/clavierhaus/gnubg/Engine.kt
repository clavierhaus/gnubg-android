package com.clavierhaus.gnubg

object Engine {
    init {
        System.loadLibrary("gnubg-engine")
    }

    external fun initialise(weightsPath: String): Boolean
    external fun evaluatePosition(board: IntArray): FloatArray?
    external fun findBestMove(board: IntArray, die0: Int, die1: Int): IntArray?
    external fun swapBoard(board: IntArray): IntArray
    external fun classifyPosition(board: IntArray): Int
    external fun applyMove(board: IntArray, move: IntArray): IntArray
    external fun getLegalMoves(board: IntArray, die0: Int, die1: Int): IntArray
    external fun isGameOver(board: IntArray): Int
    external fun pipCount(board: IntArray): IntArray
    external fun rollDice(): IntArray
    external fun newGame(): IntArray
    external fun cubeDecision(board: IntArray, cubeValue: Int, cubeOwner: Int,
                               matchTo: Int, score0: Int, score1: Int,
                               crawford: Int): IntArray?
    external fun rollout(board: IntArray, trials: Int): FloatArray?
    external fun loadSGF(path: String): Boolean
    external fun saveSGF(path: String): Boolean
}
