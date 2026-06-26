package com.clavierhaus.gnubg.tutor

/**
 * Move encoding helpers and the analysis result.
 *
 * A candidate move is gnubg's anMove[8]: up to 4 (from, to) checker moves,
 * each a board point index, with -1 marking an unused slot. This is the same
 * encoding getLegalMoves and getCandidates emit.
 */
data class CandidateMove(
    val anMove: IntArray,   // 8 ints: (from0,to0, from1,to1, from2,to2, from3,to3)
    val equity: Float       // cubeless equity, higher = better for the player
) {
    override fun equals(other: Any?): Boolean {
        if (this === other) return true
        if (other !is CandidateMove) return false
        return anMove.contentEquals(other.anMove) && equity == other.equity
    }
    override fun hashCode(): Int = anMove.contentHashCode() * 31 + equity.hashCode()
}

/**
 * The full tutor analysis of a single human move. Produced after the move is
 * confirmed, before the engine responds. In Phase 2 this is logged only.
 */
data class TutorAnalysis(
    val playedEquity: Float,
    val bestEquity: Float,
    val equityLoss: Float,            // bestEquity - playedEquity (>= 0)
    val blunderLevel: BlunderLevel,
    val bestMove: CandidateMove,
    val candidates: List<CandidateMove>,
    val comparison: FeatureComparison
)

/**
 * Decodes getCandidates output and runs the deterministic tutor analysis.
 *
 * Pipeline (called after a human move is confirmed):
 *   1. decode the candidate list (ranked, best-first) from getCandidates
 *   2. identify the best move (candidates[0]) and the played move's equity
 *   3. extract features from the played-move board and the best-move board
 *   4. compare features; classify the equity loss
 *
 * The analyzer does not call the engine itself -- the caller (GameViewModel)
 * provides the raw getCandidates array and the two boards. This keeps the
 * analyzer pure and unit-testable.
 */
object TutorAnalyzer {

    /**
     * Decode the flat IntArray from Engine.getCandidates into ranked moves.
     * Layout: [0]=n, then per candidate i: [1+i*9 .. 1+i*9+7]=anMove,
     * [1+i*9+8]=Float.fromBits(equity).
     */
    fun decodeCandidates(raw: IntArray): List<CandidateMove> {
        if (raw.isEmpty()) return emptyList()
        val n = raw[0]
        if (n <= 0) return emptyList()
        val out = ArrayList<CandidateMove>(n)
        for (i in 0 until n) {
            val base = 1 + i * 9
            if (base + 8 >= raw.size) break
            val move = IntArray(8) { j -> raw[base + j] }
            val equity = Float.fromBits(raw[base + 8])
            out.add(CandidateMove(move, equity))
        }
        return out
    }

    /**
     * Run the analysis.
     *
     * @param candidatesRaw  raw output of Engine.getCandidates for the pre-move
     *                       board and the dice that were rolled
     * @param playedEquity   equity of the move the human actually played
     *                       (from Engine.evaluatePosition on the played board,
     *                       expressed cubeless from the player's perspective)
     * @param playedBoard    board after the human's move
     * @param bestBoard      board after the engine's best move
     * @param base           blunder threshold base
     *
     * Returns null if there were no candidates (e.g. no legal move / dance).
     */
    fun analyze(
        candidatesRaw: IntArray,
        playedEquity: Float,
        playedBoard: IntArray,
        bestBoard: IntArray,
        base: Float = BlunderThreshold.NORMAL.value
    ): TutorAnalysis? {
        val candidates = decodeCandidates(candidatesRaw)
        if (candidates.isEmpty()) return null

        val best = candidates.first()
        val bestEquity = best.equity
        val equityLoss = (bestEquity - playedEquity).coerceAtLeast(0f)
        val level = BlunderClassifier.classify(equityLoss, base)

        val playedFeatures = FeatureExtractor.extract(playedBoard)
        val bestFeatures = FeatureExtractor.extract(bestBoard)
        val comparison = FeatureExtractor.compare(playedFeatures, bestFeatures)

        return TutorAnalysis(
            playedEquity = playedEquity,
            bestEquity = bestEquity,
            equityLoss = equityLoss,
            blunderLevel = level,
            bestMove = best,
            candidates = candidates,
            comparison = comparison
        )
    }

    /** One-line logcat summary for Phase 2 verification. */
    fun summarize(a: TutorAnalysis): String {
        val top = a.comparison.notableDeltas.take(3).joinToString("; ") {
            "${it.feature} ${it.playedValue}->${it.bestValue}"
        }
        return "level=${a.blunderLevel} loss=%.4f best=%.4f played=%.4f | %s"
            .format(a.equityLoss, a.bestEquity, a.playedEquity, top.ifEmpty { "no notable deltas" })
    }
}

