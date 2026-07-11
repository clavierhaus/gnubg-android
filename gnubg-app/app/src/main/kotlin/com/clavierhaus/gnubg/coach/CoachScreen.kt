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
    // Unified selection (maintainer design): index 0 is P -- the player's own
    // move -- and 1..n are gnubg's better moves; -1 = the live game board.
    // COMMON GROUND for every toggled view: the position BEFORE any move,
    // after the roll -- the same pre-move board for P and all alternatives,
    // dice in full color (nothing played), only the arrows differing. The
    // board is a constant; the moves are pure deltas.
    var selectedAlt by remember { mutableStateOf(-1) }
    var glanceDice by remember { mutableStateOf<Pair<Int, Int>?>(null) }
    LaunchedEffect(rawGlance) {
        selectedAlt = -1
        // The dice the judged move was rolled with, captured while the game
        // state still carries them (COACH_REVIEW preserves the move-entry
        // fields; after GNU's reply they are gone from live state).
        if (rawGlance != null) glanceDice = gameState.originalDice
    }
    val preMoveBoard = rawGlance?.let { v -> IntArray(50) { v[21 + it] } }

    LaunchedEffect(rawGlance) {
        // null now MEANS "cleared for judging" (confirm clears it before the
        // pre-apply verdict), so mirror it -- the panel shows the judging
        // state instead of a stale verdict. Decode failures must be LOUD and
        // must not silently keep an old verdict on screen (field report:
        // stale first-turn verdict shown against a mid-game live board).
        glance = rawGlance?.let { v ->
            try {
                val d = decodeGlance(v)
                android.util.Log.i("gnubg-coach",
                    "screen: decoded rank=${v[0]} of=${v[1]} played='${d?.playedNotation}' alts=${d?.alts?.size} " +
                    "fp(pre)=${com.clavierhaus.gnubg.engine.GameViewModel.fpOf(IntArray(50) { j -> v[21 + j] })}")
                d
            } catch (t: Throwable) {
                android.util.Log.e("gnubg-coach", "screen: decode FAILED: $t")
                null
            }
        }
        if (rawGlance == null) android.util.Log.i("gnubg-coach", "screen: glance cleared")
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
                    val exploring = selectedAlt >= 0 && preMoveBoard != null && g != null
                    if (exploring) {
                        // The toggled view: the PRE-move position -- identical
                        // for P and every alternative -- with the roll's dice
                        // in FULL color (remainingDice = the whole roll: no
                        // die used, nothing moved) and the selected move as
                        // arrows departing from the very checkers that would
                        // move. viewModel = null: this board must not accept
                        // game taps (the Analyse pattern); ENGINE_THINKING
                        // suppresses all action chrome.
                        val shownMove = if (selectedAlt == 0) g!!.playedMove
                                        else g!!.alts[selectedAlt - 1].anMove
                        val d = glanceDice
                        val fullRoll = d?.let { (d0, d1) ->
                            if (d0 == d1) listOf(d0, d0, d0, d0) else listOf(d0, d1)
                        } ?: emptyList()
                        BackgammonBoard(
                            settings = settings,
                            gameState = com.clavierhaus.gnubg.engine.BoardState(
                                board = preMoveBoard!!,
                                dice = d,
                                remainingDice = fullRoll,
                                matchScore = gameState.matchScore,
                                matchLength = gameState.matchLength,
                                phase = GamePhase.ENGINE_THINKING
                            ),
                            viewModel = null,
                            tutorMode = false,
                            coachTrace = com.clavierhaus.gnubg.play.CoachTrace(
                                played = null,
                                best = shownMove,
                                ghost = false,
                                // P view: the player's move in the player's
                                // checker color; alternatives in gnubg's green.
                                emphasisColor = if (selectedAlt == 0) pal.checkerLight else null
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
                    onSelectAlt = { n ->
                        selectedAlt = if (selectedAlt == n) -1 else n
                        android.util.Log.i("gnubg-coach",
                            "screen: toggle sel=$selectedAlt " +
                            "fp(pre)=${preMoveBoard?.let { com.clavierhaus.gnubg.engine.GameViewModel.fpOf(it) }} " +
                            "fp(live)=${com.clavierhaus.gnubg.engine.GameViewModel.fpOf(gameState.board)} dice=$glanceDice")
                    },
                    onContinue = { selectedAlt = -1; viewModel.continueCoachTurn() },
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

/** The move list (maintainer design): the player's own move is the FIRST
 *  item, chip "P" in a distinct color, then gnubg's better moves numbered --
 *  every item the same TOGGLE: tap to view that move's resulting position on
 *  the board (original position -> position after move, no mental
 *  reconstruction), tap again for the live game. Values are gnubg's. */
@Composable
private fun MoveList(
    glance: CoachGlance,
    selectedAlt: Int,
    onSelectAlt: (Int) -> Unit
) {
    val pal = LocalBoardPalette.current
    Spacer(modifier = Modifier.height(6.dp))

    // Row 0: P -- the player's move, same mechanics, its own color.
    Row(verticalAlignment = Alignment.CenterVertically) {
        Box(modifier = Modifier.width(30.dp), contentAlignment = Alignment.Center) {
            GameButton(
                label = "P",
                color = if (selectedAlt == 0) pal.uiActionRoll else pal.uiButtonNeutral,
                compact = true
            ) { onSelectAlt(0) }
        }
        Spacer(modifier = Modifier.width(6.dp))
        Text(
            glance.playedNotation,
            color = if (selectedAlt == 0) Color.White else pal.uiTextSecondary,
            fontSize = 12.sp
        )
    }

    glance.alts.forEachIndexed { i, alt ->
        Spacer(modifier = Modifier.height(3.dp))
        Row(verticalAlignment = Alignment.CenterVertically) {
            Box(modifier = Modifier.width(30.dp), contentAlignment = Alignment.Center) {
                GameButton(
                    label = "${i + 1}",
                    color = if (selectedAlt == i + 1) pal.uiChipOn else pal.uiChipOff,
                    compact = true
                ) { onSelectAlt(i + 1) }
            }
            Spacer(modifier = Modifier.width(6.dp))
            Text(
                "${alt.notation}  ${"%+.3f".format(alt.gain)}",
                color = if (selectedAlt == i + 1) Color.White else pal.uiTextSecondary,
                fontSize = 12.sp
            )
        }
    }
}

@Composable
private fun CoachPanel(
    glance: CoachGlance?,
    phase: GamePhase,
    winner: Int,
    selectedAlt: Int,
    onSelectAlt: (Int) -> Unit,
    onContinue: () -> Unit,
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
                phase == GamePhase.COACH_REVIEW -> {
                    Text(
                        "Study the verdict. GNU waits for you.",
                        color = pal.uiTextSecondary, fontSize = 12.sp
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
                        "Play your move and have it evaluated by the Coach.",
                        color = pal.uiTextSecondary, fontSize = 13.sp
                    )
                }
                g.rank == 0 -> {
                    Text(
                        "The best move.",
                        color = pal.uiActionPositive, fontSize = 14.sp,
                        fontWeight = FontWeight.Bold
                    )
                    MoveList(g, selectedAlt, onSelectAlt)
                }
                !g.flagged -> {
                    Text(
                        "Fine. ${ordinal(g.rank + 1)} of ${g.cMoves} (${"%+.3f".format(-g.loss)}).",
                        color = pal.uiTextSecondary, fontSize = 13.sp
                    )
                    MoveList(g, selectedAlt, onSelectAlt)
                }
                else -> {
                    Text(
                        "${skillLabel(g.skill)}: ${"%+.3f".format(-g.loss)} " +
                            "(${ordinal(g.rank + 1)} of ${g.cMoves})",
                        color = Color.White, fontSize = 14.sp,
                        fontWeight = FontWeight.Bold
                    )
                    MoveList(g, selectedAlt, onSelectAlt)
                }
            }
        }

        Column(horizontalAlignment = Alignment.CenterHorizontally) {
            if (phase == GamePhase.COACH_REVIEW) {
                // The player's active continuation: only now does gnubg
                // receive the move; GNU rolls and replies after this.
                GameButton("GNU's turn", pal.uiActionRoll, compact = true) { onContinue() }
                Spacer(modifier = Modifier.height(6.dp))
            }
            if (phase == GamePhase.GAME_OVER) {
                GameButton("New game", pal.uiChipOff, compact = true) { onNewGame() }
                Spacer(modifier = Modifier.height(6.dp))
            }
            GameButton("Home", pal.uiButtonNeutral, compact = true) { onHome() }
        }
    }
}
