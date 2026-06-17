package com.clavierhaus.gnubg.ui

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.*
import androidx.compose.runtime.*
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

enum class SettingsTab { GAME, BOARD, ENGINE, ANALYSIS }

private val ColorSettingsBg     = Color(0xFF082D6B)
private val ColorTabActive      = Color(0xFF1976D2)
private val ColorTabInactive    = Color(0xFF0D47A1)
private val ColorTabText        = Color(0xFFB3C9F0)
private val ColorTabTextActive  = Color(0xFFFFFFFF)
private val ColorDivider        = Color(0xFF1565C0)
private val ColorSettingText    = Color(0xFFE8F0FF)
private val ColorSettingSubtext = Color(0xFF7B9CC8)

@Composable
fun SettingsScreen(
    settings: GameSettings,
    viewModel: GameViewModel,
    onDismiss: () -> Unit
) {
    var activeTab by remember { mutableStateOf(SettingsTab.GAME) }

    Box(
        modifier = Modifier
            .fillMaxSize()
            .background(ColorSettingsBg)
    ) {
        Column(modifier = Modifier.fillMaxSize()) {

            Row(
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(horizontal = 24.dp, vertical = 12.dp),
                verticalAlignment = Alignment.CenterVertically
            ) {
                Text(
                    text = "←",
                    color = ColorTabTextActive,
                    fontSize = 20.sp,
                    modifier = Modifier
                        .clickable { onDismiss() }
                        .padding(end = 16.dp)
                )
                Text(
                    text = "Settings",
                    color = ColorTabTextActive,
                    fontSize = 18.sp,
                    fontWeight = FontWeight.Bold
                )
            }

            Row(
                modifier = Modifier
                    .fillMaxWidth()
                    .height(44.dp)
            ) {
                SettingsTab.values().forEach { tab ->
                    val isActive = tab == activeTab
                    Box(
                        modifier = Modifier
                            .weight(1f)
                            .fillMaxHeight()
                            .background(if (isActive) ColorTabActive else ColorTabInactive)
                            .clickable { activeTab = tab },
                        contentAlignment = Alignment.Center
                    ) {
                        Text(
                            text = tab.name,
                            color = if (isActive) ColorTabTextActive else ColorTabText,
                            fontSize = 12.sp,
                            fontWeight = if (isActive) FontWeight.Bold else FontWeight.Normal
                        )
                    }
                }
            }

            HorizontalDivider(color = ColorDivider, thickness = 1.dp)

            Column(
                modifier = Modifier
                    .fillMaxSize()
                    .verticalScroll(rememberScrollState())
                    .padding(24.dp)
            ) {
                when (activeTab) {
                    SettingsTab.GAME     -> GameSettingsTab(settings, viewModel)
                    SettingsTab.BOARD    -> BoardSettingsTab(settings, viewModel)
                    SettingsTab.ENGINE   -> EngineSettingsTab(settings, viewModel)
                    SettingsTab.ANALYSIS -> AnalysisSettingsTab(settings, viewModel)
                }
            }
        }
    }
}

@Composable
private fun SettingsSection(title: String, content: @Composable ColumnScope.() -> Unit) {
    Column(modifier = Modifier.fillMaxWidth()) {
        Text(
            text = title.uppercase(),
            color = ColorSettingSubtext,
            fontSize = 10.sp,
            fontWeight = FontWeight.Bold,
            letterSpacing = 1.5.sp,
            modifier = Modifier.padding(bottom = 8.dp)
        )
        Card(
            modifier = Modifier.fillMaxWidth(),
            shape = RoundedCornerShape(8.dp),
            colors = CardDefaults.cardColors(containerColor = Color(0xFF0D47A1))
        ) {
            Column(modifier = Modifier.padding(4.dp)) {
                content()
            }
        }
        Spacer(modifier = Modifier.height(20.dp))
    }
}

@Composable
private fun SettingsRow(
    label: String,
    subtitle: String? = null,
    control: @Composable () -> Unit = {}
) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(horizontal = 16.dp, vertical = 12.dp),
        verticalAlignment = Alignment.CenterVertically,
        horizontalArrangement = Arrangement.SpaceBetween
    ) {
        Column(modifier = Modifier.weight(1f)) {
            Text(label, color = ColorSettingText, fontSize = 14.sp)
            if (subtitle != null) {
                Text(subtitle, color = ColorSettingSubtext, fontSize = 11.sp)
            }
        }
        control()
    }
}

@Composable
private fun SettingsDivider() {
    HorizontalDivider(
        color = ColorDivider,
        thickness = 0.5.dp,
        modifier = Modifier.padding(horizontal = 16.dp)
    )
}

private val switchColors @Composable get() = SwitchDefaults.colors(
    checkedThumbColor = ColorTabTextActive,
    checkedTrackColor = ColorTabActive
)

private val radioColors @Composable get() = RadioButtonDefaults.colors(
    selectedColor = ColorTabTextActive,
    unselectedColor = ColorSettingSubtext
)

@Composable
private fun GameSettingsTab(settings: GameSettings, vm: GameViewModel) {
    SettingsSection("Match") {
        SettingsRow("Match Length", "Points to win") {
            Row(verticalAlignment = Alignment.CenterVertically) {
                listOf(1, 3, 5, 7).forEach { n ->
                    Text(
                        "$n",
                        color = if (settings.matchLength == n) ColorTabTextActive else ColorSettingSubtext,
                        fontSize = 14.sp,
                        fontWeight = if (settings.matchLength == n) FontWeight.Bold else FontWeight.Normal,
                        modifier = Modifier
                            .clickable { vm.setMatchLength(n) }
                            .padding(horizontal = 8.dp, vertical = 4.dp)
                    )
                }
            }
        }
        SettingsDivider()
        SettingsRow("Crawford Rule", "Disable doubling at match point") {
            Switch(settings.crawford, { vm.setCrawford(it) }, colors = switchColors)
        }
        SettingsDivider()
        SettingsRow("Jacoby Rule", "Gammons only count if cube is used") {
            Switch(settings.jacoby, { vm.setJacoby(it) }, colors = switchColors)
        }
    }
    SettingsSection("Scoring") {
        SettingsRow("Automatic Doubles", "Max automatic doubles") {
            Row(verticalAlignment = Alignment.CenterVertically) {
                Text("−", color = ColorTabTextActive, fontSize = 18.sp,
                    modifier = Modifier.clickable {
                        if (settings.automaticDoubles > 0) vm.setAutomaticDoubles(settings.automaticDoubles - 1)
                    }.padding(horizontal = 8.dp))
                Text("${settings.automaticDoubles}", color = ColorTabTextActive, fontSize = 14.sp)
                Text("+", color = ColorTabTextActive, fontSize = 18.sp,
                    modifier = Modifier.clickable {
                        if (settings.automaticDoubles < 4) vm.setAutomaticDoubles(settings.automaticDoubles + 1)
                    }.padding(horizontal = 8.dp))
            }
        }
        SettingsDivider()
        SettingsRow("Beavers", "Allow beavers") {
            Switch(settings.beavers, { vm.setBeavers(it) }, colors = switchColors)
        }
    }
}

@Composable
private fun BoardSettingsTab(settings: GameSettings, vm: GameViewModel) {
    SettingsSection("Colour Theme") {
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
    SettingsSection("Display") {
        SettingsRow("Point Numbers", "Show numbers in frame border") {
            Switch(settings.showPointNumbers, { vm.setShowPointNumbers(it) }, colors = switchColors)
        }
        SettingsDivider()
        SettingsRow("Pip Count", "Show pip count in bar") {
            Switch(settings.showPipCount, { vm.setShowPipCount(it) }, colors = switchColors)
        }
    }
}

@Composable
private fun EngineSettingsTab(settings: GameSettings, vm: GameViewModel) {
    SettingsSection("Difficulty") {
        Difficulty.values().forEachIndexed { i, d ->
            SettingsRow(d.label, d.subtitle) {
                RadioButton(
                    selected = settings.difficulty == d,
                    onClick = { vm.setDifficulty(d) },
                    colors = radioColors
                )
            }
            if (i < Difficulty.values().size - 1) SettingsDivider()
        }
    }
    SettingsSection("Tutor Mode") {
        SettingsRow("Tutor Mode", "Warn on doubtful moves") {
            Switch(settings.tutorMode, { vm.setTutorMode(it) }, colors = switchColors)
        }
        SettingsDivider()
        SettingsRow("Hint", "Show best move on request") {
            Switch(settings.hint, { vm.setHint(it) }, colors = switchColors)
        }
    }
}

@Composable
private fun AnalysisSettingsTab(settings: GameSettings, vm: GameViewModel) {
    SettingsSection("Equity Display") {
        SettingsRow("Show Equity", "Display position equity") {
            Switch(settings.showEquity, { vm.setShowEquity(it) }, colors = switchColors)
        }
        SettingsDivider()
        SettingsRow("Show MWC", "Match winning chances instead of equity") {
            Switch(settings.showMWC, { vm.setShowMWC(it) }, colors = switchColors)
        }
    }
    SettingsSection("Error Classification") {
        SettingsRow("Doubtful", "Threshold") {
            Text("%.3f".format(settings.thresholdDoubtful), color = ColorTabTextActive, fontSize = 14.sp)
        }
        SettingsDivider()
        SettingsRow("Bad", "Threshold") {
            Text("%.3f".format(settings.thresholdBad), color = ColorTabTextActive, fontSize = 14.sp)
        }
        SettingsDivider()
        SettingsRow("Very Bad", "Threshold") {
            Text("%.3f".format(settings.thresholdVeryBad), color = ColorTabTextActive, fontSize = 14.sp)
        }
    }
}
