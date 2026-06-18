package com.clavierhaus.gnubg.engine

enum class BoardTheme { OCEAN, CLASSIC, FOREST, SYSTEM }

enum class Difficulty(val label: String, val subtitle: String) {
    BEGINNER("Beginner", "Easy opponent"),
    INTERMEDIATE("Intermediate", "Moderate challenge"),
    ADVANCED("Advanced", "Strong opponent"),
    EXPERT("Expert", "World-class play")
}

enum class TutorModePreset {
    OFF,
    GENTLE,
    SERIOUS,
    CLASSIC
}

enum class TutorFeedbackThreshold {
    EVERY_DECISION,
    INACCURACIES,
    MISTAKES,
    BLUNDERS,
    END_OF_GAME
}

enum class TutorAnnotationMode {
    OFF,
    BEST_MOVE_ONLY,
    USER_VS_BEST,
    FULL
}

enum class TutorEquityDetail {
    HIDDEN,
    LOSS_ONLY,
    MOVE_EQUITIES,
    CLASSIC
}

enum class CubeTutorMode {
    OFF,
    MAJOR_ERRORS,
    ALL_DECISIONS
}

enum class TutorRolloutAccess {
    DISABLED,
    ADVANCED_ONLY,
    CLASSIC_MODE
}

data class TutorPreferences(
    val tutorModePreset: TutorModePreset = TutorModePreset.OFF,
    val tutorFeedbackThreshold: TutorFeedbackThreshold =
        TutorFeedbackThreshold.MISTAKES,
    val tutorAnnotationMode: TutorAnnotationMode =
        TutorAnnotationMode.USER_VS_BEST,
    val tutorEquityDetail: TutorEquityDetail =
        TutorEquityDetail.LOSS_ONLY,
    val cubeTutorMode: CubeTutorMode = CubeTutorMode.MAJOR_ERRORS,
    val tutorRolloutAccess: TutorRolloutAccess =
        TutorRolloutAccess.ADVANCED_ONLY,
    val offerTutorTryAgain: Boolean = true
)

data class GameSettings(
    val matchLength: Int = 1,
    val crawford: Boolean = true,
    val jacoby: Boolean = false,
    val automaticDoubles: Int = 0,
    val beavers: Boolean = false,
    val boardTheme: BoardTheme = BoardTheme.OCEAN,
    val showPointNumbers: Boolean = true,
    val showPipCount: Boolean = true,
    val difficulty: Difficulty = Difficulty.INTERMEDIATE,

    val tutorModePreset: TutorModePreset = TutorModePreset.OFF,
    val tutorFeedbackThreshold: TutorFeedbackThreshold =
        TutorFeedbackThreshold.MISTAKES,
    val tutorAnnotationMode: TutorAnnotationMode =
        TutorAnnotationMode.USER_VS_BEST,
    val tutorEquityDetail: TutorEquityDetail =
        TutorEquityDetail.LOSS_ONLY,
    val cubeTutorMode: CubeTutorMode = CubeTutorMode.MAJOR_ERRORS,
    val tutorRolloutAccess: TutorRolloutAccess =
        TutorRolloutAccess.ADVANCED_ONLY,
    val offerTutorTryAgain: Boolean = true
)
