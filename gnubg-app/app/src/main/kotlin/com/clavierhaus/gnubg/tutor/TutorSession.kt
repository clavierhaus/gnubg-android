package com.clavierhaus.gnubg.tutor

import com.clavierhaus.gnubg.engine.BoardState

enum class TutorSessionPhase {
    INTRO,
    READY,
    SHOWING_COACH_CARD,
    FINISHED
}

data class TutorSessionState(
    val phase: TutorSessionPhase = TutorSessionPhase.INTRO,
    val title: String = "Tutor Mode",
    val subtitle: String =
        "Separate learning flow. Shared infrastructure, different mode semantics.",
    val lessonTitle: String = "Prototype lesson",
    val lessonDescription: String =
        "This neutral session skeleton does not use Regular Play.",
    val boardState: BoardState = TutorBoardPreview.openingPosition(),
    val tutorUiState: TutorUiState = TutorUiState.Hidden
)

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
