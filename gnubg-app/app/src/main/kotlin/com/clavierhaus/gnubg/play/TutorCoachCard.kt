package com.clavierhaus.gnubg.play

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.clavierhaus.gnubg.tutor.TutorHint
import com.clavierhaus.gnubg.tutor.TutorSeverity

@Composable
fun TutorCoachCard(
    hint: TutorHint,
    onDismiss: () -> Unit,
    onShowBestMove: () -> Unit,
    onTryAgain: () -> Unit,
    onMoreDetail: () -> Unit,
    modifier: Modifier = Modifier
) {
    Column(
        modifier = modifier
            .width(360.dp)
            .background(Color(0xEE082D6B), RoundedCornerShape(16.dp))
            .padding(horizontal = 16.dp, vertical = 14.dp),
        verticalArrangement = Arrangement.spacedBy(8.dp)
    ) {
        Text(
            text = severityLabel(hint.severity),
            color = Color(0xFFB3C9F0),
            fontSize = 12.sp,
            fontWeight = FontWeight.Bold
        )

        Text(
            text = hint.headline,
            color = Color.White,
            fontSize = 20.sp,
            fontWeight = FontWeight.Bold
        )

        hint.shortExplanation?.let { explanation ->
            Text(
                text = explanation,
                color = Color(0xFFE8F0FF),
                fontSize = 14.sp
            )
        }

        hint.measurableFacts.take(3).forEach { fact ->
            Text(
                text = "• $fact",
                color = Color(0xFFB3C9F0),
                fontSize = 12.sp
            )
        }

        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.End
        ) {
            if (hint.allowTryAgain) {
                TextButton(onClick = onTryAgain) {
                    Text("Try Again", color = Color.White)
                }
            }

            if (hint.allowShowBestMove) {
                TextButton(onClick = onShowBestMove) {
                    Text("Best Move", color = Color.White)
                }
            }

            if (hint.allowMoreDetail) {
                TextButton(onClick = onMoreDetail) {
                    Text("More", color = Color.White)
                }
            }

            Spacer(modifier = Modifier.width(4.dp))

            TextButton(onClick = onDismiss) {
                Text("Dismiss", color = Color.White)
            }
        }
    }
}

private fun severityLabel(severity: TutorSeverity): String = when (severity) {
    TutorSeverity.GOOD -> "GOOD"
    TutorSeverity.INACCURACY -> "INACCURACY"
    TutorSeverity.MISTAKE -> "MISTAKE"
    TutorSeverity.BLUNDER -> "BLUNDER"
}
