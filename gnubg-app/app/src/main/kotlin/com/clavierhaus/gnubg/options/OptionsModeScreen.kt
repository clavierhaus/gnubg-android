package com.clavierhaus.gnubg.options

import androidx.compose.runtime.Composable

@Composable
fun OptionsModeScreen(
    onBackToHub: () -> Unit
) {
    SettingsScreen(
        onBack = onBackToHub
    )
}
