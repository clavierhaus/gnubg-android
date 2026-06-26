package com.clavierhaus.gnubg.tutor

/**
 * The difference in a single feature between the played move and the best move.
 *
 * playedValue / bestValue are the feature's value on each resulting board.
 * delta = playedValue - bestValue. `notable` is true when the magnitude crosses
 * the feature's notability threshold (see FeatureExtractor.compare).
 */
data class FeatureDelta(
    val feature: String,
    val playedValue: Int,
    val bestValue: Int,
    val delta: Int,
    val notable: Boolean
)

/**
 * The full comparison between the board after the played move and the board
 * after the engine's best move, with the notable deltas pulled out and ranked.
 */
data class FeatureComparison(
    val playedFeatures: FeatureVector,
    val bestFeatures: FeatureVector,
    val deltas: List<FeatureDelta>,
    val notableDeltas: List<FeatureDelta>  // ranked, largest magnitude first
)

