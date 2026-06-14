package com.clavierhaus.gnubg.engine

// Represents the complete state of a backgammon position
data class BoardState(
    // 50-element board encoding — anBoard[0][0..24] at indices 0..24,
    // anBoard[1][0..24] at indices 25..49
    val board: IntArray = IntArray(50),

    // Whose turn: 0 = human (light checkers), 1 = engine (dark checkers)
    val turn: Int = 0,

    // Current dice roll — null means not yet rolled
    val dice: Pair<Int, Int>? = null,

    // All legal moves for current dice as flat array (nMoves * 8)
    val legalMoves: IntArray = IntArray(0),

    // Currently selected point (-1 = none)
    val selectedPoint: Int = -1,

    // Cube value
    val cubeValue: Int = 1,

    // Cube owner: -1 = centred, 0 = human, 1 = engine
    val cubeOwner: Int = -1,

    // Pip counts
    val pipCountHuman: Int = 167,
    val pipCountEngine: Int = 167,

    // Game phase
    val phase: GamePhase = GamePhase.WAITING_FOR_ROLL,

    // Winner: -1 = no winner yet, 0 = human, 1 = engine
    val winner: Int = -1
) {
    // IntArray needs custom equals/hashCode for data class
    override fun equals(other: Any?): Boolean {
        if (this === other) return true
        if (other !is BoardState) return false
        return board.contentEquals(other.board) &&
               turn == other.turn &&
               dice == other.dice &&
               legalMoves.contentEquals(other.legalMoves) &&
               selectedPoint == other.selectedPoint &&
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
        result = 31 * result + legalMoves.contentHashCode()
        result = 31 * result + selectedPoint
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
    WAITING_FOR_ROLL,   // Player needs to tap Roll
    HUMAN_MOVING,       // Human selects and moves checkers
    ENGINE_THINKING,    // Engine is computing its move
    GAME_OVER           // Someone won
}
