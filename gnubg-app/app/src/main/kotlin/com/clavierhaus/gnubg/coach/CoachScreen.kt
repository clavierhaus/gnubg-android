package com.clavierhaus.gnubg.coach

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Settings
import androidx.compose.material3.Icon
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
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
import com.clavierhaus.gnubg.Engine
import com.clavierhaus.gnubg.engine.GamePhase
import com.clavierhaus.gnubg.engine.GameSettings
import com.clavierhaus.gnubg.engine.GameViewModel
import com.clavierhaus.gnubg.play.BackgammonBoard
import com.clavierhaus.gnubg.play.BoardPalettes
import com.clavierhaus.gnubg.play.GameButton
import com.clavierhaus.gnubg.play.LocalBoardPalette
import androidx.compose.runtime.CompositionLocalProvider

/**
 * Train with the Coach (docs/COACH.md) -- the fourth mode. A contained single
 * game against gnubg's Expert preset with the engine looking over the player's
 * shoulder. Compartmentalized: its own screen and pane, ZERO changes to the
 * Play path; underneath it drives the SAME GameViewModel and engine verbs --
 * one game loop, two front ends.
 *
 * The board carries the whole move interaction (roll, drag, Confirm/Undo are
 * on-board, Board.kt); this screen adds only the coaching pane. The glance
 * line reads gnubg's verdict on the player's last move via Engine.coachVerdict
 * (one evaluation per move, every disclosure level reads it -- vision C6),
 * fetched when control returns to the player. Silent when gnubg does not flag
 * the move (vision P2: no noise for good play).
 */

/** Decoded gnubg coach verdict -- layout documented at Engine.coachVerdict. */
private data class CoachGlance(
    val rank: Int,
    val cMoves: Int,
    val eqPlayed: Float,
    val eqBest: Float,
    val skill: Int,              // 0 very bad, 1 bad, 2 doubtful, 3 none
    val playedNotation: String,
    val bestNotation: String
) {
    val loss: Float get() = eqBest - eqPlayed
    val flagged: Boolean get() = skill != 3
}

private fun decodeGlance(v: IntArray): CoachGlance? {
    if (v.size < 166) return null
    val preBoard = IntArray(50) { v[21 + it] }
    val played = IntArray(8) { v[5 + it] }
    val best = IntArray(8) { v[13 + it] }
    return CoachGlance(
        rank = v[0],
        cMoves = v[1],
        eqPlayed = Float.fromBits(v[2]),
        eqBest = Float.fromBits(v[3]),
        skill = v[4],
        playedNotation = Engine.formatMove(preBoard, played),
        bestNotation = Engine.formatMove(preBoard, best)
    )
}

private fun skillLabel(skill: Int): String = when (skill) {
    0 -> "Very bad"
    1 -> "Bad"
    2 -> "Doubtful"
    else -> ""
}

@Composable
fun CoachScreen(
    viewModel: GameViewModel,
    settings: GameSettings,
    onReturnToHub: () -> Unit,
    onOpenSettings: (() -> Unit)? = null
) {
    val gameState by viewModel.gameState.collectAsState()
    val pal = BoardPalettes.from(settings.boardTheme)

    var glance by remember { mutableStateOf<CoachGlance?>(null) }
    var started by remember { mutableStateOf(false) }

    // One contained game, started once per entry into the mode.
    LaunchedEffect(Unit) {
        if (!started) {
            started = true
            glance = null
            viewModel.startCoachGame()
        }
    }

    // Fetch gnubg's verdict when control returns to the player after the
    // engine's reply (the record tail then holds: human move, engine move --
    // analyze_replay(NULL) scans back to the last HUMAN chequer move, so the
    // engine's reply does not disturb it). Also at game over, for the last
    // move's verdict. Returns empty when there is no human move yet.
    LaunchedEffect(gameState.phase, gameState.turn) {
        val fetch = when (gameState.phase) {
            GamePhase.WAITING_FOR_ROLL -> gameState.turn == 0
            GamePhase.GAME_OVER -> true
            else -> false
        }
        if (fetch) {
            val v = viewModel.fetchCoachVerdict()
            decodeGlance(v)?.let { glance = it }
        }
    }

    CompositionLocalProvider(LocalBoardPalette provides pal) {
        Box(
            modifier = Modifier
                .fillMaxSize()
                .background(pal.uiBackground)
        ) {
            // Gear keeps the top-left, as on every screen.
            if (onOpenSettings != null) {
                Icon(
                    imageVector = Icons.Filled.Settings,
                    contentDescription = "Settings",
                    tint = pal.uiTextSecondary,
                    modifier = Modifier
                        .align(Alignment.TopStart)
                        .padding(12.dp)
                        .size(28.dp)
                        .clickable { onOpenSettings() }
                )
            }

            Row(
                modifier = Modifier
                    .fillMaxSize()
                    .padding(start = 52.dp, top = 8.dp, end = 12.dp, bottom = 8.dp)
            ) {
                Box(modifier = Modifier.weight(1f).fillMaxHeight()) {
                    BackgammonBoard(
                        settings = settings,
                        gameState = gameState,
                        viewModel = viewModel,
                        tutorMode = false
                    )
                }

                Spacer(modifier = Modifier.width(12.dp))

                CoachPanel(
                    glance = glance,
                    phase = gameState.phase,
                    winner = gameState.winner,
                    onNewGame = {
                        glance = null
                        viewModel.startCoachGame()
                    },
                    onHome = {
                        viewModel.endCoachSession()
                        onReturnToHub()
                    }
                )
            }
        }
    }
}

@Composable
private fun CoachPanel(
    glance: CoachGlance?,
    phase: GamePhase,
    winner: Int,
    onNewGame: () -> Unit,
    onHome: () -> Unit
) {
    val pal = LocalBoardPalette.current
    Column(
        modifier = Modifier.width(200.dp).fillMaxHeight().padding(vertical = 8.dp),
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.SpaceBetween
    ) {
        Column(
            horizontalAlignment = Alignment.CenterHorizontally,
            modifier = Modifier.weight(1f, fill = false)
        ) {
            Text(
                "Coach",
                color = Color.White,
                fontSize = 20.sp,
                fontWeight = FontWeight.Bold
            )
            Spacer(modifier = Modifier.height(10.dp))

            when {
                phase == GamePhase.GAME_OVER -> {
                    Text(
                        if (winner == 0) "You win the game." else "GNU wins the game.",
                        color = Color.White, fontSize = 14.sp
                    )
                    Spacer(modifier = Modifier.height(8.dp))
                }
                phase == GamePhase.ENGINE_THINKING -> {
                    Text("GNU is replying...", color = pal.uiTextSecondary, fontSize = 13.sp)
                    Spacer(modifier = Modifier.height(8.dp))
                }
                else -> {}
            }

            val g = glance
            when {
                g == null -> {
                    Text(
                        "Play your move. The Coach judges every one -- and stays quiet when it was good.",
                        color = pal.uiTextSecondary, fontSize = 13.sp
                    )
                }
                !g.flagged -> {
                    // Vision P2/P3: silence is the signal for good play. One
                    // quiet line so the player knows the Coach saw it.
                    Text("Good. Nothing to show.", color = pal.uiTextSecondary, fontSize = 13.sp)
                }
                else -> {
                    // The glance level (vision P2 depth 1): severity, cost,
                    // played vs best notation. All gnubg's own values.
                    Text(
                        "${skillLabel(g.skill)}: ${"%+.3f".format(-g.loss)}",
                        color = Color.White, fontSize = 15.sp,
                        fontWeight = FontWeight.Bold
                    )
                    Spacer(modifier = Modifier.height(6.dp))
                    Text("You: ${g.playedNotation}", color = Color.White, fontSize = 13.sp)
                    Text("Best: ${g.bestNotation}", color = Color.White, fontSize = 13.sp)
                    Spacer(modifier = Modifier.height(4.dp))
                    Text(
                        "Ranked ${g.rank + 1} of ${g.cMoves}",
                        color = pal.uiTextSecondary, fontSize = 12.sp
                    )
                }
            }
        }

        Column(horizontalAlignment = Alignment.CenterHorizontally) {
            if (phase == GamePhase.GAME_OVER) {
                GameButton("New game", pal.uiChipOff, compact = true) { onNewGame() }
                Spacer(modifier = Modifier.height(6.dp))
            }
            GameButton("Home", pal.uiButtonNeutral, compact = true) { onHome() }
        }
    }
}
