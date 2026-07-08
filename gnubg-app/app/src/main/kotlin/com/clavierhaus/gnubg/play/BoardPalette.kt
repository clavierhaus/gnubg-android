package com.clavierhaus.gnubg.play

import androidx.compose.ui.graphics.Color
import com.clavierhaus.gnubg.engine.BoardTheme

data class BoardPalette(
    val frame:            Color,
    val boardField:       Color,
    val triangleA:        Color,  // dominant
    val triangleB:        Color,  // recessive
    val bar:              Color,
    val bearoff:          Color,
    val numbers:          Color,
    val checkerDark:      Color,
    val checkerDarkRim:   Color,
    val checkerLight:     Color,
    val checkerLightRim:  Color,
    val checkerHighlight: Color,
    val diceDark:         Color,
    val diceLight:        Color,
    val dicePip:          Color,
    val cubeFace:         Color,
    val cubeDot:          Color,
    val cubeText:         Color,
    val pipText:          Color,
    val trayOutline:      Color,
    val trayDarkBorder:   Color,
    // --- UI chrome (surrounding layout: rails, panels, buttons, text) ---
    val uiPanel:          Color,  // rails, score panel, setup background
    val uiPanelDeep:      Color,  // deeper panel / settings background
    val uiChipOn:         Color,  // selected chip / tab
    val uiChipOff:        Color,  // unselected chip / tab
    val uiTextPrimary:    Color,  // primary label text
    val uiTextSecondary:  Color,  // secondary / subtitle text
    val uiTextDisabled:   Color,  // disabled text
    val uiButtonNeutral:  Color,  // neutral / secondary button
    val uiActionRoll:     Color,  // roll / primary action (blue family)
    val uiActionPositive: Color,  // confirm / accept / new (green family)
    val uiActionNegative: Color,  // resign / drop / undo (red family)
)

object BoardPalettes {

    val OCEAN = BoardPalette(
        frame            = Color(0xFF082D6B),
        boardField       = Color(0xFF1565C0),
        triangleA        = Color(0xFF1976D2),
        triangleB        = Color(0xFF0D47A1),
        bar              = Color(0xFF0A3880),
        bearoff          = Color(0xFF0F3F8C),
        numbers          = Color(0xFFB3C9F0),
        checkerDark      = Color(0xFF0D1B4B),
        checkerDarkRim   = Color(0xFF060D26),
        checkerLight     = Color(0xFFE8F0FF),
        checkerLightRim  = Color(0xFFB3C9F0),
        checkerHighlight = Color(0x47FFFFFF),
        diceDark         = Color(0xFF0D47A1),
        diceLight        = Color(0xFF1976D2),
        dicePip          = Color(0xFFFFFFFF),
        cubeFace         = Color(0xFF0D47A1),
        cubeDot          = Color(0xFF1976D2),
        cubeText         = Color(0xFFFFFFFF),
        pipText          = Color(0xFFB3C9F0),
        trayOutline      = Color(0x8CFFFFFF),
        trayDarkBorder   = Color(0xFF051A3E),
        uiPanel          = Color(0xFF2E5A9E),
        uiPanelDeep      = Color(0xFF082D6B),
        uiChipOn         = Color(0xFF1976D2),
        uiChipOff        = Color(0xFF0D47A1),
        uiTextPrimary    = Color(0xFFFFFFFF),
        uiTextSecondary  = Color(0xFFB3C9F0),
        uiTextDisabled   = Color(0xFF8FA8D0),
        uiButtonNeutral  = Color(0xFF243B68),
        uiActionRoll     = Color(0xFF1565C0),
        uiActionPositive = Color(0xFF2E7D32),
        uiActionNegative = Color(0xFF8B1A1A),
    )

    val CLASSIC = BoardPalette(
        frame            = Color(0xFF111111),
        boardField       = Color(0xFF2C2C2C),
        triangleA        = Color(0xFF444444),  // light gray columns
        triangleB        = Color(0xFF1A1A1A),  // dark columns
        bar              = Color(0xFF0A0A0A),
        bearoff          = Color(0xFF222222),
        numbers          = Color(0xFFAAAAAA),
        checkerDark      = Color(0xFF1A1A1A),
        checkerDarkRim   = Color(0xFF444444),  // light gray column colour -- subtle
        checkerLight     = Color(0xFFF0F0F0),
        checkerLightRim  = Color(0xFF444444),  // same -- unifies the palette
        checkerHighlight = Color(0x47FFFFFF),
        diceDark         = Color(0xFF1A1A1A),
        diceLight        = Color(0xFF444444),  // light gray -- same as triangleA
        dicePip          = Color(0xFFFFFFFF),
        cubeFace         = Color(0xFF1A1A1A),
        cubeDot          = Color(0xFF444444),
        cubeText         = Color(0xFFFFFFFF),
        pipText          = Color(0xFFAAAAAA),
        trayOutline      = Color(0x8CFFFFFF),
        trayDarkBorder   = Color(0xFF000000),
        uiPanel          = Color(0xFF2C2C2C),
        uiPanelDeep      = Color(0xFF141414),
        uiChipOn         = Color(0xFF555555),
        uiChipOff        = Color(0xFF262626),
        uiTextPrimary    = Color(0xFFF0F0F0),
        uiTextSecondary  = Color(0xFFAAAAAA),
        uiTextDisabled   = Color(0xFF6E6E6E),
        uiButtonNeutral  = Color(0xFF333333),
        uiActionRoll     = Color(0xFF5A5A5A),
        uiActionPositive = Color(0xFF3E7D42),
        uiActionNegative = Color(0xFF9A3232),
    )

    val FOREST = BoardPalette(
        frame            = Color(0xFF1B3A1F),
        boardField       = Color(0xFF2E7D32),
        triangleA        = Color(0xFF388E3C),
        triangleB        = Color(0xFF1B5E20),
        bar              = Color(0xFF143A17),
        bearoff          = Color(0xFF1E4D22),
        numbers          = Color(0xFFB9F6CA),
        checkerDark      = Color(0xFF1B2E1C),
        checkerDarkRim   = Color(0xFF0A150B),
        checkerLight     = Color(0xFFE8F5E9),
        checkerLightRim  = Color(0xFFB9F6CA),
        checkerHighlight = Color(0x47FFFFFF),
        diceDark         = Color(0xFF1B5E20),
        diceLight        = Color(0xFF388E3C),
        dicePip          = Color(0xFFFFFFFF),
        cubeFace         = Color(0xFF1B5E20),
        cubeDot          = Color(0xFF388E3C),
        cubeText         = Color(0xFFFFFFFF),
        pipText          = Color(0xFFB9F6CA),
        trayOutline      = Color(0x8CFFFFFF),
        trayDarkBorder   = Color(0xFF0A150B),
        uiPanel          = Color(0xFF2E7D32),
        uiPanelDeep      = Color(0xFF1B3A1F),
        uiChipOn         = Color(0xFF388E3C),
        uiChipOff        = Color(0xFF1B5E20),
        uiTextPrimary    = Color(0xFFFFFFFF),
        uiTextSecondary  = Color(0xFFB9F6CA),
        uiTextDisabled   = Color(0xFF7FA886),
        uiButtonNeutral  = Color(0xFF24421F),
        uiActionRoll     = Color(0xFF66BB6A),
        uiActionPositive = Color(0xFF81C784),
        uiActionNegative = Color(0xFF8B1A1A),
    )

    fun from(theme: BoardTheme): BoardPalette = when (theme) {
        BoardTheme.OCEAN   -> OCEAN
        BoardTheme.CLASSIC -> CLASSIC
        BoardTheme.FOREST  -> FOREST
        BoardTheme.SYSTEM  -> OCEAN  // resolved dynamically in Board.kt
    }
}

/**
 * The active board palette, provided once at the play/settings root from the
 * selected theme so any composable can read chrome colors (LocalBoardPalette.current)
 * without threading the palette through every signature. Defaults to OCEAN.
 */
val LocalBoardPalette = androidx.compose.runtime.staticCompositionLocalOf { BoardPalettes.OCEAN }
