package com.clavierhaus.gnubg.engine

import com.clavierhaus.gnubg.tutor.BlunderLevel

data class TutorAnalysis(
    val level: BlunderLevel,
    val equityLoss: Float,
    val bestEquity: Float,
    val playedEquity: Float
)

/** Analysis-mode probability breakdown for the played move, straight from
 *  gnubg's Hint-window vector (mover's frame). Win includes gammon+bg; winGammon
 *  includes bg -- cumulative exactly as gnubg reports. */
data class MoveAnalysisDetail(
    val win: Float,
    val winGammon: Float,
    val winBackgammon: Float,
    val loseGammon: Float,
    val loseBackgammon: Float,
    val equityCubeful: Float,
    val equityCubeless: Float
)

data class MoveSnapshot(
    val board: IntArray,
    val remainingDice: List<Int>,
    val legalMoves: IntArray,
    val pipCountHuman: Int,
    val pipCountEngine: Int,
    val played: List<Int> = emptyList()
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
    // Sub-moves already applied this turn, as gnubg encodes them: a flat list of
    // (src, dest) pairs, dest = src - die, negative when bearing off. Reset every
    // turn because a fresh BoardState is built when the dice are rolled.
    val played: List<Int> = emptyList(),
    val matchScore: IntArray = IntArray(2),
    val matchLength: Int = 1,
    val diceHistory: List<List<Int>> = emptyList(),
    val legalMoves: IntArray = IntArray(0),
    val cubeValue: Int = 1,
    val cubeOwner: Int = -1,   // -1=centred, 0=human, 1=engine
    val fDoubled: Boolean = false,
    val canDouble: Boolean = false,  // engine authority (gnubg_can_double); UI cube tappability
    val unplayableDice: Set<Int> = emptySet(),  // die faces gnubg lists no move for; greyed in UI
    val resignation: Int = 0,                   // gnubg's ms.fResigned: 1 normal, 2 gammon, 3 backgammon
    val pipCountHuman: Int = 167,
    val pipCountEngine: Int = 167,
    val phase: GamePhase = GamePhase.WAITING_FOR_ROLL,
    val winner: Int = -1,
    val nPoints: Int = 1,
    val humanScore: Int = 0,
    val engineScore: Int = 0,
    val tutorAnalysis: TutorAnalysis? = null,
    val analysisDetail: MoveAnalysisDetail? = null,
) {
    override fun equals(other: Any?): Boolean {
        if (this === other) return true
        if (other !is BoardState) return false
        return board.contentEquals(other.board) &&
               turn == other.turn &&
               dice == other.dice &&
               remainingDice == other.remainingDice &&
               moveHistory == other.moveHistory &&
               played == other.played &&
               legalMoves.contentEquals(other.legalMoves) &&
               cubeValue == other.cubeValue &&
               cubeOwner == other.cubeOwner &&
               fDoubled == other.fDoubled &&
               canDouble == other.canDouble &&
               pipCountHuman == other.pipCountHuman &&
               pipCountEngine == other.pipCountEngine &&
               phase == other.phase &&
               winner == other.winner &&
               tutorAnalysis == other.tutorAnalysis &&
               analysisDetail == other.analysisDetail
    }

    override fun hashCode(): Int {
        var result = board.contentHashCode()
        result = 31 * result + turn
        result = 31 * result + (dice?.hashCode() ?: 0)
        result = 31 * result + remainingDice.hashCode()
        result = 31 * result + moveHistory.hashCode()
        result = 31 * result + played.hashCode()
        result = 31 * result + legalMoves.contentHashCode()
        result = 31 * result + cubeValue
        result = 31 * result + cubeOwner
        result = 31 * result + fDoubled.hashCode()
        result = 31 * result + canDouble.hashCode()
        result = 31 * result + pipCountHuman
        result = 31 * result + pipCountEngine
        result = 31 * result + phase.hashCode()
        result = 31 * result + winner
        result = 31 * result + (tutorAnalysis?.hashCode() ?: 0)
        result = 31 * result + (analysisDetail?.hashCode() ?: 0)
        return result
    }
}

enum class GamePhase {
    WAITING_FOR_ROLL,
    HUMAN_MOVING,
    ENGINE_THINKING,
    CUBE_OFFERED,
    RESIGNATION_OFFERED,  // GNU has resigned; gnubg refuses every roll until answered
    HUMAN_CAN_DOUBLE,
    GAME_OVER
}

/**
 * The sub-moves gnubg will accept next, given the ones already played this turn.
 *
 * [turnMoves] is gnubg's complete legal-move list for the turn, generated once
 * from the board and dice at the start of it (GenerateMoves). Each move occupies
 * eight ints: four (src, dest) pairs, dest = src - die, and a negative SOURCE
 * terminates (SaveMoves, eval.c:2571). A negative DEST is a bear-off.
 *
 * Legality in backgammon is a property of the whole move -- gnubg enforces
 * playing the maximum number of dice, and the larger die when only one can be
 * played -- so a sequence of individually legal sub-moves need not be a legal
 * play. Asking gnubg per sub-move, as this port used to, lets the player build a
 * position gnubg will refuse to commit, with nothing to explain why.
 *
 * So: keep only those moves whose first [played].size/2 sub-moves are exactly
 * what has been played, and report the next sub-move of each. What this returns
 * is always a step along some complete legal move; when it returns nothing, the
 * turn is finished. No legality is decided here -- gnubg's own list is filtered.
 *
 * Returns (src, dest) pairs, flat.
 */
fun nextSubMoves(turnMoves: IntArray, played: List<Int>): List<Pair<Int, Int>> {
    if (turnMoves.isEmpty()) return emptyList()
    val depth = played.size / 2
    if (depth >= 4) return emptyList()

    // The played pairs must be a sub-multiset of a move's pairs -- anywhere in
    // it, not as a leading run. gnubg stores one representative ordering per
    // move (it dedupes by final position), so requiring the player to tap in
    // that order would refuse legal plays: 8/3 then 13/11 is the same move as
    // 13/11 then 8/3, and only one ordering is in the list. The pairs are
    // absolute (src, dest), so their order does not change the move.
    //
    // What remains of a matching move is what may still be played.
    val playedPairs = ArrayList<Pair<Int, Int>>(depth)
    for (k in 0 until depth) playedPairs.add(played[k * 2] to played[k * 2 + 1])

    val out = LinkedHashSet<Pair<Int, Int>>()
    var m = 0
    while (m + 8 <= turnMoves.size) {
        val movePairs = ArrayList<Pair<Int, Int>>(4)
        for (k in 0 until 4) {
            val src = turnMoves[m + k * 2]
            if (src < 0) break                      // negative source terminates
            movePairs.add(src to turnMoves[m + k * 2 + 1])
        }
        if (movePairs.size > depth) {
            val leftover = ArrayList(movePairs)
            var ok = true
            for (pp in playedPairs) {
                if (!leftover.remove(pp)) { ok = false; break }
            }
            if (ok) out.addAll(leftover)
        }
        m += 8
    }
    return out.toList()
}
