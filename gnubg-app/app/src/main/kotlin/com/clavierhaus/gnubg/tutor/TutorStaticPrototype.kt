package com.clavierhaus.gnubg.tutor

object TutorStaticPrototype {
    fun coachCardForCommittedMove(
        context: TutorMoveContext?
    ): TutorUiState {
        if (context == null) return TutorUiState.Hidden

        val hint = TutorHint(
            severity = TutorSeverity.MISTAKE,
            mainTheme = TutorTheme.SAFETY,
            headline = "Tutor prototype",
            shortExplanation =
                "This is a static Coach Card. Real GNUbg evaluation is not wired yet.",
            measurableFacts = listOf(
                "Captured move: ${context.userMove}",
                "Dice: ${context.preMove.originalDice.first}-" +
                    "${context.preMove.originalDice.second}",
                "Board snapshot captured before commit"
            ),
            userMove = context.userMove,
            bestMove = "not evaluated yet",
            equityLoss = 0.000f,
            allowTryAgain = false,
            allowShowBestMove = false,
            allowMoreDetail = false
        )

        return TutorUiState.CoachCard(hint)
    }
}
