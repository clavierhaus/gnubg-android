package com.clavierhaus.gnubg.engine

enum class BoardTheme { OCEAN, CLASSIC, FOREST, SYSTEM }

enum class Difficulty(val label: String, val subtitle: String, val settingIndex: Int) {
    // gnubg's own predefined levels (aecSettings / SETTINGS_*). 0-ply with
    // descending noise; "advanced" is gnubg's strongest *named* level.
    BEGINNER("Beginner", "Noise 0.060 (0-ply)", 0),
    CASUAL("Casual play", "Noise 0.050 (0-ply)", 1),
    INTERMEDIATE("Intermediate", "Noise 0.040 (0-ply)", 2),
    ADVANCED("Advanced", "Noise 0.015 (0-ply)", 3)
}

data class GameSettings(
    val matchLength: Int = 3,
    val cubeUse: Boolean = true,
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
