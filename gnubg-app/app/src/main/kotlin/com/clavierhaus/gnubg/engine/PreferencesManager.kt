package com.clavierhaus.gnubg.engine

import android.content.Context
import androidx.datastore.core.DataStore
import androidx.datastore.preferences.core.Preferences
import androidx.datastore.preferences.core.booleanPreferencesKey
import androidx.datastore.preferences.core.edit
import androidx.datastore.preferences.core.floatPreferencesKey
import androidx.datastore.preferences.core.intPreferencesKey
import androidx.datastore.preferences.core.stringPreferencesKey
import androidx.datastore.preferences.preferencesDataStore
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.map

val Context.dataStore: DataStore<Preferences> by preferencesDataStore(name = "gnubg_prefs")

/**
 * Persists the full GameSettings to DataStore. Rather than a key/flow per field,
 * the whole settings object is read and written together, so adding a setting
 * means adding two lines here (a key + its read/write), and the ViewModel can
 * auto-save the entire object whenever any setting changes.
 *
 * Enum fields are stored by .name and resolved back with a safe fallback to the
 * default, so a value written by an older/newer build (or a removed enum entry)
 * can never crash load -- it degrades to the default.
 */
object PreferencesManager {
    private val MATCH_LENGTH        = intPreferencesKey("match_length")
    private val CUBE_USE            = booleanPreferencesKey("cube_use")
    private val MET_TABLE           = stringPreferencesKey("met_table")
    private val CRAWFORD            = booleanPreferencesKey("crawford")
    private val JACOBY              = booleanPreferencesKey("jacoby")
    private val AUTOMATIC_DOUBLES   = intPreferencesKey("automatic_doubles")
    private val BEAVERS             = booleanPreferencesKey("beavers")
    private val BOARD_THEME         = stringPreferencesKey("board_theme")
    private val SHOW_POINT_NUMBERS  = booleanPreferencesKey("show_point_numbers")
    private val SHOW_PIP_COUNT      = booleanPreferencesKey("show_pip_count")
    private val DIFFICULTY          = stringPreferencesKey("difficulty")
    private val TUTOR_MODE          = booleanPreferencesKey("tutor_mode")
    private val HINT                = booleanPreferencesKey("hint")
    private val SHOW_EQUITY         = booleanPreferencesKey("show_equity")
    private val SHOW_MWC            = booleanPreferencesKey("show_mwc")
    private val THRESHOLD_DOUBTFUL  = floatPreferencesKey("threshold_doubtful")
    private val THRESHOLD_BAD       = floatPreferencesKey("threshold_bad")
    private val THRESHOLD_VERY_BAD  = floatPreferencesKey("threshold_very_bad")

    private inline fun <reified T : Enum<T>> parseEnum(name: String?, default: T): T =
        try { if (name == null) default else enumValueOf(name) } catch (e: Exception) { default }

    /** Emits the persisted settings, falling back to GameSettings() defaults per field. */
    fun settingsFlow(context: Context): Flow<GameSettings> =
        context.dataStore.data.map { p ->
            val d = GameSettings()
            GameSettings(
                matchLength      = p[MATCH_LENGTH]       ?: d.matchLength,
                cubeUse          = p[CUBE_USE]           ?: d.cubeUse,
                metTable         = parseEnum(p[MET_TABLE], d.metTable),
                crawford         = p[CRAWFORD]           ?: d.crawford,
                jacoby           = p[JACOBY]             ?: d.jacoby,
                automaticDoubles = p[AUTOMATIC_DOUBLES]  ?: d.automaticDoubles,
                beavers          = p[BEAVERS]            ?: d.beavers,
                boardTheme       = parseEnum(p[BOARD_THEME], d.boardTheme),
                showPointNumbers = p[SHOW_POINT_NUMBERS] ?: d.showPointNumbers,
                showPipCount     = p[SHOW_PIP_COUNT]     ?: d.showPipCount,
                difficulty       = parseEnum(p[DIFFICULTY], d.difficulty),
                tutorMode        = p[TUTOR_MODE]         ?: d.tutorMode,
                hint             = p[HINT]               ?: d.hint,
                showEquity       = p[SHOW_EQUITY]        ?: d.showEquity,
                showMWC          = p[SHOW_MWC]           ?: d.showMWC,
                thresholdDoubtful = p[THRESHOLD_DOUBTFUL] ?: d.thresholdDoubtful,
                thresholdBad      = p[THRESHOLD_BAD]      ?: d.thresholdBad,
                thresholdVeryBad  = p[THRESHOLD_VERY_BAD] ?: d.thresholdVeryBad
            )
        }

    suspend fun saveSettings(context: Context, s: GameSettings) {
        context.dataStore.edit { p ->
            p[MATCH_LENGTH]       = s.matchLength
            p[CUBE_USE]           = s.cubeUse
            p[MET_TABLE]          = s.metTable.name
            p[CRAWFORD]           = s.crawford
            p[JACOBY]             = s.jacoby
            p[AUTOMATIC_DOUBLES]  = s.automaticDoubles
            p[BEAVERS]            = s.beavers
            p[BOARD_THEME]        = s.boardTheme.name
            p[SHOW_POINT_NUMBERS] = s.showPointNumbers
            p[SHOW_PIP_COUNT]     = s.showPipCount
            p[DIFFICULTY]         = s.difficulty.name
            p[TUTOR_MODE]         = s.tutorMode
            p[HINT]               = s.hint
            p[SHOW_EQUITY]        = s.showEquity
            p[SHOW_MWC]           = s.showMWC
            p[THRESHOLD_DOUBTFUL] = s.thresholdDoubtful
            p[THRESHOLD_BAD]      = s.thresholdBad
            p[THRESHOLD_VERY_BAD] = s.thresholdVeryBad
        }
    }
}
