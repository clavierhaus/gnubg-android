package com.clavierhaus.gnubg.options

import com.clavierhaus.gnubg.play.LocalBoardPalette
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.RadioButton
import androidx.compose.material3.RadioButtonColors
import androidx.compose.material3.RadioButtonDefaults
import androidx.compose.material3.Switch
import androidx.compose.material3.SwitchColors
import androidx.compose.material3.SwitchDefaults
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.clavierhaus.gnubg.engine.BoardTheme
import com.clavierhaus.gnubg.engine.Difficulty
import com.clavierhaus.gnubg.engine.GameSettings
import com.clavierhaus.gnubg.engine.MatchEquityTable
import com.clavierhaus.gnubg.engine.GameViewModel
import android.content.ClipData
import android.content.ClipboardManager
import android.content.Context
import android.os.Build
import android.widget.Toast
import androidx.compose.material3.Button
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.ui.platform.LocalContext
import com.clavierhaus.gnubg.Engine

enum class SettingsTab { GAME, REPORT, BOARD, ENGINE, ANALYSIS, LICENSE }

// Chrome colors now come from LocalBoardPalette (themed). See BoardPalette.kt.

private val switchColors: SwitchColors
    @Composable get() {
        val pal = LocalBoardPalette.current
        return SwitchDefaults.colors(
            checkedThumbColor = pal.uiTextPrimary,
            checkedTrackColor = pal.uiChipOn,
            uncheckedThumbColor = pal.uiTextSecondary,
            uncheckedTrackColor = pal.uiChipOff
        )
    }

private val radioColors: RadioButtonColors
    @Composable get() {
        val pal = LocalBoardPalette.current
        return RadioButtonDefaults.colors(
            selectedColor = pal.uiTextPrimary,
            unselectedColor = pal.uiTextSecondary
        )
    }

@Composable
fun SettingsScreen(
    settings: GameSettings,
    viewModel: GameViewModel,
    onDismiss: () -> Unit
) {
    val pal = LocalBoardPalette.current
    var activeTab by remember { mutableStateOf(SettingsTab.GAME) }

    Box(
        modifier = Modifier
            .fillMaxSize()
            .background(pal.uiPanelDeep)
    ) {
        Column(modifier = Modifier.fillMaxSize()) {
            SettingsHeader(onDismiss = onDismiss)

            SettingsTabs(
                activeTab = activeTab,
                onTab = { activeTab = it }
            )

            Column(
                modifier = Modifier
                    .fillMaxSize()
                    .verticalScroll(rememberScrollState())
                    .padding(horizontal = 18.dp, vertical = 14.dp)
            ) {
                when (activeTab) {
                    SettingsTab.GAME -> GameSettingsTab(settings, viewModel)
                    SettingsTab.REPORT -> ReportSettingsTab()
                    SettingsTab.BOARD -> BoardSettingsTab(settings, viewModel)
                    SettingsTab.ENGINE -> EngineSettingsTab(settings, viewModel)
                    SettingsTab.ANALYSIS -> AnalysisTutorSettingsTab(settings, viewModel)
                    SettingsTab.LICENSE -> LicenseSettingsTab()
                }
            }
        }
    }
}

@Composable
private fun SettingsHeader(onDismiss: () -> Unit) {
    val pal = LocalBoardPalette.current
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(horizontal = 22.dp, vertical = 12.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        Text(
            text = "<-",
            color = pal.uiTextPrimary,
            fontSize = 22.sp,
            modifier = Modifier
                .clickable { onDismiss() }
                .padding(end = 14.dp)
        )
        Column {
            Text(
                text = "Settings",
                color = pal.uiTextPrimary,
                fontSize = 22.sp,
                fontWeight = FontWeight.Bold
            )
            Text(
                text = "Configuration only. Game actions stay on the Play surface.",
                color = pal.uiTextSecondary,
                fontSize = 12.sp
            )
        }
    }
}

@Composable
private fun SettingsTabs(
    activeTab: SettingsTab,
    onTab: (SettingsTab) -> Unit
) {
    val pal = LocalBoardPalette.current
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(horizontal = 14.dp),
        horizontalArrangement = Arrangement.spacedBy(6.dp)
    ) {
        SettingsTab.values().forEach { tab ->
            val label = when (tab) {
                SettingsTab.GAME -> "Tournament"
                SettingsTab.REPORT -> "Report"
                SettingsTab.BOARD -> "Board"
                SettingsTab.ENGINE -> "Engine"
                SettingsTab.ANALYSIS -> "Analysis"
                SettingsTab.LICENSE -> "License"
            }

            Box(
                modifier = Modifier
                    .weight(1f)
                    .background(
                        if (activeTab == tab) pal.uiChipOn else pal.uiChipOff,
                        RoundedCornerShape(9.dp)
                    )
                    .clickable { onTab(tab) }
                    .padding(vertical = 9.dp),
                contentAlignment = Alignment.Center
            ) {
                Text(
                    text = label,
                    color = if (activeTab == tab) pal.uiTextPrimary else pal.uiTextSecondary,
                    fontSize = 13.sp,
                    fontWeight = if (activeTab == tab) FontWeight.Bold else FontWeight.Normal
                )
            }
        }
    }
}

@Composable
private fun GameSettingsTab(settings: GameSettings, vm: GameViewModel) {
    SettingsSection("Match") {
        StepperRow(
            title = "Match length",
            subtitle = "Default points for new matches",
            value = "${settings.matchLength}",
            onMinus = {
                if (settings.matchLength > 1) vm.setMatchLength(settings.matchLength - 1)
            },
            onPlus = {
                if (settings.matchLength < 25) vm.setMatchLength(settings.matchLength + 1)
            }
        )
        SettingsDivider()
        SettingsRow("Crawford rule", "Auto-applied at the Crawford score in match play") {
            Switch(settings.crawford, { vm.setCrawford(it) }, colors = switchColors)
        }
    }

    SettingsSection("Money / session rules") {
        SettingsRow("Jacoby rule", "Gammons/backgammons count only after a double (money play)") {
            Switch(settings.jacoby, { vm.setJacoby(it) }, colors = switchColors)
        }
        SettingsDivider()
        StepperRow(
            title = "Automatic doubles",
            subtitle = "Session configuration value",
            value = "${settings.automaticDoubles}",
            onMinus = {
                if (settings.automaticDoubles > 0) vm.setAutomaticDoubles(settings.automaticDoubles - 1)
            },
            onPlus = {
                if (settings.automaticDoubles < 12) vm.setAutomaticDoubles(settings.automaticDoubles + 1)
            }
        )
        SettingsDivider()
        SettingsRow("Beavers", "Allow immediate redouble on a take (money play)") {
            Switch(settings.beavers, { vm.setBeavers(it) }, colors = switchColors)
        }
    }

    SettingsSection("Cube rules") {
        SettingsRow("Cube enabled", "Use the doubling cube (off = single-game play)") {
            Switch(settings.cubeUse, { vm.setCubeUse(it) }, colors = switchColors)
        }
    }

    SettingsSection("Match equity table") {
        MatchEquityTable.values().forEachIndexed { i, met ->
            SettingsRow(met.displayName, if (i == 0) "Used for match-play cube decisions" else "") {
                RadioButton(
                    selected = settings.metTable == met,
                    onClick = { vm.setMet(met) },
                    colors = radioColors
                )
            }
            if (i < MatchEquityTable.values().size - 1) SettingsDivider()
        }
    }
}

@Composable
private fun BoardSettingsTab(settings: GameSettings, vm: GameViewModel) {
    SettingsSection("Appearance") {
        BoardTheme.values().forEachIndexed { i, theme ->
            SettingsRow(theme.name.lowercase().replaceFirstChar { it.uppercase() }) {
                RadioButton(
                    selected = settings.boardTheme == theme,
                    onClick = { vm.setBoardTheme(theme) },
                    colors = radioColors
                )
            }
            if (i < BoardTheme.values().size - 1) SettingsDivider()
        }
    }

    SettingsSection("Board information") {
        SettingsRow("Point numbers", "Show numbers in the frame border") {
            Switch(settings.showPointNumbers, { vm.setShowPointNumbers(it) }, colors = switchColors)
        }
        SettingsDivider()
        SettingsRow("Pip count", "Show pip counts in the bar") {
            Switch(settings.showPipCount, { vm.setShowPipCount(it) }, colors = switchColors)
        }
    }


}

@Composable
private fun EngineSettingsTab(settings: GameSettings, vm: GameViewModel) {
    SettingsSection("Playing strength") {
        Difficulty.values().forEachIndexed { i, difficulty ->
            SettingsRow(difficulty.label, difficulty.subtitle) {
                RadioButton(
                    selected = settings.difficulty == difficulty,
                    onClick = { vm.setDifficulty(difficulty) },
                    colors = radioColors
                )
            }
            if (i < Difficulty.values().size - 1) SettingsDivider()
        }
    }


}

@Composable
private fun AnalysisTutorSettingsTab(settings: GameSettings, vm: GameViewModel) {
    SettingsSection("Tutor") {
        SettingsRow("Tutor mode", "Comments on your chequer play. Implies a single game (no cube).") {
            Switch(settings.tutorMode, { vm.setTutorMode(it) }, colors = switchColors)
        }
        SettingsDivider()
        SettingsRow("Hint", "Placeholder preference; board action comes later") {
            Switch(settings.hint, { vm.setHint(it) }, colors = switchColors)
        }
    }

    SettingsSection("Output") {
        SettingsRow("Show equity", "Presentation preference") {
            Switch(settings.showEquity, { vm.setShowEquity(it) }, colors = switchColors)
        }
        SettingsDivider()
        SettingsRow("Show MWC", "Presentation preference") {
            Switch(settings.showMWC, { vm.setShowMWC(it) }, colors = switchColors)
        }
    }

    SettingsSection("Thresholds") {
        FloatStepperRow(
            title = "Doubtful",
            subtitle = "Local value; GNUbg command verified but quarantined",
            value = settings.thresholdDoubtful,
            onChange = vm::setThresholdDoubtful
        )
        SettingsDivider()
        FloatStepperRow(
            title = "Bad",
            subtitle = "Local value; GNUbg command verified but quarantined",
            value = settings.thresholdBad,
            onChange = vm::setThresholdBad
        )
        SettingsDivider()
        FloatStepperRow(
            title = "Very bad",
            subtitle = "Local value; GNUbg command verified but quarantined",
            value = settings.thresholdVeryBad,
            onChange = vm::setThresholdVeryBad
        )
    }
}

@Composable
private fun LicenseSettingsTab() {
    AboutLicenseSection()

    val context = LocalContext.current
    val pal = LocalBoardPalette.current
    // The complete license text, verbatim from COPYING, bundled as an asset.
    // Released proudly under GPL-3.0-or-later; the full text earns its own tab.
    val licenseText = remember {
        runCatching {
            context.assets.open("COPYING").bufferedReader().use { it.readText() }
        }.getOrElse { "The full license text ships as COPYING with the source distribution." }
    }
    SettingsSection("GNU General Public License, version 3") {
        Text(
            licenseText,
            color = pal.uiTextSecondary,
            fontSize = 11.sp,
            fontFamily = FontFamily.Monospace,
            modifier = Modifier.padding(horizontal = 14.dp, vertical = 8.dp)
        )
    }
}

@Composable
private fun AboutLicenseSection() {
    val pal = LocalBoardPalette.current
    SettingsSection("About & License") {
        Column(modifier = Modifier.padding(horizontal = 14.dp, vertical = 4.dp)) {
            Text("CBG -- Clavierhaus Backgammon", color = pal.uiTextPrimary, fontSize = 15.sp, fontWeight = FontWeight.Bold)
            Text("Powered by GNU Backgammon -- a modified derivative of the GNU Backgammon engine.", color = pal.uiTextSecondary, fontSize = 13.sp)
            Spacer(modifier = Modifier.height(8.dp))
            Text("This program is free software, licensed under the GNU General Public License, version 3 or (at your option) any later version (GPL-3.0-or-later).", color = pal.uiTextSecondary, fontSize = 13.sp)
            Spacer(modifier = Modifier.height(8.dp))
            Text("It comes with ABSOLUTELY NO WARRANTY. You may redistribute it under the conditions of the GPL. You have the right to the complete corresponding source code.", color = pal.uiTextSecondary, fontSize = 13.sp)
            Spacer(modifier = Modifier.height(8.dp))
            Text("GNU Backgammon is Copyright (C) the Free Software Foundation, Inc. and the GNU Backgammon AUTHORS. The Android port is Copyright (C) 2025-2026 clavierhaus.", color = pal.uiTextSecondary, fontSize = 13.sp)
            Spacer(modifier = Modifier.height(8.dp))
            Text("Source, full license (COPYING), attribution (NOTICE), and modifications (PROVENANCE.md):", color = pal.uiTextSecondary, fontSize = 13.sp)
            Text("https://github.com/clavierhaus/gnubg-android", color = pal.uiTextPrimary, fontSize = 13.sp, fontWeight = FontWeight.Medium)
            Spacer(modifier = Modifier.height(4.dp))
            Text("The full GNU GPL v3 is included in the file COPYING and at https://www.gnu.org/licenses/gpl-3.0.html", color = pal.uiTextSecondary, fontSize = 12.sp)
        }
    }
}

@Composable
private fun ReportSettingsTab() {
    val context = LocalContext.current

    // Leave no clutter: clear any diagnostic scratch on entering this tab,
    // so nothing from a previous session lingers on disk.
    LaunchedEffect(Unit) { purgeReportCache(context) }

    SettingsSection("Report a problem") {
        Text(
            "Something went wrong? Tap below to copy a diagnostic report to the " +
            "clipboard -- app version, your device, and the exact game position " +
            "(as a GNU Backgammon ID). Paste it into a GitHub issue at " +
            "github.com/clavierhaus/gnubg-android/issues so it can be reproduced. " +
            "Nothing is sent automatically; the report is only what you paste.",
            color = LocalBoardPalette.current.uiTextSecondary,
            fontSize = 12.sp,
            modifier = Modifier.padding(bottom = 12.dp)
        )
        Button(
            onClick = {
                val report = buildBugReport(context)
                val cb = context.getSystemService(Context.CLIPBOARD_SERVICE)
                        as ClipboardManager
                cb.setPrimaryClip(ClipData.newPlainText("gnubg bug report", report))
                Toast.makeText(context,
                    "Report copied -- paste it into a GitHub issue",
                    Toast.LENGTH_LONG).show()
            },
            modifier = Modifier.fillMaxWidth()
        ) {
            Text("Generate bug report")
        }
    }
}

/** Assemble a paste-ready diagnostic report. No file leaves the device; the
 *  user chooses where to paste. The position ID lets anyone reconstruct the
 *  exact board -- the one thing a backgammon bug report cannot do without. */
private fun buildBugReport(context: Context): String {
    val sb = StringBuilder()
    sb.append("CBG -- bug report\n")
    sb.append("========================================\n\n")
    try {
        val pi = context.packageManager.getPackageInfo(context.packageName, 0)
        val code = if (Build.VERSION.SDK_INT >= 28)
            pi.longVersionCode else @Suppress("DEPRECATION") pi.versionCode.toLong()
        sb.append("App: ").append(pi.versionName)
          .append(" (code ").append(code).append(")\n")
    } catch (e: Throwable) {
        sb.append("App: (version unavailable)\n")
    }
    sb.append("Android: ").append(Build.VERSION.RELEASE)
      .append(" (API ").append(Build.VERSION.SDK_INT).append(")\n")
    sb.append("Device: ").append(Build.MANUFACTURER).append(" ")
      .append(Build.MODEL).append("\n\n")

    sb.append("--- Position ---\n")
    try {
        val ids = Engine.currentIds()
        if (ids != null && ids.isNotEmpty()) {
            for (id in ids) sb.append(id).append("\n")
        } else {
            sb.append("(no active position)\n")
        }
    } catch (e: Throwable) {
        sb.append("(position unavailable: ").append(e.message).append(")\n")
    }

    sb.append("\n--- Match state ---\n")
    try {
        val d = Engine.getCubeDebugState()
        // labels mirror the verb's documented field order
        val f = listOf("gameState","turn","move","die0","die1","doubled",
                       "cubeOwner","cube","crawford","cubeUse","score0",
                       "score1","matchTo")
        for (i in d.indices) {
            if (i < f.size) sb.append(f[i]).append("=").append(d[i]).append("  ")
        }
        sb.append("\n")
    } catch (e: Throwable) {
        sb.append("(state unavailable: ").append(e.message).append(")\n")
    }

    sb.append("\n--- Describe what happened ---\n")
    sb.append("(please add: what you expected, and what the app did instead)\n")
    return sb.toString()
}

/** Delete any diagnostic scratch under the app cache. Called on tab entry and
 *  cheap to call again -- the app leaves no report clutter between sessions. */
private fun purgeReportCache(context: Context) {
    try {
        context.cacheDir.listFiles { f -> f.name.startsWith("bugreport") }
            ?.forEach { it.delete() }
    } catch (_: Throwable) { /* best-effort cleanup */ }
}

@Composable
private fun SettingsSection(
    title: String,
    content: @Composable ColumnScope.() -> Unit
) {
    val pal = LocalBoardPalette.current
    Column(
        modifier = Modifier
            .fillMaxWidth()
            .padding(bottom = 14.dp)
            .background(pal.uiPanelDeep, RoundedCornerShape(12.dp))
            .padding(vertical = 10.dp)
    ) {
        Text(
            text = title.uppercase(),
            color = pal.uiTextSecondary,
            fontSize = 12.sp,
            fontWeight = FontWeight.Bold,
            modifier = Modifier.padding(horizontal = 14.dp, vertical = 6.dp)
        )
        content()
    }
}

@Composable
private fun SettingsRow(
    title: String,
    subtitle: String? = null,
    trailing: @Composable () -> Unit
) {
    val pal = LocalBoardPalette.current
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(horizontal = 14.dp, vertical = 9.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        Column(modifier = Modifier.weight(1f)) {
            Text(title, color = pal.uiTextPrimary, fontSize = 15.sp, fontWeight = FontWeight.Medium)
            if (subtitle != null) {
                Text(subtitle, color = pal.uiTextSecondary, fontSize = 12.sp)
            }
        }
        trailing()
    }
}


@Composable
private fun StepperRow(
    title: String,
    subtitle: String,
    value: String,
    onMinus: () -> Unit,
    onPlus: () -> Unit
) {
    val pal = LocalBoardPalette.current
    SettingsRow(title, subtitle) {
        Row(verticalAlignment = Alignment.CenterVertically) {
            TextButton(onClick = onMinus) {
                Text("-", color = pal.uiTextPrimary, fontSize = 18.sp)
            }
            Text(value, color = pal.uiTextPrimary, fontSize = 15.sp, fontWeight = FontWeight.Bold)
            TextButton(onClick = onPlus) {
                Text("+", color = pal.uiTextPrimary, fontSize = 18.sp)
            }
        }
    }
}

@Composable
private fun FloatStepperRow(
    title: String,
    subtitle: String,
    value: Float,
    onChange: (Float) -> Unit
) {
    val pal = LocalBoardPalette.current
    val rounded = kotlin.math.round(value * 100f) / 100f

    SettingsRow(title, subtitle) {
        Row(verticalAlignment = Alignment.CenterVertically) {
            TextButton(onClick = {
                val next = (rounded - 0.01f).coerceAtLeast(0.00f)
                onChange(kotlin.math.round(next * 100f) / 100f)
            }) {
                Text("-", color = pal.uiTextPrimary, fontSize = 18.sp)
            }
            Text(
                "%.2f".format(rounded),
                color = pal.uiTextPrimary,
                fontSize = 15.sp,
                fontWeight = FontWeight.Bold,
                modifier = Modifier.width(48.dp)
            )
            TextButton(onClick = {
                val next = (rounded + 0.01f).coerceAtMost(9.99f)
                onChange(kotlin.math.round(next * 100f) / 100f)
            }) {
                Text("+", color = pal.uiTextPrimary, fontSize = 18.sp)
            }
        }
    }
}

@Composable
private fun SettingsDivider() {
    val pal = LocalBoardPalette.current
    Spacer(
        modifier = Modifier
            .fillMaxWidth()
            .padding(horizontal = 14.dp)
            .background(pal.uiActionRoll)
            .padding(top = 1.dp)
    )
}
