package com.clavierhaus.gnubg.stats

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.clavierhaus.gnubg.engine.BusyKind
import com.clavierhaus.gnubg.engine.GameViewModel
import com.clavierhaus.gnubg.engine.MatchError
import com.clavierhaus.gnubg.engine.MatchReport
import com.clavierhaus.gnubg.shared.PlusUi

/**
 * Your Personal Stats -- v1, the G4 vehicle (docs/PERSONAL_STATS_DESIGN.md).
 *
 * Phrase-free by law L4: this screen renders gnubg's numbers, labeled, and
 * says nothing interpretive. The laws it implements:
 *   L1 every number carries its convention in its own label;
 *   L2 layers not modes -- no display state, the details section is plain;
 *   L3 the error list leads; the rating is one context line whose noise
 *      caveat is part of the line;
 *   L5 the details section shows the native per-decision figures desktop
 *      GNU Backgammon prints for the same match file, plus how to check;
 *   L6 the x500 figure is always labeled "(gnubg scale)" -- never bare "PR".
 *
 * Player mapping: index 0 = You, index 1 = GNU -- the same engine indexing
 * the VM's score projection uses (GameViewModel: humanScore = score[0],
 * engineScore = score[1]).
 *
 * The rating ladder is gnubg's own, transliterated verbatim from
 * analysis.c:66-93 in this tree (aszRating + arThrsRating + GetRating's
 * descending scan). Skill words follow the skilltype enum order,
 * analysis.h:29-34.
 */

// analysis.c:66-72 (aszRating), in enum order RAT_BEGINNER..RAT_SUPERGRANDMASTER
private val RATING_WORDS = listOf(
    "Beginner", "Intermediate", "Advanced", "Master", "Grandmaster", "Super Grandmaster"
)

// analysis.c:83-85 (arThrsRating): { 1e38, 0.032, 0.020, 0.013, 0.008, 0.005 }
private val RATING_THRESHOLDS =
    floatArrayOf(1e38f, 0.032f, 0.020f, 0.013f, 0.008f, 0.005f)

// analysis.c:92-99 (GetRating): scan from the top; first i with rError < thr[i].
private fun ratingWord(perDecisionRate: Float): String {
    for (i in RATING_THRESHOLDS.indices.reversed())
        if (perDecisionRate < RATING_THRESHOLDS[i]) return RATING_WORDS[i]
    return RATING_WORDS[0]
}

// analysis.h:29-34 (skilltype): VERYBAD, BAD, DOUBTFUL, NONE
private val SKILL_WORDS = listOf("Very bad", "Bad", "Doubtful", "OK")
private fun skillWord(s: Int) = SKILL_WORDS.getOrElse(s) { "?" }

private val BG = Color(0xFF0B1B33)
private val CARD = Color(0xFF13294B)
private val TEXT = Color.White
private val DIM = Color(0xFFB8C4D8)

@Composable
fun PersonalStatsScreen(
    viewModel: GameViewModel,
    onClose: () -> Unit
) {
    val report by viewModel.matchReport.collectAsState()
    val busy by viewModel.busyKind.collectAsState()

    // Consumer-triggered (the fence): opening the screen asks for the
    // analysis; the VM no-ops if a report is already cached or one is
    // running. The free edition has no such screen and therefore never
    // analyses.
    LaunchedEffect(Unit) { viewModel.analyseCompletedMatch() }

    Column(
        modifier = Modifier
            .fillMaxSize()
            .background(BG)
            .padding(16.dp)
    ) {
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.SpaceBetween,
            verticalAlignment = Alignment.CenterVertically
        ) {
            Text("Your Personal Stats", color = TEXT, fontSize = 18.sp, fontWeight = FontWeight.Bold)
            Box(
                modifier = Modifier
                    .background(PlusUi.Interactive, RoundedCornerShape(8.dp))
                    .clickable { onClose() }
                    .padding(horizontal = 14.dp, vertical = 6.dp)
            ) { Text("Close", color = Color.White, fontSize = 13.sp, fontWeight = FontWeight.Bold) }
        }
        Spacer(modifier = Modifier.height(10.dp))

        val r = report
        when {
            r == null && busy == BusyKind.ANALYSING ->
                Text("Analysing your match...", color = DIM, fontSize = 14.sp)
            r == null ->
                Text(
                    "No completed match to analyse. The report covers the match " +
                    "just finished and resets when a new match starts.",
                    color = DIM, fontSize = 14.sp
                )
            !r.valid ->
                // G3 failed: the numbers are withheld, never shown (design law).
                Text(
                    "Internal consistency check failed -- stats withheld. " +
                    "See the gnubg-vm log (PR G3 MISMATCH).",
                    color = Color(0xFFE07A5F), fontSize = 14.sp
                )
            else -> ReportBody(r)
        }
    }
}

@Composable
private fun ReportBody(r: MatchReport) {
    // The app is landscape-only (sensorLandscape; the sensor flips 180° only,
    // never to portrait), so the width is always there to use. Three columns,
    // each independently scrollable, keep the whole report on one screen:
    //   1. Scorecard  -- the headline aggregates (compartmentalisation: the
    //      match totals lead; the per-move receipts are drill-down)
    //   2. Details    -- You / GNU native figures side by side, readable
    //      against each other, + the L5 verify line
    //   3. Costliest moves -- L3's receipts, in their own scroll
    Row(
        modifier = Modifier.fillMaxSize(),
        horizontalArrangement = Arrangement.spacedBy(16.dp)
    ) {
        // --- Column 1: scorecard (Rating + Result) --------------------------
        Column(
            modifier = Modifier
                .weight(1f)
                .fillMaxHeight()
                .verticalScroll(rememberScrollState()),
            verticalArrangement = Arrangement.spacedBy(10.dp)
        ) {
            Text("Games analysed: ${r.games}", color = DIM, fontSize = 13.sp)

            Text("Rating", color = TEXT, fontSize = 15.sp, fontWeight = FontWeight.Bold)
            RatingLine("You", r.prPerPlayer[0], r.perDecisionRate[0])
            RatingLine("GNU", r.prPerPlayer[1], r.perDecisionRate[1])
            Text(
                "One match -- this number settles over ~20 matches.",
                color = DIM, fontSize = 12.sp
            )

            Spacer(modifier = Modifier.height(4.dp))
            Text("Result", color = TEXT, fontSize = 15.sp, fontWeight = FontWeight.Bold)
            Text(
                "Actual %.2f  ·  luck-adjusted %.2f  (points, you)"
                    .format(r.actualResult[0], r.luckAdjResult[0]),
                color = TEXT, fontSize = 13.sp
            )
        }

        // --- Column 2: details (You / GNU) + verify line --------------------
        Column(
            modifier = Modifier
                .weight(1f)
                .fillMaxHeight()
                .verticalScroll(rememberScrollState()),
            verticalArrangement = Arrangement.spacedBy(10.dp)
        ) {
            Text("Details", color = TEXT, fontSize = 15.sp, fontWeight = FontWeight.Bold)
            DetailsBlock("You", 0, r)
            DetailsBlock("GNU", 1, r)
            Text(
                "Verify: save this match, analyse the same file in desktop " +
                "GNU Backgammon -- these numbers match.",
                color = DIM, fontSize = 12.sp
            )
        }

        // --- Column 3: costliest moves (L3 hero, own scroll) ----------------
        Column(
            modifier = Modifier
                .weight(1f)
                .fillMaxHeight(),
            verticalArrangement = Arrangement.spacedBy(10.dp)
        ) {
            Text("Your costliest moves", color = TEXT, fontSize = 15.sp, fontWeight = FontWeight.Bold)
            val yours = r.errors.filter { it.player == 0 }   // index 0 = You (VM score mapping)
            if (yours.isEmpty()) {
                Text(
                    "No chequer errors recorded for you in this match.",
                    color = DIM, fontSize = 13.sp
                )
            } else {
                LazyColumn(verticalArrangement = Arrangement.spacedBy(10.dp)) {
                    items(yours.size) { i -> ErrorRow(yours[i]) }
                }
            }
        }
    }
}

@Composable
private fun ErrorRow(e: MatchError) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .background(CARD, RoundedCornerShape(8.dp))
            .padding(horizontal = 12.dp, vertical = 8.dp),
        horizontalArrangement = Arrangement.SpaceBetween
    ) {
        Text(
            "Game ${e.gameIdx + 1} · record ${e.recIdx}",
            color = DIM, fontSize = 13.sp
        )
        Text(
            "%.3f EMG · %s".format(e.errorEmg, skillWord(e.skill)),
            color = TEXT, fontSize = 13.sp, fontWeight = FontWeight.Bold
        )
    }
}

@Composable
private fun RatingLine(who: String, er500: Float, rate: Float) {
    Row(modifier = Modifier.fillMaxWidth()) {
        Text(who, color = DIM, fontSize = 13.sp, modifier = Modifier.width(44.dp))
        Text(
            "ER %.1f (gnubg scale)  —  %s".format(er500, ratingWord(rate)),
            color = TEXT, fontSize = 13.sp
        )
    }
}

@Composable
private fun DetailsBlock(who: String, p: Int, r: MatchReport) {
    Column(
        modifier = Modifier
            .fillMaxWidth()
            .background(CARD, RoundedCornerShape(8.dp))
            .padding(12.dp)
    ) {
        Text(who, color = TEXT, fontSize = 13.sp, fontWeight = FontWeight.Bold)
        Text(
            "%.4f per decision (mEMG)  ·  unforced moves %d  ·  close cube %d"
                .format(r.perDecisionRate[p], r.unforcedMoves[p], r.closeCube[p]),
            color = DIM, fontSize = 12.sp
        )
        Text(
            "chequer error %.3f EMG  ·  cube error %.3f EMG  ·  luck %.3f EMG"
                .format(r.chequerErrEmg[p], r.cubeErrEmg[p], r.luckEmg[p]),
            color = DIM, fontSize = 12.sp
        )
        Text(
            "skill: Very bad %d · Bad %d · Doubtful %d"
                .format(r.skillHisto[p][0], r.skillHisto[p][1], r.skillHisto[p][2]),
            color = DIM, fontSize = 12.sp
        )
    }
}
