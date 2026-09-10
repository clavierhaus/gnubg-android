package com.clavierhaus.gnubg.shared

import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.BoxScope
import androidx.compose.foundation.layout.BoxWithConstraints
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.CompositionLocalProvider
import androidx.compose.runtime.compositionLocalOf
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.unit.Density
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

/*
 * THE SCREEN IS ONE PICTURE (layout law, 2026-09-10).
 *
 * The app targets a long tail of resolutions, densities and aspect ratios,
 * and nothing in it scrolls. Every screen was drawn once, on the reference
 * device, in dp and sp; those numbers ARE the design. A screen fitted to
 * another device by hand (a spacer nudged, a width tuned "so that it clears
 * the chips on a Pixel 8 Pro") is a band-aid that moves the overflow to
 * the next tenant, and every new device report reopens it.
 *
 * So: a screen is laid out AS IF the pane were always the reference size.
 * On a smaller pane, every dp and sp inside the screen is scaled by one
 * factor, min(width / REF_W, height / REF_H, 1), through the density its
 * children read. Layout, drawing and hit-testing all go through that
 * density, so the one-rectangle invariant holds by construction. At or
 * above the reference size the factor is 1 and nothing moves. A screen
 * that fits on the reference device fits everywhere, by arithmetic.
 *
 * And: a screen is a PARTITION. Rows and columns are declared by weight,
 * so the cells tile the pane and cannot overlap; corner controls are cells,
 * never overlays. Cell content is at most reference-size (from the rule
 * above), so it can only overflow a cell if it already overflowed on the
 * reference device -- which is caught once, there.
 *
 * Canvas-drawn content that is already relative to its own size (the
 * board) opts out with Unscaled. Below MIN_SCALE the screen says so rather
 * than render an unreadable one: correct-or-silent, applied to geometry.
 */

/** The reference device's landscape box: Pixel 8 Pro, 1344x2992 px at 480 dpi. */
object ScreenRef {
    val WIDTH = 997.dp
    val HEIGHT = 448.dp
    /** Below this the picture is unreadable; the screen says so instead. */
    const val MIN_SCALE = 0.55f
}

/** The density of the device itself, captured before any screen scaling. */
val LocalBaseDensity = compositionLocalOf<Density?> { null }

/** The factor the current screen is drawn at (1f at or above reference). */
val LocalScreenScale = compositionLocalOf { 1f }

/**
 * The root of every screen. Measures the pane, derives the one scale, and
 * provides the scaled density to everything inside. [content] receives the
 * scale for the rare case that needs it (logging, a debug overlay); it must
 * not use it to size anything -- that is what the density is for.
 */
@Composable
fun OnePicture(
    modifier: Modifier = Modifier,
    content: @Composable (scale: Float) -> Unit
) {
    val base = LocalBaseDensity.current ?: LocalDensity.current
    BoxWithConstraints(modifier = modifier.fillMaxSize()) {
        // maxWidth/maxHeight are read in the density in force HERE, which is
        // the base density: this Box sits outside the provider below.
        val paneW = maxWidth
        val paneH = maxHeight
        val scale = minOf(
            paneW / ScreenRef.WIDTH,
            paneH / ScreenRef.HEIGHT,
            1f
        )
        if (scale < ScreenRef.MIN_SCALE) {
            Box(modifier = Modifier.fillMaxSize().padding(16.dp), contentAlignment = Alignment.Center) {
                Text(
                    "This display is too small for CBG " +
                        "(${paneW.value.toInt()}x${paneH.value.toInt()} dp; " +
                        "needs at least ${(ScreenRef.WIDTH * ScreenRef.MIN_SCALE).value.toInt()}x" +
                        "${(ScreenRef.HEIGHT * ScreenRef.MIN_SCALE).value.toInt()}).",
                    color = Color.White,
                    fontSize = 14.sp
                )
            }
            return@BoxWithConstraints
        }
        CompositionLocalProvider(
            LocalBaseDensity provides base,
            LocalScreenScale provides scale,
            LocalDensity provides Density(density = base.density * scale, fontScale = base.fontScale)
        ) {
            content(scale)
        }
    }
}

/**
 * Restores the device's own density for content that is drawn relative to
 * its own size and must not be scaled twice -- the board canvas.
 */
@Composable
fun Unscaled(content: @Composable () -> Unit) {
    val base = LocalBaseDensity.current ?: LocalDensity.current
    CompositionLocalProvider(LocalDensity provides base) { content() }
}

/**
 * A vertical partition: every row is weighted, so the rows tile the pane
 * and cannot overflow it. There is deliberately no way to add an unweighted
 * row -- an unweighted child in a partition is exactly the defect this
 * exists to prevent (issue #7 and its siblings).
 */
@Composable
fun ScreenRows(
    modifier: Modifier = Modifier,
    content: ScreenRowsScope.() -> Unit
) {
    val scope = ScreenRowsScope().apply(content)
    Column(modifier = modifier.fillMaxSize()) {
        scope.rows.forEach { (weight, cell) ->
            Box(modifier = Modifier.weight(weight).fillMaxWidth()) { cell() }
        }
    }
}

class ScreenRowsScope internal constructor() {
    internal val rows = mutableListOf<Pair<Float, @Composable BoxScope.() -> Unit>>()
    fun row(weight: Float, content: @Composable BoxScope.() -> Unit) {
        require(weight > 0f) { "a row must have a positive weight" }
        rows += weight to content
    }
}

/** A horizontal partition, same rule as [ScreenRows]. */
@Composable
fun ScreenCols(
    modifier: Modifier = Modifier,
    content: ScreenColsScope.() -> Unit
) {
    val scope = ScreenColsScope().apply(content)
    Row(modifier = modifier.fillMaxSize()) {
        scope.cols.forEach { (weight, cell) ->
            Box(modifier = Modifier.weight(weight).fillMaxHeight()) { cell() }
        }
    }
}

class ScreenColsScope internal constructor() {
    internal val cols = mutableListOf<Pair<Float, @Composable BoxScope.() -> Unit>>()
    fun col(weight: Float, content: @Composable BoxScope.() -> Unit) {
        require(weight > 0f) { "a column must have a positive weight" }
        cols += weight to content
    }
}
