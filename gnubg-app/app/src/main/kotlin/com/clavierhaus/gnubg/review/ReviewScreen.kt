package com.clavierhaus.gnubg.review

import androidx.compose.foundation.background
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
import androidx.compose.foundation.layout.width
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.size
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Settings
import androidx.compose.material3.Icon
import com.clavierhaus.gnubg.Engine
import com.clavierhaus.gnubg.engine.BoardState
import com.clavierhaus.gnubg.engine.GamePhase
import com.clavierhaus.gnubg.engine.GameSettings
import com.clavierhaus.gnubg.play.BackgammonBoard
import com.clavierhaus.gnubg.play.BoardPalettes
import com.clavierhaus.gnubg.play.GameButton
import com.clavierhaus.gnubg.play.LocalBoardPalette
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

/**
 * Feature [3]: step through a match, with gnubg's own record as the authority.
 *
 * gnubg navigates its game record with CommandNext and CommandPrevious. This
 * screen calls them and reads the matchstate back; it holds no cursor of its own,
 * counts nothing, and decides nothing. There is no notion here of "which move we
 * are on" beyond what gnubg reports, because gnubg is the one that knows.
 *
 * Both commands refuse when plGame is NULL, report it through outputl, and return
 * void -- so "did it move" is answered by reading the state afterwards, never by
 * the return value.
 *
 * WARNING, and it is real: Engine.loadMatch replaces whatever match the engine
 * holds. Opening a file here discards a game in progress. The screen says so
 * before it opens anything.
 */

private data class ReviewPos(
    val board: IntArray,
    val dice: Pair<Int, Int>?,
    val score: Pair<Int, Int>,
    val matchTo: Int,
    val cubeValue: Int,
    val cubeOwner: Int,
    val turn: Int,
    val gameState: Int      // gnubg's ms.gs: 0 none, 1 playing, >=2 over
)

@Composable
fun ReviewScreen(
    settings: GameSettings,
    onOpenMatch: () -> Unit,
    matchPath: String?,
    onReturnToHub: () -> Unit,
    onOpenSettings: (() -> Unit)? = null
) {
    val pal = BoardPalettes.from(settings.boardTheme)
    val scope = rememberCoroutineScope()

    var pos by remember { mutableStateOf<ReviewPos?>(null) }
    var status by remember { mutableStateOf("No match loaded.") }
    var busy by remember { mutableStateOf(false) }

    suspend fun readBack(): ReviewPos = withContext(Dispatchers.Default) {
        val st = Engine.getMatchState()
        ReviewPos(
            board     = Engine.getMatchBoardHuman(),
            dice      = if (st[3] > 0) Pair(st[3], st[4]) else null,
            score     = Pair(st[10], st[11]),
            matchTo   = st[12],
            cubeValue = st[7],
            cubeOwner = st[6],
            turn      = st[1],
            gameState = st[0]
        )
    }

    // The file is chosen by MainActivity through the Storage Access Framework and
    // copied to a whitespace-free cache path, because gnubg's SGF reader takes a
    // filesystem path, not a content:// URI.
    // Keyed on the path: opening a different file reloads. A coroutine must never
    // be launched from composition itself.
    androidx.compose.runtime.LaunchedEffect(matchPath) {
        if (matchPath == null) return@LaunchedEffect
        busy = true
        pos = null
        status = "Loading..."
        val ok = withContext(Dispatchers.Default) { Engine.loadMatch(matchPath) }
        // loadMatch goes through FACADE_FILE_OP, which returns 1 unconditionally, so
        // its Boolean means "the call was made". Whether a match arrived is answered
        // by the matchstate, not by the return value.
        val p = readBack()
        if (!ok || p.gameState == 0) {
            status = "gnubg could not read that file as a match."
        } else {
            pos = p
            status = ""
        }
        busy = false
    }

    fun step(backwards: Boolean) {
        if (busy || pos == null) return
        busy = true
        scope.launch {
            withContext(Dispatchers.Default) {
                if (backwards) Engine.commandPrevious() else Engine.commandNext()
            }
            pos = readBack()
            busy = false
        }
    }

    fun stepGame(backwards: Boolean) {
        if (busy || pos == null) return
        busy = true
        scope.launch {
            withContext(Dispatchers.Default) {
                if (backwards) Engine.commandPrevious("game") else Engine.commandNext("game")
            }
            pos = readBack()
            busy = false
        }
    }

    androidx.compose.runtime.CompositionLocalProvider(LocalBoardPalette provides pal) {
        Box(modifier = Modifier.fillMaxSize()) {
        Row(modifier = Modifier.fillMaxSize().background(pal.uiPanelDeep)) {

            // Left pane: the controls. Static, as the game view is.
            Column(
                modifier = Modifier.width(200.dp).fillMaxHeight().padding(12.dp),
                horizontalAlignment = Alignment.CenterHorizontally,
                verticalArrangement = Arrangement.SpaceBetween
            ) {
                Column(horizontalAlignment = Alignment.CenterHorizontally) {
                    Text("Review Match", color = Color.White, fontSize = 16.sp,
                        fontWeight = FontWeight.Bold, maxLines = 1)

                    Spacer(Modifier.height(8.dp))

                    pos?.let { p ->
                        val len = if (p.matchTo > 0) "${p.matchTo}-point match" else "Unlimited"
                        Text(len, color = pal.uiTextSecondary, fontSize = 12.sp, maxLines = 1)
                        Text("GNU ${p.score.first} : ${p.score.second} You",
                            color = Color.White, fontSize = 13.sp, maxLines = 1)
                        Text(
                            when (p.cubeOwner) {
                                -1 -> "Cube ${p.cubeValue}, centred"
                                0  -> "Cube ${p.cubeValue}, yours"
                                else -> "Cube ${p.cubeValue}, GNU's"
                            },
                            color = pal.uiTextSecondary, fontSize = 12.sp, maxLines = 1
                        )
                        if (p.gameState >= 2) {
                            Spacer(Modifier.height(4.dp))
                            Text("End of game", color = pal.uiTextSecondary, fontSize = 12.sp, maxLines = 1)
                        }
                    }

                    if (status.isNotEmpty()) {
                        Spacer(Modifier.height(8.dp))
                        Text(status, color = pal.uiTextSecondary, fontSize = 12.sp)
                    }
                }

                Column(horizontalAlignment = Alignment.CenterHorizontally) {
                    if (pos != null) {
                        Row(horizontalArrangement = Arrangement.spacedBy(6.dp)) {
                            GameButton("< Move", pal.uiButtonNeutral) { step(true) }
                            GameButton("Move >", pal.uiButtonNeutral) { step(false) }
                        }
                        Spacer(Modifier.height(6.dp))
                        Row(horizontalArrangement = Arrangement.spacedBy(6.dp)) {
                            GameButton("< Game", pal.uiChipOff) { stepGame(true) }
                            GameButton("Game >", pal.uiChipOff) { stepGame(false) }
                        }
                        Spacer(Modifier.height(10.dp))
                    }

                    GameButton("Open match", pal.uiActionRoll) { onOpenMatch() }
                    Spacer(Modifier.height(6.dp))
                    GameButton("Home", pal.uiButtonNeutral) { onReturnToHub() }
                }
            }

            // Right pane: gnubg's position, read-only.
            Box(modifier = Modifier.fillMaxHeight().fillMaxWidth()) {
                val p = pos
                if (p == null) {
                    Text(
                        "Open a saved .sgf match to step through it.\n" +
                            "This replaces any game in progress.",
                        color = pal.uiTextSecondary, fontSize = 13.sp,
                        modifier = Modifier.align(Alignment.Center)
                    )
                } else {
                    BackgammonBoard(
                        settings = settings,
                        gameState = BoardState(
                            board = p.board,
                            dice = p.dice,
                            matchScore = intArrayOf(p.score.first, p.score.second),
                            matchLength = p.matchTo,
                            cubeValue = p.cubeValue,
                            cubeOwner = p.cubeOwner,
                            turn = p.turn,
                            // Not HUMAN_MOVING, so no Undo/Commit is drawn; not
                            // WAITING_FOR_ROLL with turn 0, so no Roll button.
                            phase = GamePhase.ENGINE_THINKING
                        ),
                        // viewModel = null makes the board read-only: Board.kt returns
                        // from every tap before touching the engine. tutorMode stays
                        // false so the cube is still drawn -- it is part of the record.
                        viewModel = null,
                        tutorMode = false
                    )
                }
            }
        }

        // Controls pane is on the LEFT here, so the gear keeps the top-left
        // corner it has everywhere else; the title is centred and clears it.
        if (onOpenSettings != null) {
            Icon(
                imageVector = Icons.Filled.Settings,
                contentDescription = "Settings",
                tint = pal.uiTextSecondary,
                modifier = Modifier
                    .align(Alignment.TopStart)
                    .padding(start = 16.dp, top = 12.dp)
                    .size(24.dp)
                    .clickable { onOpenSettings() }
            )
        }
        }
    }
}
