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
    val crawford: Boolean = false,   // gnubg's ms.fCrawford: THIS game is the Crawford game
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
               legalMoves.contentEquals(other.legalMoves) &&
               cubeValue == other.cubeValue &&
               cubeOwner == other.cubeOwner &&
               fDoubled == other.fDoubled &&
               canDouble == other.canDouble &&
               crawford == other.crawford &&
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
        result = 31 * result + legalMoves.contentHashCode()
        result = 31 * result + cubeValue
        result = 31 * result + cubeOwner
        result = 31 * result + fDoubled.hashCode()
        result = 31 * result + canDouble.hashCode()
        result = 31 * result + crawford.hashCode()
        result = 31 * result + pipCountHuman
        result = 31 * result + pipCountEngine
        result = 31 * result + phase.hashCode()
        result = 31 * result + winner
        result = 31 * result + (tutorAnalysis?.hashCode() ?: 0)
        result = 31 * result + (analysisDetail?.hashCode() ?: 0)
        return result
    }
}

/** What ENGINE_THINKING is doing -- the one app-side distinction the coach
 *  panel needs. Set only by the engine thread's beginEngineWork/settle. */
enum class BusyKind {
    NONE,
    JUDGING,   // computing the verdict on the player's move (coach)
    REPLYING,  // the player's action is delivered; GNU rolls and replies
    ANALYSING  // post-match: gnubg analyses the whole record for the PR report
}

enum class GamePhase {
    WAITING_FOR_ROLL,
    HUMAN_MOVING,
    ENGINE_THINKING,
    COACH_REVIEW,         // Coach only: the player's move is judged but NOT yet
                          // given to gnubg -- GNU has not rolled. The player
                          // studies the verdict and alternatives at the exact
                          // position they arose from, then actively continues.
    CUBE_OFFERED,
    RESIGNATION_OFFERED,  // GNU has resigned; gnubg refuses every roll until answered
    HUMAN_CAN_DOUBLE,
    GAME_OVER
}

/**
 * The decoded result of a whole-match analysis pass (PR feature). Every value
 * here is gnubg's own -- decoded from the match-stats verb (out[40]) and the
 * error-list verb (out[104]); NOTHING is computed in Kotlin beyond decoding
 * IEEE float bits and applying the display-only PR scale (rate x 500, the
 * convention the pros quote). See gnubg_mobile_match_stats /
 * gnubg_mobile_match_errors for the authoritative layouts.
 *
 * `valid` is the G3 self-consistency verdict: the error list's per-player sums
 * (from the errors verb) must equal the stats verb's own arErrorCheckerplay
 * totals within epsilon. A mismatch means a field was misread somewhere; the
 * report is marked invalid, logged loud, and never shown.
 */
data class MatchError(
    val gameIdx: Int,
    val recIdx: Int,
    val player: Int,
    val errorEmg: Float,   // <= 0; more negative = worse
    val skill: Int
)

data class MatchReport(
    val games: Int,
    val totalMoves: IntArray,           // anTotalMoves[p]
    val unforcedMoves: IntArray,        // anUnforcedMoves[p]
    val closeCube: IntArray,            // anCloseCube[p]
    val totalCube: IntArray,            // anTotalCube[p]
    val prPerPlayer: FloatArray,        // rate x 500, index 0/1
    val chequerErrEmg: FloatArray,      // arErrorCheckerplay[p][0]
    val cubeErrEmg: FloatArray,         // summed cube error [p]
    val luckEmg: FloatArray,            // arLuck[p][0]
    val perDecisionRate: FloatArray,    // aaaar combined per-move [p]
    val skillHisto: Array<IntArray>,    // [p][N_SKILLS]
    val luckHisto: Array<IntArray>,     // [p][N_LUCKS]
    val actualResult: FloatArray,       // arActualResult[p]
    val luckAdjResult: FloatArray,      // arLuckAdj[p]
    val errors: List<MatchError>,       // worst-first
    val valid: Boolean                  // G3 self-consistency verdict
)
