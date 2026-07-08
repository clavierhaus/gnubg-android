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

enum class MatchEquityTable(val displayName: String, val fileName: String) {
    // Bundled canonical gnubg MET files (assets/met/*.xml). Kazaross-XG2 is the
    // modern standard (equivalent to XG2's table); Zadeh is gnubg's built-in
    // default. fileName is resolved against the extracted met/ directory.
    KAZAROSS_XG2("Kazaross XG2", "Kazaross-XG2.xml"),
    ROCKWELL_KAZAROSS("Rockwell / Kazaross", "Rockwell-Kazaross.xml"),
    WOOLSEY("Kit Woolsey", "woolsey.xml"),
    JACOBS_TRICE("Jacobs & Trice", "jacobs.xml"),
    SNOWIE("Snowie 2.1", "snowie.xml"),
    GNUBG_11("GNUbg 11-point", "g11.xml"),
    MEC26("MEC 26", "mec26.xml"),
    ZADEH("Zadeh (default)", "zadeh.xml")
}

data class GameSettings(
    val matchLength: Int = 3,
    val cubeUse: Boolean = true,
    val metTable: MatchEquityTable = MatchEquityTable.KAZAROSS_XG2,
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
