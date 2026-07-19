package com.clavierhaus.gnubg.coach

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Settings
import androidx.compose.material3.Icon
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.animation.core.rememberInfiniteTransition
import androidx.compose.animation.core.infiniteRepeatable
import androidx.compose.animation.core.tween
import androidx.compose.animation.core.RepeatMode
import androidx.compose.animation.core.animateFloat
import androidx.compose.animation.core.FastOutSlowInEasing
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
import com.clavierhaus.gnubg.engine.BusyKind
import com.clavierhaus.gnubg.engine.Difficulty
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
    val preBoard: IntArray,      // the pre-move board[50] from the glance --
                                 // the matcher derives played/best boards from
                                 // it via gnubg's own ApplyMove (single source)
    val alts: List<CoachAlt>     // better-than-played candidates, best first, <= 3
) {
    val loss: Float get() = eqBest - eqPlayed
    val flagged: Boolean get() = skill != 3
}

private fun decodeGlance(v: IntArray): CoachGlance? {
    if (v.size < 168) return null
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
        preBoard = preBoard,
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


/** The cube verdict (M4). Only gnubg's own values: what the human did, whether
 *  it matched gnubg's decision, the equity cost, the skill band. No cube
 *  wording is invented here -- the dictionary layer comes later. */
private data class CubeGlance(
    val isBest: Boolean,
    val skill: Int,          // Skill(): 0 verybad 1 bad 2 doubtful 3 none
    val eqChosen: Float,
    val eqBest: Float,
    val takeDrop: Boolean    // false = double-or-not, true = take-or-drop
) {
    val loss: Float get() = eqBest - eqChosen
    val flagged: Boolean get() = skill != 3
}

private fun decodeCube(v: IntArray): CubeGlance? {
    if (v.size < 10) return null
    return CubeGlance(
        isBest = v[0] == 1,
        skill = v[4],
        eqChosen = Float.fromBits(v[2]),
        eqBest = Float.fromBits(v[3]),
        takeDrop = v[9] == 1
    )
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

    // Setup gate (M4): the mode opens on a setup screen -- strength + length --
    // before any board, so match length > 1 (cube in play) is a deliberate
    // choice. The game starts from the setup's Start button, not on entry.
    val showSetup by viewModel.showCoachSetup.collectAsState()
    val coachLength by viewModel.coachLength.collectAsState()
    val coachDifficulty by viewModel.coachDifficulty.collectAsState()
    if (showSetup) {
        CoachSetupScreen(
            settings = settings,
            selectedLength = coachLength,
            selectedDifficulty = coachDifficulty,
            onSelectLength = { viewModel.setCoachLength(it) },
            onSelectDifficulty = { viewModel.setCoachDifficulty(it) },
            onStart = { viewModel.startCoachGame(coachLength, coachDifficulty) },
            onReturnToHub = onReturnToHub,
            onOpenSettings = onOpenSettings
        )
        return
    }

    var glance by remember { mutableStateOf<CoachGlance?>(null) }

    // The verdict arrives via the VM's coachGlance flow -- ONE analysis per
    // move, run by confirm() in the same decoupled slot the tutor analysis
    // occupies in Play. The screen only decodes. (The first design fetched
    // from a LaunchedEffect here, stacking a third 2-ply analysis on top of
    // the Play pipeline's two; on a doubles roll the single engine thread
    // was saturated for 30+ seconds and GNU looked stuck -- field report.)
    val rawGlance by viewModel.coachGlance.collectAsState()
    // Cube glance (M4): present whenever a cube decision is under review.
    val rawCubeGlance by viewModel.coachCubeGlance.collectAsState()
    val cubeAnswer by viewModel.coachCubeAnswer.collectAsState()
    val cubeGlance = rawCubeGlance?.let { decodeCube(it) }
    val busyKind by viewModel.busyKind.collectAsState()

    // A cube double is held for review when a cube glance is present in
    // COACH_REVIEW and it was the double-or-not decision (not take/drop). While
    // it is, the cube "breathes" on the board -- you have offered, GNU's answer
    // is still open. pulse is 0..1, driven continuously; 0 when nothing pends.
    val doublePending = gameState.phase == GamePhase.COACH_REVIEW &&
        cubeGlance != null && !cubeGlance.takeDrop
    val pulse = if (doublePending) {
        val t = rememberInfiniteTransition(label = "cube-double-pending")
        t.animateFloat(
            initialValue = 0f, targetValue = 1f,
            animationSpec = infiniteRepeatable(
                animation = tween(900, easing = FastOutSlowInEasing),
                repeatMode = RepeatMode.Reverse
            ),
            label = "cube-pulse"
        ).value
    } else 0f

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
    // The toggle is a BEFORE/AFTER flip (maintainer design): first tap on a
    // chip shows the decision point -- checkers only, full dice, NO arrows --
    // the common ground identical for every chip; second tap shows the
    // position AFTER that move (gnubg's ApplyMove), dice used, green arrows
    // pointing to the destinations now occupied: where the new position came
    // from. Third tap returns to the live game.
    var selectedAlt by remember { mutableStateOf(-1) }
    var viewAfter by remember { mutableStateOf(false) }
    var afterBoard by remember { mutableStateOf<IntArray?>(null) }
    LaunchedEffect(rawGlance) { selectedAlt = -1; viewAfter = false; afterBoard = null }
    // ONE source of truth for the toggled views (maintainer audit): board,
    // moves AND dice all come from the verdict array gnubg filled -- no
    // parallel UI-side capture that could desynchronize.
    val preMoveBoard = rawGlance?.let { v -> IntArray(50) { v[21 + it] } }
    val glanceDice = rawGlance?.let { v ->
        if (v.size >= 168 && v[166] > 0) Pair(v[166], v[167]) else null
    }
    val selectedMove = glance?.let { g ->
        when {
            selectedAlt == 0 -> g.playedMove
            selectedAlt in 1..g.alts.size -> g.alts[selectedAlt - 1].anMove
            else -> null
        }
    }
    LaunchedEffect(selectedAlt, viewAfter) {
        afterBoard = if (viewAfter && preMoveBoard != null && selectedMove != null) {
            val b = Engine.applyMoveToBoard(preMoveBoard, selectedMove)
            if (b.size == 50) b else null
        } else null
    }

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
            // The left strip: gear on top (as on every screen), Home in the
            // blank area below it (maintainer design: screen estate -- the
            // right panel keeps only the coaching).
            // Left control rail (maintainer: game controls -- Home now, quit/
            // hub/etc. TBD). A tidy vertical column, minimal footprint so the
            // board and coaching pane get the reclaimed estate.
            Column(
                modifier = Modifier
                    .align(Alignment.TopStart)
                    .width(56.dp)
                    .padding(start = 8.dp, top = 12.dp),
                horizontalAlignment = Alignment.CenterHorizontally
            ) {
                if (onOpenSettings != null) {
                    Icon(
                        imageVector = Icons.Filled.Settings,
                        contentDescription = "Settings",
                        tint = pal.uiTextSecondary,
                        modifier = Modifier
                            .size(28.dp)
                            .clickable { onOpenSettings() }
                    )
                }
                Spacer(modifier = Modifier.height(16.dp))
                // Vertical Home -- one letter per line, a spine down the narrow
                // control rail (maintainer aesthetic). Built inline rather than
                // via GameButton: that shared button is maxLines=1 by contract
                // (a wrapping button label is malformed), so it collapses a
                // multi-line label -- which is exactly why the earlier "H\no\nm\ne"
                // GameButton rendered flat. Here the Text owns its own line count.
                Box(
                    modifier = Modifier
                        .background(pal.uiButtonNeutral, RoundedCornerShape(8.dp))
                        .clickable {
                            viewModel.endCoachSession()
                            onReturnToHub()
                        }
                        .padding(horizontal = 10.dp, vertical = 10.dp),
                    contentAlignment = Alignment.Center
                ) {
                    Text(
                        "H\no\nm\ne",
                        color = Color.White,
                        fontSize = 13.sp,
                        fontWeight = FontWeight.Bold,
                        lineHeight = 15.sp,
                        textAlign = androidx.compose.ui.text.style.TextAlign.Center
                    )
                }
            }

            Row(
                modifier = Modifier
                    .fillMaxSize()
                    .padding(start = 56.dp, top = 8.dp, end = 8.dp, bottom = 8.dp)
            ) {
                Box(modifier = Modifier.weight(1f).fillMaxHeight()) {
                    val g = glance
                    val showAfter = viewAfter && afterBoard != null
                    val exploring = selectedAlt >= 0 && preMoveBoard != null && g != null &&
                        (!viewAfter || afterBoard != null)
                    if (exploring) {
                        // BEFORE (first tap): the decision point, identical
                        // for every chip -- checkers only, full-color dice,
                        // NO arrows. AFTER (second tap): the position that
                        // move produces (gnubg's ApplyMove), dice used, green
                        // arrows pointing to the destinations now occupied.
                        // viewModel = null: no game taps on a study board
                        // (the Analyse pattern); ENGINE_THINKING suppresses
                        // all action chrome.
                        val d = glanceDice
                        val fullRoll = d?.let { (d0, d1) ->
                            if (d0 == d1) listOf(d0, d0, d0, d0) else listOf(d0, d1)
                        } ?: emptyList()
                        val shownBoard = if (showAfter) afterBoard!! else preMoveBoard!!
                        // gnubg's pip counts for the DISPLAYED board -- the
                        // BoardState defaults (167) lied on every study view
                        // and made mid-game positions masquerade as openings.
                        val pips = remember(shownBoard) { Engine.pipCount(shownBoard) }
                        BackgammonBoard(
                            settings = settings,
                            gameState = com.clavierhaus.gnubg.engine.BoardState(
                                board = shownBoard,
                                dice = d,
                                remainingDice = if (showAfter) emptyList() else fullRoll,
                                matchScore = gameState.matchScore,
                                matchLength = gameState.matchLength,
                                pipCountHuman = pips[0],
                                pipCountEngine = pips[1],
                                phase = GamePhase.ENGINE_THINKING
                            ),
                            viewModel = null,
                            tutorMode = false,
                            coachTrace = if (showAfter)
                                com.clavierhaus.gnubg.play.CoachTrace(
                                    played = null,
                                    best = selectedMove,
                                    ghost = false
                                ) else null,
                            // On a study view the button RETURNS to the live
                            // position (clears the toggle) rather than handing
                            // the turn on -- so continuing is always a two-step,
                            // unambiguous act: Back to reality, then GNU's turn.
                            onCoachTurn = if (gameState.phase == GamePhase.COACH_REVIEW)
                                { { selectedAlt = -1; viewAfter = false } } else null,
                            coachTurnLabel = "Back",
                            cubePendingPulse = pulse
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
                            tutorMode = false,
                            onCoachTurn = if (gameState.phase == GamePhase.COACH_REVIEW)
                                { {
                                    if (rawCubeGlance != null) viewModel.continueCoachCube()
                                    else viewModel.continueCoachTurn()
                                } } else null,
                            cubePendingPulse = pulse
                        )
                    }
                }

                Spacer(modifier = Modifier.width(12.dp))

                CoachPanel(
                    glance = glance,
                    cubeGlance = cubeGlance,
                    cubeAnswer = cubeAnswer,
                    phase = gameState.phase,
                    winner = gameState.winner,
                    canDouble = gameState.canDouble,
                    busy = busyKind,
                    cubeValue = gameState.cubeValue,
                    onTake = { viewModel.acceptDouble() },
                    onDrop = { viewModel.dropDouble() },
                    humanScore = gameState.humanScore,
                    engineScore = gameState.engineScore,
                    matchLength = gameState.matchLength,
                    selectedAlt = selectedAlt,
                    onSelectAlt = { n ->
                        // A toggle has TWO states (maintainer design): tap a
                        // chip -> BEFORE; same chip again -> AFTER; again ->
                        // BEFORE... The live game is not part of the cycle
                        // (during review it IS the played move's after-state);
                        // a different chip starts that move at BEFORE.
                        if (selectedAlt != n) { selectedAlt = n; viewAfter = false }
                        else viewAfter = !viewAfter
                        android.util.Log.i("gnubg-coach",
                            "screen: toggle sel=$selectedAlt after=$viewAfter " +
                            "fp(pre)=${preMoveBoard?.let { com.clavierhaus.gnubg.engine.GameViewModel.fpOf(it) }} " +
                            "fp(live)=${com.clavierhaus.gnubg.engine.GameViewModel.fpOf(gameState.board)} dice=$glanceDice")
                    },
                    onNewGame = {
                        glance = null
                        viewModel.startCoachGame()
                    }
                )
            }
        }
    }
}

/** A single-character identifier chip: fixed-size circular badge with a
 *  centered letter or digit. Used for the row-labels in MoveList (P for
 *  the player's move, 1..3 for gnubg's better alternatives). Distinct
 *  from GameButton, which is an action button sized for multi-character
 *  labels ("Take", "Drop", "New game") whose compact 19dp horizontal
 *  padding, wrapped in a 30dp-wide slot, previously insetted the label's
 *  own width constraint to zero and drew a coloured pill with no glyph.
 *  A fixed circular size makes that failure mode unrepresentable. */
@Composable
private fun IdentChip(
    label: String,
    color: Color,
    onClick: () -> Unit
) {
    Box(
        modifier = Modifier
            .size(26.dp)
            .background(color, androidx.compose.foundation.shape.CircleShape)
            .clickable { onClick() },
        contentAlignment = Alignment.Center
    ) {
        Text(
            label,
            color = Color.White,
            fontSize = 13.sp,
            fontWeight = FontWeight.Bold,
            maxLines = 1
        )
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

    // Row 0: P -- the player's move, same mechanics, RED marking (maintainer
    // design: visually apart from the numbered, rank-ordered suggestions).
    // Rows are full-width and left-anchored so every chip sits in one
    // vertical column flush to the board's right-hand side.
    Row(modifier = Modifier.fillMaxWidth(), verticalAlignment = Alignment.CenterVertically) {
        IdentChip(
            label = "P",
            color = if (selectedAlt == 0) pal.uiActionNegative
                    else pal.uiActionNegative.copy(alpha = 0.45f)
        ) { onSelectAlt(0) }
        Spacer(modifier = Modifier.width(6.dp))
        Text(
            glance.playedNotation,
            color = if (selectedAlt == 0) Color.White else pal.uiTextSecondary,
            fontSize = 12.sp
        )
    }

    glance.alts.forEachIndexed { i, alt ->
        Spacer(modifier = Modifier.height(3.dp))
        Row(modifier = Modifier.fillMaxWidth(), verticalAlignment = Alignment.CenterVertically) {
            IdentChip(
                label = "${i + 1}",
                color = if (selectedAlt == i + 1) pal.uiChipOn else pal.uiChipOff
            ) { onSelectAlt(i + 1) }
            Spacer(modifier = Modifier.width(6.dp))
            Text(
                "${alt.notation}  ${"%+.3f".format(alt.gain)}",
                color = if (selectedAlt == i + 1) Color.White else pal.uiTextSecondary,
                fontSize = 12.sp
            )
        }
    }
}

/** A score badge matching the tournament scoreboard: a circular avatar with
 *  the side's label inside (GNU / Player), and its points beside it. */
@Composable
private fun ScoreTag(label: String, badgeColor: Color, points: Int) {
    Row(verticalAlignment = Alignment.CenterVertically) {
        Box(
            modifier = Modifier
                .size(34.dp)
                .background(badgeColor, androidx.compose.foundation.shape.CircleShape),
            contentAlignment = Alignment.Center
        ) {
            Text(
                label, color = Color.White, fontSize = 9.sp,
                fontWeight = FontWeight.Bold, maxLines = 1
            )
        }
        Spacer(modifier = Modifier.width(5.dp))
        Text("$points", color = Color.White, fontSize = 18.sp, fontWeight = FontWeight.Bold)
    }
}

@Composable
private fun CoachPanel(
    glance: CoachGlance?,
    cubeGlance: CubeGlance?,
    cubeAnswer: Pair<Boolean, Int>?,
    phase: GamePhase,
    winner: Int,
    canDouble: Boolean,
    busy: BusyKind,
    cubeValue: Int,
    onTake: () -> Unit,
    onDrop: () -> Unit,
    humanScore: Int,
    engineScore: Int,
    matchLength: Int,
    selectedAlt: Int,
    onSelectAlt: (Int) -> Unit,
    onNewGame: () -> Unit
) {
    val pal = LocalBoardPalette.current
    Column(
        modifier = Modifier.width(240.dp).fillMaxHeight().padding(vertical = 8.dp),
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.SpaceBetween
    ) {
        Column(
            horizontalAlignment = Alignment.CenterHorizontally,
            modifier = Modifier.weight(1f, fill = false)
        ) {
            // Header: the match score flanks the centred title -- GNU on the
            // left, you on the right, each a checker-coloured dot with its
            // points. Shown only in a real match (length > 1); a 1-point game
            // has no running score to keep. The Row spans the panel so "Coach"
            // sits centred over the eval area beneath it.
            if (matchLength > 1) {
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.SpaceBetween,
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    ScoreTag("GNU", pal.uiActionRoll, engineScore)
                    Text(
                        "Coach",
                        color = Color.White, fontSize = 20.sp, fontWeight = FontWeight.Bold
                    )
                    ScoreTag("You", pal.uiActionPositive, humanScore)
                }
            } else {
                Text(
                    "Coach",
                    color = Color.White, fontSize = 20.sp, fontWeight = FontWeight.Bold
                )
            }
            Spacer(modifier = Modifier.height(10.dp))

            when {
                phase == GamePhase.GAME_OVER -> {
                    // A game ended by GNU dropping the player's cube says so --
                    // the score already moved; this names the cause, and the
                    // "New game" button below remains the acknowledgment.
                    if (cubeAnswer?.first == false) {
                        Text(
                            "GNU rejects the cube.",
                            color = Color.White, fontSize = 14.sp,
                            fontWeight = FontWeight.Bold
                        )
                        Spacer(modifier = Modifier.height(4.dp))
                    }
                    Text(
                        if (winner == 0) "You win the game." else "GNU wins the game.",
                        color = Color.White, fontSize = 14.sp
                    )
                    Spacer(modifier = Modifier.height(8.dp))
                }
                phase == GamePhase.COACH_REVIEW -> {
                    Text(
                        "Study the eval while GNU waits.",
                        color = pal.uiTextSecondary, fontSize = 11.sp,
                        maxLines = 1
                    )
                    Spacer(modifier = Modifier.height(8.dp))
                }
                phase == GamePhase.ENGINE_THINKING -> {
                    // The turn's visible sequence (maintainer order): the
                    // verdict is computed FIRST -- "Judging your move..." --
                    // then, verdict on screen, GNU rolls and replies.
                    Text(
                        when (busy) {
                            BusyKind.JUDGING  -> "Judging your move..."
                            BusyKind.REPLYING -> "GNU is replying..."
                            BusyKind.NONE     -> "GNU is thinking..."
                        },
                        color = pal.uiTextSecondary, fontSize = 13.sp
                    )
                    Spacer(modifier = Modifier.height(8.dp))
                }
                phase == GamePhase.CUBE_OFFERED -> {
                    // GNU doubles. This was M4's missing quadrant -- your cube
                    // RESPONSES were judged (actions 2/3) but nothing ever
                    // offered the choice: at CUBE_OFFERED the coach pane showed
                    // idle text and no buttons (field report: softlock after
                    // GNU redoubled). gnubg flags the offer (fDoubled) with the
                    // cube still at its pre-take value, so the offered stake is
                    // value*2. Take/Drop route through the coach diversion:
                    // judged and held first, carried out on GNU's turn.
                    Text(
                        "GNU doubles to ${cubeValue * 2}.",
                        color = Color.White, fontSize = 14.sp,
                        fontWeight = FontWeight.Bold
                    )
                    Spacer(modifier = Modifier.height(8.dp))
                    Row(horizontalArrangement = Arrangement.spacedBy(10.dp)) {
                        GameButton("Take", pal.uiActionPositive, compact = true) { onTake() }
                        GameButton("Drop", pal.uiActionNegative, compact = true) { onDrop() }
                    }
                    Spacer(modifier = Modifier.height(8.dp))
                }
                phase == GamePhase.WAITING_FOR_ROLL -> {
                    // The doubling window is HERE and easy to miss: the cube is
                    // tappable before you roll, but nothing said so (field
                    // report: "cube doesn't react" -- it does, but only at this
                    // moment, and the player had already moved). Surface it.
                    // GNU's acceptance of the player's double is part of the
                    // lesson: say it, right where the next roll is prompted.
                    if (cubeAnswer?.first == true) {
                        Text(
                            "GNU takes the cube.",
                            color = Color.White, fontSize = 14.sp,
                            fontWeight = FontWeight.Bold
                        )
                        Spacer(modifier = Modifier.height(4.dp))
                    }
                    Text(
                        if (canDouble) "Your turn. Roll, or tap the cube to double."
                        else "Your turn. Tap Roll.",
                        color = pal.uiTextSecondary, fontSize = 12.sp
                    )
                    Spacer(modifier = Modifier.height(8.dp))
                }
                else -> {}
            }

            // A cube decision under review takes the panel: only gnubg's own
            // values (M4, dictionary later) -- the equity cost and the skill
            // band, the same currency as chequer play. No invented cube words.
            if (cubeGlance != null) {
                val cg = cubeGlance
                when {
                    !cg.flagged -> {
                        // Present tense, about the CHOICE -- the double/take/
                        // drop is held, not yet applied (GNU has not responded).
                        // "Correct cube decision" read as a completed, accepted
                        // transaction (field report); this states the verdict on
                        // the action you are about to commit with GNU's turn.
                        Text(
                            when {
                                !cg.takeDrop && cg.isBest -> "Doubling is correct."
                                !cg.takeDrop              -> "Doubling is reasonable."
                                cg.isBest                 -> "The right response."
                                else                      -> "A reasonable response."
                            },
                            color = pal.uiActionPositive, fontSize = 14.sp,
                            fontWeight = FontWeight.Bold
                        )
                    }
                    else -> {
                        Text(
                            "${skillLabel(cg.skill)}: ${"%+.3f".format(-cg.loss)}",
                            color = Color.White, fontSize = 15.sp,
                            fontWeight = FontWeight.Bold
                        )
                        Spacer(modifier = Modifier.height(4.dp))
                        Text(
                            if (cg.takeDrop) "Take vs drop equity"
                            else "Double vs no-double equity",
                            color = pal.uiTextSecondary, fontSize = 12.sp
                        )
                    }
                }
                Spacer(modifier = Modifier.height(6.dp))
                Text(
                    "Your action: ${"%+.3f".format(cg.eqChosen)}   " +
                        "Best: ${"%+.3f".format(cg.eqBest)}",
                    color = pal.uiTextSecondary, fontSize = 11.sp
                )
            } else {

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

            // Seam: the explanatory "Why" layer was withdrawn from this edition;
            // the panel shows gnubg's evaluations plainly. (docs/COMPANION.md)
            } // end else (chequer verdict; cube verdict handled above)
        }

        // GNU's turn now lives ON the board (left-half mirror of Roll).
        // Only New game remains a pane button, at game over.
        if (phase == GamePhase.GAME_OVER) {
            Column(horizontalAlignment = Alignment.CenterHorizontally) {
                GameButton("New game", pal.uiChipOff, compact = true) { onNewGame() }
            }
        }
    }
}


/**
 * Coach setup -- strength + match length, chosen before the board. Modeled on
 * the tournament MatchSetupScreen (same weighted-column-with-pinned-foot law:
 * nothing scrolls, the Start button is laid out first so it can never be
 * squeezed to zero). No tutor toggle -- coaching IS the mode. Length > 1 puts
 * the cube in play, which the Coach now teaches (M4); the caption says so, the
 * differentiation from the 1-point chequer-only game made explicit.
 */
@Composable
private fun CoachSetupScreen(
    settings: GameSettings,
    selectedLength: Int,
    selectedDifficulty: Difficulty,
    onSelectLength: (Int) -> Unit,
    onSelectDifficulty: (Difficulty) -> Unit,
    onStart: () -> Unit,
    onReturnToHub: () -> Unit,
    onOpenSettings: (() -> Unit)?
) {
    val pal = BoardPalettes.from(settings.boardTheme)
    CompositionLocalProvider(LocalBoardPalette provides pal) {
        Box(
            modifier = Modifier.fillMaxSize().background(pal.uiPanelDeep),
            contentAlignment = Alignment.Center
        ) {
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
            Box(modifier = Modifier.align(Alignment.TopEnd).padding(12.dp)) {
                GameButton("Home", pal.uiButtonNeutral) { onReturnToHub() }
            }

            Column(
                horizontalAlignment = Alignment.CenterHorizontally,
                modifier = Modifier.fillMaxSize().padding(horizontal = 24.dp, vertical = 16.dp)
            ) {
                Column(
                    horizontalAlignment = Alignment.CenterHorizontally,
                    verticalArrangement = Arrangement.spacedBy(10.dp),
                    modifier = Modifier.weight(1f).fillMaxWidth()
                ) {
                    Text(
                        "Train with the Coach",
                        color = Color.White, fontSize = 26.sp, fontWeight = FontWeight.Bold
                    )

                    Text("Opponent strength", color = pal.uiTextSecondary, fontSize = 16.sp)
                    Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                        Difficulty.entries.forEach { d ->
                            GameButton(
                                label = d.label,
                                color = if (selectedDifficulty == d) pal.uiChipOn else pal.uiChipOff,
                                compact = true
                            ) { onSelectDifficulty(d) }
                        }
                    }

                    Spacer(modifier = Modifier.height(6.dp))

                    Text("Match length", color = pal.uiTextSecondary, fontSize = 16.sp)
                    // 1, 3, 5 shortcuts + a flexible chip with steppers, exactly
                    // as the tournament screen. 1 point = cube dead = chequer
                    // only; longer = cube in play, coached.
                    val shortcuts = listOf(1, 3, 5)
                    val flexible = if (selectedLength in shortcuts) 7 else selectedLength
                    Row(
                        horizontalArrangement = Arrangement.spacedBy(12.dp),
                        verticalAlignment = Alignment.Top
                    ) {
                        shortcuts.forEach { n ->
                            GameButton(
                                label = "$n",
                                color = if (selectedLength == n) pal.uiChipOn else pal.uiChipOff
                            ) { onSelectLength(n) }
                        }
                        val onFlexible = selectedLength == flexible
                        GameButton(
                            label = "$flexible",
                            color = if (selectedLength == flexible) pal.uiChipOn else pal.uiChipOff
                        ) { onSelectLength(flexible) }
                        Column(
                            horizontalAlignment = Alignment.CenterHorizontally,
                            verticalArrangement = Arrangement.spacedBy(10.dp)
                        ) {
                            GameButton(
                                label = "+", color = pal.uiButtonNeutral,
                                enabled = onFlexible && flexible < 25
                            ) { onSelectLength((flexible + 1).coerceAtMost(25)) }
                            GameButton(
                                label = "-", color = pal.uiButtonNeutral,
                                enabled = onFlexible && flexible > 1
                            ) { onSelectLength((flexible - 1).coerceAtLeast(1)) }
                        }
                    }

                    Spacer(modifier = Modifier.height(4.dp))
                    Text(
                        if (selectedLength == 1)
                            "1 point: the cube is dead -- chequer play only."
                        else
                            "$selectedLength points: the cube is in play and coached.",
                        color = pal.uiTextDisabled, fontSize = 13.sp
                    )
                }

                Spacer(modifier = Modifier.height(8.dp))
                GameButton("Start", pal.uiActionPositive) { onStart() }
            }
        }
    }
}
