package com.clavierhaus.gnubg.tutor

/**
 * Deterministic, engine-free description of a board position.
 *
 * Every field is computed by arithmetic over the gnubg flat board array
 * (IntArray(50): board[0..24] = opponent row, board[25..49] = player row,
 * index 24 within each row = the bar). No probabilities, no engine calls --
 * these are countable facts about the position. The coaching layer compares
 * two FeatureVectors (the move played vs the engine's best move) and explains
 * the notable differences in plain language.
 *
 * "player" = the side on roll (always the human, since getMatchBoard is swapped
 * to the human's perspective). "opponent" = the other side.
 */
data class FeatureVector(
    val positionType: PositionType,
    val playerPipCount: Int,
    val opponentPipCount: Int,
    val pipDifference: Int,            // opponent - player (positive = player ahead)
    val playerMadePoints: Int,         // points held by 2+ player checkers (1..24)
    val opponentMadePoints: Int,
    val playerAnchors: Int,            // player points made in opponent's home board (19..24)
    val opponentAnchors: Int,          // opponent points made in player's home board
    val playerLongestPrime: Int,       // longest run of consecutive made points (max 6)
    val opponentLongestPrime: Int,
    val playerHomeBoardStrength: Int,  // player points made in own home board (1..6)
    val opponentHomeBoardStrength: Int,
    val playerBlotCount: Int,          // player points with exactly 1 checker
    val opponentBlotCount: Int,
    val playerDirectShots: Int,        // opponent rolls (1..6) that can hit a player blot
    val playerIndirectShots: Int,      // opponent combination rolls (7..12) that can hit
    val opponentDirectShots: Int,
    val opponentIndirectShots: Int,
    val playerBarCheckers: Int,
    val opponentBarCheckers: Int,
    val playerBorneOff: Int,
    val opponentBorneOff: Int,
    val playerBackCheckers: Int,       // player checkers still in opponent's home (19..24)
    val opponentBackCheckers: Int
)

