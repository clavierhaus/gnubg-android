package com.clavierhaus.gnubg.ui

import androidx.compose.foundation.Canvas
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
import com.clavierhaus.gnubg.engine.GameSettings
import com.clavierhaus.gnubg.engine.GameViewModel

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
        .pointerInput(gameState.phase, gameState.remainingDice) {
            detectTapGestures { offset ->
                android.util.Log.d("gnubg-ui", "tap at ${offset.x},${offset.y}")
                if (viewModel == null) return@detectTapGestures
                val sx = size.width.toFloat() / TOT_W
                val sy = size.height.toFloat() / TOT_H
                val x = offset.x / sx
                val y = offset.y / sy

                // Check dice tap (swap)
                val boardCY = TOT_H / 2f
                if (y >= boardCY - DIE_W * 2 && y <= boardCY + DIE_W * 2 &&
                    x >= RIGHT_X - DIE_W * 5 && x <= RIGHT_X) {
                    viewModel.swapDice()
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
                android.util.Log.d("gnubg-ui", "tapped=$tapped x=$x y=$y")
                if (tapped >= 0) viewModel.tapSource(tapped)
            }
        }
    ) {
        Canvas(modifier = Modifier.fillMaxSize()) {
            val sx = size.width  / TOT_W
            val sy = size.height / TOT_H
            fun ux(u: Float) = u * sx
            fun uy(u: Float) = u * sy

            val r    = uy(PT_H) / 10f
            val step = uy(PT_H) / 5f

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

            // Bearoff tray — checkers rendered from game state when bearing off is implemented

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

            // Cube drawn after bar checkers
            val cubeBarCX = ux(MID_X)
            val cubeBarCY = uy(TOT_H / 2f)
            val cubeSz = ux(BAR_W * 0.75f)
            drawCube(cubeBarCX - cubeSz / 2f, cubeBarCY - cubeSz / 2f, cubeSz, 64,
                p.cubeFace, p.cubeDot, p.cubeText)

            // 6. Checkers from live board state
            for (n in 1..24) {
                val cx    = ux(pointCentreX(n))
                val isTop = n in 13..24

                // anBoard[0] = engine/dark: index (n-1) → UI point n
                val engineCount = gameState.board[24 - n]
                if (engineCount > 0) {
                    for (i in 0 until engineCount) {
                        val cy = if (isTop) uy(BRD_H) + r + i * step
                                 else       uy(TOT_H - BRD_H) - r - i * step
                        drawChecker(cx, cy, r, p.checkerDark, p.checkerDarkRim, false, p.checkerHighlight)
                    }
                }

                // anBoard[1] = human/light: index (24-n) → UI point n (mirrored)
                val humanCount = gameState.board[24 + n]
                if (humanCount > 0) {
                    for (i in 0 until humanCount) {
                        val cy = if (isTop) uy(BRD_H) + r + i * step
                                 else       uy(TOT_H - BRD_H) - r - i * step
                        drawChecker(cx, cy, r, p.checkerLight, p.checkerLightRim, true, p.checkerHighlight)
                    }
                }

                // Highlight points with moves in history
                if (gameState.moveHistory.isNotEmpty() && false) {
                    drawRect(
                        color = Color.White.copy(alpha = 0.25f),
                        topLeft = Offset(ux(pointX(n)), uy(BRD_H)),
                        size = Size(ux(PT_W), uy(TOT_H - 2 * BRD_H))
                    )
                }
            }

            // 7. Dice
            gameState.dice?.let { (d0, d1) ->
                val isDoubles = d0 == d1
                val diceToShow = if (isDoubles) listOf(d0, d0, d0, d0) else listOf(d0, d1)
                val usedCount = diceToShow.size - gameState.remainingDice.size
                val dw  = ux(DIE_W)
                val dh  = ux(DIE_W)
                val gap = ux(PT_W * 0.15f)
                val boardCentreY = uy(TOT_H / 2f)

                if (gameState.turn == 0) {
                    val totalW = diceToShow.size * dw + (diceToShow.size - 1) * gap
                    val startX = ux(RIGHT_X) - totalW - gap * 2
                    diceToShow.forEachIndexed { i, face ->
                        val isUsed = i < usedCount
                        val dieColor = if (isUsed) p.diceLight.copy(alpha = 0.35f) else p.diceLight
                        drawDie(startX + i * (dw + gap), boardCentreY - dh / 2f,
                            dw, dh, face, dieColor, p.dicePip, p.frame)
                    }
                } else {
                    val totalW = diceToShow.size * dw + (diceToShow.size - 1) * gap
                    val startX = ux(MID_X - BAR_W / 2f) - gap - totalW
                    diceToShow.forEachIndexed { i, face ->
                        val isUsed = i < usedCount
                        val dieColor = if (isUsed) p.diceDark.copy(alpha = 0.35f) else p.diceDark
                        drawDie(startX + i * (dw + gap), boardCentreY - dh / 2f,
                            dw, dh, face, dieColor, p.dicePip, p.frame)
                    }
                }
            }

            // 8. Cube + pip counts (drawn before bar checkers so checkers appear on top)
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
