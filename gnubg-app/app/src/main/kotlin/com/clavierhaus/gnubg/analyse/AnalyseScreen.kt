package com.clavierhaus.gnubg.analyse

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.text.BasicTextField
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Text
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.SolidColor
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.clavierhaus.gnubg.Engine
import com.clavierhaus.gnubg.engine.BoardState
import com.clavierhaus.gnubg.engine.GameSettings
import com.clavierhaus.gnubg.play.BackgammonBoard
import com.clavierhaus.gnubg.play.BoardPalettes
import com.clavierhaus.gnubg.play.GameButton
import com.clavierhaus.gnubg.play.LocalBoardPalette
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

/*
 * Analyse Position.
 *
 * A thin face over gnubg. Engine.setGnubgId wraps SetGNUbgID, which accepts a
 * GNU BG ID ("PositionID:MatchID") or an XGID and discriminates the dialects
 * itself. Nothing here parses an ID, ranks a move, or judges a position: the
 * board comes from getMatchBoardHuman, the context from getMatchState, the
 * candidates from hintMoves (FindnSaveBestMoves), and each move's text from
 * formatMove.
 *
 * Two display rules, both taken from gnubg's own behaviour:
 *
 *  - The match context is shown every time, unasked. A bare Position ID sets the
 *    board and leaves score, cube and match length as they were, so gnubg would
 *    evaluate correctly for a context the user never stated. gnubg's own
 *    CommandSetGNUbgID ends with ShowBoard(), which prints the position together
 *    with the score and cube, for exactly this reason.
 *
 *  - SetGNUbgID returns 2 when the position has the player on roll drawn on top.
 *    gnubg asks whether to swap. So does this screen; it is never answered on
 *    the user's behalf.
 */

private data class Candidate(val text: String, val equity: Float)

private class AnalyseResult(
    val board: IntArray,
    val dice: Pair<Int, Int>?,
    val matchTo: Int,
    val score: Pair<Int, Int>,
    val cube: Int,
    val cubeOwner: Int,
    val crawford: Boolean,
    val onRoll: Int,
    val candidates: List<Candidate>,
    val noDice: Boolean
)

private const val MAX_CANDIDATES = 8

/* gamestate, in gnubg's declared order (lib/gnubg-types.h):
 * GAME_NONE, GAME_PLAYING, GAME_OVER, GAME_RESIGNED, GAME_DROP */
private const val GAME_NONE = 0
private const val GAME_PLAYING = 1
private const val GAME_OVER = 2
private const val GAME_RESIGNED = 3
private const val GAME_DROP = 4

private fun gameStateSuffix(gs: Int): String = when (gs) {
    GAME_NONE -> " (no game started)"
    GAME_OVER -> " (the game is over)"
    GAME_RESIGNED -> " (the game was resigned)"
    GAME_DROP -> " (the cube was dropped)"
    else -> ""
}

@Composable
fun AnalyseScreen(
    settings: GameSettings,
    onBackToHub: () -> Unit
) {
    val palette = remember(settings.boardTheme) { BoardPalettes.from(settings.boardTheme) }
    val scope = rememberCoroutineScope()

    var idText by remember { mutableStateOf("") }
    var status by remember { mutableStateOf<String?>(null) }
    var askSwap by remember { mutableStateOf(false) }
    var result by remember { mutableStateOf<AnalyseResult?>(null) }
    var busy by remember { mutableStateOf(false) }

    suspend fun readBack(): AnalyseResult = withContext(Dispatchers.Default) {
        val st = Engine.getMatchState()
        val display = Engine.getMatchBoardHuman()
        val raw = Engine.getMatchBoard()
        val d0 = st[3]
        val d1 = st[4]

        val equities = FloatArray(MAX_CANDIDATES)
        val moves = IntArray(MAX_CANDIDATES * 8)
        val n = Engine.hintMoves(MAX_CANDIDATES, equities, moves)

        val cands = if (n > 0) {
            (0 until n).map { i ->
                val mv = IntArray(8) { j -> moves[i * 8 + j] }
                Candidate(Engine.formatMove(raw, mv), equities[i])
            }
        } else {
            emptyList()
        }

        AnalyseResult(
            board = display,
            dice = if (d0 > 0 && d1 > 0) Pair(d0, d1) else null,
            matchTo = st[12],
            score = Pair(st[10], st[11]),
            cube = st[7],
            cubeOwner = st[6],
            crawford = st[8] != 0,
            onRoll = st[2],
            candidates = cands,
            noDice = n == 0
        )
    }

    fun applyId() {
        if (busy || idText.isBlank()) return
        busy = true
        scope.launch {
            val rc = withContext(Dispatchers.Default) { Engine.setGnubgId(idText.trim()) }

            // SetGNUbgID discards SetBoard's return value (set.c:4873), so a 0 or 2
            // does NOT prove the position was installed. SetBoard refuses unless
            // ms.gs == GAME_PLAYING (set.c), and SetMatchID has already reset the
            // board to the opening position and set gs from the Match ID by then
            // (play.c:4205 -- FreeMatch, InitBoard, ms.gs = gs). So an ID carrying
            // a finished game silently yields the STARTING position with the final
            // score, and a bare Position ID with no game in progress silently
            // changes nothing. gnubg's own precondition is the test: if gs is not
            // GAME_PLAYING afterwards, no position was set.
            val gs = if (rc == 0 || rc == 2) {
                withContext(Dispatchers.Default) { Engine.getMatchState()[0] }
            } else {
                GAME_PLAYING
            }

            when {
                rc == 1 -> {
                    askSwap = false
                    result = null
                    status = "No valid ID found. Paste a GNU BG ID or an XGID."
                }
                rc != 0 && rc != 2 -> {
                    askSwap = false
                    result = null
                    status = "Could not read that ID."
                }
                gs != GAME_PLAYING -> {
                    askSwap = false
                    result = null
                    status = "gnubg did not set this position: it describes a game " +
                        "that is not in progress" + gameStateSuffix(gs) + ". A position " +
                        "can only be set while a game is under way, so an ID captured " +
                        "after a game ended will not load."
                }
                rc == 2 -> {
                    askSwap = true
                    status = null
                    result = readBack()
                }
                else -> {
                    askSwap = false
                    status = null
                    result = readBack()
                }
            }
            busy = false
        }
    }

    fun doSwap() {
        if (busy) return
        busy = true
        scope.launch {
            withContext(Dispatchers.Default) { Engine.swapPlayers() }
            askSwap = false
            result = readBack()
            busy = false
        }
    }

    CompositionLocalProvider(LocalBoardPalette provides palette) {
        val pal = LocalBoardPalette.current
        Row(
            modifier = Modifier
                .fillMaxSize()
                .background(pal.uiPanelDeep)
                .padding(16.dp)
        ) {
            Box(
                modifier = Modifier
                    .weight(1f)
                    .fillMaxHeight(),
                contentAlignment = Alignment.Center
            ) {
                val r = result
                if (r == null) {
                    Text(
                        "Paste a position to analyse.",
                        color = pal.uiTextSecondary,
                        fontSize = 16.sp
                    )
                } else {
                    BackgammonBoard(
                        settings = settings,
                        gameState = BoardState(
                            board = r.board,
                            dice = r.dice,
                            matchScore = intArrayOf(r.score.first, r.score.second),
                            matchLength = r.matchTo,
                            cubeValue = r.cube,
                            cubeOwner = r.cubeOwner
                        ),
                        viewModel = null,
                        tutorMode = false
                    )
                }
            }

            Spacer(modifier = Modifier.width(16.dp))

            Column(
                modifier = Modifier
                    .weight(1f)
                    .fillMaxHeight()
                    .verticalScroll(rememberScrollState()),
                verticalArrangement = Arrangement.spacedBy(12.dp)
            ) {
                Text(
                    "Analyse Position",
                    color = Color.White,
                    fontSize = 24.sp,
                    fontWeight = FontWeight.Bold
                )
                Text(
                    "Paste a GNU BG ID or an XGID. gnubg decides which it is.",
                    color = pal.uiTextSecondary,
                    fontSize = 13.sp
                )

                Box(
                    modifier = Modifier
                        .fillMaxWidth()
                        .background(pal.uiPanel, RoundedCornerShape(8.dp))
                        .padding(horizontal = 12.dp, vertical = 10.dp)
                ) {
                    if (idText.isEmpty()) {
                        Text(
                            "4HPwATDgc/ABMA:cIkqAAAAAAAA",
                            color = pal.uiTextDisabled,
                            fontSize = 14.sp
                        )
                    }
                    BasicTextField(
                        value = idText,
                        onValueChange = { idText = it },
                        singleLine = false,
                        textStyle = TextStyle(color = Color.White, fontSize = 14.sp),
                        cursorBrush = SolidColor(Color.White),
                        modifier = Modifier.fillMaxWidth()
                    )
                }

                Row(horizontalArrangement = Arrangement.spacedBy(12.dp)) {
                    GameButton(
                        label = if (busy) "Working..." else "Analyse",
                        color = pal.uiActionPositive,
                        enabled = !busy && idText.isNotBlank()
                    ) { applyId() }

                    GameButton(
                        label = "Copy current",
                        color = pal.uiActionRoll,
                        enabled = !busy
                    ) {
                        scope.launch {
                            val ids = withContext(Dispatchers.Default) { Engine.currentIds() }
                            if (ids != null && ids.size == 2) {
                                idText = ids[0] + ":" + ids[1]
                            }
                        }
                    }

                    GameButton(
                        label = "Back",
                        color = pal.uiButtonNeutral,
                        enabled = !busy
                    ) { onBackToHub() }
                }

                val msg = status
                if (msg != null) {
                    Text(msg, color = pal.uiActionNegative, fontSize = 14.sp)
                }

                if (askSwap) {
                    Text(
                        "This position has the player on roll at the top. Swap sides?",
                        color = Color.White,
                        fontSize = 14.sp
                    )
                    Row(horizontalArrangement = Arrangement.spacedBy(12.dp)) {
                        GameButton("Swap", pal.uiActionPositive, !busy) { doSwap() }
                        GameButton("Leave as is", pal.uiButtonNeutral, !busy) { askSwap = false }
                    }
                }

                val r = result
                if (r != null) {
                    MatchContext(r)

                    Text(
                        "gnubg's candidates",
                        color = pal.uiTextSecondary,
                        fontSize = 16.sp,
                        fontWeight = FontWeight.Bold
                    )

                    if (r.noDice) {
                        Text(
                            "No dice in this position, so there is no chequer play to rank.",
                            color = pal.uiTextDisabled,
                            fontSize = 13.sp
                        )
                    } else {
                        val best = if (r.candidates.isEmpty()) 0f else r.candidates[0].equity
                        r.candidates.forEachIndexed { i, c ->
                            Row(modifier = Modifier.fillMaxWidth()) {
                                Text(
                                    "" + (i + 1) + ". " + c.text,
                                    color = if (i == 0) Color.White else pal.uiTextSecondary,
                                    fontSize = 14.sp,
                                    modifier = Modifier.weight(1f)
                                )
                                Text(
                                    if (i == 0) String.format("%+.3f", c.equity)
                                    else String.format("%+.3f", c.equity - best),
                                    color = if (i == 0) Color.White else pal.uiTextDisabled,
                                    fontSize = 14.sp
                                )
                            }
                        }
                    }
                }
            }
        }
    }
}

/* Every field here is gnubg's, read back after the ID was installed. */
@Composable
private fun MatchContext(r: AnalyseResult) {
    val pal = LocalBoardPalette.current
    Column(verticalArrangement = Arrangement.spacedBy(2.dp)) {
        Text(
            "Match context",
            color = pal.uiTextSecondary,
            fontSize = 16.sp,
            fontWeight = FontWeight.Bold
        )
        Text(
            if (r.matchTo == 0) "Money play" else "" + r.matchTo + "-point match",
            color = Color.White,
            fontSize = 14.sp
        )
        if (r.matchTo > 0) {
            Text(
                "Score " + r.score.first + " - " + r.score.second,
                color = Color.White,
                fontSize = 14.sp
            )
            if (r.crawford) {
                Text("Crawford game", color = Color.White, fontSize = 14.sp)
            }
        }
        Text(
            "Cube " + r.cube + ", " + when (r.cubeOwner) {
                -1 -> "centred"
                0 -> "owned by you"
                else -> "owned by opponent"
            },
            color = Color.White,
            fontSize = 14.sp
        )
        Text(
            if (r.onRoll == 0) "You are on roll" else "Opponent is on roll",
            color = Color.White,
            fontSize = 14.sp
        )
        val d = r.dice
        if (d != null) {
            Text("Dice " + d.first + " " + d.second, color = Color.White, fontSize = 14.sp)
        }
    }
}
