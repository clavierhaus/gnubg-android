package com.clavierhaus.gnubg.tutor

/**
 * Neutral board-region concept used by Tutor Mode lessons.
 *
 * These regions are Tutor concepts, not Compose concepts. UI code may
 * render their labels, but should not own the point-to-region mapping.
 */
enum class TutorBoardRegion(
    val displayName: String
) {
    YOUR_HOME_BOARD("your home board"),
    YOUR_OUTER_BOARD("your outer board"),
    OPPONENT_OUTER_BOARD("opponent outer board"),
    OPPONENT_HOME_BOARD("opponent home board");

    companion object {
        fun fromPoint(point: Int): TutorBoardRegion? {
            return when (point) {
                in 1..6 -> YOUR_HOME_BOARD
                in 7..12 -> YOUR_OUTER_BOARD
                in 13..18 -> OPPONENT_OUTER_BOARD
                in 19..24 -> OPPONENT_HOME_BOARD
                else -> null
            }
        }
    }
}

/**
 * Small neutral lesson payload for a selected board point.
 *
 * This is intentionally data-shaped. Future lessons can replace the
 * generated text with curated lesson copy without touching Compose UI.
 */
data class TutorPointLesson(
    val point: Int,
    val region: TutorBoardRegion,
    val title: String,
    val body: String
)

/**
 * Current static board-point lesson catalogue.
 *
 * 0.9.1 only teaches board regions. Later milestones can add richer
 * concepts such as builders, anchors, blots, shots, and prime structure.
 */
object TutorBoardLessonCatalog {
    const val DEFAULT_PROMPT: String = "Tap a point on the tutor board."

    fun pointLesson(point: Int): TutorPointLesson? {
        val region = TutorBoardRegion.fromPoint(point) ?: return null

        return TutorPointLesson(
            point = point,
            region = region,
            title = "Point $point",
            body = "Point $point is in ${region.displayName}. " +
                "Tutor Mode can attach teaching text to board selection " +
                "without using Regular Play actions."
        )
    }

    fun selectedPointText(point: Int?): String {
        if (point == null) {
            return DEFAULT_PROMPT
        }

        return pointLesson(point)?.body ?: DEFAULT_PROMPT
    }
}
