package com.clavierhaus.gnubg.tutorui

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.clavierhaus.gnubg.play.TutorCoachCard
import com.clavierhaus.gnubg.tutor.TutorStaticPrototype
import com.clavierhaus.gnubg.tutor.TutorUiState

@Composable
fun TutorModeScreen(
    onBackToHub: () -> Unit
) {
    var tutorUiState by remember {
        mutableStateOf<TutorUiState>(TutorUiState.Hidden)
    }

    Box(
        modifier = Modifier
            .fillMaxSize()
            .background(Color(0xFF082D6B))
    ) {
        Text(
            text = "←",
            color = Color.White,
            fontSize = 24.sp,
            modifier = Modifier
                .align(Alignment.TopStart)
                .padding(start = 22.dp, top = 16.dp)
                .clickable { onBackToHub() }
        )

        Column(
            modifier = Modifier
                .align(Alignment.CenterStart)
                .padding(start = 56.dp),
            verticalArrangement = Arrangement.spacedBy(10.dp)
        ) {
            Text(
                text = "Tutor Mode",
                color = Color.White,
                fontSize = 30.sp,
                fontWeight = FontWeight.Bold
            )

            Text(
                text = "Separate learning flow. Shared infrastructure, " +
                    "different mode semantics.",
                color = Color(0xFFB3C9F0),
                fontSize = 15.sp
            )

            Spacer(modifier = Modifier.height(8.dp))

            Box(
                modifier = Modifier
                    .background(Color(0xFF6A4C93), RoundedCornerShape(9.dp))
                    .clickable {
                        tutorUiState = TutorStaticPrototype.demoCoachCard()
                    }
                    .padding(horizontal = 18.dp, vertical = 10.dp)
            ) {
                Text(
                    text = "Show Coach Card prototype",
                    color = Color.White,
                    fontSize = 14.sp,
                    fontWeight = FontWeight.Bold
                )
            }
        }

        when (val tutor = tutorUiState) {
            is TutorUiState.CoachCard -> TutorCoachCard(
                hint = tutor.hint,
                onDismiss = { tutorUiState = TutorUiState.Hidden },
                onShowBestMove = {},
                onTryAgain = {},
                onMoreDetail = {},
                modifier = Modifier
                    .align(Alignment.BottomEnd)
                    .padding(end = 32.dp, bottom = 28.dp)
            )

            else -> Unit
        }

        TextButton(
            onClick = onBackToHub,
            modifier = Modifier
                .align(Alignment.BottomStart)
                .padding(start = 36.dp, bottom = 24.dp)
        ) {
            Text("Back to hub", color = Color.White)
        }
    }
}
