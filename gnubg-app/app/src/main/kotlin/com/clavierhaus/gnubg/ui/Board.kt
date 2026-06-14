package com.clavierhaus.gnubg.ui

import androidx.compose.foundation.Canvas
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

private val ColorBoardField       = Color(0xFF1565C0)
private val ColorTriangleA        = Color(0xFF1976D2)
private val ColorTriangleB        = Color(0xFF0D47A1)
private val ColorBar              = Color(0xFF0A3880)
private val ColorBearoff          = Color(0xFF0F3F8C)
private val ColorFrame            = Color(0xFF082D6B)
private val ColorNumbers          = Color(0xFFB3C9F0)
private val ColorCheckerDark      = Color(0xFF0D1B4B)
private val ColorCheckerDarkRim   = Color(0xFF060D26)
private val ColorCheckerLight     = Color(0xFFE8F0FF)
private val ColorCheckerLightRim  = Color(0xFFB3C9F0)
private val ColorCheckerHighlight = Color(0x47FFFFFF)
private val ColorDiceDark         = Color(0xFF0D47A1)  // dark triangle colour
private val ColorDiceLight        = Color(0xFF1976D2)  // light triangle colour
private val ColorDicePip          = Color(0xFFFFFFFF)

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

// Dice size
private const val DIE_W   = PT_W * 0.8f

private val STARTING_POSITION = mapOf(
    24 to Pair(0, 2),
    13 to Pair(0, 5),
    8  to Pair(0, 3),
    6  to Pair(0, 5),
    1  to Pair(1, 2),
    12 to Pair(1, 5),
    17 to Pair(1, 3),
    19 to Pair(1, 5)
)

// Pip positions for faces 1-6 as fractions of die size
// Each entry is list of (fx, fy) where 0.0=top-left, 1.0=bottom-right
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
fun BackgammonBoard() {
    Canvas(modifier = Modifier.fillMaxSize()) {
        val sx = size.width  / TOT_W
        val sy = size.height / TOT_H

        fun ux(u: Float) = u * sx
        fun uy(u: Float) = u * sy

        val r    = uy(PT_H) / 10f
        val step = uy(PT_H) / 5f

        // 1. Frame
        drawRect(ColorFrame, size = size)

        // 2. Board field
        drawRect(
            color = ColorBoardField,
            topLeft = Offset(ux(BRD_W), uy(BRD_H)),
            size = Size(ux(TOT_W - 2 * BRD_W), uy(TOT_H - 2 * BRD_H))
        )

        // 3. Triangles
        for (n in 1..24) {
            val x = pointX(n)
            val color = if (n % 2 == 0) ColorTriangleA else ColorTriangleB
            drawTriangle(color, ux(x), uy(BRD_H), ux(PT_W), uy(PT_H), true)
            drawTriangle(color, ux(x), uy(TOT_H - BRD_H), ux(PT_W), uy(PT_H), false)
        }

        // 4. Bearoff tray
        drawRect(
            color = ColorBearoff,
            topLeft = Offset(ux(RIGHT_X), uy(BRD_H)),
            size = Size(ux(BRF_W), uy(TOT_H - 2 * BRD_H))
        )

        // 5. Bar
        drawRect(
            color = ColorBar,
            topLeft = Offset(ux(MID_X - BAR_W / 2f), uy(BRD_H)),
            size = Size(ux(BAR_W), uy(TOT_H - 2 * BRD_H))
        )

        // 6. Checkers
        for ((point, playerCount) in STARTING_POSITION) {
            val (player, count) = playerCount
            val cx = ux(pointCentreX(point))
            val isTop = point in 13..24
            val fill = if (player == 0) ColorCheckerDark else ColorCheckerLight
            val rim  = if (player == 0) ColorCheckerDarkRim else ColorCheckerLightRim

            for (i in 0 until count) {
                val cy = if (isTop) {
                    uy(BRD_H) + r + i * step
                } else {
                    uy(TOT_H - BRD_H) - r - i * step
                }
                drawChecker(cx, cy, r, fill, rim, player == 1)
            }
        }

        // 7. Dice — placeholder roll 3-1
        // Dark player (0): centre of right half
        // Light player (1): centre of left half
        val dw = ux(DIE_W)
        val dh = ux(DIE_W)  // square die
        val gap = ux(PT_W * 0.15f)
        val boardCentreY = uy(TOT_H / 2f)

        // Dark player dice in right half centre
        val rightHalfCentreX = ux((MID_X + BAR_W / 2f + RIGHT_X) / 2f)
        drawDie(rightHalfCentreX - dw - gap / 2f, boardCentreY - dh / 2f, dw, dh, 3, ColorDiceDark)
        drawDie(rightHalfCentreX + gap / 2f,       boardCentreY - dh / 2f, dw, dh, 1, ColorDiceDark)

        // Light player dice in left half centre
        val leftHalfCentreX = ux((LEFT_X + MID_X - BAR_W / 2f) / 2f)
        drawDie(leftHalfCentreX - dw - gap / 2f, boardCentreY - dh / 2f, dw, dh, 3, ColorDiceLight)
        drawDie(leftHalfCentreX + gap / 2f,       boardCentreY - dh / 2f, dw, dh, 1, ColorDiceLight)

        // 8. Point numbers in frame
        val textSize = uy(BRD_H) * 0.75f
        drawIntoCanvas { canvas ->
            val paint = android.graphics.Paint().apply {
                color = ColorNumbers.toArgb()
                this.textSize = textSize
                textAlign = android.graphics.Paint.Align.CENTER
                isAntiAlias = true
            }
            for (n in 13..24) {
                canvas.nativeCanvas.drawText(
                    n.toString(), ux(pointCentreX(n)), uy(BRD_H) * 0.85f, paint
                )
            }
            for (n in 12 downTo 1) {
                canvas.nativeCanvas.drawText(
                    n.toString(), ux(pointCentreX(n)), uy(TOT_H) - uy(BRD_H) * 0.15f, paint
                )
            }
        }
    }
}

private fun DrawScope.drawDie(
    left: Float, top: Float, w: Float, h: Float,
    face: Int, dieColor: Color
) {
    val corner = w * 0.15f
    val path = Path().apply {
        moveTo(left + corner, top)
        lineTo(left + w - corner, top)
        quadraticBezierTo(left + w, top, left + w, top + corner)
        lineTo(left + w, top + h - corner)
        quadraticBezierTo(left + w, top + h, left + w - corner, top + h)
        lineTo(left + corner, top + h)
        quadraticBezierTo(left, top + h, left, top + h - corner)
        lineTo(left, top + corner)
        quadraticBezierTo(left, top, left + corner, top)
        close()
    }
    drawPath(path, dieColor)
    drawPath(path, ColorFrame, style = Stroke(width = w * 0.04f))

    // Pips
    val pipR = w * 0.08f
    PIP_POSITIONS[face]?.forEach { (fx, fy) ->
        drawCircle(
            color = ColorDicePip,
            radius = pipR,
            center = Offset(left + fx * w, top + fy * h)
        )
    }
}

private fun DrawScope.drawChecker(
    cx: Float, cy: Float, r: Float,
    fill: Color, rim: Color, highlight: Boolean
) {
    drawCircle(fill, r, Offset(cx, cy))
    drawCircle(
        color = rim, radius = r, center = Offset(cx, cy),
        style = Stroke(width = r * 0.08f)
    )
    if (highlight) {
        drawOval(
            color = ColorCheckerHighlight,
            topLeft = Offset(cx - r * 0.66f, cy - r * 0.47f),
            size = Size(r * 0.76f, r * 0.38f)
        )
    }
}

private fun DrawScope.drawTriangle(
    color: Color, x: Float, y: Float,
    width: Float, height: Float, pointingDown: Boolean
) {
    val path = Path().apply {
        if (pointingDown) {
            moveTo(x, y)
            lineTo(x + width, y)
            lineTo(x + width / 2f, y + height)
        } else {
            moveTo(x, y)
            lineTo(x + width, y)
            lineTo(x + width / 2f, y - height)
        }
        close()
    }
    drawPath(path, color)
}
