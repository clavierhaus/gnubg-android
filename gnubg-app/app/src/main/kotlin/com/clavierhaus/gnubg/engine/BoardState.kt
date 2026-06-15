package com.clavierhaus.gnubg.engine

data class BoardState(
    // Display board — ms.anBoard, swapped when fTurn==1 for human-bottom perspective
    val board: IntArray = IntArray(50),

    // Board at start of human turn — needed by findMove() to locate the move
    val oldBoard: IntArray = IntArray(50),

    // ms.fTurn: 0 = human, 1 = engine
    val turn: Int = 0,

    // Dice for display — from ms.anDice at roll time, consumed by ApplySubMove taps
    val dice: Pair<Int, Int>? = null,

    // Original dice at start of turn — for findMove() at confirm time
    val originalDice: Pair<Int, Int>? = null,

    // Engine dice from last engine roll — for display during WAITING_FOR_ROLL
    val engineDice: Pair<Int, Int>? = null,

    // Remaining dice to play this turn
    val remainingDice: List<Int> = emptyList(),

    // Board snapshots before each sub-move tap — for undo
    val boardHistory: List<IntArray> = emptyList(),

    // Dice list before each sub-move tap — for undo
    val diceHistory: List<List<Int>> = emptyList(),

    // Legal moves for current remaining dice — flat array (nMoves * 8)
    val legalMoves: IntArray = IntArray(0),

    // Cube — from ms
    val cubeValue: Int = 1,
    val cubeOwner: Int = -1,

    // Pip counts
    val pipCountHuman: Int = 167,
    val pipCountEngine: Int = 167,

    // Game phase
    val phase: GamePhase = GamePhase.WAITING_FOR_ROLL,

    // Winner: -1 = none, 0 = human, 1 = engine
    val winner: Int = -1
) {
    override fun equals(other: Any?): Boolean {
        if (this === other) return true
        if (other !is BoardState) return false
        return board.contentEquals(other.board) &&
               turn == other.turn &&
               dice == other.dice &&
               remainingDice == other.remainingDice &&
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
