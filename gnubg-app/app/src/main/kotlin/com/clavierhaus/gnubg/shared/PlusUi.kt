package com.clavierhaus.gnubg.shared

import androidx.compose.ui.graphics.Color

/**
 * Plus-edition UI convention (PLUS.md):
 *
 *   Every interactive element that exists ONLY in the Plus edition is
 *   coloured [Interactive] orange -- exclusively. The free edition uses no
 *   orange outside the hub's identity styling; therefore, in a running app,
 *   orange control == Plus feature, at a glance.
 *
 * All Plus-only controls take their colour from here, never a local constant.
 */
object PlusUi {
    /** #F5A623 -- the brand orange, reserved in-UI for Plus interactivity. */
    val Interactive = Color(0xFFF5A623)
}
