package com.clavierhaus.gnubg.engine

import android.content.Context
import androidx.datastore.core.DataStore
import androidx.datastore.preferences.core.Preferences
import androidx.datastore.preferences.core.edit
import androidx.datastore.preferences.core.longPreferencesKey
import androidx.datastore.preferences.preferencesDataStore
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.map

/** Own store file, so a future "reset statistics" clears tallies without
 *  touching settings (gnubg_prefs). */
val Context.tallyStore: DataStore<Preferences> by preferencesDataStore(name = "gnubg_tally")

/**
 * All-time tally (FOSS, issue-tracker request): per-difficulty counters --
 * your match wins/losses, and rolls / doubles / face-pip sums for both
 * sides. Bookkeeping only; the analytic career (gnubg's match analysis) is
 * a different feature and never touches this store.
 *
 * Ground rules (also stated in the issue reply):
 *  - decided matches only; an abandoned match counts nowhere
 *  - Coach sessions never count (the gym is not the record)
 *  - counting starts with the release that ships this -- nothing
 *    retroactive exists to mine
 *  - GNU's W/L is your mirror, computed at display time, never stored
 *
 * Roll counts arrive as the 8-int payload of gnubg_mobile_tally_rolls
 * (gnubg's own moverecords, walked once at match end). If its self-check
 * slot [7] is nonzero the roll numbers are NOT recorded -- LOUD log, W/L
 * still counted since it does not depend on the walk (G3 spirit: never
 * keep numbers a self-check doubts).
 *
 * Keys are per Difficulty.name -- appending enum entries is safe, same
 * convention as PreferencesManager.
 */
object MatchTally {

    data class LevelTally(
        val won: Long = 0,        // your match wins at this level
        val lost: Long = 0,       // your match losses at this level
        val rollsYou: Long = 0,
        val doublesYou: Long = 0,
        val pipsYou: Long = 0,    // face-value sum of your rolls
        val rollsGnu: Long = 0,
        val doublesGnu: Long = 0,
        val pipsGnu: Long = 0
    ) {
        val played: Long get() = won + lost
    }

    private fun key(level: Difficulty, field: String) =
        longPreferencesKey("tally_${level.name}_$field")

    /** One atomic write per decided match. [rolls] is the tallyRolls payload;
     *  pass null when the walk was unavailable -- W/L is still recorded. */
    suspend fun recordMatch(
        context: Context,
        level: Difficulty,
        humanWonMatch: Boolean,
        rolls: IntArray?
    ) {
        val rollsOk = rolls != null && rolls.size >= 8 && rolls[7] == 0
        if (rolls != null && !rollsOk) {
            android.util.Log.e("gnubg-tally",
                "LOUD: tallyRolls self-check failed (skipped=${rolls.getOrNull(7)}, " +
                "len=${rolls.size}) -- roll counts NOT recorded, W/L recorded")
        }
        context.tallyStore.edit { p ->
            fun bump(field: String, by: Long) {
                if (by != 0L) { val k = key(level, field); p[k] = (p[k] ?: 0L) + by }
            }
            bump(if (humanWonMatch) "won" else "lost", 1)
            if (rollsOk) {
                bump("rolls_you",   rolls!![0].toLong())
                bump("doubles_you", rolls[1].toLong())
                bump("pips_you",    rolls[2].toLong())
                bump("rolls_gnu",   rolls[3].toLong())
                bump("doubles_gnu", rolls[4].toLong())
                bump("pips_gnu",    rolls[5].toLong())
            }
        }
    }

    /** Levels with at least one decided match, for the statistics screen. */
    fun tallyFlow(context: Context): Flow<Map<Difficulty, LevelTally>> =
        context.tallyStore.data.map { p ->
            Difficulty.entries.mapNotNull { lvl ->
                val t = LevelTally(
                    won        = p[key(lvl, "won")] ?: 0L,
                    lost       = p[key(lvl, "lost")] ?: 0L,
                    rollsYou   = p[key(lvl, "rolls_you")] ?: 0L,
                    doublesYou = p[key(lvl, "doubles_you")] ?: 0L,
                    pipsYou    = p[key(lvl, "pips_you")] ?: 0L,
                    rollsGnu   = p[key(lvl, "rolls_gnu")] ?: 0L,
                    doublesGnu = p[key(lvl, "doubles_gnu")] ?: 0L,
                    pipsGnu    = p[key(lvl, "pips_gnu")] ?: 0L
                )
                if (t.played > 0 || t.rollsYou > 0) lvl to t else null
            }.toMap()
        }

    /** For a future "reset statistics" action; not wired to any UI yet. */
    suspend fun reset(context: Context) {
        context.tallyStore.edit { it.clear() }
    }
}
