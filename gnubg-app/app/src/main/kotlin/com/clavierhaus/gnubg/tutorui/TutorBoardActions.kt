package com.clavierhaus.gnubg.tutorui

import com.clavierhaus.gnubg.play.BoardActions
import com.clavierhaus.gnubg.tutor.TutorSessionController
import com.clavierhaus.gnubg.tutor.TutorSessionState

class TutorBoardActions(
    private val controller: TutorSessionController,
    private val getState: () -> TutorSessionState,
    private val setState: (TutorSessionState) -> Unit
) : BoardActions {
    override fun offerDouble() {
        // Tutor Mode owns cube teaching separately; no-op for now.
    }

    override fun rollDice() {
        // Tutor Mode will use guided dice/session flow later.
    }

    override fun swapDice() {
        // Tutor Mode has no dice-order interaction yet.
    }

    override fun undo() {
        // Tutor Try Again will be a separate neutral restore contract.
    }

    override fun confirm() {
        // Tutor commits will be wired through Tutor session semantics later.
    }

    override fun tapSource(point: Int) {
        setState(controller.selectPoint(getState(), point))
    }
}
