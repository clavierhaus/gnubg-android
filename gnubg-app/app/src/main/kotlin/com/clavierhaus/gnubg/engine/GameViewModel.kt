package com.clavierhaus.gnubg.engine

import android.app.Application
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch

enum class BoardTheme { OCEAN, CLASSIC, FOREST, SYSTEM }

enum class Difficulty(val ply: Int, val label: String, val subtitle: String) {
    BEGINNER(2,     "Beginner",     "2-ply evaluation"),
    INTERMEDIATE(3, "Intermediate", "3-ply evaluation"),
    ADVANCED(4,     "Advanced",     "4-ply evaluation"),
    EXPERT(0,       "Expert",       "Rollout-based")
}

data class GameSettings(
    val matchLength: Int          = 7,
    val crawford: Boolean         = true,
    val jacoby: Boolean           = false,
    val automaticDoubles: Int     = 0,
    val beavers: Boolean          = false,
    val boardTheme: BoardTheme    = BoardTheme.OCEAN,
    val showPointNumbers: Boolean = true,
    val showPipCount: Boolean     = true,
    val difficulty: Difficulty    = Difficulty.ADVANCED,
    val tutorMode: Boolean        = false,
    val hint: Boolean             = true,
    val showEquity: Boolean       = true,
    val showMWC: Boolean          = false,
    val thresholdDoubtful: Float  = 0.010f,
    val thresholdBad: Float       = 0.050f,
    val thresholdVeryBad: Float   = 0.100f
)

class GameViewModel(application: Application) : AndroidViewModel(application) {
    private val _settings = MutableStateFlow(GameSettings())
    val settings: StateFlow<GameSettings> = _settings.asStateFlow()

    init {
        // Load persisted theme on startup
        viewModelScope.launch {
            PreferencesManager.boardThemeFlow(application).collect { theme ->
                _settings.value = _settings.value.copy(boardTheme = theme)
            }
        }
    }

    fun setMatchLength(n: Int)           { _settings.value = _settings.value.copy(matchLength = n) }
    fun setCrawford(on: Boolean)         { _settings.value = _settings.value.copy(crawford = on) }
    fun setJacoby(on: Boolean)           { _settings.value = _settings.value.copy(jacoby = on) }
    fun setAutomaticDoubles(n: Int)      { _settings.value = _settings.value.copy(automaticDoubles = n) }
    fun setBeavers(on: Boolean)          { _settings.value = _settings.value.copy(beavers = on) }
    fun setShowPointNumbers(on: Boolean) { _settings.value = _settings.value.copy(showPointNumbers = on) }
    fun setShowPipCount(on: Boolean)     { _settings.value = _settings.value.copy(showPipCount = on) }
    fun setDifficulty(d: Difficulty)     { _settings.value = _settings.value.copy(difficulty = d) }
    fun setTutorMode(on: Boolean)        { _settings.value = _settings.value.copy(tutorMode = on) }
    fun setHint(on: Boolean)             { _settings.value = _settings.value.copy(hint = on) }
    fun setShowEquity(on: Boolean)       { _settings.value = _settings.value.copy(showEquity = on) }
    fun setShowMWC(on: Boolean)          { _settings.value = _settings.value.copy(showMWC = on) }
    fun setThresholdDoubtful(v: Float)   { _settings.value = _settings.value.copy(thresholdDoubtful = v) }
    fun setThresholdBad(v: Float)        { _settings.value = _settings.value.copy(thresholdBad = v) }
    fun setThresholdVeryBad(v: Float)    { _settings.value = _settings.value.copy(thresholdVeryBad = v) }

    fun setBoardTheme(t: BoardTheme) {
        _settings.value = _settings.value.copy(boardTheme = t)
        viewModelScope.launch {
            PreferencesManager.saveBoardTheme(getApplication(), t)
        }
    }
}
