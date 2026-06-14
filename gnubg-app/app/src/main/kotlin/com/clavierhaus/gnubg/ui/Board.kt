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
import androidx.compose.material3.MaterialTheme
import com.clavierhaus.gnubg.engine.BoardTheme
import com.clavierhaus.gnubg.engine.GameSettings

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

private val STARTING_POSITION = mapOf(
    24 to Pair(0, 2), 13 to Pair(0, 5),
    8  to Pair(0, 3), 6  to Pair(0, 5),
    1  to Pair(1, 2), 12 to Pair(1, 5),
    17 to Pair(1, 3), 19 to Pair(1, 5)
)

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
fun BackgammonBoard(settings: GameSettings = GameSettings()) {
    val cs = MaterialTheme.colorScheme
    val p = if (settings.boardTheme == BoardTheme.SYSTEM) {
        BoardPalette(
            frame            = cs.surface,
            boardField       = cs.tertiaryContainer,
            triangleA        = cs.tertiary,
            triangleB        = cs.tertiaryContainer.copy(alpha = 0.6f),
            bar              = cs.surfaceVariant,
            bearoff          = cs.secondaryContainer,
            numbers          = cs.onSecondaryContainer,
            checkerDark      = cs.primary,
            checkerDarkRim   = cs.onPrimary.copy(alpha = 0.5f),
            checkerLight     = cs.primaryContainer,
            checkerLightRim  = cs.primary.copy(alpha = 0.5f),
            checkerHighlight = androidx.compose.ui.graphics.Color.White.copy(alpha = 0.28f),
            diceDark         = cs.secondary,
            diceLight        = cs.secondaryContainer,
            dicePip          = cs.onSecondary,
            cubeFace         = cs.tertiary,
            cubeDot          = cs.onTertiary,
            cubeText         = cs.onTertiary,
            pipText          = cs.onSurfaceVariant,
            trayOutline      = cs.onSurface.copy(alpha = 0.3f),
            trayDarkBorder   = cs.surface,
        )
    } else {
        BoardPalettes.from(settings.boardTheme)
    }

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
            val color = if (n % 2 == 0) p.triangleA else p.triangleB
            drawTriangle(color, ux(x), uy(BRD_H), ux(PT_W), uy(PT_H), true)
            drawTriangle(color, ux(x), uy(TOT_H - BRD_H), ux(PT_W), uy(PT_H), false)
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

        // Bearoff checkers — example
        val trayR    = tw * 0.42f
        val trayStep = trayR * 2.1f
        val trayCX   = tx + tw / 2f
        val slotPad = ol * 3f
        val slotH  = trayH / 18f
        val slotG  = trayH / 90f
        val slotW  = tw - slotPad * 2f
        val slotX  = tx + slotPad
        for (i in 0 until 3) {
            val slotY = topTrayY + slotPad + i * (slotH + slotG)
            drawTrayChecker(slotX, slotY, slotW, slotH, p.checkerLight, p.checkerLightRim)
        }
        for (i in 0 until 4) {
            val slotY = botTrayY + trayH - slotPad - (i + 1) * slotH - i * slotG
            drawTrayChecker(slotX, slotY, slotW, slotH, p.checkerDark, p.checkerDarkRim)
        }

        // 5. Bar
        drawRect(p.bar,
            topLeft = Offset(ux(MID_X - BAR_W / 2f), uy(BRD_H)),
            size = Size(ux(BAR_W), uy(TOT_H - 2 * BRD_H)))

        // 6. Checkers
        for ((point, playerCount) in STARTING_POSITION) {
            val (player, count) = playerCount
            val cx    = ux(pointCentreX(point))
            val isTop = point in 13..24
            val fill  = if (player == 0) p.checkerDark  else p.checkerLight
            val rim   = if (player == 0) p.checkerDarkRim else p.checkerLightRim
            for (i in 0 until count) {
                val cy = if (isTop) uy(BRD_H) + r + i * step
                         else       uy(TOT_H - BRD_H) - r - i * step
                drawChecker(cx, cy, r, fill, rim, player == 1, p.checkerHighlight)
            }
        }

        // 7. Dice — placeholder 3-1
        val dw  = ux(DIE_W); val dh = ux(DIE_W)
        val gap = ux(PT_W * 0.15f)
        val boardCentreY = uy(TOT_H / 2f)
        val rightHalfCX  = ux((MID_X + BAR_W / 2f + RIGHT_X) / 2f)
        val leftHalfCX   = ux((LEFT_X + MID_X - BAR_W / 2f) / 2f)
        drawDie(rightHalfCX - dw - gap / 2f, boardCentreY - dh / 2f, dw, dh, 3, p.diceDark,  p.dicePip, p.frame)
        drawDie(rightHalfCX + gap / 2f,       boardCentreY - dh / 2f, dw, dh, 1, p.diceDark,  p.dicePip, p.frame)
        drawDie(leftHalfCX  - dw - gap / 2f, boardCentreY - dh / 2f, dw, dh, 3, p.diceLight, p.dicePip, p.frame)
        drawDie(leftHalfCX  + gap / 2f,       boardCentreY - dh / 2f, dw, dh, 1, p.diceLight, p.dicePip, p.frame)

        // 8. Cube + pip counts
        val barCX    = ux(MID_X)
        val barCY    = uy(TOT_H / 2f)
        val cubeSize = ux(BAR_W * 0.75f)
        drawCube(barCX - cubeSize / 2f, barCY - cubeSize / 2f, cubeSize, 64, p.cubeFace, p.cubeDot, p.cubeText)

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
                canvas.nativeCanvas.drawText("167", barCX, topPipY, pipPaint)
                canvas.nativeCanvas.drawText("167", barCX, botPipY, pipPaint)
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
    }
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
    fill: Color, rim: Color, highlight: Boolean,
    highlightColor: Color
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
    left: Float, top: Float, w: Float, h: Float,
    fill: Color, rim: Color
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
