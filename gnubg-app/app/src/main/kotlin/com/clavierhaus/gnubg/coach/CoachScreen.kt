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
/** One of gnubg's better alternatives: its move, notation, and how much it
 *  gains over the played move (gnubg equities, subtracted for display). */
private data class CoachAlt(
    val anMove: IntArray,
    val notation: String,
    val gain: Float
)

private data class CoachGlance(
    val rank: Int,
    val cMoves: Int,
    val eqPlayed: Float,
    val eqBest: Float,
    val skill: Int,              // 0 very bad, 1 bad, 2 doubtful, 3 none
    val playedNotation: String,
    val bestNotation: String,
    val playedMove: IntArray,    // anMove[8], human mover frame -- for the trace
    val bestMove: IntArray,
    val alts: List<CoachAlt>     // better-than-played candidates, best first, <= 3
) {
    val loss: Float get() = eqBest - eqPlayed
    val flagged: Boolean get() = skill != 3
}

private fun decodeGlance(v: IntArray): CoachGlance? {
    if (v.size < 166) return null
    val preBoard = IntArray(50) { v[21 + it] }
    val played = IntArray(8) { v[5 + it] }
    val best = IntArray(8) { v[13 + it] }
    // The verdict's candidate rows (base 86, 16 ints each: anMove[8],
    // equity bits, arEvalMove[7] bits) are ranked from best; the ones ranked
    // ABOVE the played move are its better alternatives. Up to three.
    val eqPlayed = Float.fromBits(v[2])
    val k = v[85].coerceIn(0, 5)
    val alts = buildList {
        val better = minOf(v[0], k, 3)
        for (i in 0 until better) {
            val base = 86 + i * 16
            val mv = IntArray(8) { j -> v[base + j] }
            val eq = Float.fromBits(v[base + 8])
            add(CoachAlt(mv, Engine.formatMove(preBoard, mv), eq - eqPlayed))
        }
    }
    return CoachGlance(
        rank = v[0],
        cMoves = v[1],
        eqPlayed = eqPlayed,
        eqBest = Float.fromBits(v[3]),
        skill = v[4],
        playedNotation = Engine.formatMove(preBoard, played),
        bestNotation = Engine.formatMove(preBoard, best),
        playedMove = played,
        bestMove = best,
        alts = alts
    )
}

private fun ordinal(n: Int): String = when {
    n % 100 in 11..13 -> "${n}th"
    n % 10 == 1 -> "${n}st"
    n % 10 == 2 -> "${n}nd"
    n % 10 == 3 -> "${n}rd"
    else -> "${n}th"
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

    // The verdict arrives via the VM's coachGlance flow -- ONE analysis per
    // move, run by confirm() in the same decoupled slot the tutor analysis
    // occupies in Play. The screen only decodes. (The first design fetched
    // from a LaunchedEffect here, stacking a third 2-ply analysis on top of
    // the Play pipeline's two; on a doubles roll the single engine thread
    // was saturated for 30+ seconds and GNU looked stuck -- field report.)
    val rawGlance by viewModel.coachGlance.collectAsState()

    // Candidate explorer (maintainer design): tapping a numbered alternative
    // shows THAT move's resulting position -- checkers fully colored, movement
    // as green arrows -- on the board; tapping it again returns to the live
    // game. The result board comes from gnubg's own ApplyMove via the facade.
    var selectedAlt by remember { mutableStateOf(-1) }
    var altBoard by remember { mutableStateOf<IntArray?>(null) }
    LaunchedEffect(rawGlance) { selectedAlt = -1; altBoard = null }
    LaunchedEffect(selectedAlt) {
        val g = glance
        altBoard = if (selectedAlt >= 0 && g != null && selectedAlt < g.alts.size) {
            val preBoard = rawGlance?.let { v -> IntArray(50) { v[21 + it] } }
            preBoard?.let { pb ->
                val b = Engine.applyMoveToBoard(pb, g.alts[selectedAlt].anMove)
                if (b.size == 50) b else null
            }
        } else null
    }

    LaunchedEffect(rawGlance) {
        // null now MEANS "cleared for judging" (confirm clears it before the
        // pre-apply verdict), so mirror it -- the panel shows the judging
        // state instead of a stale verdict.
        glance = rawGlance?.let { decodeGlance(it) }
    }

    CompositionLocalProvider(LocalBoardPalette provides pal) {
        Box(
            modifier = Modifier
                .fillMaxSize()
                .background(pal.uiPanelDeep)
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
                    val g = glance
                    val exploring = selectedAlt >= 0 && altBoard != null && g != null
                    if (exploring) {
                        // Counterfactual view: the alternative's RESULTING
                        // position (gnubg's ApplyMove), its movement as green
                        // arrows, checkers fully colored. viewModel = null:
                        // a counterfactual board must not accept game taps
                        // (the Analyse pattern). ENGINE_THINKING phase
                        // suppresses all action chrome.
                        BackgammonBoard(
                            settings = settings,
                            gameState = com.clavierhaus.gnubg.engine.BoardState(
                                board = altBoard!!,
                                matchScore = gameState.matchScore,
                                matchLength = gameState.matchLength,
                                phase = GamePhase.ENGINE_THINKING
                            ),
                            viewModel = null,
                            tutorMode = false,
                            coachTrace = com.clavierhaus.gnubg.play.CoachTrace(
                                played = null,
                                best = g!!.alts[selectedAlt].anMove,
                                ghost = false
                            )
                        )
                    } else {
                        // The live game board carries NO arrows (maintainer
                        // design): it simply shows the position the player's
                        // move produced. Arrows exist only inside the numbered
                        // alternative views.
                        BackgammonBoard(
                            settings = settings,
                            gameState = gameState,
                            viewModel = viewModel,
                            tutorMode = false
                        )
                    }
                }

                Spacer(modifier = Modifier.width(12.dp))

                CoachPanel(
                    glance = glance,
                    phase = gameState.phase,
                    winner = gameState.winner,
                    selectedAlt = selectedAlt,
                    onSelectAlt = { n -> selectedAlt = if (selectedAlt == n) -1 else n },
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

/** The better alternatives, numbered; each number is a TOGGLE (maintainer
 *  design): tap to view that move's resulting position on the board, tap
 *  again to return to the live game. Values are gnubg's; the gain shown is
 *  the candidate's equity minus the played move's. */
@Composable
private fun AltList(
    alts: List<CoachAlt>,
    selectedAlt: Int,
    onSelectAlt: (Int) -> Unit
) {
    val pal = LocalBoardPalette.current
    if (alts.isEmpty()) return
    Spacer(modifier = Modifier.height(6.dp))
    Text("Better:", color = pal.uiTextSecondary, fontSize = 11.sp)
    alts.forEachIndexed { i, alt ->
        Spacer(modifier = Modifier.height(3.dp))
        Row(verticalAlignment = Alignment.CenterVertically) {
            GameButton(
                label = "${i + 1}",
                color = if (selectedAlt == i) pal.uiChipOn else pal.uiChipOff,
                compact = true
            ) { onSelectAlt(i) }
            Spacer(modifier = Modifier.width(6.dp))
            Text(
                "${alt.notation}  ${"%+.3f".format(alt.gain)}",
                color = if (selectedAlt == i) Color.White else pal.uiTextSecondary,
                fontSize = 12.sp
            )
        }
    }
    if (selectedAlt >= 0) {
        Spacer(modifier = Modifier.height(4.dp))
        Text(
            "Viewing ${selectedAlt + 1}. Tap ${selectedAlt + 1} again for the game.",
            color = pal.uiTextDisabled, fontSize = 10.sp
        )
    }
}

@Composable
private fun CoachPanel(
    glance: CoachGlance?,
    phase: GamePhase,
    winner: Int,
    selectedAlt: Int,
    onSelectAlt: (Int) -> Unit,
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
                    // The turn's visible sequence (maintainer order): the
                    // verdict is computed FIRST -- "Judging your move..." --
                    // then, verdict on screen, GNU rolls and replies.
                    Text(
                        if (glance == null) "Judging your move..." else "GNU is replying...",
                        color = pal.uiTextSecondary, fontSize = 13.sp
                    )
                    Spacer(modifier = Modifier.height(8.dp))
                }
                else -> {}
            }

            val g = glance
            // Every verdict is ANCHORED to the move it judges by naming it --
            // field report: the message lands a beat after GNU's reply, when
            // the player has mentally moved on, so an unanchored "Good" was
            // ambiguous. Three honest tiers instead of binary silence (field
            // report: "way too often Nothing to show"): the BEST move earns
            // its affirmation; fine-but-not-best shows what was better,
            // muted; flagged moves speak up. All values gnubg's own.
            when {
                g == null -> {
                    Text(
                        "Play your move. The Coach judges every one.",
                        color = pal.uiTextSecondary, fontSize = 13.sp
                    )
                }
                g.rank == 0 -> {
                    Text("Your ${g.playedNotation}", color = pal.uiTextSecondary, fontSize = 12.sp)
                    Text(
                        "The best move.",
                        color = pal.uiActionPositive, fontSize = 14.sp,
                        fontWeight = FontWeight.Bold
                    )
                }
                !g.flagged -> {
                    Text("Your ${g.playedNotation}", color = pal.uiTextSecondary, fontSize = 12.sp)
                    Text(
                        "Fine. ${ordinal(g.rank + 1)} of ${g.cMoves} (${"%+.3f".format(-g.loss)}).",
                        color = pal.uiTextSecondary, fontSize = 13.sp
                    )
                    AltList(g.alts, selectedAlt, onSelectAlt)
                }
                else -> {
                    Text("Your ${g.playedNotation}", color = pal.uiTextSecondary, fontSize = 12.sp)
                    Text(
                        "${skillLabel(g.skill)}: ${"%+.3f".format(-g.loss)}",
                        color = Color.White, fontSize = 15.sp,
                        fontWeight = FontWeight.Bold
                    )
                    Spacer(modifier = Modifier.height(2.dp))
                    Text(
                        "${ordinal(g.rank + 1)} of ${g.cMoves} legal moves",
                        color = pal.uiTextSecondary, fontSize = 12.sp
                    )
                    AltList(g.alts, selectedAlt, onSelectAlt)
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
