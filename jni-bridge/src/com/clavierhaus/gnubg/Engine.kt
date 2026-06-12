package com.clavierhaus.gnubg

/**
 * JNI bridge to the GNU Backgammon evaluation engine.
 *
 * GNU Backgammon by clavierhaus.at
 *
 * Board encoding: IntArray of 50 elements.
 *   elements  0..24 → anBoard[0] (opponent's checkers from their perspective)
 *   elements 25..49 → anBoard[1] (current player's checkers)
 *
 * Call [initialise] once with the path to the extracted gnubg.weights file
 * before calling any evaluation function.
 */
object Engine {

    init {
        System.loadLibrary("gnubg-engine")
    }

    /**
     * Initialise the evaluation engine.
     * @param weightsPath Absolute path to gnubg.weights on device storage.
     * @return true on success.
     */
    external fun initialise(weightsPath: String): Boolean

    /**
     * Evaluate a position.
     * @param board 50-element board encoding.
     * @return FloatArray[5]: [winNormal, winGammon, winBackgammon,
     *                         loseGammon, loseBackgammon], or null on error.
     */
    external fun evaluatePosition(board: IntArray): FloatArray?

    /**
     * Find the best move for the given dice roll.
     * @param board 50-element board encoding.
     * @param die0  First die value (1–6).
     * @param die1  Second die value (1–6).
     * @return IntArray[8] move encoding, unused slots are -1, or null on error.
     */
    external fun findBestMove(board: IntArray, die0: Int, die1: Int): IntArray?

    /**
     * Classify a position (race, contact, bearoff, etc.)
     * @param board 50-element board encoding.
     * @return positionclass integer value, or -1 if not initialised.
     */
    external fun classifyPosition(board: IntArray): Int

    /**
     * Apply a move to a board.
     * @param board 50-element board encoding.
     * @param move  IntArray[8] move encoding from [findBestMove].
     * @return New 50-element board after the move.
     */
    external fun applyMove(board: IntArray, move: IntArray): IntArray
}
