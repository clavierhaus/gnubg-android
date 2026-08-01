package com.clavierhaus.gnubg.clock

import android.content.Context
import androidx.datastore.preferences.core.edit
import androidx.datastore.preferences.core.stringPreferencesKey
import androidx.datastore.preferences.preferencesDataStore
import kotlinx.coroutines.flow.first

/**
 * The tournament match clock (Plus): competition-conforming, no free knobs.
 * Parameters are the federation standard verified from the sources
 * (USBGF/UKBGF; Galaxy follows the same convention): Bronstein/US delay of
 * 12 seconds per move -- the bank depletes only after the free seconds --
 * with a reserve of minutes-per-point x match length. one clock, one
 * convention: 2 min/pt. Delay, never increment: the
 * increment styles are documented to corrupt dice handling at the table.
 *
 * Provenance rule (maintainer, 2026-07-31): the clock's conditions are a
 * fact of the record. Every career entry states its clock mode, and a
 * time-decided match states that too -- nothing unlabeled is ever
 * cumulated, so togglability cannot distort the ledger.
 */
enum class ClockMode(val label: String, val minutesPerPoint: Int) {
    OFF("Off", 0),
    COMPETITION("Competition", 2);

    /** The value written into the signed career entry. */
    val ledgerName: String get() = name.lowercase()

    fun reserveMs(matchLength: Int): Long =
        minutesPerPoint.toLong() * matchLength * 60_000L
}

const val DELAY_MS: Long = 12_000L

/** One clock is active at a time (maintainer ruling): the UI shows the
 *  running side's reserve, colored by that player's chequer. */
data class ClockUiState(
    /** 0 = you, 1 = the engine, -1 = none running (paused / between games). */
    val activeSide: Int,
    val delayLeftMs: Long,
    val reserveYouMs: Long,
    val reserveGnuMs: Long,
    /** Set when a reserve reached zero: 0 = your time ran out, 1 = GNU's. */
    val timeoutSide: Int? = null
) {
    val activeReserveMs: Long
        get() = if (activeSide == 1) reserveGnuMs else reserveYouMs
}

fun formatClock(ms: Long): String {
    val total = (ms.coerceAtLeast(0L) + 999) / 1000
    return "%d:%02d".format(total / 60, total % 60)
}

private val Context.clockPrefs by preferencesDataStore(name = "cbg_clock")
private val MODE_KEY = stringPreferencesKey("clock_mode")

object ClockPrefs {
    suspend fun load(ctx: Context): ClockMode = runCatching {
        ClockMode.valueOf(ctx.clockPrefs.data.first()[MODE_KEY] ?: ClockMode.OFF.name)
    }.getOrDefault(ClockMode.OFF)

    suspend fun save(ctx: Context, mode: ClockMode) {
        runCatching { ctx.clockPrefs.edit { it[MODE_KEY] = mode.name } }
    }
}
