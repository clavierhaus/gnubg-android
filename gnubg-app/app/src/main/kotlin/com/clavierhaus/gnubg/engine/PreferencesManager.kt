package com.clavierhaus.gnubg.engine

import android.content.Context
import androidx.datastore.core.DataStore
import androidx.datastore.preferences.core.Preferences
import androidx.datastore.preferences.core.booleanPreferencesKey
import androidx.datastore.preferences.core.edit
import androidx.datastore.preferences.core.stringPreferencesKey
import androidx.datastore.preferences.preferencesDataStore
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.map

val Context.dataStore: DataStore<Preferences> by preferencesDataStore(name = "gnubg_prefs")

object PreferencesManager {
    private val BOARD_THEME_KEY = stringPreferencesKey("board_theme")

    private val TUTOR_MODE_PRESET_KEY =
        stringPreferencesKey("tutor_mode_preset")
    private val TUTOR_FEEDBACK_THRESHOLD_KEY =
        stringPreferencesKey("tutor_feedback_threshold")
    private val TUTOR_ANNOTATION_MODE_KEY =
        stringPreferencesKey("tutor_annotation_mode")
    private val TUTOR_EQUITY_DETAIL_KEY =
        stringPreferencesKey("tutor_equity_detail")
    private val CUBE_TUTOR_MODE_KEY =
        stringPreferencesKey("cube_tutor_mode")
    private val TUTOR_ROLLOUT_ACCESS_KEY =
        stringPreferencesKey("tutor_rollout_access")
    private val OFFER_TUTOR_TRY_AGAIN_KEY =
        booleanPreferencesKey("offer_tutor_try_again")

    fun boardThemeFlow(context: Context): Flow<BoardTheme> =
        context.dataStore.data.map { prefs ->
            enumOrDefault(prefs[BOARD_THEME_KEY], BoardTheme.OCEAN)
        }

    fun tutorPreferencesFlow(context: Context): Flow<TutorPreferences> =
        context.dataStore.data.map { prefs ->
            TutorPreferences(
                tutorModePreset = enumOrDefault(
                    prefs[TUTOR_MODE_PRESET_KEY],
                    TutorModePreset.OFF
                ),
                tutorFeedbackThreshold = enumOrDefault(
                    prefs[TUTOR_FEEDBACK_THRESHOLD_KEY],
                    TutorFeedbackThreshold.MISTAKES
                ),
                tutorAnnotationMode = enumOrDefault(
                    prefs[TUTOR_ANNOTATION_MODE_KEY],
                    TutorAnnotationMode.USER_VS_BEST
                ),
                tutorEquityDetail = enumOrDefault(
                    prefs[TUTOR_EQUITY_DETAIL_KEY],
                    TutorEquityDetail.LOSS_ONLY
                ),
                cubeTutorMode = enumOrDefault(
                    prefs[CUBE_TUTOR_MODE_KEY],
                    CubeTutorMode.MAJOR_ERRORS
                ),
                tutorRolloutAccess = enumOrDefault(
                    prefs[TUTOR_ROLLOUT_ACCESS_KEY],
                    TutorRolloutAccess.ADVANCED_ONLY
                ),
                offerTutorTryAgain =
                    prefs[OFFER_TUTOR_TRY_AGAIN_KEY] ?: true
            )
        }

    suspend fun saveBoardTheme(context: Context, theme: BoardTheme) {
        context.dataStore.edit { prefs ->
            prefs[BOARD_THEME_KEY] = theme.name
        }
    }

    suspend fun saveTutorModePreset(
        context: Context,
        value: TutorModePreset
    ) {
        context.dataStore.edit { prefs ->
            prefs[TUTOR_MODE_PRESET_KEY] = value.name
        }
    }

    suspend fun saveTutorFeedbackThreshold(
        context: Context,
        value: TutorFeedbackThreshold
    ) {
        context.dataStore.edit { prefs ->
            prefs[TUTOR_FEEDBACK_THRESHOLD_KEY] = value.name
        }
    }

    suspend fun saveTutorAnnotationMode(
        context: Context,
        value: TutorAnnotationMode
    ) {
        context.dataStore.edit { prefs ->
            prefs[TUTOR_ANNOTATION_MODE_KEY] = value.name
        }
    }

    suspend fun saveTutorEquityDetail(
        context: Context,
        value: TutorEquityDetail
    ) {
        context.dataStore.edit { prefs ->
            prefs[TUTOR_EQUITY_DETAIL_KEY] = value.name
        }
    }

    suspend fun saveCubeTutorMode(
        context: Context,
        value: CubeTutorMode
    ) {
        context.dataStore.edit { prefs ->
            prefs[CUBE_TUTOR_MODE_KEY] = value.name
        }
    }

    suspend fun saveTutorRolloutAccess(
        context: Context,
        value: TutorRolloutAccess
    ) {
        context.dataStore.edit { prefs ->
            prefs[TUTOR_ROLLOUT_ACCESS_KEY] = value.name
        }
    }

    suspend fun saveOfferTutorTryAgain(
        context: Context,
        value: Boolean
    ) {
        context.dataStore.edit { prefs ->
            prefs[OFFER_TUTOR_TRY_AGAIN_KEY] = value
        }
    }

    private inline fun <reified T : Enum<T>> enumOrDefault(
        name: String?,
        defaultValue: T
    ): T {
        return try {
            if (name == null) defaultValue else enumValueOf<T>(name)
        } catch (_: IllegalArgumentException) {
            defaultValue
        }
    }
}
