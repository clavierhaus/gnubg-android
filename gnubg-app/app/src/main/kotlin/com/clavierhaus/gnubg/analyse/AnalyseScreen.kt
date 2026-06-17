package com.clavierhaus.gnubg.analyse

import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.text.BasicText
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp

@Composable
fun AnalyseScreen(
    onBackToHub: () -> Unit
) {
    Box(
        modifier = Modifier
            .fillMaxSize()
            .padding(32.dp)
    ) {
        BasicText(
            text = "analyse",
            modifier = Modifier.align(Alignment.Center)
        )

        BasicText(
            text = "back",
            modifier = Modifier
                .align(Alignment.BottomEnd)
                .clickable(onClick = onBackToHub)
                .padding(16.dp)
        )
    }
}
