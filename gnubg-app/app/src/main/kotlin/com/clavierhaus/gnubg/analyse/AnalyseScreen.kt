package com.clavierhaus.gnubg.analyse

import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
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
    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(32.dp),
        verticalArrangement = Arrangement.SpaceBetween
    ) {
        Column(
            verticalArrangement = Arrangement.spacedBy(24.dp)
        ) {
            BasicText(text = "analyse")

            Column(
                verticalArrangement = Arrangement.spacedBy(16.dp)
            ) {
                AnalyseSection(
                    title = "review position",
                    description = "Future entry point for examining a board position with GNUbg analysis."
                )

                AnalyseSection(
                    title = "set up board",
                    description = "Future location for manual position setup. Not a top-level Home Hub mode."
                )

                AnalyseSection(
                    title = "import or resume",
                    description = "Future area for loading saved matches, positions, or notation."
                )

                AnalyseSection(
                    title = "engine output",
                    description = "Future area for equity, rollout, cube, and move explanation views."
                )
            }
        }

        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.End,
            verticalAlignment = Alignment.CenterVertically
        ) {
            Spacer(modifier = Modifier.weight(1f))

            BasicText(
                text = "back",
                modifier = Modifier
                    .clickable(onClick = onBackToHub)
                    .padding(16.dp)
            )
        }
    }
}

@Composable
private fun AnalyseSection(
    title: String,
    description: String
) {
    Column(
        verticalArrangement = Arrangement.spacedBy(4.dp)
    ) {
        BasicText(text = title)
        BasicText(text = description)
    }
}
