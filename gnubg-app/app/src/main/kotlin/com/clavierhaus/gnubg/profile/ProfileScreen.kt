package com.clavierhaus.gnubg.profile

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
fun ProfileScreen(
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
            BasicText(text = "profile")

            Column(
                verticalArrangement = Arrangement.spacedBy(16.dp)
            ) {
                ProfileSection(
                    title = "player",
                    description = "Name, side preference, and match identity defaults."
                )

                ProfileSection(
                    title = "new games",
                    description = "Default match length, cube use, and preferred setup choices."
                )

                ProfileSection(
                    title = "help during play",
                    description = "Future defaults for hints, explanations, warnings, and learning support."
                )

                ProfileSection(
                    title = "look and feel",
                    description = "Future board appearance, notation, accessibility, and display preferences."
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
private fun ProfileSection(
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
