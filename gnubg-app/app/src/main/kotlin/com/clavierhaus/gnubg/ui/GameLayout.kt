package com.clavierhaus.gnubg.ui

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.safeDrawingPadding
import androidx.compose.material3.MaterialTheme
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color

@Composable
fun GameLayout() {
    Box(
        modifier = Modifier
            .fillMaxSize()
            .background(Color.Black)
            .safeDrawingPadding()
    ) {
        Row(modifier = Modifier.fillMaxSize()) {
            // Player panel placeholder — left 25%
            Box(
                modifier = Modifier
                    .fillMaxHeight()
                    .fillMaxWidth(0.25f)
                    .background(MaterialTheme.colorScheme.surfaceVariant)
            )
            // Board — right 75%
            Box(
                modifier = Modifier
                    .fillMaxHeight()
                    .fillMaxWidth()
            ) {
                BackgammonBoard()
            }
        }
    }
}
