package com.clavierhaus.gnubg.engine

import android.content.Context
import androidx.datastore.core.DataStore
import androidx.datastore.preferences.core.Preferences
import androidx.datastore.preferences.core.edit
import androidx.datastore.preferences.core.stringPreferencesKey
import androidx.datastore.preferences.preferencesDataStore
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.map

val Context.dataStore: DataStore<Preferences> by preferencesDataStore(name = "gnubg_prefs")

object PreferencesManager {
    private val BOARD_THEME_KEY = stringPreferencesKey("board_theme")

    fun boardThemeFlow(context: Context): Flow<BoardTheme> =
        context.dataStore.data.map { prefs ->
            val name = prefs[BOARD_THEME_KEY] ?: BoardTheme.OCEAN.name
            BoardTheme.valueOf(name)
        }

    suspend fun saveBoardTheme(context: Context, theme: BoardTheme) {
        context.dataStore.edit { prefs ->
            prefs[BOARD_THEME_KEY] = theme.name
        }
    }
}
