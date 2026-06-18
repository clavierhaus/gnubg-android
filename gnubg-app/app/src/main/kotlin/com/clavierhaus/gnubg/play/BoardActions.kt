package com.clavierhaus.gnubg.play

interface BoardActions {
    fun offerDouble()
    fun rollDice()
    fun swapDice()
    fun undo()
    fun confirm()
    fun tapSource(point: Int)
}
