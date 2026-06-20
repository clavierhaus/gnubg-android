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
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Button
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
import com.clavierhaus.gnubg.engine.GameSettings
import com.clavierhaus.gnubg.play.BackgammonBoard
import com.clavierhaus.gnubg.play.TutorCoachCard
import com.clavierhaus.gnubg.tutor.TutorSessionController
import com.clavierhaus.gnubg.tutor.TutorSessionState
import com.clavierhaus.gnubg.tutor.TutorBoardLessonCatalog
import com.clavierhaus.gnubg.tutor.TutorUiState

@Composable
fun TutorModeScreen(
    onBackToHub: () -> Unit
) {
    val controller = remember { TutorSessionController() }
    var sessionState by remember {
        mutableStateOf(TutorSessionState())
    }
    val boardActions = remember(controller) {
        TutorBoardActions(
            controller = controller,
            getState = { sessionState },
            setState = { sessionState = it }
        )
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

        Row(
            modifier = Modifier
                .fillMaxSize()
                .padding(start = 34.dp, top = 54.dp, end = 22.dp, bottom = 50.dp),
            horizontalArrangement = Arrangement.spacedBy(16.dp),
            verticalAlignment = Alignment.CenterVertically
        ) {
            Column(
                modifier = Modifier.width(250.dp),
                verticalArrangement = Arrangement.spacedBy(8.dp)
            ) {
                Text(
                    text = sessionState.lesson.title,
                    color = Color.White,
                    fontSize = 26.sp,
                    fontWeight = FontWeight.Bold
                )

                Text(
                    text = sessionState.lesson.subtitle,
                    color = Color(0xFFB3C9F0),
                    fontSize = 13.sp
                )

                Spacer(modifier = Modifier.height(8.dp))

                Text(
                    text = sessionState.currentStep?.title ?: sessionState.lesson.title,
                    color = Color.White,
                    fontSize = 18.sp,
                    fontWeight = FontWeight.Bold
                )

                Text(
                    text = sessionState.currentStep?.instruction ?: sessionState.lesson.subtitle,
                    color = Color(0xFFB3C9F0),
                    fontSize = 13.sp
                )

                Text(
                    text = sessionState.stepProgressText,
                    color = Color(0xFF8FAADC),
                    fontSize = 12.sp
                )

                Text(
                    text = "Interaction: ${sessionState.currentInteraction.name}",
                    color = Color(0xFF8FAADC),
                    fontSize = 11.sp
                )

                Row(
                    horizontalArrangement = Arrangement.spacedBy(8.dp),
                    modifier = Modifier.fillMaxWidth()
                ) {
                    Button(
                        onClick = {
                            sessionState =
                                controller.previousStep(sessionState)
                        },
                        enabled = sessionState.currentStepIndex > 0,
                        modifier = Modifier.weight(1f)
                    ) {
                        Text("Back")
                    }

                    Button(
                        onClick = {
                            sessionState =
                                controller.nextStep(sessionState)
                        },
                        enabled =
                            sessionState.currentStepIndex <
                                sessionState.stepCount - 1,
                        modifier = Modifier.weight(1f)
                    ) {
                        Text("Next")
                    }
                }

                val selectedLesson = sessionState.selectedPointLesson

                Text(
                    text = selectedLesson?.title
                        ?: TutorBoardLessonCatalog.DEFAULT_TITLE,
                    color = Color.White,
                    fontSize = 13.sp,
                    fontWeight = FontWeight.Bold
                )

                Spacer(modifier = Modifier.height(6.dp))

                Text(
                    text = selectedLesson?.body
                        ?: sessionState.selectedPointText,
                    color = Color(0xFFE8F0FF),
                    fontSize = 12.sp,
                    lineHeight = 16.sp
                )

                Box(
                    modifier = Modifier
                        .fillMaxWidth()
                        .background(Color(0xFF6A4C93), RoundedCornerShape(9.dp))
                        .clickable {
                            sessionState =
                                if (sessionState.phase.name == "INTRO") {
                                    controller.startPrototypeLesson(sessionState)
                                } else {
                                    controller.showPrototypeCoachCard(sessionState)
                                }
                        }
                        .padding(horizontal = 12.dp, vertical = 9.dp)
                ) {
                    Text(
                        text =
                            if (sessionState.phase.name == "INTRO") {
                                "Start Tutor prototype"
                            } else {
                                "Show Coach Card prototype"
                            },
                        color = Color.White,
                        fontSize = 12.sp,
                        fontWeight = FontWeight.Bold
                    )
                }
            }

            Box(
                modifier = Modifier
                    .fillMaxHeight()
                    .weight(1f)
                    .background(Color(0xFF061D46), RoundedCornerShape(16.dp))
                    .padding(10.dp)
            ) {
                BackgammonBoard(
                    settings = GameSettings(),
                    gameState = sessionState.boardState,
                    actions = boardActions,
                    highlightedPoints =
                        sessionState.selectedPoint?.let { setOf(it) }
                            ?: emptySet()
                )
            }
        }

        when (val tutor = sessionState.tutorUiState) {
            is TutorUiState.CoachCard -> TutorCoachCard(
                hint = tutor.hint,
                onDismiss = {
                    sessionState = controller.dismissCoachCard(sessionState)
                },
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
