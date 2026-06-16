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
                // Left panel — fixed proportion of screen
                Box(
                    modifier = Modifier
                        .fillMaxHeight()
                        .weight(0.18f)
                        .background(MaterialTheme.colorScheme.surfaceVariant),
                    contentAlignment = Alignment.Center
                ) {
                    Column(
                        horizontalAlignment = Alignment.CenterHorizontally,
                        verticalArrangement = Arrangement.spacedBy(8.dp),
                        modifier = Modifier.padding(8.dp)
                    ) {
                        // Engine avatar + score
                        Box(
                            modifier = Modifier
                                .size(48.dp)
                                .background(Color(0xFF1565C0), RoundedCornerShape(24.dp)),
                            contentAlignment = Alignment.Center
                        ) { Text("GNU", color = Color.White, fontSize = 11.sp, fontWeight = FontWeight.Bold) }
                        Text("${gameState.engineScore}", color = Color.White, fontSize = 22.sp, fontWeight = FontWeight.Bold)
                        Text("vs", color = Color(0xFFB3C9F0), fontSize = 12.sp)
                        Text("${gameState.humanScore}", color = Color.White, fontSize = 22.sp, fontWeight = FontWeight.Bold)
                        // Human avatar
                        Box(
                            modifier = Modifier
                                .size(48.dp)
                                .background(Color(0xFF2E7D32), RoundedCornerShape(24.dp)),
                            contentAlignment = Alignment.Center
                        ) { Text("You", color = Color.White, fontSize = 11.sp, fontWeight = FontWeight.Bold) }

                        Spacer(modifier = Modifier.height(4.dp))

                        when {
                            gameState.phase == GamePhase.GAME_OVER -> {
                                val resultText = when {
                                    gameState.winner == 0 && gameState.nPoints >= 3 -> "You win\nBackgammon!"
                                    gameState.winner == 0 && gameState.nPoints >= 2 -> "You win\nGammon!"
                                    gameState.winner == 0 -> "You win"
                                    gameState.nPoints >= 3 -> "Engine wins\nBackgammon"
                                    gameState.nPoints >= 2 -> "Engine wins\nGammon"
                                    else -> "Engine wins"
                                }
                                Text(resultText, color = Color.White, fontSize = 18.sp,
                                    fontWeight = FontWeight.Bold,
                                    textAlign = androidx.compose.ui.text.style.TextAlign.Center)
                                Spacer(modifier = Modifier.height(12.dp))
                                GameButton("New Game", Color(0xFF1565C0)) { viewModel.newGame() }
                            }
                            gameState.phase == GamePhase.CUBE_OFFERED -> {
                                Text("Cube offered!", color = Color.White, fontSize = 16.sp,
                                    fontWeight = FontWeight.Bold)
                                Spacer(modifier = Modifier.height(8.dp))
                                GameButton("Accept", Color(0xFF2E7D32)) { viewModel.acceptDouble() }
                                Spacer(modifier = Modifier.height(6.dp))
                                GameButton("Drop", Color(0xFF8B1A1A)) { viewModel.dropDouble() }
                            }
                            gameState.phase == GamePhase.ENGINE_THINKING -> {
                                Text("Thinking…", color = Color(0xFFB3C9F0), fontSize = 18.sp)
                            }
                            gameState.phase == GamePhase.WAITING_FOR_ROLL && gameState.turn == 0 -> {
                                Text("Tap dice\nto roll", color = Color(0xFFB3C9F0), fontSize = 18.sp,
                                    textAlign = androidx.compose.ui.text.style.TextAlign.Center)
                            }
                            gameState.phase == GamePhase.HUMAN_MOVING -> {
                                Text("Moving", color = Color(0xFFB3C9F0), fontSize = 18.sp)
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
                            fontSize = 16.sp
                        )
                    }
                }

                // Board — remaining space
                Box(
                    modifier = Modifier
                        .fillMaxHeight()
                        .weight(0.82f)
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
fun GameButton(
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
            fontSize = 16.sp,
            fontWeight = FontWeight.Bold
        )
    }
}
