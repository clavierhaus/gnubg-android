package com.clavierhaus.gnubg.engine

enum class BoardTheme { OCEAN, CLASSIC, FOREST, SYSTEM }

enum class Difficulty(val label: String, val subtitle: String) {
    BEGINNER("Beginner", "Easy opponent"),
    INTERMEDIATE("Intermediate", "Moderate challenge"),
    ADVANCED("Advanced", "Strong opponent"),
    EXPERT("Expert", "World-class play")
}

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
    val tutorMode: Boolean = false,
    val hint: Boolean = false,
    val showEquity: Boolean = false,
    val showMWC: Boolean = false,
    val thresholdDoubtful: Float = 0.04f,
    val thresholdBad: Float = 0.08f,
    val thresholdVeryBad: Float = 0.16f
)
