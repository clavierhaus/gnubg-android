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
    val lesson: TutorLesson = TutorLessonCatalog.defaultLesson(),
    val currentStepIndex: Int = 0,
    val selectedPointText: String = "Tap a point on the tutor board.",
    val boardState: BoardState = TutorBoardPreview.openingPosition(),
    val selectedPointLesson: TutorPointLesson? = null,
    val selectedPoint: Int? = null,
    val tutorUiState: TutorUiState = TutorUiState.Hidden
) {
    val currentStep: TutorLessonStep?
        get() = lesson.steps.getOrNull(currentStepIndex)

    val stepCount: Int
        get() = lesson.steps.size

    val stepProgressText: String
        get() = if (stepCount == 0) {
            "No lesson steps"
        } else {
            "Step ${currentStepIndex + 1} of $stepCount"
        }
}

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
            phase = TutorSessionPhase.READY
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


    fun nextStep(
        state: TutorSessionState
    ): TutorSessionState {
        val lastIndex = state.lesson.steps.lastIndex
        if (lastIndex < 0) return state

        val nextIndex = (state.currentStepIndex + 1)
            .coerceAtMost(lastIndex)

        return state.copy(
            currentStepIndex = nextIndex,
            selectedPoint = null,
            selectedPointLesson = null,
            selectedPointText = TutorBoardLessonCatalog.DEFAULT_PROMPT,
            tutorUiState = TutorUiState.Hidden
        )
    }

    fun previousStep(
        state: TutorSessionState
    ): TutorSessionState {
        val previousIndex = (state.currentStepIndex - 1)
            .coerceAtLeast(0)

        return state.copy(
            currentStepIndex = previousIndex,
            selectedPoint = null,
            selectedPointLesson = null,
            selectedPointText = TutorBoardLessonCatalog.DEFAULT_PROMPT,
            tutorUiState = TutorUiState.Hidden
        )
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
