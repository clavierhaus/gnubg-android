package com.clavierhaus.gnubg.options

import androidx.compose.runtime.Composable
import com.clavierhaus.gnubg.engine.GameSettings
import com.clavierhaus.gnubg.engine.GameViewModel

@Composable
fun OptionsModeScreen(
    settings: GameSettings,
    viewModel: GameViewModel,
    onBackToHub: () -> Unit
) {
    SettingsScreen(
        settings = settings,
        viewModel = viewModel,
        onDismiss = onBackToHub
    )
}
