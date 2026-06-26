package com.clavierhaus.gnubg.tutor

/**
 * Computes a FeatureVector from a gnubg flat board array, and compares two
 * boards (played vs best) into a FeatureComparison.
 *
 * Board convention (gnubg TanBoard, flattened to IntArray(50)):
 *   board[0..24]   = opponent's row.  board[0+p]  = opponent checkers, p=0..23
 *                    are the opponent's points 1..24; board[24] = opponent bar.
 *   board[25..49]  = player's row.    board[25+p] = player checkers, p=0..23
 *                    are the player's points 1..24; board[49] = player bar.
 *
 * Both rows count from each side's own ace point. The player's home board is
 * points 1..6 (board[25..30]); the player's outfield runs up to point 24.
 * A player checker on the player's point N (1..24) sits at board[25 + (N-1)].
 * Off-the-board counts are derived: 15 - (checkers on board + bar).
 *
 * All values are non-negative counts. No engine calls; pure arithmetic.
 */
object FeatureExtractor {

    private const val PLAYER_OFF_BASE = 25   // player row start
    private const val OPP_OFF_BASE = 0       // opponent row start
    private const val BAR = 24               // bar index within a row

    /** Player checkers on the player's own point n (1..24). */
    private fun playerAt(board: IntArray, n: Int): Int = board[PLAYER_OFF_BASE + (n - 1)]
    /** Opponent checkers on the opponent's own point n (1..24). */
    private fun oppAt(board: IntArray, n: Int): Int = board[OPP_OFF_BASE + (n - 1)]

    private fun playerBar(board: IntArray): Int = board[PLAYER_OFF_BASE + BAR]
    private fun oppBar(board: IntArray): Int = board[OPP_OFF_BASE + BAR]

    fun extract(board: IntArray): FeatureVector {
        val playerOnBoard = (1..24).sumOf { playerAt(board, it) } + playerBar(board)
        val oppOnBoard = (1..24).sumOf { oppAt(board, it) } + oppBar(board)
        val playerOff = (15 - playerOnBoard).coerceAtLeast(0)
        val oppOff = (15 - oppOnBoard).coerceAtLeast(0)

        // Pip counts. Player checker on point n is n pips from bearing off.
        // Player bar checker is 25 pips. Opponent symmetric on its own row.
        var playerPips = playerBar(board) * 25
        var oppPips = oppBar(board) * 25
        for (n in 1..24) {
            playerPips += playerAt(board, n) * n
            oppPips += oppAt(board, n) * n
        }

        val playerMade = (1..24).count { playerAt(board, it) >= 2 }
        val oppMade = (1..24).count { oppAt(board, it) >= 2 }

        // Player anchors: player points made in the opponent's home board.
        // From the player's perspective those are the player's points 19..24.
        val playerAnchors = (19..24).count { playerAt(board, it) >= 2 }
        val oppAnchors = (19..24).count { oppAt(board, it) >= 2 }

        val playerHome = (1..6).count { playerAt(board, it) >= 2 }
        val oppHome = (1..6).count { oppAt(board, it) >= 2 }

        val playerLongestPrime = longestPrime(board, ::playerAt)
        val oppLongestPrime = longestPrime(board, ::oppAt)

        val playerBlots = (1..24).count { playerAt(board, it) == 1 }
        val oppBlots = (1..24).count { oppAt(board, it) == 1 }

        // Back checkers: player checkers still deep in opponent territory (19..24).
        val playerBack = (19..24).sumOf { playerAt(board, it) } + playerBar(board)
        val oppBack = (19..24).sumOf { oppAt(board, it) } + oppBar(board)

        // Shots against the player: opponent is the hitter (opponent bar enters).
        val (pDirect, pIndirect) = shotsAgainst(board, ::playerAt, ::oppAt, oppBar(board))
        // Shots against the opponent: player is the hitter (player bar enters).
        val (oDirect, oIndirect) = shotsAgainst(board, ::oppAt, ::playerAt, playerBar(board))

        val posType = classifyPosition(
            board, playerBar(board), oppBar(board),
            playerBack, oppBack, oppHome, playerAnchors, oppAnchors,
            playerLongestPrime, oppLongestPrime
        )

        return FeatureVector(
            positionType = posType,
            playerPipCount = playerPips,
            opponentPipCount = oppPips,
            pipDifference = oppPips - playerPips,
            playerMadePoints = playerMade,
            opponentMadePoints = oppMade,
            playerAnchors = playerAnchors,
            opponentAnchors = oppAnchors,
            playerLongestPrime = playerLongestPrime,
            opponentLongestPrime = oppLongestPrime,
            playerHomeBoardStrength = playerHome,
            opponentHomeBoardStrength = oppHome,
            playerBlotCount = playerBlots,
            opponentBlotCount = oppBlots,
            playerDirectShots = pDirect,
            playerIndirectShots = pIndirect,
            opponentDirectShots = oDirect,
            opponentIndirectShots = oIndirect,
            playerBarCheckers = playerBar(board),
            opponentBarCheckers = oppBar(board),
            playerBorneOff = playerOff,
            opponentBorneOff = oppOff,
            playerBackCheckers = playerBack,
            opponentBackCheckers = oppBack
        )
    }

    /** Longest run of consecutive made points (2+ checkers), capped at 6. */
    private fun longestPrime(board: IntArray, at: (IntArray, Int) -> Int): Int {
        var best = 0
        var run = 0
        for (n in 1..24) {
            if (at(board, n) >= 2) {
                run++
                if (run > best) best = run
            } else {
                run = 0
            }
        }
        return best.coerceAtMost(6)
    }

    /**
     * Count direct (single-die, distance 1..6) and indirect (combination,
     * distance 7..12 via an open intermediate) shots the hitter has against
     * the victim's blots.
     *
     * "victim" owns the blots we check; "hitter" is the opponent who would roll
     * to hit them. Because the two sides count from opposite ends, a hitter
     * checker bearing down on a victim blot is measured by the gap between the
     * hitter's checkers (on the hitter's own coordinates) and the blot's mirror
     * position. We approximate using the standard board distance on the shared
     * 1..24 line where the hitter advances from high to low on the victim's
     * frame.
     *
     * This counts distinct dice pips that produce a hit (direct) and distinct
     * two-die sums 7..12 with a landable intermediate (indirect). It is a
     * count of threats, used only for relative coaching ("your move leaves more
     * shots"), not for exact probability.
     */
    private fun shotsAgainst(
        board: IntArray,
        victimAt: (IntArray, Int) -> Int,
        hitterAt: (IntArray, Int) -> Int,
        hitterBar: Int
    ): Pair<Int, Int> {
        // Victim blots on points 1..24 (victim coordinates).
        val blots = (1..24).filter { victimAt(board, it) == 1 }
        if (blots.isEmpty()) return 0 to 0

        // Hitter checkers, expressed on the victim's coordinate frame: a hitter
        // checker on the hitter's own point h sits opposite the victim's point
        // (25 - h). The hitter advances toward its own off, i.e. toward lower
        // victim-coordinates, so it threatens blots below it.
        val hitterPositions = mutableListOf<Int>()
        for (h in 1..24) if (hitterAt(board, h) > 0) hitterPositions.add(25 - h)
        // A hitter checker on the bar enters at the top of the victim's frame
        // (victim-coordinate 25), reaching points 24..19 at distance 1..6.
        if (hitterBar > 0) hitterPositions.add(25)

        val directDice = sortedSetOf<Int>()
        val indirectSums = sortedSetOf<Int>()

        for (blot in blots) {
            for (pos in hitterPositions) {
                // The hitter advances toward its own off, i.e. from higher
                // victim-coordinates down toward the blot. A hit needs the
                // hitter at a higher victim-coordinate than the blot, at
                // distance d = pos - blot.
                val d = pos - blot
                if (d in 1..6) {
                    directDice.add(d)
                } else if (d in 7..12) {
                    // indirect: some split a+b=d (a,b in 1..6) must land on an
                    // open intermediate (not a victim-made point of 2+).
                    for (a in 1..6) {
                        val b = d - a
                        if (b in 1..6) {
                            val mid = pos - a
                            if (mid in 1..24 && victimAt(board, mid) < 2) {
                                indirectSums.add(d)
                                break
                            }
                        }
                    }
                }
            }
        }
        return directDice.size to indirectSums.size
    }

    private fun classifyPosition(
        board: IntArray,
        playerBar: Int,
        oppBar: Int,
        playerBack: Int,
        oppBack: Int,
        oppHome: Int,
        playerAnchors: Int,
        oppAnchors: Int,
        playerPrime: Int,
        oppPrime: Int
    ): PositionType {
        val contact = hasContact(board)
        if (!contact) return PositionType.RACE

        // Backgame: player holds 2+ anchors in opponent's home board.
        if (playerAnchors >= 2) return PositionType.BACKGAME

        // Prime vs prime: both sides have a 4+ prime.
        if (playerPrime >= 4 && oppPrime >= 4) return PositionType.PRIME_VS_PRIME

        // Blitz: opponent has checkers back (on bar or deep) and player's home
        // board is strong (4+ points), i.e. player is attacking.
        if ((oppBar > 0 || oppBack >= 2) && oppHome >= 4) return PositionType.BLITZ

        // Holding: player holds an advanced anchor (points 20..23) in the
        // opponent's home board. The starting 24-point anchor alone is not a
        // holding game -- it is the normal opening contact position.
        val playerAdvancedAnchor = (20..23).any { playerAt(board, it) >= 2 }
        if (playerAdvancedAnchor) return PositionType.HOLDING

        return PositionType.CONTACT
    }

    /** Contact exists if any player checker is behind any opponent checker. */
    private fun hasContact(board: IntArray): Boolean {
        // Highest player point occupied (player coordinate).
        var playerMax = 0
        for (n in 24 downTo 1) if (playerAt(board, n) > 0) { playerMax = n; break }
        if (board[PLAYER_OFF_BASE + BAR] > 0) playerMax = 25
        // Highest opponent point occupied (opponent coordinate).
        var oppMax = 0
        for (n in 24 downTo 1) if (oppAt(board, n) > 0) { oppMax = n; break }
        if (board[OPP_OFF_BASE + BAR] > 0) oppMax = 25
        // Contact if the two leading checkers can still meet: player's highest
        // point (player frame) plus opponent's highest (opp frame) exceeds 25.
        return playerMax + oppMax > 25
    }

    // -- Comparison -----------------------------------------------------------

    /**
     * Notability thresholds per feature: the minimum absolute delta for a
     * difference to be flagged as notable (worth coaching about).
     */
    private val thresholds: Map<String, Int> = mapOf(
        "playerDirectShots" to 1,
        "opponentDirectShots" to 1,
        "playerBlotCount" to 1,
        "playerMadePoints" to 1,
        "playerHomeBoardStrength" to 1,
        "playerLongestPrime" to 1,
        "playerAnchors" to 1,
        "pipDifference" to 4,
        "playerBackCheckers" to 1,
        "playerBarCheckers" to 1
    )

    fun compare(played: FeatureVector, best: FeatureVector): FeatureComparison {
        val pairs: List<Triple<String, Int, Int>> = listOf(
            Triple("pipDifference", played.pipDifference, best.pipDifference),
            Triple("playerMadePoints", played.playerMadePoints, best.playerMadePoints),
            Triple("playerAnchors", played.playerAnchors, best.playerAnchors),
            Triple("playerLongestPrime", played.playerLongestPrime, best.playerLongestPrime),
            Triple("playerHomeBoardStrength", played.playerHomeBoardStrength, best.playerHomeBoardStrength),
            Triple("playerBlotCount", played.playerBlotCount, best.playerBlotCount),
            Triple("playerDirectShots", played.playerDirectShots, best.playerDirectShots),
            Triple("playerIndirectShots", played.playerIndirectShots, best.playerIndirectShots),
            Triple("opponentDirectShots", played.opponentDirectShots, best.opponentDirectShots),
            Triple("playerBarCheckers", played.playerBarCheckers, best.playerBarCheckers),
            Triple("playerBackCheckers", played.playerBackCheckers, best.playerBackCheckers)
        )

        val deltas = pairs.map { (name, p, b) ->
            val d = p - b
            val thr = thresholds[name] ?: Int.MAX_VALUE
            FeatureDelta(name, p, b, d, kotlin.math.abs(d) >= thr)
        }

        val notable = deltas
            .filter { it.notable }
            .sortedByDescending { kotlin.math.abs(it.delta) }

        return FeatureComparison(played, best, deltas, notable)
    }
}

