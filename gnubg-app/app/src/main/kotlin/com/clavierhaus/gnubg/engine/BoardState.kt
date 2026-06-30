package com.clavierhaus.gnubg.engine

data class MoveSnapshot(
    val board: IntArray,
    val remainingDice: List<Int>,
    val legalMoves: IntArray,
    val blockedDice: Set<Int>,
    val pipCountHuman: Int,
    val pipCountEngine: Int
)

data class BoardState(
    val board: IntArray = IntArray(50),
    val oldBoard: IntArray = IntArray(50),
    val turn: Int = 0,
    val dice: Pair<Int, Int>? = null,
    val originalDice: Pair<Int, Int>? = null,
    val engineDice: Pair<Int, Int>? = null,
    val remainingDice: List<Int> = emptyList(),
    val moveHistory: List<MoveSnapshot> = emptyList(),
    val matchScore: IntArray = IntArray(2),
    val matchLength: Int = 1,
    val diceHistory: List<List<Int>> = emptyList(),
    val legalMoves: IntArray = IntArray(0),
    val cubeValue: Int = 1,
    val cubeOwner: Int = -1,   // -1=centred, 0=human, 1=engine
    val fDoubled: Boolean = false,
    val canDouble: Boolean = false,  // engine authority (gnubg_can_double); UI cube tappability
    val pipCountHuman: Int = 167,
    val pipCountEngine: Int = 167,
    val phase: GamePhase = GamePhase.WAITING_FOR_ROLL,
    val winner: Int = -1,
    val nPoints: Int = 1,
    val blockedDice: Set<Int> = emptySet(),
    val humanScore: Int = 0,
    val engineScore: Int = 0,
) {
    override fun equals(other: Any?): Boolean {
        if (this === other) return true
        if (other !is BoardState) return false
        return board.contentEquals(other.board) &&
               turn == other.turn &&
               dice == other.dice &&
               remainingDice == other.remainingDice &&
               moveHistory == other.moveHistory &&
               legalMoves.contentEquals(other.legalMoves) &&
               cubeValue == other.cubeValue &&
               cubeOwner == other.cubeOwner &&
               fDoubled == other.fDoubled &&
               canDouble == other.canDouble &&
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
        result = 31 * result + moveHistory.hashCode()
        result = 31 * result + legalMoves.contentHashCode()
        result = 31 * result + cubeValue
        result = 31 * result + cubeOwner
        result = 31 * result + fDoubled.hashCode()
        result = 31 * result + canDouble.hashCode()
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
    CUBE_OFFERED,
    HUMAN_CAN_DOUBLE,
    GAME_OVER
}
