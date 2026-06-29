package com.clavierhaus.gnubg.options

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.RadioButton
import androidx.compose.material3.RadioButtonDefaults
import androidx.compose.material3.Switch
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
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.clavierhaus.gnubg.engine.BoardTheme
import com.clavierhaus.gnubg.engine.Difficulty
import com.clavierhaus.gnubg.engine.GameSettings
import com.clavierhaus.gnubg.engine.GameViewModel

enum class SettingsTab { GAME, BOARD, ENGINE, ANALYSIS, EXPERT }

private val ColorSettingsBg     = Color(0xFF082D6B)
private val ColorPanelBg        = Color(0xFF0A3880)
private val ColorTabActive      = Color(0xFF1976D2)
private val ColorTabInactive    = Color(0xFF0D47A1)
private val ColorTabText        = Color(0xFFB3C9F0)
private val ColorTabTextActive  = Color(0xFFFFFFFF)
private val ColorDivider        = Color(0xFF1565C0)
private val ColorSettingText    = Color(0xFFE8F0FF)
private val ColorSettingSubtext = Color(0xFF7B9CC8)
private val ColorDisabledText   = Color(0xFF5D7EA8)

private val switchColors
    @Composable get() = SwitchDefaults.colors(
        checkedThumbColor = ColorTabTextActive,
        checkedTrackColor = ColorTabActive,
        uncheckedThumbColor = ColorTabText,
        uncheckedTrackColor = ColorTabInactive
    )

private val radioColors
    @Composable get() = RadioButtonDefaults.colors(
        selectedColor = ColorTabTextActive,
        unselectedColor = ColorTabText
    )

@Composable
fun SettingsScreen(
    settings: GameSettings,
    viewModel: GameViewModel,
    onDismiss: () -> Unit
) {
    var activeTab by remember { mutableStateOf(SettingsTab.EXPERT) }

    Box(
        modifier = Modifier
            .fillMaxSize()
            .background(ColorSettingsBg)
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
                    SettingsTab.BOARD -> BoardSettingsTab(settings, viewModel)
                    SettingsTab.ENGINE -> EngineSettingsTab(settings, viewModel)
                    SettingsTab.ANALYSIS -> AnalysisTutorSettingsTab(settings, viewModel)
                    SettingsTab.EXPERT -> ExpertSettingsTab()
                }
            }
        }
    }
}

@Composable
private fun SettingsHeader(onDismiss: () -> Unit) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(horizontal = 22.dp, vertical = 12.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        Text(
            text = "<-",
            color = ColorTabTextActive,
            fontSize = 22.sp,
            modifier = Modifier
                .clickable { onDismiss() }
                .padding(end = 14.dp)
        )
        Column {
            Text(
                text = "Settings",
                color = ColorTabTextActive,
                fontSize = 22.sp,
                fontWeight = FontWeight.Bold
            )
            Text(
                text = "Configuration only. Game actions stay on the Play surface.",
                color = ColorSettingSubtext,
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
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(horizontal = 14.dp),
        horizontalArrangement = Arrangement.spacedBy(6.dp)
    ) {
        SettingsTab.values().forEach { tab ->
            val label = when (tab) {
                SettingsTab.GAME -> "Game"
                SettingsTab.BOARD -> "Board"
                SettingsTab.ENGINE -> "Engine"
                SettingsTab.ANALYSIS -> "Analysis"
                SettingsTab.EXPERT -> "Expert"
            }

            Box(
                modifier = Modifier
                    .weight(1f)
                    .background(
                        if (activeTab == tab) ColorTabActive else ColorTabInactive,
                        RoundedCornerShape(9.dp)
                    )
                    .clickable { onTab(tab) }
                    .padding(vertical = 9.dp),
                contentAlignment = Alignment.Center
            ) {
                Text(
                    text = label,
                    color = if (activeTab == tab) ColorTabTextActive else ColorTabText,
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
        SettingsRow("Crawford rule", "Stored locally until GNUbg timing is safe") {
            Switch(settings.crawford, { vm.setCrawford(it) }, colors = switchColors)
        }
    }

    SettingsSection("Money / session rules") {
        SettingsRow("Jacoby rule", "Stored locally until GNUbg timing is safe") {
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
        SettingsRow("Beavers", "Stored locally until GNUbg timing is safe") {
            Switch(settings.beavers, { vm.setBeavers(it) }, colors = switchColors)
        }
    }

    SettingsSection("Cube rules") {
        DisabledSettingsRow("Cube enabled", "GNUbg pendant: set cube use on/off")
        SettingsDivider()
        DisabledSettingsRow("Maximum cube", "GNUbg pendant to be audited")
        SettingsDivider()
        DisabledSettingsRow("Crawford handling", "Will be bound through lifecycle-safe match setup")
    }

    SettingsSection("Opening / session defaults") {
        DisabledSettingsRow("Starting side", "Future GNUbg-backed session option")
        SettingsDivider()
        DisabledSettingsRow("Dice / roll policy", "Future automatic roll / manual dice options")
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
        SettingsDivider()
        DisabledSettingsRow("Move landing hints", "Currently always available by long-press")
    }

    SettingsSection("Interaction") {
        DisabledSettingsRow("Destination-stack tap helper", "Android-only candidate")
        SettingsDivider()
        DisabledSettingsRow("Dice swap gesture", "Currently available on the board surface")
        SettingsDivider()
        DisabledSettingsRow("Board orientation", "Future display preference")
    }

    SettingsSection("Accessibility / display") {
        DisabledSettingsRow("Larger point numbers", "Android-only candidate")
        SettingsDivider()
        DisabledSettingsRow("High contrast checkers", "Android-only candidate")
        SettingsDivider()
        DisabledSettingsRow("Animation speed", "Android-only unless safely mapped")
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

    SettingsSection("Evaluation behaviour") {
        DisabledSettingsRow("Evaluation depth", "GNUbg pendant: set evaluation plies ...")
        SettingsDivider()
        DisabledSettingsRow("Move filter", "GNUbg pendant: set evaluation movefilter ...")
        SettingsDivider()
        DisabledSettingsRow("Cube decision strength", "GNUbg pendant: set player ... cubedecision ...")
        SettingsDivider()
        DisabledSettingsRow("Plies / search depth", "Will be lifecycle-safe before activation")
    }

    SettingsSection("Rollout") {
        DisabledSettingsRow("Rollout trials", "GNUbg pendant: set rollout trials ...")
        SettingsDivider()
        DisabledSettingsRow("Variance reduction / JSD", "GNUbg pendant: set rollout varredn/jsd ...")
        SettingsDivider()
        DisabledSettingsRow("Deterministic test mode", "GNUbg evaluation pendant to be audited")
        SettingsDivider()
        DisabledSettingsRow("Rollout seed", "GNUbg pendant: set rollout seed ...")
    }
}

@Composable
private fun AnalysisTutorSettingsTab(settings: GameSettings, vm: GameViewModel) {
    SettingsSection("Tutor") {
        SettingsRow("Tutor mode", "Stored locally until GNUbg timing is safe") {
            Switch(settings.tutorMode, { vm.setTutorMode(it) }, colors = switchColors)
        }
        SettingsDivider()
        SettingsRow("Hint", "Placeholder preference; board action comes later") {
            Switch(settings.hint, { vm.setHint(it) }, colors = switchColors)
        }
        SettingsDivider()
        DisabledSettingsRow("Warn before bad move", "GNUbg pendant: set warning / set tutor skill")
        SettingsDivider()
        DisabledSettingsRow("Explain move choice", "Future analysis output surface")
    }

    SettingsSection("Output") {
        SettingsRow("Show equity", "Presentation preference") {
            Switch(settings.showEquity, { vm.setShowEquity(it) }, colors = switchColors)
        }
        SettingsDivider()
        SettingsRow("Show MWC", "Presentation preference") {
            Switch(settings.showMWC, { vm.setShowMWC(it) }, colors = switchColors)
        }
        SettingsDivider()
        DisabledSettingsRow("Show cube action", "Future analysis output control")
        SettingsDivider()
        DisabledSettingsRow("Show best move", "Future analysis output control")
        SettingsDivider()
        DisabledSettingsRow("Show alternatives", "Future analysis output control")
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
private fun ExpertSettingsTab() {
    SettingsSection("GNUbg command bridge") {
        DisabledSettingsRow("Restricted command bridge", "Implemented, but not fired from live Settings")
        SettingsDivider()
        DisabledSettingsRow("Lifecycle-safe dispatch", "Required before command-backed Settings are re-enabled")
        SettingsDivider()
        DisabledSettingsRow("Capture GNUbg output", "Needed for show commands and diagnostics")
    }

    SettingsSection("Command grammar / diagnostics") {
        DisabledSettingsRow("Show command allowlist", "Future diagnostic view")
        SettingsDivider()
        DisabledSettingsRow("Show current GNUbg settings", "Future read-only show-command surface")
        SettingsDivider()
        DisabledSettingsRow("Dry-run settings command", "Future safe parser test")
        SettingsDivider()
        DisabledSettingsRow("Command result log", "Future bridge diagnostics")
    }

    SettingsSection("Advanced engine configuration") {
        DisabledSettingsRow("Raw evaluation plies", "GNUbg pendant: set evaluation plies ...")
        SettingsDivider()
        DisabledSettingsRow("Player chequerplay settings", "GNUbg pendant: set player ... chequerplay ...")
        SettingsDivider()
        DisabledSettingsRow("Player cube-decision settings", "GNUbg pendant: set player ... cubedecision ...")
        SettingsDivider()
        DisabledSettingsRow("Rollout seed / RNG", "GNUbg pendant: set rollout seed / rng ...")
        SettingsDivider()
        DisabledSettingsRow("Deterministic evaluation", "GNUbg evaluation pendant to be audited")
    }

    SettingsSection("Experimental / unsafe") {
        DisabledSettingsRow("Direct set command execution", "Intentionally disabled")
        SettingsDivider()
        DisabledSettingsRow("Match/session command timing", "Unsafe until lifecycle-scoped")
        SettingsDivider()
        DisabledSettingsRow("Player setup timing", "Unsafe until match start sequencing is verified")
    }
}

@Composable
private fun SettingsSection(
    title: String,
    content: @Composable ColumnScope.() -> Unit
) {
    Column(
        modifier = Modifier
            .fillMaxWidth()
            .padding(bottom = 14.dp)
            .background(ColorPanelBg, RoundedCornerShape(12.dp))
            .padding(vertical = 10.dp)
    ) {
        Text(
            text = title.uppercase(),
            color = ColorTabText,
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
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(horizontal = 14.dp, vertical = 9.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        Column(modifier = Modifier.weight(1f)) {
            Text(title, color = ColorSettingText, fontSize = 15.sp, fontWeight = FontWeight.Medium)
            if (subtitle != null) {
                Text(subtitle, color = ColorSettingSubtext, fontSize = 12.sp)
            }
        }
        trailing()
    }
}

@Composable
private fun DisabledSettingsRow(
    title: String,
    subtitle: String
) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(horizontal = 14.dp, vertical = 9.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        Column(modifier = Modifier.weight(1f)) {
            Text(title, color = ColorDisabledText, fontSize = 15.sp, fontWeight = FontWeight.Medium)
            Text(subtitle, color = ColorDisabledText, fontSize = 12.sp)
        }
        Text("Later", color = ColorDisabledText, fontSize = 12.sp)
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
    SettingsRow(title, subtitle) {
        Row(verticalAlignment = Alignment.CenterVertically) {
            TextButton(onClick = onMinus) {
                Text("-", color = ColorTabTextActive, fontSize = 18.sp)
            }
            Text(value, color = ColorTabTextActive, fontSize = 15.sp, fontWeight = FontWeight.Bold)
            TextButton(onClick = onPlus) {
                Text("+", color = ColorTabTextActive, fontSize = 18.sp)
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
    val rounded = kotlin.math.round(value * 100f) / 100f

    SettingsRow(title, subtitle) {
        Row(verticalAlignment = Alignment.CenterVertically) {
            TextButton(onClick = {
                val next = (rounded - 0.01f).coerceAtLeast(0.00f)
                onChange(kotlin.math.round(next * 100f) / 100f)
            }) {
                Text("-", color = ColorTabTextActive, fontSize = 18.sp)
            }
            Text(
                "%.2f".format(rounded),
                color = ColorTabTextActive,
                fontSize = 15.sp,
                fontWeight = FontWeight.Bold,
                modifier = Modifier.width(48.dp)
            )
            TextButton(onClick = {
                val next = (rounded + 0.01f).coerceAtMost(9.99f)
                onChange(kotlin.math.round(next * 100f) / 100f)
            }) {
                Text("+", color = ColorTabTextActive, fontSize = 18.sp)
            }
        }
    }
}

@Composable
private fun SettingsDivider() {
    Spacer(
        modifier = Modifier
            .fillMaxWidth()
            .padding(horizontal = 14.dp)
            .background(ColorDivider)
            .padding(top = 1.dp)
    )
}
