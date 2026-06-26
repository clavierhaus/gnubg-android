package com.clavierhaus.gnubg.tutor

/**
 * High-level classification of a backgammon position.
 *
 * Determined deterministically from the board by FeatureExtractor. Used to
 * select the scene-setting opener phrase and to weight which feature deltas
 * matter most in a given position.
 */
enum class PositionType {
    RACE,            // no contact: pure pip race
    CONTACT,         // generic contact, no specialised structure
    BLITZ,           // aggressive attack on a weak opponent home board
    HOLDING,         // player holds an anchor in opponent's home/outfield
    PRIME_VS_PRIME,  // both sides building/holding primes
    BACKGAME,        // player holds two or more anchors deep, playing for timing
    UNKNOWN
}

