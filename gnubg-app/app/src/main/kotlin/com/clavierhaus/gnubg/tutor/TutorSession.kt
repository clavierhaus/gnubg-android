package com.clavierhaus.gnubg.tutor

import com.clavierhaus.gnubg.engine.BoardState

enum class TutorSessionPhase {
    INTRO,
    READY,
    SHOWING_COACH_CARD,
    FINISHED
}

/**
 * Neutral state for the separate Tutor Mode flow.
 *
 * This state is intentionally independent from Regular Play. Compose
 * renders it, but Tutor session transitions belong to
 * [TutorSessionController].
 */
data class TutorSessionState(
    val phase: TutorSessionPhase = TutorSessionPhase.INTRO,
    val title: String = "Tutor Mode",
    val subtitle: String =
        "Separate learning flow. Shared infrastructure, different mode semantics.",
    val lessonTitle: String = "Prototype lesson",
    val lessonDescription: String =
        "This neutral session skeleton does not use Regular Play.",
    val selectedPointText: String = "Tap a point on the tutor board.",
    val boardState: BoardState = TutorBoardPreview.openingPosition(),
    val selectedPointLesson: TutorPointLesson? = null,
    val selectedPoint: Int? = null,
    val tutorUiState: TutorUiState = TutorUiState.Hidden
)

/**
 * Neutral controller for Tutor Mode session transitions.
 *
 * This class owns Tutor flow semantics. Android UI code should dispatch
 * user intent here rather than embedding lesson behaviour in Compose.
 */
class TutorSessionController {
    fun startPrototypeLesson(
        state: TutorSessionState = TutorSessionState()
    ): TutorSessionState {
        return state.copy(
            phase = TutorSessionPhase.READY,
            lessonTitle = "Prototype Coach Card",
            lessonDescription =
                "This proves Tutor Mode has its own flow before GNUbg " +
                    "evaluation is wired."
        )
    }

    fun selectPoint(
        state: TutorSessionState,
        point: Int
    ): TutorSessionState {
        if (point !in 1..24) return state

        val nextPoint = if (state.selectedPoint == point) null else point
        val lesson = nextPoint?.let {
            TutorBoardLessonCatalog.pointLesson(it)
        }

        return state.copy(
            phase = TutorSessionPhase.READY,
            selectedPoint = nextPoint,
            selectedPointLesson = lesson,
            selectedPointText = selectedPointText(lesson),
            tutorUiState = TutorUiState.Hidden
        )
    }

    private fun selectedPointText(lesson: TutorPointLesson?): String {
        return lesson?.body ?: TutorBoardLessonCatalog.DEFAULT_PROMPT
    }

    fun showPrototypeCoachCard(
        state: TutorSessionState
    ): TutorSessionState {
        return state.copy(
            phase = TutorSessionPhase.SHOWING_COACH_CARD,
            tutorUiState = TutorStaticPrototype.demoCoachCard()
        )
    }

    fun dismissCoachCard(
        state: TutorSessionState
    ): TutorSessionState {
        return state.copy(
            phase = TutorSessionPhase.READY,
            tutorUiState = TutorUiState.Hidden
        )
    }

    fun finish(
        state: TutorSessionState
    ): TutorSessionState {
        return state.copy(
            phase = TutorSessionPhase.FINISHED,
            tutorUiState = TutorUiState.Hidden
        )
    }
}
