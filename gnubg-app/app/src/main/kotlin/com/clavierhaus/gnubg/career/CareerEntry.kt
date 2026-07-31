package com.clavierhaus.gnubg.career

import android.content.Context
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.widthIn
import androidx.compose.foundation.clickable
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.datastore.preferences.core.booleanPreferencesKey
import androidx.datastore.preferences.core.edit
import androidx.datastore.preferences.preferencesDataStore
import com.clavierhaus.gnubg.play.LocalBoardPalette
import com.clavierhaus.gnubg.shared.PlusUi
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.first
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

/**
 * The career where it belongs: tournament mode's entry screen (maintainer
 * ruling -- the career is calculated in and exclusive to tournament play,
 * so that is where it resides; Settings keeps only the Details tier).
 *
 * Teaching wording shows ONCE, acknowledged, then never again -- repeated,
 * it would decay into wallpaper. Afterwards a circled i beside the live
 * status recalls the same explanation on demand. All strings are the
 * maintainer-approved set; nothing here invents copy.
 */

private val Context.careerPrefs by preferencesDataStore(name = "cbg_career")
private val INTRO_ACK = booleanPreferencesKey("intro_acknowledged")

private const val INTRO_TEXT =
    "Your career starts with your next finished match. Every tournament " +
    "match is analysed and recorded — yours to keep, and yours to prove.\n" +
    "Coach matches are practice: the gym doesn't go on your record."

@Composable
fun CareerEntryBlock(modifier: Modifier = Modifier) {
    val ctx = LocalContext.current
    val pal = LocalBoardPalette.current
    val scope = rememberCoroutineScope()

    var acked by remember { mutableStateOf<Boolean?>(null) }   // null = loading
    var status by remember { mutableStateOf<CareerVerify.Result?>(null) }
    var infoOpen by remember { mutableStateOf(false) }

    LaunchedEffect(Unit) {
        acked = withContext(Dispatchers.IO) {
            runCatching { ctx.careerPrefs.data.first()[INTRO_ACK] == true }
                .getOrDefault(false)
        }
        status = withContext(Dispatchers.Default) { CareerVerify.run(ctx) }
    }

    val a = acked ?: return   // brief first-frame silence beats a flicker

    Column(modifier = modifier.widthIn(max = 340.dp)) {
        Text(
            "Your career",
            color = PlusUi.Interactive, fontSize = 14.sp, fontWeight = FontWeight.Bold
        )
        Spacer(Modifier.height(3.dp))

        if (!a) {
            // First encounter: the teaching, once, acknowledged.
            Text(INTRO_TEXT, color = pal.uiTextPrimary, fontSize = 12.sp)
            Spacer(Modifier.height(6.dp))
            Text(
                "Understood",
                color = PlusUi.Interactive, fontSize = 13.sp, fontWeight = FontWeight.Bold,
                modifier = Modifier.clickable {
                    acked = true
                    scope.launch(Dispatchers.IO) {
                        runCatching { ctx.careerPrefs.edit { it[INTRO_ACK] = true } }
                    }
                }
            )
            return@Column
        }

        // Acknowledged: the live status, with the circled i recalling the intro.
        val r = status
        val line = when {
            r == null -> "Reading your record…"
            r.empty -> "Your career starts with your next finished match."
            r.intact ->
                "Your career: ${r.entries} ${if (r.entries == 1) "match" else "matches"}, " +
                "every one verified — nothing has been changed since it was played."
            else -> r.findings.first()
        }
        Text(
            "$line  ⓘ",
            color = if (r != null && !r.empty && !r.intact) pal.uiTextPrimary
                    else pal.uiTextSecondary,
            fontSize = 12.sp,
            modifier = Modifier.clickable { infoOpen = !infoOpen }
        )
        if (infoOpen) {
            Spacer(Modifier.height(4.dp))
            Text(INTRO_TEXT, color = pal.uiTextSecondary, fontSize = 11.sp)
        }
    }
}
