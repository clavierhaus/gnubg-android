package com.clavierhaus.gnubg.play

import com.clavierhaus.gnubg.Engine

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.layout.BoxWithConstraints
import androidx.compose.foundation.layout.absoluteOffset
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.ui.text.font.FontWeight
import android.util.Log
import androidx.compose.foundation.gestures.detectDragGestures
import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.drawscope.DrawScope
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.graphics.drawscope.drawIntoCanvas
import androidx.compose.ui.graphics.nativeCanvas
import androidx.compose.ui.graphics.toArgb
import androidx.compose.ui.input.pointer.pointerInput
import com.clavierhaus.gnubg.engine.BoardState
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import com.clavierhaus.gnubg.engine.GamePhase
import com.clavierhaus.gnubg.engine.GameSettings
import com.clavierhaus.gnubg.engine.GameViewModel
import androidx.compose.ui.geometry.Rect
import androidx.compose.ui.layout.onSizeChanged

private const val TOT_W  = 102f
private const val TOT_H  = 82f
private const val BRD_W  = 3f
private const val BRD_H  = 3f
private const val BAR_W  = 7f
private const val BRF_W  = 6f
private const val PT_H   = 36.2f
private const val LEFT_X  = BRD_W
private const val RIGHT_X = TOT_W - BRD_W - BRF_W
private const val MID_X   = (LEFT_X + RIGHT_X) / 2f
private const val HALF_W  = (RIGHT_X - LEFT_X - BAR_W) / 2f
private const val PT_W    = HALF_W / 6f
private const val DIE_W   = PT_W * 0.8f

// Position-editor zones (BackgammonBoard.onEditTap).
const val EDIT_ZONE_BAR_HUMAN  = 0
const val EDIT_ZONE_BAR_ENGINE = 25
const val EDIT_ZONE_TRAY       = -2

/**
 * Every rectangle on the board, computed once from the canvas size, in pixels.
 *
 * CLAUDE.md, "UI GEOMETRY: ONE SOURCE, EVERY DEVICE". The board fills the screen
 * horizontally and absorbs aspect ratio vertically, so sx != sy is normal --
 * measured 0.959 at 16:11, 1.172 at 16:9, 1.468 at 20:9. Two consequences:
 *
 *  - Board features stretch. Points, bar, frame and trays are anchored with
 *    ux()/uy(), each axis scaled by its own factor.
 *  - Pieces do not. Checkers, dice, cube and buttons keep their shape, so their
 *    SIZES come from pc(), which never follows sy. A cube that is square in
 *    pixels is not square in board units, which is exactly why hit-testing it in
 *    board units left 16% of it dead at the top and bottom on a 20:9 phone, and
 *    made the tap rect overhang it on a 16:11 one.
 *
 * The invariant: an element's hit rectangle IS the rectangle it was drawn from.
 * Both sides read these values. Nothing is computed twice.
 */
private class BoardGeom(val w: Float, val h: Float, cubeOwner: Int, private val diceCount: Int) {
    val sx = w / TOT_W
    val sy = h / TOT_H

    fun ux(u: Float) = u * sx          // board anchor, x
    fun uy(u: Float) = u * sy          // board anchor, y
    private fun pc(u: Float) = u * sx  // piece size -- never sy

    val boardCY    = uy(TOT_H / 2f)
    val rightGapCX = ux(MID_X + BAR_W / 2f + HALF_W / 2f)
    val leftGapCX  = ux(MID_X - BAR_W / 2f - HALF_W / 2f)

    val dieSize = pc(DIE_W)
    val diceGap = pc(PT_W * 0.15f)

    private fun diceRow(centreX: Float, top: Float, count: Int): List<Rect> {
        val totalW = count * dieSize + (count - 1) * diceGap
        val startX = centreX - totalW / 2f
        return (0 until count).map { i ->
            val l = startX + i * (dieSize + diceGap)
            Rect(l, top, l + dieSize, top + dieSize)
        }
    }

    private val diceTop = boardCY - dieSize - diceGap / 2f
    fun playerDice(count: Int = diceCount) = diceRow(rightGapCX, diceTop, count)
    fun engineDice(count: Int = diceCount) = diceRow(leftGapCX, diceTop, count)
    fun engineDiceCentred(count: Int = 2)  = diceRow(leftGapCX, boardCY - dieSize / 2f, count)

    /** The drawn dice, as one rectangle. Tapping them swaps their order. */
    val swapDiceRect: Rect = playerDice().let { Rect(it.first().left, it.first().top, it.last().right, it.last().bottom) }

    private val btnW   = dieSize * 2f
    private val btnH   = dieSize
    private val btnTop = boardCY + diceGap / 2f
    val undoRect   = Rect(rightGapCX - diceGap / 2f - btnW, btnTop, rightGapCX - diceGap / 2f, btnTop + btnH)
    val commitRect = Rect(rightGapCX + diceGap / 2f, btnTop, rightGapCX + diceGap / 2f + btnW, btnTop + btnH)
    /** The no-move "Continue" affordance occupies both button slots. */
    val passRect   = Rect(undoRect.left, undoRect.top, commitRect.right, commitRect.bottom)

    private val rollW = dieSize * 2f + diceGap
    private val rollH = dieSize * 1.2f
    val rollRect = Rect(rightGapCX - rollW / 2f, boardCY - dieSize / 2f,
                        rightGapCX + rollW / 2f, boardCY - dieSize / 2f + rollH)

    val cubeSize = pc(BAR_W * 0.75f)
    private val cubeGap = cubeSize * 0.18f
    // One cube-height clear of centre. A piece offset, so it is a pixel length.
    val cubeCY = when (cubeOwner) {
        1    -> boardCY - cubeSize - cubeGap
        0    -> boardCY + cubeSize + cubeGap
        else -> boardCY
    }
    val cubeRect = Rect(ux(MID_X) - cubeSize / 2f, cubeCY - cubeSize / 2f,
                        ux(MID_X) + cubeSize / 2f, cubeCY + cubeSize / 2f)

    /** Lower half of the bar: where a human checker sits waiting to re-enter. */
    val barBottomRect = Rect(ux(MID_X - BAR_W / 2f), boardCY, ux(MID_X + BAR_W / 2f), uy(TOT_H - BRD_H))

    /** Upper half of the bar (the engine's side). Used by the position editor. */
    val barTopRect = Rect(ux(MID_X - BAR_W / 2f), uy(BRD_H), ux(MID_X + BAR_W / 2f), boardCY)

    /** The bear-off tray column on the right. In edit mode, tapping it clears the
     *  board -- gnubg's own edit-mode gesture (clicking a tray clears). */
    val trayRect = Rect(ux(RIGHT_X), uy(BRD_H), ux(TOT_W - BRD_W), uy(TOT_H - BRD_H))

    // Checkers. A piece, but one that must also fit inside a stretched half-board:
    // the radius is the smaller of what the point width allows and what the half
    // board height allows, so two opposing stacks of five never collide on a squat
    // pane. This is the one place it is computed; the stack and the checker that
    // follows the finger both read it.
    val boardTop    = uy(BRD_H)
    val boardBottom = h - uy(BRD_H)
    private val maxVisibleCheckers = 5f
    private val stepFactor  = 2.05f
    private val insetFactor = 0.12f
    private val centreClearance = uy(TOT_H - 2f * BRD_H) * 0.035f
    private val halfStackHeight = (boardBottom - boardTop - centreClearance) / 2f
    val checkerR = minOf(
        ux(PT_W) * 0.40f,
        halfStackHeight / (2f + (maxVisibleCheckers - 1f) * stepFactor + 2f * insetFactor)
    )
    val checkerInset = checkerR * insetFactor
    val checkerStep  = checkerR * stepFactor

    /** Points stretch with the board, so both axes scale. */
    fun pointRect(n: Int): Rect {
        val left = ux(pointX(n))
        val top  = if (n in 13..24) uy(BRD_H) else uy(TOT_H - BRD_H - PT_H)
        return Rect(left, top, left + ux(PT_W), top + uy(PT_H))
    }

    fun pointAt(o: Offset): Int {
        for (n in 1..24) if (pointRect(n).contains(o)) return n
        return -1
    }
}

private val PIP_POSITIONS = mapOf(
    1 to listOf(Pair(0.5f, 0.5f)),
    2 to listOf(Pair(0.25f, 0.25f), Pair(0.75f, 0.75f)),
    3 to listOf(Pair(0.25f, 0.25f), Pair(0.5f, 0.5f), Pair(0.75f, 0.75f)),
    4 to listOf(Pair(0.25f, 0.25f), Pair(0.75f, 0.25f), Pair(0.25f, 0.75f), Pair(0.75f, 0.75f)),
    5 to listOf(Pair(0.25f, 0.25f), Pair(0.75f, 0.25f), Pair(0.5f, 0.5f), Pair(0.25f, 0.75f), Pair(0.75f, 0.75f)),
    6 to listOf(Pair(0.25f, 0.2f), Pair(0.75f, 0.2f), Pair(0.25f, 0.5f), Pair(0.75f, 0.5f), Pair(0.25f, 0.8f), Pair(0.75f, 0.8f))
)

/** The number of dice drawn, so the hit rect covers exactly them. */
private fun diceCountOf(state: BoardState): Int {
    val d = state.dice ?: return 2
    return when {
        d.first == d.second && d.second > 0 -> 4
        d.second < 0 -> 1
        else -> 2
    }
}

private fun pointX(n: Int): Float = when {
    n in 1..6   -> MID_X + BAR_W / 2f + (6 - n) * PT_W
    n in 7..12  -> LEFT_X + (12 - n) * PT_W
    n in 13..18 -> LEFT_X + (n - 13) * PT_W
    else        -> MID_X + BAR_W / 2f + (n - 19) * PT_W
}

private fun pointCentreX(n: Int): Float = pointX(n) + PT_W / 2f



/**
 * Where this checker may legally go now, read out of gnubg's own move list.
 *
 * gameState.legalMoves is GenerateMoves() over the CURRENT board with the dice
 * that remain, so the first sub-move of each generated move is exactly a legal
 * step from here. Take the ones whose source is this checker.
 *
 * A previous version pooled every (src,dst) pair from every move into one edge
 * list and breadth-first searched it. That splices unrelated moves together --
 * an 8->6 step from one move and a 6->1 step from another let it walk 8->6->1,
 * a play gnubg never offered -- so it lit up unreachable points. It also read
 * the turn-start board while using the current dice.
 *
 * A bear-off destination is -1. gnubg clamps every negative destination to -1
 * in SaveMoves (eval.c), so it is a sentinel for "off" and says nothing about
 * which die was used. It has no point to highlight, so it is skipped here; the
 * tap itself is validated by gnubg through Engine.applySubMove.
 */
private fun landingPointsForSource(gameState: BoardState, sourcePoint: Int): Set<Int> {
    if (gameState.phase != GamePhase.HUMAN_MOVING) return emptySet()
    if (sourcePoint !in 1..24) return emptySet()
    if (gameState.board[24 + sourcePoint] <= 0) return emptySet()

    val moves = gameState.legalMoves
    if (moves.isEmpty()) return emptySet()

    val origin = sourcePoint - 1
    val dests = linkedSetOf<Int>()
    var m = 0
    while (m + 1 < moves.size) {
        val src = moves[m]          // first sub-move of this move
        val dst = moves[m + 1]
        if (src == origin && dst in 0..23) dests.add(dst + 1)
        m += 8
    }
    return dests
}

// Legal bar-entry target points for a human on the bar. gnubg is the authority:
// try re-entry (src = 24, the bar) with each remaining die and keep the entry
// point (board 25 - d) only where applySubMove succeeds.
private fun barEntryPoints(gameState: BoardState): Set<Int> {
    if (gameState.phase != GamePhase.HUMAN_MOVING) return emptySet()
    if (gameState.board[49] <= 0) return emptySet()
    val out = linkedSetOf<Int>()
    for (d in gameState.remainingDice.distinct()) {
        val b = Engine.applySubMove(gameState.board, 24, d)
        if (b.isNotEmpty()) out.add(25 - d)
    }
    return out
}

/**
 * Coach visual WHY (vision P1, its most emphasized point): the played and best
 * moves rendered as per-leg traced motion ON this board's own geometry -- the
 * same points and proportions the player has stared at all game. anMove[8] in
 * the human mover frame (gnubg ApplyMove semantics, eval.c: pairs of src,dst;
 * src 24 = bar, dst < 0 = bear-off, legs end at first negative src). Pure
 * rendering of gnubg-returned data; nothing is classified.
 */
class CoachTrace(val played: IntArray, val best: IntArray)

@Composable
fun BackgammonBoard(
    settings: GameSettings = GameSettings(),
    gameState: BoardState = BoardState(),
    viewModel: GameViewModel? = null,
    tutorMode: Boolean = false,
    coachTrace: CoachTrace? = null,
    /**
     * Position-editor hook. Non-null puts the board in EDIT: every tap is
     * reported as a zone and nothing reaches the game. Zones: 1..24 a point,
     * EDIT_ZONE_BAR_HUMAN / EDIT_ZONE_BAR_ENGINE the bar halves,
     * EDIT_ZONE_TRAY the bear-off column (gnubg's clear-board gesture).
     */
    onEditTap: ((zone: Int) -> Unit)? = null
) {
    val p = BoardPalettes.from(settings.boardTheme)
    var highlightedLandingPoints by remember { mutableStateOf<Set<Int>>(emptySet()) }
    // Drag-to-move prototype state. draggingFrom is the source point (1..24) the
    // finger picked up from; dragPosUnits is the current finger position in PIXELS
    // units. Both null when no drag is in progress. Drag coexists with tap: this
    // is a separate pointerInput, and detectDragGestures only fires past touch slop.
    var draggingFrom by remember { mutableStateOf<Int?>(null) }
    var dragPosUnits by remember { mutableStateOf<Offset?>(null) }

    Box(modifier = Modifier
        .fillMaxSize()
        .pointerInput(
            viewModel,
            onEditTap,
            gameState.phase,
            gameState.turn,
            gameState.fDoubled,
            gameState.cubeOwner,
            gameState.cubeValue,
            gameState.legalMoves,
            gameState.remainingDice
        ) {
            highlightedLandingPoints = emptySet()

            detectTapGestures(
                onPress = {
                    try {
                        awaitRelease()
                    } finally {
                        if (draggingFrom == null) highlightedLandingPoints = emptySet()
                    }
                },
                onLongPress = { offset ->
                    if (onEditTap != null) return@detectTapGestures
                    val g = BoardGeom(size.width.toFloat(), size.height.toFloat(),
                                      gameState.cubeOwner, diceCountOf(gameState))
                    highlightedLandingPoints = landingPointsForSource(gameState, g.pointAt(offset))
                },
                onTap = { offset ->
                    highlightedLandingPoints = emptySet()

                // EDIT: the editor owns every tap. Zones only; no game logic.
                if (onEditTap != null) {
                    val g = BoardGeom(size.width.toFloat(), size.height.toFloat(),
                                      gameState.cubeOwner, diceCountOf(gameState))
                    val zone = when {
                        g.trayRect.contains(offset)      -> EDIT_ZONE_TRAY
                        g.barBottomRect.contains(offset) -> EDIT_ZONE_BAR_HUMAN
                        g.barTopRect.contains(offset)    -> EDIT_ZONE_BAR_ENGINE
                        else -> g.pointAt(offset)
                    }
                    if (zone != -1) onEditTap.invoke(zone)
                    return@detectTapGestures
                }
                if (viewModel == null) return@detectTapGestures

                // One geometry. The rectangles below are the rectangles drawn.
                val g = BoardGeom(size.width.toFloat(), size.height.toFloat(),
                                  gameState.cubeOwner, diceCountOf(gameState))

                // Cube -- first, so it is not intercepted by the bar or Roll.
                if (!tutorMode && g.cubeRect.contains(offset)) {
                    // Engine is the sole authority (gnubg_can_double). The UI does not
                    // reimplement any subset of the cube rule; it reads the flag computed
                    // on the engine thread in readMatchState.
                    val uiAllowsDouble = gameState.canDouble
                    Log.i("gnubg-vm",
                        "Board cube tap: phase=${gameState.phase} turn=${gameState.turn} " +
                            "fDoubled=${gameState.fDoubled} cubeOwner=${gameState.cubeOwner} " +
                            "cubeValue=${gameState.cubeValue} uiAllowsDouble=$uiAllowsDouble")
                    if (uiAllowsDouble) viewModel.offerDouble()
                    else Log.i("gnubg-vm", "Board cube tap ignored by UI gate")
                    return@detectTapGestures
                }

                if (gameState.phase == GamePhase.WAITING_FOR_ROLL && gameState.turn == 0 &&
                    g.rollRect.contains(offset)) {
                    viewModel.rollDice()
                    return@detectTapGestures
                }

                val tapCannotMove = gameState.phase == GamePhase.HUMAN_MOVING &&
                    gameState.legalMoves.isEmpty() &&
                    gameState.board.contentEquals(gameState.oldBoard)

                if (gameState.phase == GamePhase.HUMAN_MOVING && gameState.turn == 0 &&
                    !tapCannotMove && g.swapDiceRect.contains(offset)) {
                    viewModel.swapDice()
                    return@detectTapGestures
                }

                if (tapCannotMove && g.passRect.contains(offset)) {
                    viewModel.passTurn()
                    return@detectTapGestures
                }

                if (gameState.phase == GamePhase.HUMAN_MOVING && !tapCannotMove) {
                    if (g.undoRect.contains(offset))   { viewModel.undo();    return@detectTapGestures }
                    if (g.commitRect.contains(offset)) { viewModel.confirm(); return@detectTapGestures }
                }

                // Human checker on the bar -- point 0 is the bar signal; tapSource
                // checks whether one is actually there.
                if (g.barBottomRect.contains(offset)) {
                    viewModel.tapSource(0)
                    return@detectTapGestures
                }

                val tapped = g.pointAt(offset)
                if (tapped >= 0) viewModel.tapSource(tapped)
                }
            )
        }
        .pointerInput(
            viewModel,
            onEditTap,
            gameState.phase,
            gameState.turn,
            gameState.legalMoves,
            gameState.remainingDice
        ) {
            detectDragGestures(
                onDragStart = { offset ->
                    if (onEditTap != null) return@detectDragGestures
                    if (viewModel == null) return@detectDragGestures
                    if (gameState.phase != GamePhase.HUMAN_MOVING) return@detectDragGestures
                    val g = BoardGeom(size.width.toFloat(), size.height.toFloat(),
                                      gameState.cubeOwner, diceCountOf(gameState))
                    if (gameState.board[49] > 0 && g.barBottomRect.contains(offset)) {
                        draggingFrom = 0
                        dragPosUnits = offset
                        highlightedLandingPoints = barEntryPoints(gameState)
                        return@detectDragGestures
                    }
                    val src = g.pointAt(offset)
                    if (src in 1..24 && gameState.board[24 + src] > 0) {
                        draggingFrom = src
                        dragPosUnits = offset
                        highlightedLandingPoints = landingPointsForSource(gameState, src)
                    }
                },
                onDrag = { change, _ ->
                    if (draggingFrom == null) return@detectDragGestures
                    dragPosUnits = change.position
                },
                onDragEnd = {
                    val from = draggingFrom
                    val pos = dragPosUnits
                    if (viewModel != null && from != null && pos != null) {
                        val g = BoardGeom(size.width.toFloat(), size.height.toFloat(),
                                          gameState.cubeOwner, diceCountOf(gameState))
                        val target = g.pointAt(pos)
                        // Release on a highlighted legal landing -> dispatch through the
                        // existing move path. tapSource(target) routes to the destination
                        // resolver (tryDestinationStackMove) for empty/opponent points;
                        // releasing back on the source is a no-op selection.
                        if (target in highlightedLandingPoints) {
                            viewModel.dragMove(from, target)
                        }
                    }
                    draggingFrom = null
                    dragPosUnits = null
                    highlightedLandingPoints = emptySet()
                },
                onDragCancel = {
                    draggingFrom = null
                    dragPosUnits = null
                    highlightedLandingPoints = emptySet()
                }
            )
        }
    ) {
        Canvas(modifier = Modifier.fillMaxSize()) {
            // The same rectangles the tap handler uses. Never recompute one here.
            val g = BoardGeom(size.width, size.height, gameState.cubeOwner, diceCountOf(gameState))
            val sx = size.width  / TOT_W
            val sy = size.height / TOT_H
            fun ux(u: Float) = u * sx
            fun uy(u: Float) = u * sy

            // Checker metrics: computed once, in BoardGeom.
            val boardBottom = g.boardBottom
            val boardTop    = g.boardTop
            val r     = g.checkerR
            val inset = g.checkerInset
            val step  = g.checkerStep

            // 1. Frame
            drawRect(p.frame, size = size)

            // 2. Board field
            drawRect(p.boardField,
                topLeft = Offset(ux(BRD_W), uy(BRD_H)),
                size = Size(ux(TOT_W - 2 * BRD_W), uy(TOT_H - 2 * BRD_H)))

            // 3. Triangles
            for (n in 1..24) {
                val x = pointX(n)
                // Bottom triangles: points 1-12, alternate starting with A at point 1
                // Top triangles: points 13-24, must alternate opposite to bottom
                val bottomColor = if (n % 2 == 1) p.triangleA else p.triangleB
                val topColor    = if (n % 2 == 1) p.triangleB else p.triangleA
                drawTriangle(topColor,    ux(x), uy(BRD_H),         ux(PT_W), uy(PT_H), true)
                drawTriangle(bottomColor, ux(x), uy(TOT_H - BRD_H), ux(PT_W), uy(PT_H), false)
            }

            // 4. Bearoff trays
            val tx      = ux(RIGHT_X)
            val tw      = ux(BRF_W)
            val trayGap = uy(6f)
            val trayH   = (uy(TOT_H - 2 * BRD_H) - trayGap) / 2f
            val topTrayY = uy(BRD_H)
            val botTrayY = uy(BRD_H) + trayH + trayGap
            val ol = 1.5f

            listOf(topTrayY, botTrayY).forEach { ty ->
                drawRect(p.bearoff, topLeft = Offset(tx, ty), size = Size(tw, trayH))
                drawRect(p.trayDarkBorder, topLeft = Offset(tx, ty), size = Size(tw, trayH),
                    style = Stroke(width = 1f))
                drawLine(p.trayOutline, Offset(tx + ol, ty + ol), Offset(tx + ol, ty + trayH - ol), ol)
                drawLine(p.trayOutline, Offset(tx + ol, ty + ol), Offset(tx + tw, ty + ol), ol)
                drawLine(p.trayOutline, Offset(tx + ol, ty + trayH - ol), Offset(tx + tw, ty + trayH - ol), ol)
            }

            // Bearoff tray checkers
            // Count borne-off: 15 minus all checkers still on board (points + bar)
            val humanOnBoard  = (25..49).sumOf { gameState.board[it] }
            val engineOnBoard = (0..24).sumOf { gameState.board[it] }
            val humanBorneOff  = 15 - humanOnBoard
            val engineBorneOff = 15 - engineOnBoard

            val tcH = trayH / 15f           // height per tray checker slot
            val tcW = tw * 0.85f            // checker width in tray
            val tcX = tx + (tw - tcW) / 2f  // centred in tray

            // Engine borne off -- top tray, stacked from top
            for (i in 0 until engineBorneOff) {
                val cy = topTrayY + i * tcH
                drawTrayChecker(tcX, cy, tcW, tcH * 0.92f, p.checkerDark, p.checkerDarkRim)
            }
            // Human borne off -- bottom tray, stacked from top
            for (i in 0 until humanBorneOff) {
                val cy = botTrayY + i * tcH
                drawTrayChecker(tcX, cy, tcW, tcH * 0.92f, p.checkerLight, p.checkerLightRim)
            }

            // 5. Bar
            drawRect(p.bar,
                topLeft = Offset(ux(MID_X - BAR_W / 2f), uy(BRD_H)),
                size = Size(ux(BAR_W), uy(TOT_H - 2 * BRD_H)))

            // Bar checkers
            // anBoard[0][24] = engine/dark checkers on bar (shown in top half)
            // anBoard[1][24] = human/light checkers on bar (shown in bottom half)
            val barCentreX = ux(MID_X)
            val engineOnBar = gameState.board[24]
            val humanOnBar  = gameState.board[49] - (if (draggingFrom == 0) 1 else 0)
            val barR = r * 0.9f

            // Engine bar checkers: below top pip count
            val engineBarStartY = uy(BRD_H + 9f)
            for (i in 0 until engineOnBar) {
                val cy = engineBarStartY + barR + i * barR * 2.1f
                drawChecker(barCentreX, cy, barR, p.checkerDark, p.checkerDarkRim, false, p.checkerHighlight)
            }
            // Human bar checkers: above bottom pip count
            val humanBarStartY = uy(TOT_H - BRD_H - 9f)
            for (i in 0 until humanOnBar) {
                val cy = humanBarStartY - barR - i * barR * 2.1f
                drawChecker(barCentreX, cy, barR, p.checkerLight, p.checkerLightRim, true, p.checkerHighlight)
            }

            // Cube drawn after bar checkers.
            // Position is centre-relative (clears bar checkers):
            // - centred cube (no owner): middle of the bar, display 64
            // - engine-owned cube: one cube-height above centre
            // - human-owned cube: one cube-height below centre
            // Cube is not part of tutor / live-analysis mode: no doubling, so
            // nothing to draw (not even the centred 64).
            if (!tutorMode) {
                val cubeDisplayValue = if (gameState.cubeOwner == -1) 64 else gameState.cubeValue
                drawCube(g.cubeRect.left, g.cubeRect.top, g.cubeSize, cubeDisplayValue,
                    p.cubeFace, p.cubeDot, p.cubeText)
            }

            // 6. Checkers from live board state
            for (n in 1..24) {
                val cx    = ux(pointCentreX(n))
                val isTop = n in 13..24

                // anBoard[0] = engine/dark: index (n-1) -> UI point n
                // anBoard[0] = engine/dark
                val engineCount = gameState.board[24 - n]
                if (engineCount > 0) {
                    val show = minOf(engineCount, 5)
                    for (i in 0 until show) {
                        val cy = if (isTop) boardTop + inset + r + i * step
                                 else       boardBottom - inset - r - i * step
                        drawChecker(cx, cy, r, p.checkerDark, p.checkerDarkRim, false, p.checkerHighlight)
                    }
                    if (engineCount > 5) {
                        val topCy = if (isTop) boardTop + inset + r + 4 * step else boardBottom - inset - r - 4 * step
                        drawIntoCanvas { canvas ->
                            val paint = android.graphics.Paint().apply {
                                color = android.graphics.Color.WHITE; textSize = r * 1.1f
                                textAlign = android.graphics.Paint.Align.CENTER; isAntiAlias = true; isFakeBoldText = true
                            }
                            canvas.nativeCanvas.drawText("$engineCount", cx, topCy - (paint.descent() + paint.ascent()) / 2f, paint)
                        }
                    }
                }

                // anBoard[1] = human/light. While dragging, the picked-up checker
                // leaves its source stack: subtract one at draggingFrom so it does
                // not render both in the stack and under the finger.
                val humanCount = gameState.board[24 + n] - (if (draggingFrom == n) 1 else 0)
                if (humanCount > 0) {
                    val show = minOf(humanCount, 5)
                    for (i in 0 until show) {
                        val cy = if (isTop) boardTop + inset + r + i * step
                                 else       boardBottom - inset - r - i * step
                        drawChecker(cx, cy, r, p.checkerLight, p.checkerLightRim, true, p.checkerHighlight)
                    }
                    if (humanCount > 5) {
                        val topCy = if (isTop) boardTop + inset + r + 4 * step else boardBottom - inset - r - 4 * step
                        drawIntoCanvas { canvas ->
                            val paint = android.graphics.Paint().apply {
                                color = android.graphics.Color.BLACK; textSize = r * 1.1f
                                textAlign = android.graphics.Paint.Align.CENTER; isAntiAlias = true; isFakeBoldText = true
                            }
                            canvas.nativeCanvas.drawText("$humanCount", cx, topCy - (paint.descent() + paint.ascent()) / 2f, paint)
                        }
                    }
                }


            }

            // 7. Dice
            gameState.dice?.let { (d0, d1) ->
                val diceToShow = when {
                    d0 == d1 && d1 > 0 -> listOf(d0, d0, d0, d0)
                    d1 < 0             -> listOf(d0)
                    else               -> listOf(d0, d1)
                }

                val remainingCounts = gameState.remainingDice.groupingBy { it }.eachCount().toMutableMap()
                val usedMask = diceToShow.map { face ->
                    val count = remainingCounts[face] ?: 0
                    if (count > 0) {
                        remainingCounts[face] = count - 1
                        false
                    } else {
                        true
                    }
                }
                val diceDimmed = gameState.phase == GamePhase.HUMAN_MOVING && gameState.legalMoves.isEmpty() && gameState.board.contentEquals(gameState.oldBoard)
                val rects = if (gameState.turn == 0) g.playerDice(diceToShow.size)
                            else                     g.engineDice(diceToShow.size)
                diceToShow.forEachIndexed { i, face ->
                    val r = rects[i]
                    val isUsed = usedMask.getOrElse(i) { false }
                    if (gameState.turn == 0) {
                        // Grey a die when gnubg lists no legal play for that face
                        // (unplayableDice, from the legal-move list), or the whole-
                        // turn no-move case. Used dice keep their spent styling.
                        val dimmed = !isUsed && (diceDimmed || face in gameState.unplayableDice)
                        val baseColor = if (isUsed) Color(0xFF6F8FB8) else p.triangleB
                        drawDie(r.left, r.top, r.width, r.height, face,
                            if (dimmed) Color(0xFF888888) else baseColor,
                            if (dimmed) Color(0xFF444444) else p.dicePip, p.frame)
                    } else {
                        drawDie(r.left, r.top, r.width, r.height, face,
                            if (isUsed) Color(0xFF1F3F6E) else p.diceDark, p.dicePip, p.frame)
                    }
                }
            }

            // During ENGINE_THINKING: the engine has already rolled (gnubg rolls
            // before it searches); show its dice grayed on its half, so the
            // player sees what it is thinking about -- and starts thinking too.
            if (gameState.phase == GamePhase.ENGINE_THINKING) {
                gameState.engineDice?.let { (e0, e1) ->
                    val rects = g.engineDiceCentred()
                    listOf(e0, e1).forEachIndexed { i, face ->
                        val r = rects[i]
                        drawDie(r.left, r.top, r.width, r.height, face, p.diceDark, p.dicePip, p.frame)
                    }
                }
            }

            // Coach visual WHY: traced per-leg motion for played (muted) vs
            // best (emphasized, arrowhead), plus translucent ghost checkers at
            // gnubg's destinations. Drawn over the checkers so the difference
            // is what the eye receives.
            if (coachTrace != null) {
                drawMoveTrace(g, coachTrace.played,
                    p.uiTextDisabled.copy(alpha = 0.55f), g.checkerR * 0.18f, ghost = false, p)
                drawMoveTrace(g, coachTrace.best,
                    p.uiActionPositive.copy(alpha = 0.9f), g.checkerR * 0.30f, ghost = true, p)
            }

            // During WAITING_FOR_ROLL: show engine dice (left half) + Roll button (right half)
            if (gameState.phase == GamePhase.WAITING_FOR_ROLL && gameState.turn == 0) {
                // Engine dice -- left half, grayed
                gameState.engineDice?.let { (e0, e1) ->
                    val rects = g.engineDiceCentred()
                    listOf(e0, e1).forEachIndexed { i, face ->
                        val r = rects[i]
                        drawDie(r.left, r.top, r.width, r.height, face, p.diceDark, p.dicePip, p.frame)
                    }
                }

                // Roll button -- right half, where player dice will appear.
                // This IS g.rollRect: the tap handler tests the same rectangle.
                val btnW    = g.rollRect.width
                val btnH    = g.rollRect.height
                val btnX    = g.rollRect.left
                val btnY    = g.rollRect.top
                val corner  = btnW * 0.08f
                val rollPath = Path().apply {
                    moveTo(btnX + corner, btnY)
                    lineTo(btnX + btnW - corner, btnY)
                    quadraticTo(btnX + btnW, btnY, btnX + btnW, btnY + corner)
                    lineTo(btnX + btnW, btnY + btnH - corner)
                    quadraticTo(btnX + btnW, btnY + btnH, btnX + btnW - corner, btnY + btnH)
                    lineTo(btnX + corner, btnY + btnH)
                    quadraticTo(btnX, btnY + btnH, btnX, btnY + btnH - corner)
                    lineTo(btnX, btnY + corner)
                    quadraticTo(btnX, btnY, btnX + corner, btnY)
                    close()
                }
                drawPath(rollPath, p.uiActionRoll)
                drawIntoCanvas { canvas ->
                    val paint = android.graphics.Paint().apply {
                        color = android.graphics.Color.WHITE
                        textSize = btnH * 0.55f
                        textAlign = android.graphics.Paint.Align.CENTER
                        isAntiAlias = true; isFakeBoldText = true
                    }
                    canvas.nativeCanvas.drawText("Roll",
                        btnX + btnW / 2f,
                        btnY + btnH / 2f - (paint.descent() + paint.ascent()) / 2f,
                        paint)
                }
            }

            // 8. Undo/Commit / Continue buttons drawn on canvas
            val drawCannotMove = gameState.phase == GamePhase.HUMAN_MOVING &&
                gameState.legalMoves.isEmpty() &&
                gameState.board.contentEquals(gameState.oldBoard)
            if (gameState.phase == GamePhase.HUMAN_MOVING && !drawCannotMove) {
                // These ARE g.undoRect and g.commitRect: the tap handler tests the
                // same rectangles. btnY used to be uy(TOT_H/2) + ux(gap)/2 -- an
                // x-scaled length added to a y coordinate, so the buttons slid down
                // as the pane widened.
                val undoLeft = g.undoRect.left
                val btnY     = g.undoRect.top
                val btnH     = g.undoRect.height
                val btnW     = g.undoRect.width
                val diceGap  = g.diceGap
                val corner   = btnW * 0.15f
                // Undo
                val undoPath = Path().apply {
                    moveTo(undoLeft + corner, btnY)
                    lineTo(undoLeft + btnW - corner, btnY)
                    quadraticTo(undoLeft + btnW, btnY, undoLeft + btnW, btnY + corner)
                    lineTo(undoLeft + btnW, btnY + btnH - corner)
                    quadraticTo(undoLeft + btnW, btnY + btnH, undoLeft + btnW - corner, btnY + btnH)
                    lineTo(undoLeft + corner, btnY + btnH)
                    quadraticTo(undoLeft, btnY + btnH, undoLeft, btnY + btnH - corner)
                    lineTo(undoLeft, btnY + corner)
                    quadraticTo(undoLeft, btnY, undoLeft + corner, btnY)
                    close()
                }
                drawPath(undoPath, p.uiActionNegative)
                // Commit
                val cx = g.commitRect.left       // IS the Commit hit rect
                val commitPath = Path().apply {
                    moveTo(cx + corner, btnY)
                    lineTo(cx + btnW - corner, btnY)
                    quadraticTo(cx + btnW, btnY, cx + btnW, btnY + corner)
                    lineTo(cx + btnW, btnY + btnH - corner)
                    quadraticTo(cx + btnW, btnY + btnH, cx + btnW - corner, btnY + btnH)
                    lineTo(cx + corner, btnY + btnH)
                    quadraticTo(cx, btnY + btnH, cx, btnY + btnH - corner)
                    lineTo(cx, btnY + corner)
                    quadraticTo(cx, btnY, cx + corner, btnY)
                    close()
                }
                drawPath(commitPath, p.uiActionPositive)
                drawIntoCanvas { canvas ->
                    val btnPaint = android.graphics.Paint().apply {
                        color = android.graphics.Color.WHITE
                        textSize = btnH * 0.45f
                        textAlign = android.graphics.Paint.Align.CENTER
                        isAntiAlias = true; isFakeBoldText = true
                    }
                    val textY = btnY + btnH / 2f - (btnPaint.descent() + btnPaint.ascent()) / 2f
                    canvas.nativeCanvas.drawText("Undo", undoLeft + btnW / 2f, textY, btnPaint)
                    canvas.nativeCanvas.drawText("Commit", cx + btnW / 2f, textY, btnPaint)
                }
            }

            if (drawCannotMove) {
                // IS g.passRect: both button slots plus the gap between them.
                val contLeft = g.passRect.left
                val contW    = g.passRect.width
                val btnY     = g.passRect.top
                val btnH     = g.passRect.height
                val corner   = (contW - g.diceGap) / 2f * 0.15f
                val contPath = Path().apply {
                    moveTo(contLeft + corner, btnY)
                    lineTo(contLeft + contW - corner, btnY)
                    quadraticTo(contLeft + contW, btnY, contLeft + contW, btnY + corner)
                    lineTo(contLeft + contW, btnY + btnH - corner)
                    quadraticTo(contLeft + contW, btnY + btnH, contLeft + contW - corner, btnY + btnH)
                    lineTo(contLeft + corner, btnY + btnH)
                    quadraticTo(contLeft, btnY + btnH, contLeft, btnY + btnH - corner)
                    lineTo(contLeft, btnY + corner)
                    quadraticTo(contLeft, btnY, contLeft + corner, btnY)
                    close()
                }
                drawPath(contPath, p.uiActionPositive)
                drawIntoCanvas { canvas ->
                    val btnPaint = android.graphics.Paint().apply {
                        color = android.graphics.Color.WHITE
                        textSize = btnH * 0.45f
                        textAlign = android.graphics.Paint.Align.CENTER
                        isAntiAlias = true; isFakeBoldText = true
                    }
                    val textY = btnY + btnH / 2f - (btnPaint.descent() + btnPaint.ascent()) / 2f
                    canvas.nativeCanvas.drawText("Continue", contLeft + contW / 2f, textY, btnPaint)
                }
            }

            // 8b. Cube + pip counts (drawn before bar checkers so checkers appear on top)
            val barCX    = ux(MID_X)
            val barCY    = uy(TOT_H / 2f)
            val pipTextSize = ux(BAR_W * 0.35f)
            val topPipY     = uy(BRD_H + 5f)
            val botPipY     = uy(TOT_H - BRD_H - 3.5f)

            drawIntoCanvas { canvas ->
                val pipPaint = android.graphics.Paint().apply {
                    color = p.pipText.toArgb()
                    textSize = pipTextSize
                    textAlign = android.graphics.Paint.Align.CENTER
                    isAntiAlias = true; isFakeBoldText = true
                }
                if (settings.showPipCount) {
                    canvas.nativeCanvas.drawText("${gameState.pipCountHuman}", barCX, topPipY, pipPaint)
                    canvas.nativeCanvas.drawText("${gameState.pipCountEngine}", barCX, botPipY, pipPaint)
                }
                val numPaint = android.graphics.Paint().apply {
                    color = p.numbers.toArgb()
                    textSize = uy(BRD_H) * 0.75f
                    textAlign = android.graphics.Paint.Align.CENTER
                    isAntiAlias = true
                }
                val hintNumPaint = android.graphics.Paint(numPaint).apply {
                    color = p.uiActionPositive.toArgb()
                    isFakeBoldText = true
                }
                if (settings.showPointNumbers || highlightedLandingPoints.isNotEmpty()) {
                    for (n in 13..24) {
                        val paint = if (n in highlightedLandingPoints) hintNumPaint else numPaint
                        if (settings.showPointNumbers || n in highlightedLandingPoints) {
                            canvas.nativeCanvas.drawText(
                                n.toString(), ux(pointCentreX(n)), uy(BRD_H) * 0.85f, paint)
                        }
                    }
                    for (n in 12 downTo 1) {
                        val paint = if (n in highlightedLandingPoints) hintNumPaint else numPaint
                        if (settings.showPointNumbers || n in highlightedLandingPoints) {
                            canvas.nativeCanvas.drawText(
                                n.toString(), ux(pointCentreX(n)), uy(TOT_H) - uy(BRD_H) * 0.15f, paint)
                        }
                    }
                }
            }

            // Floating checker follows the finger during a drag (prototype).
            dragPosUnits?.let { pos ->
                // Same radius as the stacks, from the same place. pos is already in
                // pixels -- the space the finger reported in.
                drawChecker(pos.x, pos.y, g.checkerR, p.checkerLight, p.checkerLightRim, true, p.checkerHighlight)
            }
        } // end Canvas
        
    } // end Box
}

private fun DrawScope.drawCube(
    left: Float, top: Float, size: Float, value: Int,
    face: Color, dot: Color, text: Color
) {
    val corner = size * 0.15f
    val path = Path().apply {
        moveTo(left + corner, top)
        lineTo(left + size - corner, top)
        quadraticTo(left + size, top, left + size, top + corner)
        lineTo(left + size, top + size - corner)
        quadraticTo(left + size, top + size, left + size - corner, top + size)
        lineTo(left + corner, top + size)
        quadraticTo(left, top + size, left, top + size - corner)
        lineTo(left, top + corner)
        quadraticTo(left, top, left + corner, top)
        close()
    }
    drawPath(path, face)
    drawPath(path, dot, style = Stroke(width = size * 0.05f))
    drawIntoCanvas { canvas ->
        val paint = android.graphics.Paint().apply {
            color = text.toArgb()
            textSize = size * 0.5f
            textAlign = android.graphics.Paint.Align.CENTER
            isAntiAlias = true; isFakeBoldText = true
        }
        canvas.nativeCanvas.drawText(
            value.toString(), left + size / 2f,
            top + size / 2f - (paint.descent() + paint.ascent()) / 2f, paint)
    }
}

private fun DrawScope.drawDie(
    left: Float, top: Float, w: Float, h: Float,
    face: Int, dieColor: Color, pipColor: Color, borderColor: Color
) {
    val corner = w * 0.15f
    val path = Path().apply {
        moveTo(left + corner, top)
        lineTo(left + w - corner, top)
        quadraticTo(left + w, top, left + w, top + corner)
        lineTo(left + w, top + h - corner)
        quadraticTo(left + w, top + h, left + w - corner, top + h)
        lineTo(left + corner, top + h)
        quadraticTo(left, top + h, left, top + h - corner)
        lineTo(left, top + corner)
        quadraticTo(left, top, left + corner, top)
        close()
    }
    drawPath(path, dieColor)
    drawPath(path, borderColor, style = Stroke(width = w * 0.04f))
    val pipR = w * 0.08f
    PIP_POSITIONS[face]?.forEach { (fx, fy) ->
        drawCircle(pipColor, pipR, Offset(left + fx * w, top + fy * h))
    }
}

/* One anchor per anMove endpoint, human mover frame: internal point i (0..23)
 * is UI point i+1 (pointRect); 24 is the bar (MID_X centre); dst < 0 is
 * bear-off, anchored at the right edge on the source's row. */
private fun traceAnchor(g: BoardGeom, internalPt: Int, srcY: Float): Offset = when {
    internalPt == 24 -> Offset(g.pointRect(6).right + (g.pointRect(19).left - g.pointRect(6).right) / 2f, g.boardCY)
    internalPt < 0   -> Offset(g.w * 0.985f, srcY)
    else -> g.pointRect(internalPt + 1).let { r ->
        // Anchor at the point's stack root: top points root at their top edge,
        // bottom points at their bottom edge, nudged inward.
        val y = if (internalPt + 1 in 13..24) r.top + r.height * 0.30f else r.bottom - r.height * 0.30f
        Offset(r.left + r.width / 2f, y)
    }
}

private fun DrawScope.drawMoveTrace(
    g: BoardGeom, anMove: IntArray, color: Color, stroke: Float, ghost: Boolean, p: BoardPalette
) {
    var i = 0
    while (i < 8 && i + 1 < anMove.size && anMove[i] >= 0) {
        val src = anMove[i]; val dst = anMove[i + 1]
        val a = traceAnchor(g, src, 0f)
        val b = traceAnchor(g, dst, a.y)
        // The leg: a line with a source dot and an arrowhead at the target,
        // so a compound move reads as its hops, not an abstraction.
        drawCircle(color, stroke * 1.6f, a)
        drawLine(color, a, b, strokeWidth = stroke, cap = StrokeCap.Round)
        val ang = kotlin.math.atan2(b.y - a.y, b.x - a.x)
        val ah = stroke * 4.5f
        val p1 = Offset(b.x - ah * kotlin.math.cos(ang - 0.45f), b.y - ah * kotlin.math.sin(ang - 0.45f))
        val p2 = Offset(b.x - ah * kotlin.math.cos(ang + 0.45f), b.y - ah * kotlin.math.sin(ang + 0.45f))
        val head = Path().apply { moveTo(b.x, b.y); lineTo(p1.x, p1.y); lineTo(p2.x, p2.y); close() }
        drawPath(head, color)
        // Ghost checker at gnubg's destination (best move only): the
        // translucent preview of where the checker would stand.
        if (ghost && dst in 0..23) {
            drawCircle(p.checkerLight.copy(alpha = 0.30f), g.checkerR, b)
            drawCircle(color.copy(alpha = 0.55f), g.checkerR, b, style = Stroke(width = stroke * 0.8f))
        }
        i += 2
    }
}

private fun DrawScope.drawChecker(
    cx: Float, cy: Float, r: Float,
    fill: Color, rim: Color, highlight: Boolean, highlightColor: Color
) {
    drawCircle(fill, r, Offset(cx, cy))
    drawCircle(rim, r, Offset(cx, cy), style = Stroke(width = r * 0.08f))
    if (highlight) {
        drawOval(highlightColor,
            topLeft = Offset(cx - r * 0.66f, cy - r * 0.47f),
            size = Size(r * 0.76f, r * 0.38f))
    }
}

private fun DrawScope.drawTrayChecker(
    left: Float, top: Float, w: Float, h: Float, fill: Color, rim: Color
) {
    val corner = h * 0.3f
    val path = Path().apply {
        moveTo(left + corner, top)
        lineTo(left + w - corner, top)
        quadraticTo(left + w, top, left + w, top + corner)
        lineTo(left + w, top + h - corner)
        quadraticTo(left + w, top + h, left + w - corner, top + h)
        lineTo(left + corner, top + h)
        quadraticTo(left, top + h, left, top + h - corner)
        lineTo(left, top + corner)
        quadraticTo(left, top, left + corner, top)
        close()
    }
    drawPath(path, fill)
    drawPath(path, rim, style = Stroke(width = h * 0.12f))
}

private fun DrawScope.drawTriangle(
    color: Color, x: Float, y: Float,
    width: Float, height: Float, pointingDown: Boolean
) {
    val path = Path().apply {
        if (pointingDown) {
            moveTo(x, y); lineTo(x + width, y); lineTo(x + width / 2f, y + height)
        } else {
            moveTo(x, y); lineTo(x + width, y); lineTo(x + width / 2f, y - height)
        }
        close()
    }
    drawPath(path, color)
}