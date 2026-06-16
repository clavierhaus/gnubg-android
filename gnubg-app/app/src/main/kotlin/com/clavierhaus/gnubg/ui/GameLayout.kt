package com.clavierhaus.gnubg.ui

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Settings
import androidx.compose.material3.Icon
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import com.clavierhaus.gnubg.engine.GamePhase
import com.clavierhaus.gnubg.engine.GameViewModel

@Composable
fun GameLayout(viewModel: GameViewModel) {
    var showSettings by remember { mutableStateOf(false) }
    val settings by viewModel.settings.collectAsStateWithLifecycle()
    val gameState by viewModel.gameState.collectAsStateWithLifecycle()
    val engineReady by viewModel.engineReady.collectAsStateWithLifecycle()

    if (showSettings) {
        SettingsScreen(
            settings = settings,
            viewModel = viewModel,
            onDismiss = { showSettings = false }
        )
    } else {
        Box(modifier = Modifier.fillMaxSize()) {
            Row(modifier = Modifier.fillMaxSize()) {
                // Player panel — left 25%
                Box(
                    modifier = Modifier
                        .fillMaxHeight()
                        .fillMaxWidth(0.25f)
                        .background(MaterialTheme.colorScheme.surfaceVariant),
                    contentAlignment = Alignment.Center
                ) {
                    Column(
                        horizontalAlignment = Alignment.CenterHorizontally,
                        verticalArrangement = Arrangement.spacedBy(12.dp)
                    ) {
                        when {
                            gameState.phase == GamePhase.GAME_OVER -> {
                                val resultText = when {
                                    gameState.winner == 0 && gameState.nPoints >= 3 -> "You win — Backgammon!"
                                    gameState.winner == 0 && gameState.nPoints >= 2 -> "You win — Gammon!"
                                    gameState.winner == 0 -> "You win"
                                    gameState.nPoints >= 3 -> "Engine wins — Backgammon"
                                    gameState.nPoints >= 2 -> "Engine wins — Gammon"
                                    else -> "Engine wins"
                                }
                                Text(resultText, color = Color.White, fontSize = 16.sp,
                                    fontWeight = FontWeight.Bold)
                                Spacer(modifier = Modifier.height(8.dp))
                                GameButton("New Game", Color(0xFF1565C0)) { viewModel.newGame() }
                            }
                            gameState.phase == GamePhase.ENGINE_THINKING -> {
                                Text("Thinking…", color = Color(0xFFB3C9F0), fontSize = 12.sp)
                            }
                            gameState.phase == GamePhase.WAITING_FOR_ROLL && gameState.turn == 0 -> {
                                Text("Tap dice to roll", color = Color(0xFFB3C9F0), fontSize = 11.sp)
                            }
                            gameState.phase == GamePhase.HUMAN_MOVING -> {
                                Text("Moving", color = Color(0xFFB3C9F0), fontSize = 12.sp)
                            }
                        }

                        Spacer(modifier = Modifier.height(8.dp))

                        Text(
                            text = when {
                                gameState.phase == GamePhase.WAITING_FOR_ROLL && gameState.turn == 0 -> "Your turn"
                                gameState.phase == GamePhase.WAITING_FOR_ROLL && gameState.turn == 1 -> "Engine's turn"
                                else -> ""
                            },
                            color = Color(0xFFB3C9F0),
                            fontSize = 11.sp
                        )
                    }
                }

                // Board — right 75%
                Box(
                    modifier = Modifier
                        .fillMaxHeight()
                        .fillMaxWidth()
                ) {
                    BackgammonBoard(settings, gameState, viewModel)
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

@Composable
private fun GameButton(
    label: String,
    color: Color,
    enabled: Boolean = true,
    onClick: () -> Unit
) {
    Box(
        modifier = Modifier
            .background(color, RoundedCornerShape(8.dp))
            .clickable(enabled = enabled) { onClick() }
            .padding(horizontal = 24.dp, vertical = 12.dp),
        contentAlignment = Alignment.Center
    ) {
        Text(
            label,
            color = if (enabled) Color.White else Color(0xFF888888),
            fontSize = 14.sp,
            fontWeight = FontWeight.Bold
        )
    }
}
