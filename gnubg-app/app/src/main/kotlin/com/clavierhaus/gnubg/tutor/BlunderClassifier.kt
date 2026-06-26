package com.clavierhaus.gnubg.tutor

import kotlin.math.abs

/**
 * Classifies a move's equity loss into a BlunderLevel using a base threshold.
 *
 * equityLoss is best_equity - played_equity (>= 0; larger = worse move).
 * Bands are multiples of the base threshold:
 *   < 0.25x  -> NONE
 *   < 0.5x   -> INACCURACY
 *   < 1.0x   -> MISTAKE
 *   < 3.0x   -> BLUNDER
 *   >= 3.0x  -> HUGE_BLUNDER
 */
object BlunderClassifier {

    fun classify(equityLoss: Float, base: Float = BlunderThreshold.NORMAL.value): BlunderLevel {
        val loss = abs(equityLoss)
        return when {
            loss < 0.25f * base -> BlunderLevel.NONE
            loss < 0.5f * base  -> BlunderLevel.INACCURACY
            loss < 1.0f * base  -> BlunderLevel.MISTAKE
            loss < 3.0f * base  -> BlunderLevel.BLUNDER
            else                -> BlunderLevel.HUGE_BLUNDER
        }
    }
}

