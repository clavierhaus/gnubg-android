package com.clavierhaus.gnubg.analyse

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.text.BasicTextField
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
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Settings
import androidx.compose.material3.Icon
import com.clavierhaus.gnubg.play.BackgammonBoard
import com.clavierhaus.gnubg.play.EDIT_ZONE_BAR_ENGINE
import com.clavierhaus.gnubg.play.EDIT_ZONE_BAR_HUMAN
import com.clavierhaus.gnubg.play.EDIT_ZONE_TRAY
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
    val noDice: Boolean,
    // The cube verdict, present when the position has no dice. gnubg's own
    // semantic, shared with its desktop edit mode: dice set = a chequer
    // decision, no dice = a cube decision.
    val cubeText: String? = null,        // GetCubeRecommendation, gnubg's words
    val cubeWin: FloatArray? = null,     // aarOutput[0][0..6] for the player on roll
    val cubeEq: FloatArray? = null       // arDouble: optimal, no-double, take, drop
)

private const val MAX_CANDIDATES = 8

private fun pct(f: Float): String = String.format("%.1f%%", f * 100f)
private fun eq(f: Float): String = String.format("%+.3f", f)

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
    onBackToHub: () -> Unit,
    onOpenSettings: (() -> Unit)? = null
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

        // No dice = a cube decision. cubeDecision reads cubeinfo from the LIVE
        // matchstate (GetMatchStateCubeInfo(&ci, &ms)), which is exactly the
        // state setGnubgId just installed, and takes the on-roll-frame board.
        var cubeText: String? = null
        var cubeWin: FloatArray? = null
        var cubeEq: FloatArray? = null
        if (d0 == 0) {
            val cd = Engine.cubeDecision(raw)
            if (cd != null && cd.size >= 19) {
                cubeText = Engine.cubeRecommendation(cd[18])
                cubeWin = FloatArray(7) { i -> Float.fromBits(cd[i]) }
                cubeEq = FloatArray(4) { i -> Float.fromBits(cd[14 + i]) }
            }
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
            noDice = n == 0,
            cubeText = cubeText,
            cubeWin = cubeWin,
            cubeEq = cubeEq
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

    // ------------------------- Position editor -------------------------
    //
    // Pure UI bookkeeping. Nothing here decides backgammon: the edited state is
    // encoded by gnubg (Engine.idsFromState -> PositionID + MatchIDFromMatchState)
    // and installed through the same setGnubgId path a pasted ID takes, where
    // gnubg's own CheckPosition validates it. The editor only keeps taps from
    // producing what gnubg would refuse (a 16th checker, both colours on a point).
    var editing by remember { mutableStateOf(false) }
    var editBoard by remember { mutableStateOf(IntArray(50)) }
    var editTool by remember { mutableStateOf(0) }        // 0 add white, 1 add black, 2 erase
    var editD0 by remember { mutableStateOf(0) }          // 0 = no dice = cube decision
    var editD1 by remember { mutableStateOf(0) }
    var editTurn by remember { mutableStateOf(0) }        // 0 you, 1 GNU
    var editScoreH by remember { mutableStateOf(0) }
    var editScoreE by remember { mutableStateOf(0) }
    var editMatchTo by remember { mutableStateOf(7) }     // 0 = money game
    var editCube by remember { mutableStateOf(1) }
    var editCubeOwner by remember { mutableStateOf(-1) }  // -1 centred, 0 you, 1 GNU
    var editCrawford by remember { mutableStateOf(false) }

    fun beginEdit() {
        val r = result
        if (r != null) {
            // Start from what is on the screen.
            editBoard = r.board.copyOf()
            editD0 = r.dice?.first ?: 0
            editD1 = r.dice?.second ?: 0
            editTurn = r.onRoll
            editScoreH = r.score.first
            editScoreE = r.score.second
            editMatchTo = r.matchTo
            editCube = r.cube
            editCubeOwner = r.cubeOwner
            editCrawford = r.crawford
        } else {
            // Empty board. The use case is recreating a position that exists in
            // front of the user -- a chouette, a book diagram, a screenshot --
            // and that means PLACING what they see, not first erasing thirty
            // checkers of opening position. gnubg's own manual says it: setting
            // up is often easier from an empty board. (An earlier version seeded
            // the standard opening here, reintroducing the exact chore the XG
            // Mobile editor is criticised for.) Editing a position already on
            // screen still starts from that position, above.
            editBoard = IntArray(50)
            editD0 = 0; editD1 = 0
            editTurn = 0
            editScoreH = 0; editScoreE = 0
            editMatchTo = 7
            editCube = 1; editCubeOwner = -1
            editCrawford = false
        }
        status = null
        askSwap = false
        editing = true
    }

    fun editTap(zone: Int) {
        val b = editBoard.copyOf()
        val humanTotal = (0..24).sumOf { b[25 + it] }
        val engineTotal = (0..24).sumOf { b[it] }
        when (zone) {
            EDIT_ZONE_TRAY -> {
                // gnubg's own edit-mode gesture: tapping a tray clears the board.
                editBoard = IntArray(50)
                return
            }
            EDIT_ZONE_BAR_HUMAN -> when (editTool) {
                2 -> if (b[49] > 0) b[49]--
                else -> if (humanTotal < 15) b[49]++
            }
            EDIT_ZONE_BAR_ENGINE -> when (editTool) {
                2 -> if (b[24] > 0) b[24]--
                else -> if (engineTotal < 15) b[24]++
            }
            in 1..24 -> {
                val h = 25 + (zone - 1)          // human checkers on this point
                val e = 24 - zone                // engine frame: its point 25-zone
                when (editTool) {
                    0 -> {                        // add white (human)
                        if (b[e] > 0) { b[e] = 0; b[h] = 1 }   // an editor replaces
                        else if (humanTotal < 15) b[h]++
                    }
                    1 -> {                        // add black (engine)
                        if (b[h] > 0) { b[h] = 0; b[e] = 1 }
                        else if (engineTotal < 15) b[e]++
                    }
                    else -> {                     // erase whichever occupies
                        if (b[h] > 0) b[h]-- else if (b[e] > 0) b[e]--
                    }
                }
            }
            else -> return
        }
        editBoard = b
    }

    fun evaluateEdit() {
        if (busy) return
        busy = true
        scope.launch {
            val ids = withContext(Dispatchers.Default) {
                Engine.idsFromState(
                    editBoard, editD0, editD1, editTurn,
                    editScoreH, editScoreE, editMatchTo,
                    editCube, editCubeOwner, if (editCrawford) 1 else 0
                )
            }
            busy = false
            if (ids == null) {
                status = "gnubg could not encode this position."
                return@launch
            }
            idText = ids       // visible and copyable, like a pasted one
            editing = false
            applyId()          // the same install + validation path as pasting
        }
    }

    CompositionLocalProvider(LocalBoardPalette provides palette) {
        val pal = LocalBoardPalette.current
        Box(modifier = Modifier.fillMaxSize()) {
        Row(
            modifier = Modifier
                .fillMaxSize()
                .background(pal.uiPanelDeep)
                .padding(16.dp)
        ) {
            Box(
                modifier = Modifier
                    .weight(1f)
                    .fillMaxHeight()
                    // Below the gear (which ends at 36dp): the position window is
                    // centred in the space that remains, and the gear overlaps
                    // nothing tappable.
                    .padding(top = 44.dp),
                contentAlignment = Alignment.Center
            ) {
                val r = result
                if (editing) {
                    BackgammonBoard(
                        settings = settings,
                        gameState = BoardState(
                            board = editBoard,
                            dice = if (editD0 > 0 && editD1 > 0) Pair(editD0, editD1) else null,
                            matchScore = intArrayOf(editScoreH, editScoreE),
                            matchLength = editMatchTo,
                            cubeValue = editCube,
                            cubeOwner = editCubeOwner,
                            turn = editTurn
                        ),
                        viewModel = null,
                        tutorMode = false,
                        onEditTap = { zone -> editTap(zone) }
                    )
                } else if (r == null) {
                    Text(
                        "Paste a position to analyse, or set one up.",
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

            // This pane does NOT scroll. Nothing in this app scrolls: the game
            // view law. The editor fits by being 20% smaller, not by moving.
            Column(
                modifier = Modifier
                    .weight(1f)
                    .fillMaxHeight(),
                verticalArrangement = Arrangement.spacedBy(8.dp)
            ) {
                Text(
                    "Analyse Position",
                    color = Color.White,
                    fontSize = 24.sp,
                    fontWeight = FontWeight.Bold
                )
                // Everything state-specific lives in this weighted region; the
                // Home row below is a fixed sibling measured first, so it is
                // ALWAYS on screen. Field report: after analysing a pasted
                // position there was no visible way back -- the user killed the
                // app to leave. The exit is no longer hostage to which sub-state
                // (paste / editor / result) is showing.
                Column(
                    verticalArrangement = Arrangement.spacedBy(8.dp),
                    modifier = Modifier.weight(1f).fillMaxWidth()
                ) {
                if (editing) {
                    // Controls in a single weighted region (no nested weight --
                    // two competing weight(1f) let the last row overflow UNDER
                    // the pinned Analyse row, which is exactly what clipped the
                    // Cube row: measured at ~0.067 of screen height over budget,
                    // issue #1). Tight 4dp pitch + no instruction line reclaim far
                    // more than that, proportionally, on any aspect ratio.
                    Column(
                        verticalArrangement = Arrangement.spacedBy(4.dp),
                        modifier = Modifier
                            .weight(1f)
                            .fillMaxWidth()
                    ) {
                    // Tool + Start-position preset on one row: place checkers, or
                    // fill the standard opening (issue #1 asked for the preset).
                    Row(
                        horizontalArrangement = Arrangement.spacedBy(6.dp),
                        verticalAlignment = Alignment.CenterVertically
                    ) {
                        GameButton("White", if (editTool == 0) pal.uiChipOn else pal.uiChipOff, compact = true) { editTool = 0 }
                        GameButton("Black", if (editTool == 1) pal.uiChipOn else pal.uiChipOff, compact = true) { editTool = 1 }
                        GameButton("Erase", if (editTool == 2) pal.uiChipOn else pal.uiChipOff, compact = true) { editTool = 2 }
                        GameButton("Start pos", pal.uiButtonNeutral, compact = true) {
                            // The standard opening, in the editor's own board
                            // encoding (b[25+(p-1)] human point p; b[24-p] engine
                            // frame; verified against editTap): 2 on 24, 5 on 13,
                            // 3 on 8, 5 on 6, mirrored.
                            editBoard = intArrayOf(
                                2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 3, 0, 5, 0, 0, 0, 0, 0, 0,
                                0, 0, 0, 0, 0, 5, 0, 3, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0
                            )
                        }
                    }

                    // On roll + Dice on ONE row -- both are small "who / what"
                    // selectors, and merging them removes a full row of height
                    // (measured ~0.088 of screen), which is what let the Cube row
                    // fall under the pinned buttons on taller phones (issue #1).
                    // A divider keeps the two groups legible.
                    Row(
                        horizontalArrangement = Arrangement.spacedBy(6.dp),
                        verticalAlignment = Alignment.CenterVertically
                    ) {
                        Text("On roll", color = pal.uiTextSecondary, fontSize = 13.sp)
                        GameButton("You", if (editTurn == 0) pal.uiChipOn else pal.uiChipOff, compact = true) { editTurn = 0 }
                        GameButton("GNU", if (editTurn == 1) pal.uiChipOn else pal.uiChipOff, compact = true) { editTurn = 1 }
                        Spacer(modifier = Modifier.width(4.dp))
                        Text("Dice", color = pal.uiTextSecondary, fontSize = 13.sp)
                        // Tap to cycle: -, 1..6. No dice is a CUBE decision -- the
                        // same rule as gnubg's desktop edit mode.
                        GameButton(if (editD0 == 0) "-" else "" + editD0, pal.uiChipOff, compact = true) {
                            editD0 = (editD0 + 1) % 7
                            // Dice come in pairs or not at all: (1,0) is not a roll.
                            editD1 = if (editD0 == 0) 0 else if (editD1 == 0) editD0 else editD1
                        }
                        GameButton(if (editD1 == 0) "-" else "" + editD1, pal.uiChipOff, compact = true) {
                            if (editD0 > 0) editD1 = if (editD1 >= 6) 1 else editD1 + 1
                        }
                    }

                    // The description sits UNDER the merged row as a caption for
                    // the whole roll/dice choice -- elegant because it explains the
                    // consequence (which decision gnubg will make) rather than
                    // labelling one control, and it costs almost no height.
                    Text(
                        if (editD0 == 0) "No dice → gnubg makes the cube decision"
                        else "Dice set → gnubg ranks the chequer plays",
                        color = pal.uiTextDisabled, fontSize = 11.sp
                    )

                    Row(
                        horizontalArrangement = Arrangement.spacedBy(6.dp),
                        verticalAlignment = Alignment.CenterVertically
                    ) {
                        Text("Match to", color = pal.uiTextSecondary, fontSize = 13.sp)
                        GameButton("-", pal.uiButtonNeutral, editMatchTo > 0, compact = true) {
                            editMatchTo--
                            if (editMatchTo == 0) editCrawford = false
                        }
                        Text(
                            if (editMatchTo == 0) "money" else "" + editMatchTo,
                            color = Color.White, fontSize = 14.sp
                        )
                        GameButton("+", pal.uiButtonNeutral, editMatchTo < 25, compact = true) { editMatchTo++ }
                        if (editMatchTo > 0) {
                            GameButton(
                                "Crawford",
                                if (editCrawford) pal.uiChipOn else pal.uiChipOff,
                                compact = true
                            ) { editCrawford = !editCrawford }
                        }
                    }

                    if (editMatchTo > 0) {
                        Row(
                            horizontalArrangement = Arrangement.spacedBy(6.dp),
                            verticalAlignment = Alignment.CenterVertically
                        ) {
                            Text("Score you", color = pal.uiTextSecondary, fontSize = 13.sp)
                            GameButton("-", pal.uiButtonNeutral, editScoreH > 0, compact = true) { editScoreH-- }
                            Text("" + editScoreH, color = Color.White, fontSize = 14.sp)
                            GameButton("+", pal.uiButtonNeutral, editScoreH < editMatchTo - 1, compact = true) { editScoreH++ }
                            Text("GNU", color = pal.uiTextSecondary, fontSize = 13.sp)
                            GameButton("-", pal.uiButtonNeutral, editScoreE > 0, compact = true) { editScoreE-- }
                            Text("" + editScoreE, color = Color.White, fontSize = 14.sp)
                            GameButton("+", pal.uiButtonNeutral, editScoreE < editMatchTo - 1, compact = true) { editScoreE++ }
                        }
                    }

                    Row(
                        horizontalArrangement = Arrangement.spacedBy(6.dp),
                        verticalAlignment = Alignment.CenterVertically
                    ) {
                        Text("Cube", color = pal.uiTextSecondary, fontSize = 13.sp)
                        GameButton("" + editCube, pal.uiChipOff, compact = true) {
                            editCube = if (editCube >= 64) 1 else editCube * 2
                            if (editCube == 1) editCubeOwner = -1
                        }
                        GameButton("Centred", if (editCubeOwner == -1) pal.uiChipOn else pal.uiChipOff, compact = true) { editCubeOwner = -1 }
                        GameButton("You", if (editCubeOwner == 0) pal.uiChipOn else pal.uiChipOff, compact = true) { editCubeOwner = 0 }
                        GameButton("GNU", if (editCubeOwner == 1) pal.uiChipOn else pal.uiChipOff, compact = true) { editCubeOwner = 1 }
                    }

                    } // end weighted editor controls

                    Row(horizontalArrangement = Arrangement.spacedBy(12.dp)) {
                        GameButton(
                            label = if (busy) "Working..." else "Analyse",
                            color = pal.uiActionPositive,
                            enabled = !busy,
                            compact = true
                        ) { evaluateEdit() }
                        GameButton("Cancel", pal.uiButtonNeutral, !busy, compact = true) { editing = false }
                    }
                } else {
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
                        label = "Set up",
                        color = pal.uiChipOff,
                        enabled = !busy
                    ) { beginEdit() }
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
                    // The result takes exactly the space left below the buttons.
                    // Field report: "the analysis doesn't show up" -- it rendered
                    // BELOW the pane and was clipped, because nothing scrolls and
                    // nothing bounded it. Now the verdict and context are always
                    // visible; on the shortest panes the tail of the candidate
                    // list is what gives.
                    Column(
                        verticalArrangement = Arrangement.spacedBy(6.dp),
                        modifier = Modifier
                            .weight(1f)
                            .fillMaxWidth()
                    ) {
                    MatchContext(r)

                    Text(
                        if (r.cubeText != null) "gnubg's verdict" else "gnubg's candidates",
                        color = pal.uiTextSecondary,
                        fontSize = 16.sp,
                        fontWeight = FontWeight.Bold
                    )

                    if (r.cubeText != null) {
                        // No dice = a cube decision, and this is gnubg's verdict on
                        // it: GetCubeRecommendation's own words, the winning chances
                        // behind them, and the three equities it compared.
                        Text(
                            r.cubeText,
                            color = Color.White,
                            fontSize = 16.sp,
                            fontWeight = FontWeight.Bold
                        )
                        r.cubeWin?.let { w ->
                            Text(
                                "Win " + pct(w[0]) + "  (G " + pct(w[1]) + ", BG " + pct(w[2]) + ")",
                                color = pal.uiTextSecondary, fontSize = 13.sp
                            )
                            Text(
                                "Lose gammon " + pct(w[3]) + ", backgammon " + pct(w[4]),
                                color = pal.uiTextSecondary, fontSize = 13.sp
                            )
                        }
                        r.cubeEq?.let { e ->
                            Text(
                                "No double " + eq(e[1]) + "   Double/take " + eq(e[2]) +
                                    "   Double/pass " + eq(e[3]),
                                color = pal.uiTextSecondary, fontSize = 13.sp
                            )
                        }
                    } else if (r.noDice) {
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
                    } // end weighted result region
                }
                } // end !editing
                } // end weighted state region

                // Pinned foot: always-present exit. compact so it never crowds
                // the content above it on a short pane.
                GameButton(
                    label = "Home",
                    color = pal.uiButtonNeutral,
                    enabled = !busy,
                    compact = true
                ) { onBackToHub() }
            }
        }

        // The gear is top-left on EVERY screen -- no exceptions, by order. The
        // board pane below is padded down past it, so the gear cannot sit on
        // point 13 and steal editor taps: the thing in the way moved, not the
        // gear.
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
        } // end root Box
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
