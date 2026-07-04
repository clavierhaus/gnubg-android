package com.clavierhaus.gnubg.play

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
import com.clavierhaus.gnubg.engine.Difficulty
import com.clavierhaus.gnubg.engine.GamePhase
import com.clavierhaus.gnubg.engine.GameViewModel
import com.clavierhaus.gnubg.options.SettingsScreen
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.TextButton

@Composable
fun GameLayout(
    viewModel: GameViewModel,
    onReturnToHub: (() -> Unit)? = null,
    tutorMode: Boolean = false
) {
    var showSettings by remember { mutableStateOf(false) }
    var pendingLifecycleAction by remember { mutableStateOf<PlayLifecycleAction?>(null) }
    val settings by viewModel.settings.collectAsStateWithLifecycle()
    val gameState by viewModel.gameState.collectAsStateWithLifecycle()
    val engineReady by viewModel.engineReady.collectAsStateWithLifecycle()
    val showMatchSetup by viewModel.showMatchSetup.collectAsStateWithLifecycle()

    if (showSettings) {
        SettingsScreen(
            settings = settings,
            viewModel = viewModel,
            onDismiss = { showSettings = false }
        )
    } else if (showMatchSetup) {
        MatchSetupScreen(
            tutorMode = tutorMode,
            selectedLength = settings.matchLength,
            selectedDifficulty = settings.difficulty,
            engineReady = engineReady,
            onSelectLength = { viewModel.setMatchLength(it) },
            onSelectDifficulty = { viewModel.setDifficulty(it) },
            onStart = { viewModel.startMatch(if (tutorMode) 1 else settings.matchLength) },
            onSettings = { showSettings = true }
        )
    } else {
        Box(modifier = Modifier.fillMaxSize()) {
            Row(modifier = Modifier.fillMaxSize()) {
                // Left panel -- fixed proportion of screen
                BoxWithConstraints(
                    modifier = Modifier
                        .fillMaxHeight()
                        .weight(0.18f)
                        .background(MaterialTheme.colorScheme.surfaceVariant),
                    contentAlignment = Alignment.Center
                ) {
                    // Avatar diameter scales with panel width so it adapts across
                    // devices (phone/tablet) like the board checkers do, rather than
                    // a fixed dp. Capped so it never dominates a very wide panel.
                    val avatarSize = (maxWidth * 0.29f).coerceIn(20.dp, 45.dp)
                    Column(
                        horizontalAlignment = Alignment.CenterHorizontally,
                        verticalArrangement = Arrangement.SpaceBetween,
                        modifier = Modifier
                            .fillMaxSize()
                            .padding(start = 8.dp, end = 8.dp, top = 44.dp, bottom = 8.dp)
                    ) {
                        // Top group: scoreboard + divider + phase content
                        Column(
                            horizontalAlignment = Alignment.CenterHorizontally,
                            verticalArrangement = Arrangement.spacedBy(8.dp),
                            modifier = Modifier.fillMaxWidth()
                        ) {
                        // Single-row scoreboard: GNU score .... You score
                        Row(
                            verticalAlignment = Alignment.CenterVertically,
                            horizontalArrangement = Arrangement.spacedBy(6.dp),
                            modifier = Modifier.fillMaxWidth()
                        ) {
                            Box(
                                modifier = Modifier
                                    .size(avatarSize)
                                    .background(Color(0xFF1565C0), RoundedCornerShape(avatarSize / 2)),
                                contentAlignment = Alignment.Center
                            ) { Text("GNU", color = Color.White, fontSize = 9.sp, fontWeight = FontWeight.Bold) }
                            Text("${gameState.engineScore}", color = Color.White, fontSize = 20.sp, fontWeight = FontWeight.Bold)
                            Spacer(modifier = Modifier.weight(1f))
                            Box(
                                modifier = Modifier
                                    .size(avatarSize)
                                    .background(Color(0xFF2E7D32), RoundedCornerShape(avatarSize / 2)),
                                contentAlignment = Alignment.Center
                            ) { Text("You", color = Color.White, fontSize = 9.sp, fontWeight = FontWeight.Bold) }
                            Text("${gameState.humanScore}", color = Color.White, fontSize = 20.sp, fontWeight = FontWeight.Bold)
                        }

                        // Thin blue divider below the scoreboard; space below is reserved
                        // for future match-context / tutor UI.
                        Box(
                            modifier = Modifier
                                .fillMaxWidth()
                                .height(1.dp)
                                .background(Color(0xFF2E5A9E))
                        )

                        when {
                            gameState.phase == GamePhase.GAME_OVER -> {
                                val humanWonMatch =
                                    gameState.matchLength > 1 &&
                                    gameState.humanScore >= gameState.matchLength
                                val engineWonMatch =
                                    gameState.matchLength > 1 &&
                                    gameState.engineScore >= gameState.matchLength
                                val matchInProgress =
                                    gameState.matchLength > 1 &&
                                    !humanWonMatch &&
                                    !engineWonMatch

                                if (matchInProgress) {
                                    androidx.compose.runtime.LaunchedEffect(
                                        gameState.humanScore,
                                        gameState.engineScore,
                                        gameState.matchLength
                                    ) {
                                        viewModel.newGame()
                                    }
                                    Text(
                                        "Continuing match...",
                                        color = Color.White,
                                        fontSize = 18.sp,
                                        fontWeight = FontWeight.Bold,
                                        textAlign = androidx.compose.ui.text.style.TextAlign.Center
                                    )
                                } else {
                                    val resultText = when {
                                        humanWonMatch -> "You win\nthe match!"
                                        engineWonMatch -> "Engine wins\nthe match"
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
                                    GameButton("New Match", Color(0xFF1565C0)) { viewModel.newGame() }
                                    if (onReturnToHub != null) {
                                        Spacer(modifier = Modifier.height(6.dp))
                                        GameButton("Exit", Color(0xFF243B68)) { onReturnToHub() }
                                    }
                                }
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
                                Text("Thinking...", color = Color(0xFFB3C9F0), fontSize = 18.sp)
                            }
                            gameState.phase == GamePhase.WAITING_FOR_ROLL && gameState.turn == 0 -> {
                                // The roll action is shown directly on the board surface.
                            }
                        }
                        }

                        if (tutorMode) {
                            TutorAnalysisPanel(gameState.tutorAnalysis, gameState.analysisDetail)
                        } else {
                            PlayLifecyclePanel(
                                onResign = { pendingLifecycleAction = PlayLifecycleAction.RESIGN },
                                onNewGame = { pendingLifecycleAction = PlayLifecycleAction.NEW_GAME },
                                onNewMatch = { pendingLifecycleAction = PlayLifecycleAction.NEW_MATCH },
                                onReturnHome = onReturnToHub
                            )
                        }
                    }
                }

                // Board -- remaining space
                Box(
                    modifier = Modifier
                        .fillMaxHeight()
                        .weight(0.82f)
                ) {
                    BackgammonBoard(settings, gameState, viewModel, tutorMode)
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

            PlayLifecycleConfirmationDialog(
                action = pendingLifecycleAction,
                onDismiss = { pendingLifecycleAction = null },
                onConfirmResignNormal = {
                    pendingLifecycleAction = null
                    viewModel.commandResign("n")
                },
                onConfirmResignGammon = {
                    pendingLifecycleAction = null
                    viewModel.commandResign("g")
                },
                onConfirmResignBackgammon = {
                    pendingLifecycleAction = null
                    viewModel.commandResign("b")
                },
                onConfirmNewGame = {
                    pendingLifecycleAction = null
                    viewModel.commandNewGame()
                },
                onConfirmNewMatch = {
                    pendingLifecycleAction = null
                    viewModel.commandNewMatch(settings.matchLength)
                }
            )
        }
    }
}


private enum class PlayLifecycleAction {
    RESIGN,
    NEW_GAME,
    NEW_MATCH
}

@Composable
private fun PlayLifecyclePanel(
    onResign: () -> Unit,
    onNewGame: () -> Unit,
    onNewMatch: () -> Unit,
    onReturnHome: (() -> Unit)?
) {
    Column(
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.spacedBy(6.dp)
    ) {
        Box(
            modifier = Modifier
                .width(144.dp)
                .height(1.dp)
                .background(Color(0xFF315A9A))
        )

        Row(horizontalArrangement = Arrangement.spacedBy(6.dp)) {
            LifecycleButton("Resign", Color(0xFF8B1A1A), onResign)
            LifecycleButton("New game", Color(0xFF1565C0), onNewGame)
        }

        Row(horizontalArrangement = Arrangement.spacedBy(6.dp)) {
            LifecycleButton("New match", Color(0xFF0D47A1), onNewMatch)
            LifecycleButton(
                label = "Home",
                color = Color(0xFF243B68),
                onClick = { onReturnHome?.invoke() },
                enabled = onReturnHome != null
            )
        }
    }
}

@Composable
private fun LifecycleButton(
    label: String,
    color: Color,
    onClick: () -> Unit,
    enabled: Boolean = true
) {
    Box(
        modifier = Modifier
            .width(68.dp)
            .background(
                if (enabled) color else Color(0xFF243B68),
                RoundedCornerShape(7.dp)
            )
            .clickable(enabled = enabled, onClick = onClick)
            .padding(vertical = 6.dp),
        contentAlignment = Alignment.Center
    ) {
        Text(
            text = label,
            color = if (enabled) Color.White else Color(0xFF8FA8D0),
            fontSize = 10.sp,
            fontWeight = FontWeight.Bold
        )
    }
}

@Composable
private fun PlayLifecycleConfirmationDialog(
    action: PlayLifecycleAction?,
    onDismiss: () -> Unit,
    onConfirmResignNormal: () -> Unit,
    onConfirmResignGammon: () -> Unit,
    onConfirmResignBackgammon: () -> Unit,
    onConfirmNewGame: () -> Unit,
    onConfirmNewMatch: () -> Unit
) {
    when (action) {
        PlayLifecycleAction.RESIGN -> {
            AlertDialog(
                onDismissRequest = onDismiss,
                title = { Text("Resign game?") },
                text = { Text("Choose the resignation value to offer to GNU Backgammon.") },
                confirmButton = {
                    TextButton(onClick = onConfirmResignNormal) {
                        Text("Normal")
                    }
                },
                dismissButton = {
                    Row {
                        TextButton(onClick = onConfirmResignGammon) {
                            Text("Gammon")
                        }
                        TextButton(onClick = onConfirmResignBackgammon) {
                            Text("Backgammon")
                        }
                        TextButton(onClick = onDismiss) {
                            Text("Cancel")
                        }
                    }
                }
            )
        }

        PlayLifecycleAction.NEW_GAME -> {
            AlertDialog(
                onDismissRequest = onDismiss,
                title = { Text("Start new game?") },
                text = { Text("This calls GNU Backgammon's new-game command for the current match/session.") },
                confirmButton = {
                    TextButton(onClick = onConfirmNewGame) {
                        Text("New game")
                    }
                },
                dismissButton = {
                    TextButton(onClick = onDismiss) {
                        Text("Cancel")
                    }
                }
            )
        }

        PlayLifecycleAction.NEW_MATCH -> {
            AlertDialog(
                onDismissRequest = onDismiss,
                title = { Text("Start new match?") },
                text = { Text("This calls GNU Backgammon's new-match command using the configured match length.") },
                confirmButton = {
                    TextButton(onClick = onConfirmNewMatch) {
                        Text("New match")
                    }
                },
                dismissButton = {
                    TextButton(onClick = onDismiss) {
                        Text("Cancel")
                    }
                }
            )
        }

        null -> Unit
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


@Composable
private fun MatchSetupScreen(
    tutorMode: Boolean,
    selectedLength: Int,
    selectedDifficulty: Difficulty,
    engineReady: Boolean,
    onSelectLength: (Int) -> Unit,
    onSelectDifficulty: (Difficulty) -> Unit,
    onStart: () -> Unit,
    onSettings: () -> Unit
) {
    Box(
        modifier = Modifier
            .fillMaxSize()
            .background(Color(0xFF082D6B)),
        contentAlignment = Alignment.Center
    ) {
        Column(
            horizontalAlignment = Alignment.CenterHorizontally,
            verticalArrangement = Arrangement.spacedBy(18.dp),
            modifier = Modifier.padding(24.dp)
        ) {
            Text(
                if (tutorMode) "GNU Backgammon Live Game Analysis"
                else "GNU Backgammon Tournament Match",
                color = Color.White,
                fontSize = 30.sp,
                fontWeight = FontWeight.Bold
            )

            Text(
                "Opponent strength",
                color = Color(0xFFB3C9F0),
                fontSize = 18.sp
            )

            Row(horizontalArrangement = Arrangement.spacedBy(12.dp)) {
                listOf(
                    Difficulty.BEGINNER to "Beginner",
                    Difficulty.CASUAL to "Casual play",
                    Difficulty.INTERMEDIATE to "Intermediate",
                    Difficulty.ADVANCED to "Advanced"
                ).forEach { (difficulty, label) ->
                    val selected = selectedDifficulty == difficulty
                    GameButton(
                        label = label,
                        color = if (selected) Color(0xFF1976D2) else Color(0xFF0D47A1),
                        enabled = engineReady
                    ) {
                        onSelectDifficulty(difficulty)
                    }
                }
            }

            Spacer(modifier = Modifier.height(6.dp))

            if (!tutorMode) {
                Text(
                    "Match length",
                    color = Color(0xFFB3C9F0),
                    fontSize = 18.sp
                )

                Row(horizontalArrangement = Arrangement.spacedBy(12.dp)) {
                    listOf(1, 3, 5, 7).forEach { n ->
                        val selected = selectedLength == n
                        GameButton(
                            label = "$n",
                            color = if (selected) Color(0xFF1976D2) else Color(0xFF0D47A1),
                            enabled = engineReady
                        ) {
                            onSelectLength(n)
                        }
                    }
                }
            }

            Spacer(modifier = Modifier.height(4.dp))

            GameButton(
                label = if (engineReady) "Start Match" else "Loading engine...",
                color = Color(0xFF2E7D32),
                enabled = engineReady
            ) {
                onStart()
            }

            GameButton(
                label = "Settings",
                color = Color(0xFF1565C0),
                enabled = true
            ) {
                onSettings()
            }
        }
    }
}

@Composable
private fun TutorAnalysisPanel(
    analysis: com.clavierhaus.gnubg.engine.TutorAnalysis?,
    detail: com.clavierhaus.gnubg.engine.MoveAnalysisDetail? = null
) {
    Column(
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.spacedBy(6.dp),
        modifier = Modifier.fillMaxWidth().padding(8.dp)
    ) {
        Box(
            modifier = Modifier
                .fillMaxWidth()
                .height(1.dp)
                .background(Color(0xFF2E5A9E))
        )
        if (analysis == null) {
            Text(
                "Play a move to see analysis",
                color = Color(0xFFB3C9F0),
                fontSize = 13.sp,
                textAlign = androidx.compose.ui.text.style.TextAlign.Center
            )
        } else {
            // gnubg labels a move only when it is a mistake (aszSkillType:
            // very bad / bad / doubtful; NULL for SKILL_NONE). No invented verdict.
            val label = analysis.level.gnubgLabel
            if (label != null) {
                val verdictColor = when (analysis.level) {
                    com.clavierhaus.gnubg.tutor.BlunderLevel.VERY_BAD -> Color(0xFFE05252)
                    com.clavierhaus.gnubg.tutor.BlunderLevel.BAD      -> Color(0xFFE0A052)
                    com.clavierhaus.gnubg.tutor.BlunderLevel.DOUBTFUL -> Color(0xFFE0D052)
                    com.clavierhaus.gnubg.tutor.BlunderLevel.NONE     -> Color.White
                }
                Text(
                    label.replaceFirstChar { it.uppercase() },
                    color = verdictColor,
                    fontSize = 18.sp,
                    fontWeight = FontWeight.Bold
                )
            }
            // Analysis-mode probability breakdown (gnubg Hint-window vector),
            // stacked for phone readability. All values straight from gnubg;
            // Win includes gammon+bg, winGammon includes bg (cumulative).
            if (detail != null) {
                val pct = { f: Float -> "%.1f%%".format(f * 100f) }
                Text(
                    "Win  ${pct(detail.win)}",
                    color = Color.White, fontSize = 20.sp, fontWeight = FontWeight.Bold
                )
                Text(
                    "Gammon  ${pct(detail.winGammon)} for  ·  ${pct(detail.loseGammon)} against",
                    color = Color(0xFFDCE6F5), fontSize = 13.sp
                )
                Text(
                    "Backgammon  ${pct(detail.winBackgammon)}  ·  ${pct(detail.loseBackgammon)}",
                    color = Color(0xFFB3C9F0), fontSize = 12.sp
                )
                val mp = (analysis.equityLoss * 1000f).toInt()
                Text(
                    "Equity  ${"%+.3f".format(detail.equityCubeless)}" +
                        if (analysis.equityLoss > 0.0005f) "   (−$mp mP vs best)" else "   (best)",
                    color = Color.White, fontSize = 13.sp
                )
            } else {
                Text("Equity lost: ${"%.3f".format(analysis.equityLoss)}", color = Color.White, fontSize = 13.sp)
            }
        }
    }
}
