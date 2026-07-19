package com.clavierhaus.gnubg.play

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.height
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.clavierhaus.gnubg.engine.GamePhase
import com.clavierhaus.gnubg.engine.GameViewModel

/**
 * The gameplay decisions gnubg REFUSES to advance past until answered:
 * a resignation offer and a cube offer. gnubg blocks every roll while either
 * stands (play.c:1335 for resignation), so if a screen forgets to render the
 * response buttons the game softlocks -- which is exactly what happened when
 * the coach rail reimplemented the phase chain and omitted RESIGNATION_OFFERED
 * (field report, 2026-07-19: a 100%-won bear-off that could never be closed).
 *
 * This composable is the single source of those buttons. Every screen that
 * shows a live game -- the play rail AND the coach rail -- renders it, so no
 * screen can drop a blocking decision again. It draws nothing (returns false-y
 * by rendering an empty column) for non-decision phases; callers keep their
 * own per-phase content for everything else.
 *
 * Returns true if it handled the phase (rendered a decision), so the caller
 * can skip its own branch for that phase.
 */
@Composable
fun GameplayDecisions(
    viewModel: GameViewModel,
    phase: GamePhase,
    resignation: Int,
    cubeValue: Int,
    compact: Boolean = false
): Boolean {
    val pal = LocalBoardPalette.current
    return when (phase) {
        GamePhase.RESIGNATION_OFFERED -> {
            Column(horizontalAlignment = androidx.compose.ui.Alignment.CenterHorizontally) {
                Text(
                    when (resignation) {
                        3 -> "GNU resigns a backgammon"
                        2 -> "GNU resigns a gammon"
                        else -> "GNU resigns"
                    },
                    color = Color.White, fontSize = 16.sp, fontWeight = FontWeight.Bold
                )
                Spacer(Modifier.height(4.dp))
                Text(
                    "Worth " + (resignation * cubeValue) +
                        (if (resignation * cubeValue == 1) " point" else " points"),
                    color = pal.uiTextSecondary, fontSize = 13.sp
                )
                Spacer(Modifier.height(8.dp))
                GameButton("Accept", pal.uiActionPositive, compact = compact) { viewModel.acceptResignation() }
                Spacer(Modifier.height(6.dp))
                GameButton("Play on", pal.uiButtonNeutral, compact = compact) { viewModel.declineResignation() }
            }
            true
        }
        GamePhase.CUBE_OFFERED -> {
            Column(horizontalAlignment = androidx.compose.ui.Alignment.CenterHorizontally) {
                Text(
                    "GNU doubles to ${cubeValue * 2}.",
                    color = Color.White, fontSize = 16.sp, fontWeight = FontWeight.Bold
                )
                Spacer(Modifier.height(8.dp))
                Row(horizontalArrangement = Arrangement.spacedBy(10.dp)) {
                    GameButton("Take", pal.uiActionPositive, compact = compact) { viewModel.acceptDouble() }
                    GameButton("Drop", pal.uiActionNegative, compact = compact) { viewModel.dropDouble() }
                }
            }
            true
        }
        else -> false
    }
}
