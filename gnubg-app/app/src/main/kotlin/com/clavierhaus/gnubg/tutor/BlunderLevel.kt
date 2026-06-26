package com.clavierhaus.gnubg.tutor

/**
 * Severity of a move error, measured by equity loss versus the engine's best
 * move. Thresholds are multiples of a configurable base threshold.
 */
enum class BlunderLevel {
    NONE,
    INACCURACY,
    MISTAKE,
    BLUNDER,
    HUGE_BLUNDER
}

/**
 * Base equity-loss threshold presets. The classifier multiplies the base by
 * 0.25 / 0.5 / 1.0 / 3.0 to get the four severity bands.
 */
enum class BlunderThreshold(val value: Float) {
    LENIENT(0.15f),
    NORMAL(0.08f),
    STRICT(0.04f)
}

