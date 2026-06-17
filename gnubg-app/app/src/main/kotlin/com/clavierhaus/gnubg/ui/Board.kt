package com.clavierhaus.gnubg.ui

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
import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
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

private val PIP_POSITIONS = mapOf(
    1 to listOf(Pair(0.5f, 0.5f)),
    2 to listOf(Pair(0.25f, 0.25f), Pair(0.75f, 0.75f)),
    3 to listOf(Pair(0.25f, 0.25f), Pair(0.5f, 0.5f), Pair(0.75f, 0.75f)),
    4 to listOf(Pair(0.25f, 0.25f), Pair(0.75f, 0.25f), Pair(0.25f, 0.75f), Pair(0.75f, 0.75f)),
    5 to listOf(Pair(0.25f, 0.25f), Pair(0.75f, 0.25f), Pair(0.5f, 0.5f), Pair(0.25f, 0.75f), Pair(0.75f, 0.75f)),
    6 to listOf(Pair(0.25f, 0.2f), Pair(0.75f, 0.2f), Pair(0.25f, 0.5f), Pair(0.75f, 0.5f), Pair(0.25f, 0.8f), Pair(0.75f, 0.8f))
)

private fun pointX(n: Int): Float = when {
    n in 1..6   -> MID_X + BAR_W / 2f + (6 - n) * PT_W
    n in 7..12  -> LEFT_X + (12 - n) * PT_W
    n in 13..18 -> LEFT_X + (n - 13) * PT_W
    else        -> MID_X + BAR_W / 2f + (n - 19) * PT_W
}

private fun pointCentreX(n: Int): Float = pointX(n) + PT_W / 2f

@Composable
fun BackgammonBoard(
    settings: GameSettings = GameSettings(),
    gameState: BoardState = BoardState(),
    viewModel: GameViewModel? = null
) {
    val p = BoardPalettes.from(settings.boardTheme)

    Box(modifier = Modifier
        .fillMaxSize()
        .pointerInput(
            viewModel,
            gameState.phase,
            gameState.turn,
            gameState.fDoubled,
            gameState.cubeOwner,
            gameState.cubeValue
        ) {
            detectTapGestures { offset ->
                if (viewModel == null) return@detectTapGestures
                val sx = size.width.toFloat() / TOT_W
                val sy = size.height.toFloat() / TOT_H
                val x = offset.x / sx
                val y = offset.y / sy

                val boardCY = TOT_H / 2f
                val diceGap = PT_W * 0.15f
                val totalDW = DIE_W * 2f + diceGap
                val undoLeft = MID_X + BAR_W / 2f + HALF_W / 2f - DIE_W - diceGap / 2f

                // Tap cube to double — must be FIRST to avoid Roll/bar interception.
                // This uses the same board-relative centre/size as the drawn cube.
                // Detailed cube legality remains in GameViewModel/GNUbg.
                val cubeSzU = BAR_W * 0.75f
                val cubeGapU = cubeSzU * 0.18f
                val upperPipCounterY = BRD_H + 9f
                val lowerPipCounterY = TOT_H - BRD_H - 9f
                val cubeCYU = when (gameState.cubeOwner) {
                    1 -> upperPipCounterY + cubeSzU + cubeGapU
                    0 -> lowerPipCounterY - cubeSzU - cubeGapU
                    else -> TOT_H / 2f
                }
                val cubeHit =
                    x >= MID_X - cubeSzU / 2f && x <= MID_X + cubeSzU / 2f &&
                    y >= cubeCYU - cubeSzU / 2f && y <= cubeCYU + cubeSzU / 2f

                if (cubeHit) {
                    val uiAllowsDouble =
                        gameState.phase == GamePhase.WAITING_FOR_ROLL &&
                        gameState.turn == 0 &&
                        !gameState.fDoubled

                    Log.i(
                        "gnubg-vm",
                        "Board cube tap: x=$x y=$y phase=${gameState.phase} turn=${gameState.turn} " +
                            "fDoubled=${gameState.fDoubled} cubeOwner=${gameState.cubeOwner} " +
                            "cubeValue=${gameState.cubeValue} uiAllowsDouble=$uiAllowsDouble"
                    )

                    if (uiAllowsDouble) {
                        viewModel.offerDouble()
                    } else {
                        Log.i("gnubg-vm", "Board cube tap ignored by UI gate")
                    }
                    return@detectTapGestures
                }

                // Tap Roll button (right half, lower tray gap) during WAITING_FOR_ROLL
                val rightHalfCX = MID_X + BAR_W / 2f + HALF_W / 2f
                val rollBtnW    = DIE_W * 2f + diceGap
                if (gameState.phase == GamePhase.WAITING_FOR_ROLL && gameState.turn == 0 &&
                    y >= boardCY - DIE_W && y <= boardCY + DIE_W * 2.5f &&
                    x >= rightHalfCX - rollBtnW / 2f && x <= rightHalfCX + rollBtnW / 2f) {
                    viewModel.rollDice()
                    return@detectTapGestures
                }
                // Tap dice area: swap dice (board units)
                if (y >= boardCY - DIE_W * 2 && y <= boardCY &&
                    x >= undoLeft && x <= RIGHT_X) {
                    viewModel.swapDice()
                    return@detectTapGestures
                }
                // Tap Undo button (board units) — lower half of tray gap
                if (gameState.phase == GamePhase.HUMAN_MOVING &&
                    y >= boardCY && y <= boardCY + DIE_W * 2.5f &&
                    x >= undoLeft && x <= undoLeft + DIE_W) {
                    viewModel.undo()
                    return@detectTapGestures
                }
                // Tap Commit button (board units) — lower half of tray gap
                if (gameState.phase == GamePhase.HUMAN_MOVING &&
                    y >= boardCY && y <= boardCY + DIE_W * 2.5f &&
                    x >= undoLeft + DIE_W + diceGap && x <= RIGHT_X) {
                    viewModel.confirm()
                    return@detectTapGestures
                }

                // Tap on human bar checker (bottom half of bar) — triggers re-entry
                val barLeft  = MID_X - BAR_W / 2f
                val barRight = MID_X + BAR_W / 2f
                if (x >= barLeft && x <= barRight && y >= TOT_H / 2f && y <= TOT_H - BRD_H) {
                    viewModel.tapSource(0)  // point 0 = bar signal; tapSource checks humanOnBar
                    return@detectTapGestures
                }

                // Find tapped point
                var tapped = -1
                for (n in 1..24) {
                    val px = pointX(n)
                    val isTop = n in 13..24
                    val py = if (isTop) BRD_H else TOT_H - BRD_H - PT_H
                    if (x >= px && x <= px + PT_W && y >= py && y <= py + PT_H) {
                        tapped = n
                        break
                    }
                }
                if (tapped >= 0) viewModel.tapSource(tapped)
            }
        }
    ) {
        Canvas(modifier = Modifier.fillMaxSize()) {
            val sx = size.width  / TOT_W
            val sy = size.height / TOT_H
            fun ux(u: Float) = u * sx
            fun uy(u: Float) = u * sy

            // Checker radius: fits within point width with gap to border and between checkers
            val boardBottom = size.height - uy(BRD_H)
            val boardTop    = size.height - boardBottom  // mirrors boardBottom exactly

            // Checker metrics are constrained by both point width and available half-board height.
            // This keeps two opposing visible stacks of five from colliding on short/wide screens.
            val maxVisibleCheckers = 5f
            val centreClearance = uy(TOT_H - 2f * BRD_H) * 0.035f
            val halfStackHeight = (boardBottom - boardTop - centreClearance) / 2f
            val stepFactor = 2.05f
            val insetFactor = 0.12f
            val maxRByWidth = ux(PT_W) * 0.40f
            val maxRByHeight = halfStackHeight / (2f + (maxVisibleCheckers - 1f) * stepFactor + 2f * insetFactor)
            val r = minOf(maxRByWidth, maxRByHeight)
            val inset = r * insetFactor
            val step = r * stepFactor

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

            // Engine borne off — top tray, stacked from top
            for (i in 0 until engineBorneOff) {
                val cy = topTrayY + i * tcH
                drawTrayChecker(tcX, cy, tcW, tcH * 0.92f, p.checkerDark, p.checkerDarkRim)
            }
            // Human borne off — bottom tray, stacked from top
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
            val humanOnBar  = gameState.board[49]
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
            // Position is bar-relative:
            // - centred cube: middle of the bar, display 64
            // - human-owned cube: below the upper pip counter
            // - engine-owned cube: above the lower pip counter
            val cubeSzU = BAR_W * 0.75f
            val cubeGapU = cubeSzU * 0.18f
            val cubeCXU = MID_X
            val upperPipCounterY = BRD_H + 9f
            val lowerPipCounterY = TOT_H - BRD_H - 9f
            val cubeCYU = when (gameState.cubeOwner) {
                1 -> upperPipCounterY + cubeSzU + cubeGapU
                0 -> lowerPipCounterY - cubeSzU - cubeGapU
                else -> TOT_H / 2f
            }
            val cubeBarCX = ux(cubeCXU)
            val cubeBarCY = uy(cubeCYU)
            val cubeSz = ux(cubeSzU)
            val cubeDisplayValue = if (gameState.cubeOwner == -1) 64 else gameState.cubeValue
            drawCube(cubeBarCX - cubeSz / 2f, cubeBarCY - cubeSz / 2f, cubeSz, cubeDisplayValue,
                p.cubeFace, p.cubeDot, p.cubeText)

            // 6. Checkers from live board state
            for (n in 1..24) {
                val cx    = ux(pointCentreX(n))
                val isTop = n in 13..24

                // anBoard[0] = engine/dark: index (n-1) → UI point n
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

                // anBoard[1] = human/light
                val humanCount = gameState.board[24 + n]
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
                val dw  = ux(DIE_W)
                val dh  = ux(DIE_W)
                val gap = ux(PT_W * 0.15f)
                val boardCentreY = uy(TOT_H / 2f)

                if (gameState.turn == 0) {
                    val totalW = diceToShow.size * dw + (diceToShow.size - 1) * gap
                    val startX = ux(MID_X + BAR_W / 2f + HALF_W / 2f) - totalW / 2f
                    diceToShow.forEachIndexed { i, face ->
                        val isUsed = usedMask.getOrElse(i) { false }
                        val dieColor = if (isUsed) Color(0xFF6F8FB8) else p.triangleB
                        drawDie(startX + i * (dw + gap), boardCentreY - dh - gap / 2f,
                            dw, dh, face, dieColor, p.dicePip, p.frame)
                    }
                } else {
                    val totalW = diceToShow.size * dw + (diceToShow.size - 1) * gap
                    val startX = ux(MID_X - BAR_W / 2f - HALF_W / 2f) - totalW / 2f
                    diceToShow.forEachIndexed { i, face ->
                        val isUsed = usedMask.getOrElse(i) { false }
                        val dieColor = if (isUsed) Color(0xFF1F3F6E) else p.diceDark
                        drawDie(startX + i * (dw + gap), boardCentreY - dh - gap / 2f,
                            dw, dh, face, dieColor, p.dicePip, p.frame)
                    }
                }
            }

            // During WAITING_FOR_ROLL: show engine dice (left half) + Roll button (right half)
            if (gameState.phase == GamePhase.WAITING_FOR_ROLL && gameState.turn == 0) {
                val dw   = ux(DIE_W)
                val gap  = ux(PT_W * 0.15f)
                val cy   = uy(TOT_H / 2f)

                // Engine dice — left half, grayed
                gameState.engineDice?.let { (e0, e1) ->
                    val totalW = dw * 2f + gap
                    val startX = ux(MID_X - BAR_W / 2f - HALF_W / 2f) - totalW / 2f
                    listOf(e0, e1).forEachIndexed { i, face ->
                        drawDie(startX + i * (dw + gap), cy - dw / 2f,
                            dw, dw, face, p.diceDark, p.dicePip, p.frame)
                    }
                }

                // Roll button — right half, where player dice will appear
                val gapCX   = ux(MID_X + BAR_W / 2f + HALF_W / 2f)
                val btnW    = dw * 2f + gap
                val btnH    = dw * 1.2f
                val btnX    = gapCX - btnW / 2f
                val btnY    = cy - dw / 2f
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
                drawPath(rollPath, Color(0xFF1976D2))
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

            // 8. Undo/Commit buttons drawn on canvas — same coordinate system as dice
            if (gameState.phase == GamePhase.HUMAN_MOVING) {
                val diceGap  = ux(PT_W * 0.15f)
                val dw       = ux(DIE_W)
                val bw       = dw * 2f                              // buttons twice die width
                val gapCX    = ux(MID_X + BAR_W / 2f + HALF_W / 2f)
                val undoLeft = gapCX - diceGap * 0.5f - bw         // Undo left edge
                val btnY     = uy(TOT_H / 2f) + diceGap * 0.5f
                val btnH     = uy(DIE_W)
                val btnW     = bw
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
                drawPath(undoPath, Color(0xFF8B1A1A))
                // Commit
                val cx = gapCX + diceGap * 0.5f  // button2 left edge
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
                drawPath(commitPath, Color(0xFF2E7D32))
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
                if (settings.showPointNumbers) {
                    for (n in 13..24) canvas.nativeCanvas.drawText(
                        n.toString(), ux(pointCentreX(n)), uy(BRD_H) * 0.85f, numPaint)
                    for (n in 12 downTo 1) canvas.nativeCanvas.drawText(
                        n.toString(), ux(pointCentreX(n)), uy(TOT_H) - uy(BRD_H) * 0.15f, numPaint)
                }
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