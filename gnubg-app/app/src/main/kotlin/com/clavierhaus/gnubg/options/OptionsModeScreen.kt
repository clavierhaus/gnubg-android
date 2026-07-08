package com.clavierhaus.gnubg.options

import androidx.compose.runtime.Composable
import androidx.compose.runtime.CompositionLocalProvider
import com.clavierhaus.gnubg.engine.GameSettings
import com.clavierhaus.gnubg.engine.GameViewModel
import com.clavierhaus.gnubg.play.BoardPalettes
import com.clavierhaus.gnubg.play.LocalBoardPalette

@Composable
fun OptionsModeScreen(
    settings: GameSettings,
    viewModel: GameViewModel,
    onBackToHub: () -> Unit
) {
    // Provide the themed palette so SettingsScreen (and its chrome) picks up the
    // selected board theme when opened from the Home menu, not just in-game.
    val palette = BoardPalettes.from(settings.boardTheme)
    CompositionLocalProvider(LocalBoardPalette provides palette) {
        SettingsScreen(
            settings = settings,
            viewModel = viewModel,
            onDismiss = onBackToHub
        )
    }
}
