package com.clavierhaus.gnubg.tutor

object TutorStaticPrototype {
    fun demoCoachCard(): TutorUiState {
        val hint = TutorHint(
            severity = TutorSeverity.MISTAKE,
            mainTheme = TutorTheme.SAFETY,
            headline = "Tutor Mode prototype",
            shortExplanation =
                "This card belongs to Tutor Mode, not Regular Play. " +
                    "Real GNUbg evaluation is not wired yet.",
            measurableFacts = listOf(
                "Separate Tutor Mode entry point",
                "Reusable Coach Card UI",
                "No Regular Play interruption"
            ),
            userMove = "prototype move",
            bestMove = "not evaluated yet",
            equityLoss = 0.000f,
            allowTryAgain = false,
            allowShowBestMove = false,
            allowMoreDetail = false
        )

        return TutorUiState.CoachCard(hint)
    }
}
