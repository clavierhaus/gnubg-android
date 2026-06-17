package com.clavierhaus.gnubg.hub

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.offset
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.text.BasicText
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp

@Composable
fun HomeHubScreen(
    onPlay: () -> Unit,
    onLearn: () -> Unit,
    onAnalyse: () -> Unit,
    onOptions: () -> Unit,
    onProfile: () -> Unit
) {
    Box(
        modifier = Modifier
            .fillMaxSize()
            .padding(32.dp)
    ) {
        BasicText(
            text = "gnu backgammon",
            modifier = Modifier.align(Alignment.TopStart)
        )

        // Reserved central visual field. Intentionally blank in this structural prototype.

        Canvas(
            modifier = Modifier
                .align(Alignment.CenterStart)
                .offset(x = 64.dp)
                .height(210.dp)
        ) {
            drawLine(
                color = Color.Black,
                start = Offset(0f, 0f),
                end = Offset(0f, size.height),
                strokeWidth = 1f
            )
        }

        Column(
            modifier = Modifier
                .align(Alignment.CenterStart)
                .offset(x = 96.dp)
        ) {
            HomeHubEntry("play", onPlay)
            Spacer(modifier = Modifier.height(22.dp))
            HomeHubEntry("learn", onLearn)
            Spacer(modifier = Modifier.height(22.dp))
            HomeHubEntry("analyse", onAnalyse)
            Spacer(modifier = Modifier.height(22.dp))
            HomeHubEntry("options", onOptions)
        }

        BasicText(
            text = "profile",
            modifier = Modifier
                .align(Alignment.BottomEnd)
                .clickable(onClick = onProfile)
                .padding(16.dp)
        )
    }
}

@Composable
private fun HomeHubEntry(
    label: String,
    onClick: () -> Unit
) {
    BasicText(
        text = label,
        modifier = Modifier
            .clickable(onClick = onClick)
            .padding(vertical = 4.dp)
    )
}
