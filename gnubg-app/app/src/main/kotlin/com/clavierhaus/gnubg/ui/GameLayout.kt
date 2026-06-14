package com.clavierhaus.gnubg.ui

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Settings
import androidx.compose.material3.Icon
import androidx.compose.material3.MaterialTheme
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import com.clavierhaus.gnubg.engine.GameViewModel

@Composable
fun GameLayout(viewModel: GameViewModel) {
    var showSettings by remember { mutableStateOf(false) }
    val settings by viewModel.settings.collectAsStateWithLifecycle()

    if (showSettings) {
        SettingsScreen(
            settings = settings,
            viewModel = viewModel,
            onDismiss = { showSettings = false }
        )
    } else {
        Box(modifier = Modifier.fillMaxSize()) {
            Row(modifier = Modifier.fillMaxSize()) {
                Box(
                    modifier = Modifier
                        .fillMaxHeight()
                        .fillMaxWidth(0.25f)
                        .background(MaterialTheme.colorScheme.surfaceVariant)
                )
                Box(
                    modifier = Modifier
                        .fillMaxHeight()
                        .fillMaxWidth()
                ) {
                    BackgammonBoard(settings)
                }
            }

            Icon(
                imageVector = Icons.Filled.Settings,
                contentDescription = "Settings",
                tint = Color(0xFFB3C9F0),
                modifier = Modifier
                    .align(Alignment.TopStart)
                    .padding(start = 16.dp, top = 12.dp)
                    .size(24.dp)
                    .clickable { showSettings = true }
            )
        }
    }
}
