package com.clavierhaus.gnubg.coach

import android.content.Context
import com.clavierhaus.gnubg.Engine
import org.json.JSONObject

/**
 * The Insight matcher (CORPUS_HARVEST_PLAN Phase G).
 *
 * Ports EXACTLY the scoring validated offline by tools/harvest/matcher_proto.py
 * (63/63 top-1 on the C.1 corpus): every signature term must pass ALL its
 * gates -- direction + min_abs on the delta, optional max_abs, optional
 * played_in/best_in VALUE ranges on the raw input, entry-level positionclass
 * constraints -- or the entry scores zero. A passing weighted term contributes
 * weight * min(|delta| / (3 * min_abs), 1). Up to [MAX_FIRE] entries clearing
 * zero fire, ranked by score, ties toward MORE terms then lexicographic id.
 *
 * The matcher does NO position interpretation: it reads gnubg's own inputs
 * (Engine.positionFeatures = CalculateHalfInputs both sides), pip counts and
 * position class for the played and best boards, subtracts, and sorts a JSON
 * table against the delta vector. No board[] is inspected here.
 *
 * Frame note: coach boards are mover-frame (anBoard[1] = player on roll), and
 * positionFeatures packs side 0 first -- so "me" is the SECOND block.
 */
class InsightMatcher(context: Context) {

    data class Insight(
        val id: String,
        val phrase: String,
        val category: String,       // race | board | threat
        val score: Float
    )

    private class Term(
        val term: String, val side: String, val direction: String,
        val minAbs: Float, val maxAbs: Float?, val weight: Float,
        val playedIn: Pair<Float, Float>?, val bestIn: Pair<Float, Float>?
    )

    private class Entry(
        val id: String, val phraseFlag: String?, val phrasePraise: String?,
        val category: String, val severity: Set<String>,
        val classPlayed: Int?, val classBest: Int?, val terms: List<Term>
    )

    private val entries: List<Entry>
    private val inputIndex: Map<String, Int>

    init {
        // eval.h:553 enum order -- the same table the pilot harness pins with
        // a _Static_assert against MORE_INPUTS.
        val names = listOf(
            "I_OFF1", "I_OFF2", "I_OFF3",
            "I_BREAK_CONTACT", "I_BACK_CHEQUER", "I_BACK_ANCHOR",
            "I_FORWARD_ANCHOR", "I_PIPLOSS", "I_P1", "I_P2", "I_BACKESCAPES",
            "I_ACONTAIN", "I_ACONTAIN2", "I_CONTAIN", "I_CONTAIN2",
            "I_MOBILITY", "I_MOMENT2", "I_ENTER", "I_ENTER2",
            "I_TIMING", "I_BACKBONE", "I_BACKG", "I_BACKG1",
            "I_FREEPIP", "I_BACKRESCAPES"
        )
        inputIndex = names.withIndex().associate { (i, n) -> n to i }

        val asset = context.assets.list("")?.let { files ->
            when {
                files.contains("insights_v0.json") -> "insights_v0.json"
                files.contains("insights_dev.json") -> "insights_dev.json"
                else -> null
            }
        }
        entries = if (asset == null) emptyList() else {
            val doc = JSONObject(
                context.assets.open(asset).bufferedReader().readText())
            val out = ArrayList<Entry>()
            val arr = doc.getJSONArray("entries")
            for (i in 0 until arr.length()) {
                val e = arr.getJSONObject(i)
                val sig = e.getJSONObject("signature")
                val terms = ArrayList<Term>()
                val ta = sig.getJSONArray("terms")
                for (j in 0 until ta.length()) {
                    val t = ta.getJSONObject(j)
                    terms.add(Term(
                        term = t.getString("term"),
                        side = t.optString("side", ""),
                        direction = t.getString("direction"),
                        minAbs = t.optDouble("min_abs", 0.0).toFloat(),
                        maxAbs = if (t.has("max_abs"))
                            t.getDouble("max_abs").toFloat() else null,
                        weight = t.optDouble("weight", 1.0).toFloat(),
                        playedIn = t.optJSONArray("played_in")?.let {
                            Pair(it.getDouble(0).toFloat(), it.getDouble(1).toFloat()) },
                        bestIn = t.optJSONArray("best_in")?.let {
                            Pair(it.getDouble(0).toFloat(), it.getDouble(1).toFloat()) }
                    ))
                }
                out.add(Entry(
                    id = e.getString("id"),
                    phraseFlag = e.optString("phrase_flag").takeIf {
                        it.isNotEmpty() && it != "null" },
                    phrasePraise = e.optString("phrase_praise").takeIf {
                        it.isNotEmpty() && it != "null" },
                    category = e.getString("category"),
                    severity = buildSet {
                        val sv = e.getJSONArray("severity_hint")
                        for (k in 0 until sv.length()) add(sv.getString(k))
                    },
                    classPlayed = if (sig.has("class_played"))
                        sig.getInt("class_played") else null,
                    classBest = if (sig.has("class_best"))
                        sig.getInt("class_best") else null,
                    terms = terms
                ))
            }
            out
        }
    }

    val available: Boolean get() = entries.isNotEmpty()

    private class Snap(board: IntArray) {
        val feat: FloatArray = Engine.positionFeatures(board)
        val pips: IntArray = Engine.pipCount(board)
        val clazz: Int = Engine.classifyPosition(board)
    }

    private fun value(s: Snap, t: Term): Float {
        if (t.term == "PipCount.opp") return s.pips[0].toFloat()  // side 0 = opp
        val i = inputIndex[t.term] ?: return 0f
        val n = inputIndex.size
        return if (t.side == "me") s.feat[n + i] else s.feat[i]
    }

    /** Both boards mover-frame board[50], as the coach glance provides. */
    fun match(played: IntArray, best: IntArray, skillWord: String): List<Insight> {
        if (entries.isEmpty()) return emptyList()
        val sp = Snap(played)
        val sb = Snap(best)
        val fired = ArrayList<Triple<Float, Int, Entry>>()
        for (e in entries) {
            if (skillWord.isNotEmpty() && skillWord !in e.severity) continue
            if (e.classPlayed != null && sp.clazz != e.classPlayed) continue
            if (e.classBest != null && sb.clazz != e.classBest) continue
            var total = 0f
            var pass = true
            for (t in e.terms) {
                val vp = value(sp, t)
                val vb = value(sb, t)
                val d = vb - vp
                when (t.direction) {
                    "up" -> if (d < t.minAbs) { pass = false; break }
                    "down" -> if (d > -t.minAbs) { pass = false; break }
                    // "any": context term, range/max gates only
                }
                if (t.maxAbs != null && kotlin.math.abs(d) > t.maxAbs) {
                    pass = false; break
                }
                if (t.playedIn != null &&
                    (vp < t.playedIn.first || vp > t.playedIn.second)) {
                    pass = false; break
                }
                if (t.bestIn != null &&
                    (vb < t.bestIn.first || vb > t.bestIn.second)) {
                    pass = false; break
                }
                if (t.weight > 0f) {
                    val mag = if (t.minAbs > 0f)
                        minOf(kotlin.math.abs(d) / (3f * t.minAbs), 1f) else 1f
                    total += t.weight * mag
                }
            }
            if (pass && total > 0f) fired.add(Triple(total, e.terms.size, e))
        }
        fired.sortWith(compareByDescending<Triple<Float, Int, Entry>> { it.first }
            .thenByDescending { it.second }
            .thenBy { it.third.id })
        return fired.take(MAX_FIRE).mapNotNull { (score, _, e) ->
            val phrase = e.phraseFlag ?: e.phrasePraise ?: return@mapNotNull null
            Insight(e.id, phrase, e.category, score)
        }
    }

    companion object {
        const val MAX_FIRE = 2
    }
}
