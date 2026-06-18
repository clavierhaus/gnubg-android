package com.clavierhaus.gnubg.tutor

import com.clavierhaus.gnubg.engine.BoardState
import com.clavierhaus.gnubg.engine.GamePhase

/**
 * Neutral board preview positions for Tutor Mode.
 *
 * These positions are not Regular Play sessions. They are lesson surfaces
 * used by Tutor Mode until real Tutor session setup is connected to GNUbg.
 */
object TutorBoardPreview {
    fun openingPosition(): BoardState {
        val board = IntArray(50)

        // Engine-side checkers. The board renderer uses 0..24 for the
        // opponent side and 25..49 for the human side.
        board[24 - 24] = 2
        board[24 - 13] = 5
        board[24 - 8] = 3
        board[24 - 6] = 5

        // Human-side checkers.
        board[24 + 24] = 2
        board[24 + 13] = 5
        board[24 + 8] = 3
        board[24 + 6] = 5

        return BoardState(
            board = board,
            oldBoard = board.copyOf(),
            turn = 0,
            dice = null,
            originalDice = null,
            remainingDice = emptyList(),
            legalMoves = IntArray(0),
            cubeValue = 1,
            cubeOwner = -1,
            pipCountHuman = 167,
            pipCountEngine = 167,
            phase = GamePhase.WAITING_FOR_ROLL,
            humanScore = 0,
            engineScore = 0
        )
    }
}
