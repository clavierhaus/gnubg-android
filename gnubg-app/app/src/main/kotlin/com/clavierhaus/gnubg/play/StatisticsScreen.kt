package com.clavierhaus.gnubg.play

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.material3.Text
import com.clavierhaus.gnubg.engine.Difficulty
import com.clavierhaus.gnubg.engine.MatchTally

/**
 * All-time tally (FOSS, issue-tracker request): the tournament's own
 * scoreboard. Reached from the Play match-over panel -- statistics are not a
 * mode, so this is deliberately NOT a hub entry.
 *
 * Display law: every rate sits beside its sample size and its expected
 * value, so the numbers teach convergence instead of inviting small-sample
 * readings. Expected doubles = 6/36 = 16.7%; expected face-pip mean = 7.00
 * (face values as rolled -- movement with doubles counted fourfold would be
 * 8.17, which these tallies deliberately are not). GNU's match record is
 * your mirror, computed here, never stored.
 */
@Composable
fun StatisticsScreen(onBack: () -> Unit) {
    val context = LocalContext.current
    val tallies by MatchTally.tallyFlow(context).collectAsState(initial = emptyMap())

    Box(
        modifier = Modifier
            .fillMaxSize()
            .background(Color(0xFF1B3A5C))
            .padding(24.dp)
    ) {
        Column(
            modifier = Modifier
                .fillMaxSize()
                .verticalScroll(rememberScrollState())
        ) {
            Text("Statistics", color = Color.White,
                fontSize = 22.sp, fontWeight = FontWeight.Bold)
            Text("All finished tournament matches, by level. Coach sessions are not counted.",
                color = Color(0xBBFFFFFF), fontSize = 12.sp)
            Spacer(Modifier.height(16.dp))

            if (tallies.isEmpty()) {
                Text("No finished matches yet. Statistics begin with your first decided match.",
                    color = Color.White, fontSize = 14.sp)
            }

            Difficulty.entries.forEach { level ->
                val t = tallies[level] ?: return@forEach
                Column(
                    modifier = Modifier
                        .fillMaxWidth()
                        .background(Color(0x1AFFFFFF))
                        .padding(12.dp)
                ) {
                    Row(verticalAlignment = Alignment.CenterVertically) {
                        Text(level.label, color = Color(0xFFF5A623),
                            fontSize = 16.sp, fontWeight = FontWeight.Bold)
                        Spacer(Modifier.width(10.dp))
                        Text("${t.played} match${if (t.played == 1L) "" else "es"}",
                            color = Color(0xBBFFFFFF), fontSize = 12.sp)
                    }
                    Spacer(Modifier.height(6.dp))
                    Row(modifier = Modifier.fillMaxWidth()) {
                        StatCell("", "You", "GNU", header = true)
                    }
                    StatCell("Matches won",
                        "${t.won}", "${t.lost}")
                    StatCell("Matches lost",
                        "${t.lost}", "${t.won}")
                    StatCell("Rolls",
                        "${t.rollsYou}", "${t.rollsGnu}")
                    StatCell("Doubles (expect 16.7%)",
                        pct(t.doublesYou, t.rollsYou), pct(t.doublesGnu, t.rollsGnu))
                    StatCell("Pips per roll (expect 7.00)",
                        mean(t.pipsYou, t.rollsYou), mean(t.pipsGnu, t.rollsGnu))
                }
                Spacer(Modifier.height(12.dp))
            }
        }

        Text("Back", color = Color.White, fontSize = 16.sp,
            fontWeight = FontWeight.Bold,
            modifier = Modifier
                .align(Alignment.BottomEnd)
                .clickable(onClick = onBack)
                .padding(16.dp))
    }
}

@Composable
private fun StatCell(label: String, you: String, gnu: String, header: Boolean = false) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(vertical = 2.dp),
        horizontalArrangement = Arrangement.SpaceBetween
    ) {
        Text(label, color = if (header) Color(0xBBFFFFFF) else Color.White,
            fontSize = 13.sp, modifier = Modifier.weight(1.6f))
        Text(you, color = if (header) Color(0xBBFFFFFF) else Color.White,
            fontSize = 13.sp,
            fontWeight = if (header) FontWeight.Bold else FontWeight.Normal,
            modifier = Modifier.weight(0.7f))
        Text(gnu, color = if (header) Color(0xBBFFFFFF) else Color.White,
            fontSize = 13.sp,
            fontWeight = if (header) FontWeight.Bold else FontWeight.Normal,
            modifier = Modifier.weight(0.7f))
    }
}

/* Rates carry their sample size: "16.7% of 38" -- a rate without its n
 * invites exactly the misreading this screen exists to prevent. */
private fun pct(part: Long, total: Long): String =
    if (total == 0L) "—"
    else "%.1f%% of %d".format(100.0 * part / total, total)

private fun mean(sum: Long, count: Long): String =
    if (count == 0L) "—"
    else "%.2f".format(sum.toDouble() / count)
