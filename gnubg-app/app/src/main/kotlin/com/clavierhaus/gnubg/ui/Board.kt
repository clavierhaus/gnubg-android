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

// Ocean blue palette — fixed seed #1565C0
private val ColorBoardField   = Color(0xFF1565C0)
private val ColorTriangleA    = Color(0xFF1976D2)
private val ColorTriangleB    = Color(0xFF0D47A1)
private val ColorBar          = Color(0xFF0A3880)
private val ColorBearoff      = Color(0xFF0D47A1)
private val ColorFrame        = Color(0xFF082D6B)

// Constants from boarddim.h
private const val CW     = 6f
private const val CH     = 6f
private const val BRD_W  = 3f
private const val BRD_H  = 3f
private const val BAR_W  = 12f
private const val BRF_IN = 6f
private const val BRF_W  = 9f
private const val TOT_W  = 102f
private const val TOT_H  = 82f
private const val PT_H   = 30f

private fun pointX(n: Int): Float = when {
    n < 7  -> TOT_W - BRF_W - n * CW
    n < 13 -> (TOT_W - BAR_W) / 2f - (n - 6) * CW
    n < 19 -> BRF_W + (n - 13) * CW
    else   -> (TOT_W + BAR_W) / 2f + (n - 19) * CW
}

@Composable
fun BackgammonBoard() {
    Canvas(modifier = Modifier.fillMaxSize()) {
        val sx = size.width  / TOT_W
        val sy = size.height / TOT_H

        fun ux(u: Float) = u * sx
        fun uy(u: Float) = u * sy

        // Frame
        drawRect(ColorFrame, size = size)

        // Board field
        drawRect(
            color = ColorBoardField,
            topLeft = Offset(ux(BRD_W), uy(BRD_H)),
            size = Size(ux(TOT_W - 2 * BRD_W), uy(TOT_H - 2 * BRD_H))
        )

        // Left bearoff tray
        drawRect(
            color = ColorBearoff,
            topLeft = Offset(ux(BRD_W), uy(BRD_H)),
            size = Size(ux(BRF_IN), uy(TOT_H - 2 * BRD_H))
        )

        // Right bearoff tray
        drawRect(
            color = ColorBearoff,
            topLeft = Offset(ux(TOT_W - BRD_W - BRF_IN), uy(BRD_H)),
            size = Size(ux(BRF_IN), uy(TOT_H - 2 * BRD_H))
        )

        // Bar
        drawRect(
            color = ColorBar,
            topLeft = Offset(ux((TOT_W - BAR_W) / 2f), uy(BRD_H)),
            size = Size(ux(BAR_W), uy(TOT_H - 2 * BRD_H))
        )

        // 24 points
        for (n in 1..24) {
            val x = pointX(n)
            val color = if (n % 2 == 0) ColorTriangleA else ColorTriangleB

            drawTriangle(color, ux(x), uy(BRD_H), ux(CW), uy(PT_H), true)
            drawTriangle(color, ux(x), uy(TOT_H - BRD_H), ux(CW), uy(PT_H), false)
        }
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
