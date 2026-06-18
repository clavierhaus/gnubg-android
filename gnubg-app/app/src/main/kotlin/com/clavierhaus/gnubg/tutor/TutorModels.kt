package com.clavierhaus.gnubg.tutor

enum class TutorSeverity {
    GOOD,
    INACCURACY,
    MISTAKE,
    BLUNDER
}

enum class TutorTheme {
    SAFETY,
    SHOT_COUNT,
    POINT_MAKING,
    EXPOSURE,
    RACE,
    CUBE_TAKE_PASS,
    GAMMON_DANGER
}

data class TutorMove(
    val move: String,
    val equity: Float,
    val rank: Int
)

data class TutorEvaluation(
    val userMove: String,
    val bestMove: String,
    val userEquity: Float,
    val bestEquity: Float,
    val equityLoss: Float,
    val severity: TutorSeverity,
    val topMoves: List<TutorMove> = emptyList()
)

data class TutorFeatureDelta(
    val userShotsLeft: Int? = null,
    val bestShotsLeft: Int? = null,
    val userBlots: Int = 0,
    val bestBlots: Int = 0,
    val userMadePoints: List<Int> = emptyList(),
    val bestMadePoints: List<Int> = emptyList(),
    val userPipCount: Int = 0,
    val bestPipCount: Int = 0,
    val keyPointMadeByBest: Int? = null
)

data class TutorHint(
    val severity: TutorSeverity,
    val mainTheme: TutorTheme?,
    val headline: String,
    val shortExplanation: String?,
    val measurableFacts: List<String> = emptyList(),
    val userMove: String,
    val bestMove: String,
    val equityLoss: Float,
    val allowTryAgain: Boolean = true,
    val allowShowBestMove: Boolean = true,
    val allowMoreDetail: Boolean = true
)

sealed interface TutorUiState {
    data object Hidden : TutorUiState

    data class CoachCard(
        val hint: TutorHint
    ) : TutorUiState

    data class CompareMoves(
        val hint: TutorHint
    ) : TutorUiState

    data class TryAgain(
        val hint: TutorHint
    ) : TutorUiState

    data class Detail(
        val evaluation: TutorEvaluation
    ) : TutorUiState
}

data class MoveArrow(
    val fromPoint: Int,
    val toPoint: Int,
    val checkerCount: Int = 1
)

data class ShotBadge(
    val point: Int,
    val diceValues: List<Int>
)

data class BoardTutorAnnotations(
    val userMoveArrows: List<MoveArrow> = emptyList(),
    val bestMoveArrows: List<MoveArrow> = emptyList(),
    val highlightedPoints: List<Int> = emptyList(),
    val shotBadges: List<ShotBadge> = emptyList()
)
