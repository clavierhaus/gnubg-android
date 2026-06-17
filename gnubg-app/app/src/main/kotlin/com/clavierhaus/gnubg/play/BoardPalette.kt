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
        checkerDarkRim   = Color(0xFF444444),  // light gray column colour — subtle
        checkerLight     = Color(0xFFF0F0F0),
        checkerLightRim  = Color(0xFF444444),  // same — unifies the palette
        checkerHighlight = Color(0x47FFFFFF),
        diceDark         = Color(0xFF1A1A1A),
        diceLight        = Color(0xFF444444),  // light gray — same as triangleA
        dicePip          = Color(0xFFFFFFFF),
        cubeFace         = Color(0xFF1A1A1A),
        cubeDot          = Color(0xFF444444),
        cubeText         = Color(0xFFFFFFFF),
        pipText          = Color(0xFFAAAAAA),
        trayOutline      = Color(0x8CFFFFFF),
        trayDarkBorder   = Color(0xFF000000),
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
    )

    fun from(theme: BoardTheme): BoardPalette = when (theme) {
        BoardTheme.OCEAN   -> OCEAN
        BoardTheme.CLASSIC -> CLASSIC
        BoardTheme.FOREST  -> FOREST
        BoardTheme.SYSTEM  -> OCEAN  // resolved dynamically in Board.kt
    }
}
