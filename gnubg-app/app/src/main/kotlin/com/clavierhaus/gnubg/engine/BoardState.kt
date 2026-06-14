package com.clavierhaus.gnubg.engine

data class BoardState(
    // 50-element board encoding
    val board: IntArray = IntArray(50),

    // Whose turn: 0 = human (light), 1 = engine (dark)
    val turn: Int = 0,

    // Original dice roll — null means not yet rolled
    val dice: Pair<Int, Int>? = null,

    // Remaining dice to be played this turn (e.g. [5,3] or [4,4,4,4])
    // A die is grayed out if it has no legal moves from any point
    val remainingDice: List<Int> = emptyList(),

    // Dice that are unavailable (no legal moves) — shown grayed
    val blockedDice: List<Int> = emptyList(),

    // Stack of board states before each move — for Cancel reversal
    // Each entry is the board state before that move was applied
    val moveHistory: List<IntArray> = emptyList(),

    // Stack of remaining dice before each move — for Cancel reversal
    val diceHistory: List<List<Int>> = emptyList(),

    // All legal moves for current remaining dice as flat array (nMoves * 8)
    val legalMoves: IntArray = IntArray(0),

    // Cube value
    val cubeValue: Int = 1,

    // Cube owner: -1 = centred, 0 = human, 1 = engine
    val cubeOwner: Int = -1,

    // Pip counts
    val pipCountHuman: Int = 167,
    val pipCountEngine: Int = 167,

    // Game phase
    val phase: GamePhase = GamePhase.WAITING_FOR_ROLL,

    // Winner: -1 = none, 0 = human, 1 = engine
    val winner: Int = -1
) {
    // Commit is active when all available dice are used
    val canCommit: Boolean get() =
        phase == GamePhase.HUMAN_MOVING &&
        (remainingDice.isEmpty() || remainingDice.all { it in blockedDice })

    // Cancel is active when there are moves to undo
    val canCancel: Boolean get() =
        phase == GamePhase.HUMAN_MOVING && moveHistory.isNotEmpty()

    override fun equals(other: Any?): Boolean {
        if (this === other) return true
        if (other !is BoardState) return false
        return board.contentEquals(other.board) &&
               turn == other.turn &&
               dice == other.dice &&
               remainingDice == other.remainingDice &&
               blockedDice == other.blockedDice &&
               legalMoves.contentEquals(other.legalMoves) &&
               cubeValue == other.cubeValue &&
               cubeOwner == other.cubeOwner &&
               pipCountHuman == other.pipCountHuman &&
               pipCountEngine == other.pipCountEngine &&
               phase == other.phase &&
               winner == other.winner
    }

    override fun hashCode(): Int {
        var result = board.contentHashCode()
        result = 31 * result + turn
        result = 31 * result + (dice?.hashCode() ?: 0)
        result = 31 * result + remainingDice.hashCode()
        result = 31 * result + blockedDice.hashCode()
        result = 31 * result + legalMoves.contentHashCode()
        result = 31 * result + cubeValue
        result = 31 * result + cubeOwner
        result = 31 * result + pipCountHuman
        result = 31 * result + pipCountEngine
        result = 31 * result + phase.hashCode()
        result = 31 * result + winner
        return result
    }
}

enum class GamePhase {
    WAITING_FOR_ROLL,
    HUMAN_MOVING,
    ENGINE_THINKING,
    GAME_OVER
}
