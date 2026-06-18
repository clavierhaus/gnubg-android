package com.clavierhaus.gnubg.play

/**
 * Abstract input contract for the shared board renderer.
 *
 * The board renderer must not know whether it is used by Regular Play,
 * Tutor Mode, or a future analysis surface. It reports user intent through
 * this interface, and the active mode decides what the intent means.
 */
interface BoardActions {
    fun offerDouble()
    fun rollDice()
    fun swapDice()
    fun undo()
    fun confirm()
    fun tapSource(point: Int)
}
