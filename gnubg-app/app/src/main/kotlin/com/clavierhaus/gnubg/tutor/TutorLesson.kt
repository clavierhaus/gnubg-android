package com.clavierhaus.gnubg.tutor

/**
 * Stable identifier for a Tutor lesson.
 *
 * Lesson ids are neutral model ids, not Android route names.
 */
enum class TutorLessonId {
    BOARD_REGIONS
}

/**
 * A single neutral Tutor lesson step.
 *
 * This is deliberately data-shaped. Android UI may render it, but should
 * not decide lesson meaning from strings or point numbers.
 */
data class TutorLessonStep(
    val id: String,
    val title: String,
    val instruction: String
)

/**
 * Neutral Tutor lesson definition.
 *
 * Lessons are owned by the Tutor layer. Compose screens render the active
 * lesson and dispatch user intent back to the session controller.
 */
data class TutorLesson(
    val id: TutorLessonId,
    val title: String,
    val subtitle: String,
    val steps: List<TutorLessonStep>
) {
    val firstStep: TutorLessonStep?
        get() = steps.firstOrNull()
}

/**
 * Static catalogue for built-in Tutor lessons.
 *
 * 0.9.1 starts with a single board-regions lesson. Later milestones can
 * add lesson progression, prerequisites, and GNUbg-backed evaluations
 * without changing Android UI ownership.
 */
object TutorLessonCatalog {
    val BOARD_REGIONS = TutorLesson(
        id = TutorLessonId.BOARD_REGIONS,
        title = "Board Regions",
        subtitle = "Learn how the board is divided before studying moves.",
        steps = listOf(
            TutorLessonStep(
                id = "select-points",
                title = "Tap points to identify board regions",
                instruction = "Tap any point on the board. Tutor Mode will " +
                    "identify the region without starting a Regular Play move."
            ),
            TutorLessonStep(
                id = "compare-sides",
                title = "Compare your side and opponent side",
                instruction = "Use the selected-point explanation to compare " +
                    "your home and outer boards with the opponent's boards."
            )
        )
    )

    fun defaultLesson(): TutorLesson {
        return BOARD_REGIONS
    }
}
