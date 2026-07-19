package com.clavierhaus.gnubg.coach

import android.content.Context
import com.clavierhaus.gnubg.Engine
import org.json.JSONObject

/**
 * The fallback tier (docs/DELTA_NARRATOR_PLAYBOOK.md, binding): deterministic
 * sentences from gnubg's own deltas when a FLAGGED move matches no corpus
 * signature. Exact transliteration of tools/narrator/narrator_proto.py --
 * the proto is the reference; divergence is a defect. Subordinate to the
 * corpus (L3): the call site consults the matcher first.
 * Presentation taxonomy credited: yairwein/backgammon-teacher (MIT).
 */
class DeltaNarrator(context: Context) {

    private data class Rule(
        val id: String, val category: String, val term: String,
        val side: String, val direction: String, val notable: Float,
        val playedIn: Pair<Float, Float>?, val sentence: String
    )

    private val rules: List<Rule> = try {
        val txt = context.assets.open("narrator_rules_v0.json")
            .bufferedReader().use { it.readText() }
        val doc = JSONObject(txt)
        val arr = doc.getJSONArray("rules")
        buildList {
            for (i in 0 until arr.length()) {
                val r = arr.getJSONObject(i)
                add(Rule(
                    id = r.getString("id"),
                    category = r.getString("category"),
                    term = r.getString("term"),
                    side = r.optString("side", ""),
                    direction = r.getString("direction"),
                    notable = r.getDouble("notable").toFloat(),
                    playedIn = r.optJSONArray("played_in")?.let {
                        Pair(it.getDouble(0).toFloat(), it.getDouble(1).toFloat())
                    },
                    sentence = r.getString("sentence")
                ))
            }
        }
    } catch (e: Exception) {
        android.util.Log.e("gnubg-insight", "narrator: asset load failed: $e")
        emptyList()
    }

    init {
        android.util.Log.i("gnubg-insight", "narrator: loaded rules=" + rules.size)
    }

    val available: Boolean get() = rules.isNotEmpty()

    /** Same frame as InsightMatcher: me = feat[n+i], opp = feat[i],
     *  PipCount.opp = pips[0]. One source of frame truth per playbook L5. */
    fun narrate(played: IntArray, best: IntArray): List<InsightMatcher.Insight> {
        if (rules.isEmpty()) return emptyList()
        val fp = Engine.positionFeatures(played)
        val fb = Engine.positionFeatures(best)
        val pp = Engine.pipCount(played)
        val pb = Engine.pipCount(best)
        val n = fp.size / 2
        fun value(feat: FloatArray, pips: IntArray, side: String, term: String): Float {
            if (term == "PipCount.opp") return pips[0].toFloat()
            val i = InsightMatcher.INPUT_ORDER.indexOf(term)
            if (i < 0) return 0f
            return if (side == "me") feat[n + i] else feat[i]
        }
        data class Fired(val score: Float, val rule: Rule)
        val fired = ArrayList<Fired>()
        for (r in rules) {
            val vp = value(fp, pp, r.side, r.term)
            val vb = value(fb, pb, r.side, r.term)
            val d = vb - vp
            when (r.direction) {
                "up" -> if (d < r.notable) continue
                "down" -> if (d > -r.notable) continue
            }
            if (r.playedIn != null && (vp < r.playedIn.first || vp > r.playedIn.second)) continue
            fired.add(Fired(kotlin.math.abs(d) / r.notable, r))
        }
        val bestPerCat = HashMap<String, Fired>()
        for (f in fired) {
            val cur = bestPerCat[f.rule.category]
            if (cur == null || f.score > cur.score) bestPerCat[f.rule.category] = f
        }
        val ranked = bestPerCat.values.sortedByDescending { it.score }.take(2)
        android.util.Log.i("gnubg-insight",
            "narrator: consulted=" + rules.size + " candidates=" + fired.size +
            " narrated=" + ranked.size +
            (if (ranked.isNotEmpty()) " [" + ranked.joinToString(",") { it.rule.id } + "]" else ""))
        return ranked.map {
            InsightMatcher.Insight(it.rule.id, it.rule.sentence, it.rule.category, it.score)
        }
    }
}
