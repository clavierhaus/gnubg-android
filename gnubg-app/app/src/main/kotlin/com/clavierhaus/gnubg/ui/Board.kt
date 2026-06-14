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
import androidx.compose.ui.graphics.drawscope.drawIntoCanvas
import androidx.compose.ui.graphics.nativeCanvas
import android.graphics.Paint

private val ColorBoardField = Color(0xFF1565C0)
private val ColorTriangleA  = Color(0xFF1976D2)
private val ColorTriangleB  = Color(0xFF0D47A1)
private val ColorBar        = Color(0xFF0A3880)
private val ColorBearoff    = Color(0xFF0F3F8C)
private val ColorFrame      = Color(0xFF082D6B)
private val ColorNumbers    = Color(0xFFB3C9F0)

private const val TOT_W  = 102f
private const val TOT_H  = 82f
private const val BRD_W  = 3f
private const val BRD_H  = 3f
private const val BAR_W  = 7f
private const val BRF_W  = 6f
private const val PT_H   = 30f

// LEFT_X = BRD_W = 3
// RIGHT_X = TOT_W - BRD_W - BRF_W = 102 - 3 - 6 = 93
// MID_X = (LEFT_X + RIGHT_X) / 2 = (3 + 93) / 2 = 48
// HALF_W = (RIGHT_X - LEFT_X - BAR_W) / 2 = (93 - 3 - 7) / 2 = 41.5
// PT_W = HALF_W / 6 = 41.5 / 6 ≈ 6.917

private const val LEFT_X  = BRD_W                  // 3
private const val RIGHT_X = TOT_W - BRD_W - BRF_W  // 93
private const val MID_X   = (LEFT_X + RIGHT_X) / 2f // 48
private const val HALF_W  = (RIGHT_X - LEFT_X - BAR_W) / 2f // 41.5
private const val PT_W    = HALF_W / 6f             // 6.917

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

        // 4. Bearoff tray — right side only
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

        // 6. Point numbers in frame
        val textSize = uy(BRD_H) * 0.75f

        drawIntoCanvas { canvas ->
            val paint = Paint().apply {
                color = android.graphics.Color.argb(
                    (ColorNumbers.alpha * 255).toInt(),
                    (ColorNumbers.red * 255).toInt(),
                    (ColorNumbers.green * 255).toInt(),
                    (ColorNumbers.blue * 255).toInt()
                )
                this.textSize = textSize
                textAlign = Paint.Align.CENTER
                isAntiAlias = true
            }

            for (n in 13..24) {
                val cx = ux(pointCentreX(n))
                val cy = uy(BRD_H) * 0.85f
                canvas.nativeCanvas.drawText(n.toString(), cx, cy, paint)
            }

            for (n in 12 downTo 1) {
                val cx = ux(pointCentreX(n))
                val cy = uy(TOT_H) - uy(BRD_H) * 0.15f
                canvas.nativeCanvas.drawText(n.toString(), cx, cy, paint)
            }
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
