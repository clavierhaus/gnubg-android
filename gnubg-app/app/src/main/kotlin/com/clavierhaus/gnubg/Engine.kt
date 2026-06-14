package com.clavierhaus.gnubg

/**
 * JNI bridge to the GNU Backgammon evaluation engine.
 * GNU Backgammon by clavierhaus.at
 *
 * Board encoding: IntArray of 50 elements.
 *   elements  0..24 → anBoard[0] (opponent's checkers)
 *   elements 25..49 → anBoard[1] (current player's checkers)
 */
object Engine {

    init {
        System.loadLibrary("gnubg-engine")
    }

    external fun initialise(weightsPath: String): Boolean
    external fun evaluatePosition(board: IntArray): FloatArray?
    external fun findBestMove(board: IntArray, die0: Int, die1: Int): IntArray?
    external fun classifyPosition(board: IntArray): Int
    external fun applyMove(board: IntArray, move: IntArray): IntArray

    /**
     * Evaluate a cube decision.
     *
     * @param board      50-element board encoding
     * @param cubeValue  Current cube value (1, 2, 4, 8...)
     * @param cubeOwner  Cube owner: -1=centred, 0=opponent, 1=player
     * @param matchTo    Match length (0 for money game)
     * @param score0     Opponent's score
     * @param score1     Player's score
     * @param crawford   1 if Crawford game, 0 otherwise
     *
     * @return IntArray[16] where:
     *   bits [0..6]  = aarOutput[0] as Float.fromBits() — if-double equities
     *   bits [7..13] = aarOutput[1] as Float.fromBits() — no-double equities
     *   [14] = cubedecision enum value (see CubeDecision below)
     *   [15] = reserved
     *
     * Unpack equity: val equity = Float.fromBits(result[i])
     */
    external fun cubeDecision(
        board: IntArray,
        cubeValue: Int,
        cubeOwner: Int,
        matchTo: Int,
        score0: Int,
        score1: Int,
        crawford: Int
    ): IntArray?

    /** Cube decision recommendations returned by [cubeDecision] at index [14]. */
    enum class CubeDecision(val value: Int) {
        DOUBLE_TAKE(0),
        DOUBLE_PASS(1),
        NODOUBLE_TAKE(2),
        TOOGOOD_TAKE(3),
        TOOGOOD_PASS(4),
        DOUBLE_BEAVER(5),
        NODOUBLE_BEAVER(6),
        REDOUBLE_TAKE(7),
        REDOUBLE_PASS(8),
        NO_REDOUBLE_TAKE(9),
        TOOGOODRE_TAKE(10),
        TOOGOODRE_PASS(11),
        NO_REDOUBLE_BEAVER(12),
        NODOUBLE_DEADCUBE(13),
        NO_REDOUBLE_DEADCUBE(14),
        NOT_AVAILABLE(15),
        OPTIONAL_DOUBLE_TAKE(16),
        OPTIONAL_REDOUBLE_TAKE(17),
        OPTIONAL_DOUBLE_BEAVER(18),
        OPTIONAL_DOUBLE_PASS(19),
        OPTIONAL_REDOUBLE_PASS(20);

        companion object {
            fun fromInt(v: Int) = entries.firstOrNull { it.value == v } ?: NOT_AVAILABLE
        }
    }
    /**
     * Load a match from an SGF file.
     */
    external fun rollDice(): IntArray
    external fun getLegalMoves(board: IntArray, die0: Int, die1: Int): IntArray
    external fun isGameOver(board: IntArray): Int
    external fun newGame(): IntArray

    external fun loadSGF(path: String): Boolean

    /**
     * Save the current match to an SGF file.
     */
    external fun saveSGF(path: String): Boolean

    external fun testGenerateMoves(board: IntArray, die0: Int, die1: Int): Int
}