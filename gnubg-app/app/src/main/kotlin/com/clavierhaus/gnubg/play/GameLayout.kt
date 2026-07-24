package com.clavierhaus.gnubg.play

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Settings
import androidx.compose.material3.Icon
import androidx.compose.material3.Text
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import com.clavierhaus.gnubg.engine.Difficulty
import com.clavierhaus.gnubg.engine.GamePhase
import com.clavierhaus.gnubg.engine.GameViewModel
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.TextButton

@Composable
fun GameLayout(
    viewModel: GameViewModel,
    onReturnToHub: (() -> Unit)? = null,
    onSaveMatch: (() -> Unit)? = null,
    onOpenSettings: (() -> Unit)? = null
) {
    var showStatistics by remember { androidx.compose.runtime.mutableStateOf(false) }
    var pendingLifecycleAction by remember { mutableStateOf<PlayLifecycleAction?>(null) }
    val settings by viewModel.settings.collectAsStateWithLifecycle()
    val gameState by viewModel.gameState.collectAsStateWithLifecycle()
    val engineReady by viewModel.engineReady.collectAsStateWithLifecycle()
    val showMatchSetup by viewModel.showMatchSetup.collectAsStateWithLifecycle()

    // The chequer-play tutor is a persisted setting, chosen at match setup.
    // (It used to be a hard-coded parameter per hub entry, while this setting
    // was written, persisted, and never read by anything.)
    val tutorMode = settings.tutorMode
    val palette = BoardPalettes.from(settings.boardTheme)
    val pal = palette
    androidx.compose.runtime.CompositionLocalProvider(LocalBoardPalette provides palette) {
    if (showStatistics) {
        // All-time tally: full-screen while open; Back returns to the
        // match-over panel exactly as it was.
        StatisticsScreen(onBack = { showStatistics = false })
    } else if (showMatchSetup) {
        MatchSetupScreen(
            tutorMode = tutorMode,
            selectedLength = settings.matchLength,
            selectedDifficulty = settings.difficulty,
            engineReady = engineReady,
            onSelectLength = { viewModel.setMatchLength(it) },
            onSelectDifficulty = { viewModel.setDifficulty(it) },
            onToggleTutor = { viewModel.setTutorMode(it) },
            // Length is not forced behind the user's back: enabling the tutor
            // pins it to 1 visibly (see setTutorMode), and the setup screen
            // says why. startMatch simply honours what is shown.
            onStart = { viewModel.startMatch(settings.matchLength) },
            onReturnToHub = onReturnToHub,
            onOpenSettings = onOpenSettings
        )
    } else {
        Box(modifier = Modifier.fillMaxSize()) {
            Row(modifier = Modifier.fillMaxSize()) {
                // Left panel -- fixed proportion of screen
                BoxWithConstraints(
                    modifier = Modifier
                        .fillMaxHeight()
                        .weight(0.18f)
                        .background(pal.uiPanel),
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
                                    .background(pal.uiActionRoll, RoundedCornerShape(avatarSize / 2)),
                                contentAlignment = Alignment.Center
                            ) { Text("GNU", color = Color.White, fontSize = 9.sp, fontWeight = FontWeight.Bold) }
                            Text("${gameState.engineScore}", color = Color.White, fontSize = 20.sp, fontWeight = FontWeight.Bold)
                            Spacer(modifier = Modifier.weight(1f))
                            Box(
                                modifier = Modifier
                                    .size(avatarSize)
                                    .background(pal.uiActionPositive, RoundedCornerShape(avatarSize / 2)),
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
                                .background(pal.uiPanel)
                        )

                        // Match context: gnubg says this is the Crawford game
                        // (leader one point from the match, cube dead for the
                        // game). Shown from the engine's own flag, never from
                        // score arithmetic here.
                        if (gameState.crawford && gameState.matchLength > 1) {
                            Text(
                                "Crawford",
                                color = Color.White,
                                fontSize = 12.sp,
                                fontWeight = FontWeight.Bold,
                                textAlign = TextAlign.Center,
                                modifier = Modifier.fillMaxWidth()
                            )
                        }

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
                                    // One line. The pane does not scroll, so the result
                                    // must not push the panel below it off the screen.
                                    val resultText = when {
                                        humanWonMatch -> "You win the match!"
                                        engineWonMatch -> "Engine wins the match"
                                        gameState.winner == 0 && gameState.nPoints >= 3 -> "You win Backgammon!"
                                        gameState.winner == 0 && gameState.nPoints >= 2 -> "You win Gammon!"
                                        gameState.winner == 0 -> "You win"
                                        gameState.nPoints >= 3 -> "Engine wins Backgammon"
                                        gameState.nPoints >= 2 -> "Engine wins Gammon"
                                        else -> "Engine wins"
                                    }
                                    // Up to two lines: "You win the match!" wraps
                                    // in this narrow rail, and maxLines=1 was
                                    // clipping it to "You win the". Two lines of
                                    // 14sp stays compact enough not to push the
                                    // button row off the non-scrolling pane.
                                    Text(resultText, color = Color.White, fontSize = 14.sp,
                                        fontWeight = FontWeight.Bold, maxLines = 2,
                                        overflow = androidx.compose.ui.text.style.TextOverflow.Ellipsis,
                                        textAlign = androidx.compose.ui.text.style.TextAlign.Center)
                                    // New match / Home live in the hoisted row below,
                                    // the same place as in every other phase. "Exit" is
                                    // gone: the action is Home, and it is called Home
                                    // everywhere.
                                    Spacer(modifier = Modifier.height(10.dp))
                                    // All-time tally: the tournament's own scoreboard,
                                    // offered at the moment the match concludes.
                                    // Deliberately not a hub entry -- statistics are
                                    // not a mode.
                                    LifecycleButton("Statistics", pal.uiChipOff, onClick = {
                                        showStatistics = true
                                    })
                                }
                            }
                            gameState.phase == GamePhase.CUBE_OFFERED -> {
                                GameplayDecisions(viewModel, gameState.phase,
                                    gameState.resignation, gameState.cubeValue)
                            }
                            // GNU resigns by itself when the position is lost
                            // (play.c:1335). gnubg refuses every roll until this is
                            // answered, so it must be asked, not assumed.
                            gameState.phase == GamePhase.RESIGNATION_OFFERED -> {
                                GameplayDecisions(viewModel, gameState.phase,
                                    gameState.resignation, gameState.cubeValue)
                            }
                            gameState.phase == GamePhase.ENGINE_THINKING -> {
                                val d = gameState.engineDice
                                Text(
                                    if (d != null) "Rolled ${d.first}-${d.second}. Thinking..."
                                    else "Thinking...",
                                    color = pal.uiTextSecondary, fontSize = 18.sp
                                )
                            }
                            gameState.phase == GamePhase.WAITING_FOR_ROLL && gameState.turn == 0 -> {
                                // The roll action is shown directly on the board surface.
                            }
                            gameState.phase == GamePhase.WAITING_FOR_ROLL && gameState.turn == 1 -> {
                                // Engine is on roll (e.g. it won the opening). Never
                                // leave this blank -- that reads as frozen at slow
                                // levels. Field report on the opening turn.
                                Text("GNU to roll...", color = pal.uiTextSecondary, fontSize = 18.sp)
                            }
                        }
                        }

                        // The game view is static: it never scrolls. Anything that does
                        // not fit must be made to fit.
                        Column(
                            horizontalAlignment = Alignment.CenterHorizontally,
                            modifier = Modifier.fillMaxWidth()
                        ) {
                            if (tutorMode) {
                                TutorAnalysisPanel(gameState.tutorAnalysis, gameState.analysisDetail, settings.showEquity)
                            } else {
                                // Resign / New game are LIVE-PLAY actions. When the
                                // MATCH is decided neither is valid -- no game to
                                // resign, no next game in a settled match -- so both
                                // are greyed rather than removed, so the player sees
                                // they are unavailable. (Field report: "New game"
                                // active after a 7-point match was won.) New match /
                                // Home, below, are the terminal actions.
                                val matchOver = gameState.matchLength > 1 &&
                                    (gameState.humanScore >= gameState.matchLength ||
                                        gameState.engineScore >= gameState.matchLength)
                                PlayLifecyclePanel(
                                    onResign = { pendingLifecycleAction = PlayLifecycleAction.RESIGN },
                                    onNewGame = { pendingLifecycleAction = PlayLifecycleAction.NEW_GAME },
                                    resignEnabled = !matchOver,
                                    newGameEnabled = !matchOver
                                )
                            }

                            // The consistent pair, present in EVERY phase and both modes:
                            // "New match" restarts with the same parameters, "Home" leaves
                            // to the hub. They used to live inside PlayLifecyclePanel,
                            // which the tutor panel and the game-over panel replace
                            // wholesale -- so in tutor mode there was no way home at all.
                            // Mid-game the restart asks first; at game over there is
                            // nothing left to lose and it just starts.
                            Spacer(modifier = Modifier.height(8.dp))
                            Row(horizontalArrangement = Arrangement.spacedBy(6.dp)) {
                                LifecycleButton("New match", pal.uiChipOff, onClick = {
                                    if (gameState.phase == GamePhase.GAME_OVER)
                                        viewModel.commandNewMatch(settings.matchLength)
                                    else
                                        pendingLifecycleAction = PlayLifecycleAction.NEW_MATCH
                                })
                                LifecycleButton(
                                    label = "Home",
                                    color = pal.uiButtonNeutral,
                                    onClick = {
                                        // Leaving a live match ends it (and frees
                                        // the setup screen for new parameters), so
                                        // ask first. At game over there is nothing
                                        // to lose -- leave straight away.
                                        if (gameState.phase == GamePhase.GAME_OVER) {
                                            viewModel.leaveMatch()
                                            onReturnToHub?.invoke()
                                        } else {
                                            pendingLifecycleAction = PlayLifecycleAction.LEAVE_MATCH
                                        }
                                    },
                                    enabled = onReturnToHub != null
                                )
                            }

                            // Save match sits outside every phase branch. It used to live
                            // inside PlayLifecyclePanel, which the tutor panel and the
                            // game-over panel each replace wholesale -- so it vanished in
                            // tutor mode, and at the end of a game, which is exactly when
                            // the match is worth saving. gnubg writes the whole match so
                            // far, at any point, so it is always meaningful.
                            if (onSaveMatch != null) {
                                Spacer(modifier = Modifier.height(8.dp))
                                LifecycleButton(
                                    label = "Save match",
                                    color = pal.uiActionRoll,
                                    onClick = { onSaveMatch() }
                                )
                            }
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
                tint = pal.uiTextSecondary,
                modifier = Modifier
                    .align(Alignment.TopStart)
                    .padding(start = 16.dp, top = 12.dp)
                    .size(28.dp)
                    .clickable { onOpenSettings?.invoke() }
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
                },
                onConfirmLeaveMatch = {
                    pendingLifecycleAction = null
                    viewModel.leaveMatch()
                    onReturnToHub?.invoke()
                }
            )
        }
    }
    }
}


private enum class PlayLifecycleAction {
    RESIGN,
    NEW_GAME,
    NEW_MATCH,
    LEAVE_MATCH
}

@Composable
private fun PlayLifecyclePanel(
    onResign: () -> Unit,
    onNewGame: () -> Unit,
    resignEnabled: Boolean = true,
    newGameEnabled: Boolean = true
) {
    val pal = LocalBoardPalette.current
    Column(
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.spacedBy(6.dp)
    ) {
        Box(
            modifier = Modifier
                .width(144.dp)
                .height(1.dp)
                .background(pal.uiPanel)
        )

        Row(horizontalArrangement = Arrangement.spacedBy(6.dp)) {
            LifecycleButton("Resign", pal.uiActionNegative, onResign, enabled = resignEnabled)
            LifecycleButton("New game", pal.uiActionRoll, onNewGame, enabled = newGameEnabled)
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
    val pal = LocalBoardPalette.current
    Box(
        modifier = Modifier
            .width(68.dp)
            .background(
                if (enabled) color else pal.uiButtonNeutral,
                RoundedCornerShape(7.dp)
            )
            .clickable(enabled = enabled, onClick = onClick)
            .padding(vertical = 6.dp),
        contentAlignment = Alignment.Center
    ) {
        Text(
            text = label,
            color = if (enabled) Color.White else pal.uiTextDisabled,
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
    onConfirmNewMatch: () -> Unit,
    onConfirmLeaveMatch: () -> Unit
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

        PlayLifecycleAction.LEAVE_MATCH -> {
            AlertDialog(
                onDismissRequest = onDismiss,
                title = { Text("Leave this match?") },
                text = { Text("The current match will end and you'll return to the home screen, where you can start a new match with different strength and length.") },
                confirmButton = {
                    TextButton(onClick = onConfirmLeaveMatch) {
                        Text("Leave match")
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
    // 80% metrics, for panes that must hold many controls WITHOUT scrolling.
    // The game view law applies everywhere: nothing scrolls; what does not
    // fit is made to fit.
    compact: Boolean = false,
    onClick: () -> Unit
) {
    val pal = LocalBoardPalette.current
    Box(
        modifier = Modifier
            .background(color, RoundedCornerShape(8.dp))
            .clickable(enabled = enabled) { onClick() }
            .padding(
                horizontal = if (compact) 19.dp else 24.dp,
                vertical = if (compact) 9.dp else 12.dp
            ),
        contentAlignment = Alignment.Center
    ) {
        Text(
            label,
            color = if (enabled) Color.White else pal.uiTextDisabled,
            fontSize = if (compact) 13.sp else 16.sp,
            fontWeight = FontWeight.Bold,
            // A button label that wraps is a malformed button (field report:
            // the cube chips on narrow panes). One line, always.
            maxLines = 1
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
    onToggleTutor: (Boolean) -> Unit,
    onStart: () -> Unit,
    onReturnToHub: (() -> Unit)? = null,
    onOpenSettings: (() -> Unit)? = null
) {
    val pal = LocalBoardPalette.current
    Box(
        modifier = Modifier
            .fillMaxSize()
            .background(pal.uiPanelDeep),
        contentAlignment = Alignment.Center
    ) {
        // Aligned children: they do not touch the weighted setup Column, which
        // was hard-won (a scroll modifier once broke its weights entirely). The
        // gear keeps the top-left, as on every screen; Home takes the opposite
        // corner rather than colliding with it.
        if (onOpenSettings != null) {
            Icon(
                imageVector = Icons.Filled.Settings,
                contentDescription = "Settings",
                tint = pal.uiTextSecondary,
                modifier = Modifier
                    .align(Alignment.TopStart)
                    .padding(start = 16.dp, top = 12.dp)
                    .size(28.dp)
                    .clickable { onOpenSettings() }
            )
        }
        if (onReturnToHub != null) {
            Box(modifier = Modifier.align(Alignment.TopEnd).padding(12.dp)) {
                GameButton("Home", pal.uiButtonNeutral) { onReturnToHub() }
            }
        }
        // The Column owns the full height, so weighted spacers can distribute
        // what is left over after the controls have measured themselves. It must
        // NOT scroll: a scrollable Column measures with unbounded height, which
        // makes weight() meaningless and simply stacks children top-to-bottom --
        // which is exactly why Start Match was landing against the bottom edge
        // with free space sitting unusable above it.
        Column(
            horizontalAlignment = Alignment.CenterHorizontally,
            modifier = Modifier
                .fillMaxSize()
                .padding(horizontal = 24.dp, vertical = 16.dp)
        ) {
            // The Start button is pinned at the foot (below, outside this weighted
            // region). Everything else lives in a weight(1f) area that takes only
            // the space left after the button is placed -- so the button can never
            // be squeezed to zero height, which is exactly what happened on short
            // 20:9 phones in landscape: weighted spacers distribute LEFTOVER space,
            // and when the controls already fill the screen there is none, so the
            // button got zero pixels and vanished (no "Loading engine..." either).
            Column(
                horizontalAlignment = Alignment.CenterHorizontally,
                verticalArrangement = Arrangement.spacedBy(10.dp),
                modifier = Modifier
                    .weight(1f)
                    .fillMaxWidth()
            ) {
            Text(
                if (tutorMode) "Chequer-Play Tutor"
                else "Tournament Match",
                color = Color.White,
                fontSize = 26.sp,
                fontWeight = FontWeight.Bold
            )

            Text(
                "Opponent strength",
                color = pal.uiTextSecondary,
                fontSize = 16.sp
            )

            // Every level the enum knows, so new engine strengths appear here
            // without a second hardcoded list to forget. Compact: seven chips
            // must fit one row on a 16:9 phone.
            Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                Difficulty.entries.forEach { difficulty ->
                    val selected = selectedDifficulty == difficulty
                    GameButton(
                        label = difficulty.label,
                        color = if (selected) pal.uiChipOn else pal.uiChipOff,
                        enabled = engineReady,
                        compact = true
                    ) {
                        onSelectDifficulty(difficulty)
                    }
                }
            }

            Spacer(modifier = Modifier.height(6.dp))

            // Tutor and match length sit side by side: this screen is
            // landscape-only and has horizontal room to spare but no vertical
            // slack -- stacking them overflowed the column and clipped the
            // Start button off-screen.
            Row(
                modifier = Modifier.fillMaxWidth(),
                verticalAlignment = Alignment.Top
            ) {
                Spacer(modifier = Modifier.weight(1f))

                Column(horizontalAlignment = Alignment.CenterHorizontally) {
                    Text(
                        "Chequer-play tutor",
                        color = pal.uiTextSecondary,
                        fontSize = 18.sp
                    )
                    Spacer(modifier = Modifier.height(10.dp))
                    Row(horizontalArrangement = Arrangement.spacedBy(12.dp)) {
                        GameButton(
                            label = "Off",
                            color = if (!tutorMode) pal.uiChipOn else pal.uiChipOff,
                            enabled = engineReady
                        ) {
                            onToggleTutor(false)
                        }
                        GameButton(
                            label = "On",
                            color = if (tutorMode) pal.uiChipOn else pal.uiChipOff,
                            enabled = engineReady
                        ) {
                            onToggleTutor(true)
                        }
                    }
                }

                // Blind space: a weighted spacer, so the separation is whatever
                // the screen has left over rather than a fixed distance that
                // only looks right on one device.
                Spacer(modifier = Modifier.weight(1f))

                Column(horizontalAlignment = Alignment.CenterHorizontally) {
                    Text(
                        "Match length",
                        color = pal.uiTextSecondary,
                        fontSize = 18.sp
                    )
                    Spacer(modifier = Modifier.height(10.dp))
                    if (tutorMode) {
                        // The tutored game is a single game: at 1 point the cube
                        // is out of play (you cannot double past the match), so
                        // the tutor only ever has chequer decisions to comment
                        // on. Say so, rather than showing four dead chips.
                        Text(
                            "Single game",
                            color = Color.White,
                            fontSize = 16.sp,
                            fontWeight = FontWeight.Bold
                        )
                        Spacer(modifier = Modifier.height(4.dp))
                        Text(
                            "The cube is not in play,\nso the tutor comments on\nchequer play only.",
                            color = pal.uiTextDisabled,
                            fontSize = 13.sp
                        )
                    } else {
                        // 1, 3 and 5 are fixed shortcuts. The fourth slot is
                        // flexible: it shows whatever length is actually set when
                        // that length is not one of the shortcuts, so an 11-point
                        // match chosen in Settings is visible here rather than
                        // leaving every chip unselected. +/- adjust it in place,
                        // over the same 1..25 range as the Settings stepper.
                        val shortcuts = listOf(1, 3, 5)
                        val flexible = if (selectedLength in shortcuts) 7 else selectedLength

                        Row(
                            horizontalArrangement = Arrangement.spacedBy(12.dp),
                            // Top-align so + sits level with the top of the
                            // chips; the +/- stack is taller than one chip and
                            // centring it pushed the chips down out of line with
                            // the tutor buttons opposite.
                            verticalAlignment = Alignment.Top
                        ) {
                            shortcuts.forEach { n ->
                                GameButton(
                                    label = "$n",
                                    color = if (selectedLength == n) pal.uiChipOn else pal.uiChipOff,
                                    enabled = engineReady
                                ) {
                                    onSelectLength(n)
                                }
                            }

                            // +/- belong to the flexible chip and only act when
                            // it is the selected length. Tap the chip to enter
                            // the flexible range; then step it. (Letting +/- act
                            // from a shortcut meant "-" on 3 raised the length
                            // to 7, which is nonsense.)
                            val onFlexible = selectedLength == flexible

                            GameButton(
                                label = "$flexible",
                                color = if (selectedLength == flexible) pal.uiChipOn else pal.uiChipOff,
                                enabled = engineReady
                            ) {
                                onSelectLength(flexible)
                            }

                            // Steppers stack to the right of the flexible chip.
                            // The enclosing Row is Alignment.Top, so "+" sits
                            // level with the top of the chip and "-" hangs below
                            // it. Both keep full-size tap targets: nothing here
                            // is offset after layout, so what is drawn is what is
                            // tappable.
                            Column(
                                horizontalAlignment = Alignment.CenterHorizontally,
                                verticalArrangement = Arrangement.spacedBy(10.dp)
                            ) {
                                GameButton(
                                    label = "+",
                                    color = pal.uiButtonNeutral,
                                    enabled = engineReady && onFlexible && flexible < 25
                                ) {
                                    onSelectLength((flexible + 1).coerceAtMost(25))
                                }
                                GameButton(
                                    label = "-",
                                    color = pal.uiButtonNeutral,
                                    enabled = engineReady && onFlexible && flexible > 1
                                ) {
                                    onSelectLength((flexible - 1).coerceAtLeast(1))
                                }
                            }
                        }
                    }
                }

                Spacer(modifier = Modifier.weight(1f))
            }
            } // end weighted content Column

            // Pinned at the foot: a fixed-height row that is laid out BEFORE the
            // weighted region above, so it always has its natural size. It is the
            // only green control here, set apart by colour and position. Settings
            // is not repeated -- it is one tap from the hub and from the board.
            Spacer(modifier = Modifier.height(8.dp))
            GameButton(
                label = if (engineReady) "Start Match" else "Loading engine...",
                color = pal.uiActionPositive,
                enabled = engineReady
            ) {
                onStart()
            }
        }
    }
}

@Composable
private fun TutorAnalysisPanel(
    analysis: com.clavierhaus.gnubg.engine.TutorAnalysis?,
    detail: com.clavierhaus.gnubg.engine.MoveAnalysisDetail? = null,
    showEquity: Boolean = true
) {
    val pal = LocalBoardPalette.current
    Column(
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.spacedBy(6.dp),
        modifier = Modifier.fillMaxWidth().padding(8.dp)
    ) {
        Box(
            modifier = Modifier
                .fillMaxWidth()
                .height(1.dp)
                .background(pal.uiPanel)
        )
        if (analysis == null) {
            Text(
                "Play a move to see analysis",
                color = pal.uiTextSecondary,
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
                    color = pal.uiTextPrimary, fontSize = 13.sp
                )
                Text(
                    "Backgammon  ${pct(detail.winBackgammon)}  ·  ${pct(detail.loseBackgammon)}",
                    color = pal.uiTextSecondary, fontSize = 12.sp
                )
                if (showEquity) {
                    val mp = (analysis.equityLoss * 1000f).toInt()
                    Text(
                        "Equity  ${"%+.3f".format(detail.equityCubeless)}" +
                            if (analysis.equityLoss > 0.0005f) "   (−$mp mP vs best)" else "   (best)",
                        color = Color.White, fontSize = 13.sp
                    )
                }
            } else if (showEquity) {
                Text("Equity lost: ${"%.3f".format(analysis.equityLoss)}", color = Color.White, fontSize = 13.sp)
            }
        }
    }
}
