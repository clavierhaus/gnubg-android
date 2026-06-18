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
    val selectedPointText: String = "Tap a point on the tutor board.",
    val boardState: BoardState = TutorBoardPreview.openingPosition(),
    val selectedPoint: Int? = null,
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

    fun selectPoint(
        state: TutorSessionState,
        point: Int
    ): TutorSessionState {
        if (point !in 1..24) return state

        val nextPoint = if (state.selectedPoint == point) null else point

        return state.copy(
            phase = TutorSessionPhase.READY,
            selectedPoint = nextPoint,
            selectedPointText = selectedPointText(nextPoint),
            tutorUiState = TutorUiState.Hidden
        )
    }

    private fun selectedPointText(point: Int?): String {
        if (point == null) return "Tap a point on the tutor board."

        val boardArea = when (point) {
            in 1..6 -> "your home board"
            in 7..12 -> "your outer board"
            in 13..18 -> "opponent outer board"
            else -> "opponent home board"
        }

        return "Point $point is in $boardArea. " +
            "Tutor Mode can now attach teaching text to board selection " +
            "without using Regular Play actions."
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
