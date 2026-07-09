package com.clavierhaus.gnubg

import android.content.pm.ActivityInfo
import android.os.Bundle
import android.view.WindowManager
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.viewModels
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.core.view.WindowCompat
import androidx.core.view.WindowInsetsCompat
import androidx.core.view.WindowInsetsControllerCompat
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import com.clavierhaus.gnubg.analyse.AnalyseScreen
import com.clavierhaus.gnubg.engine.GameViewModel
import com.clavierhaus.gnubg.hub.HomeHubScreen
import com.clavierhaus.gnubg.learn.LearnScreen
import com.clavierhaus.gnubg.options.OptionsModeScreen
import com.clavierhaus.gnubg.play.GameLayout
import com.clavierhaus.gnubg.profile.ProfileScreen
import com.clavierhaus.gnubg.shared.AppMode
import com.clavierhaus.gnubg.ui.theme.GnubgTheme

class MainActivity : ComponentActivity() {
    private val viewModel: GameViewModel by viewModels()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        requestedOrientation = ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE

        WindowCompat.setDecorFitsSystemWindows(window, false)
        val controller = WindowInsetsControllerCompat(window, window.decorView)
        controller.hide(WindowInsetsCompat.Type.systemBars())
        controller.systemBarsBehavior =
            WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE
        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)

        setContent {
            GnubgTheme {
                var mode by remember { mutableStateOf(AppMode.HUB) }
                val settings by viewModel.settings.collectAsStateWithLifecycle()

                when (mode) {
                    AppMode.HUB -> HomeHubScreen(
                        onPlay = { mode = AppMode.PLAY },
                        onAnalysePosition = { mode = AppMode.ANALYSE },
                        onOptions = { mode = AppMode.OPTIONS },
                        onProfile = { mode = AppMode.PROFILE }
                    )

                    AppMode.PLAY -> GameLayout(
                        viewModel = viewModel,
                        onReturnToHub = { mode = AppMode.HUB }
                    )

                    AppMode.LEARN -> LearnScreen(
                        onBackToHub = { mode = AppMode.HUB }
                    )

                    AppMode.ANALYSE -> AnalyseScreen(
                        settings = settings,
                        onBackToHub = { mode = AppMode.HUB }
                    )

                    AppMode.OPTIONS -> OptionsModeScreen(
                        settings = settings,
                        viewModel = viewModel,
                        onBackToHub = { mode = AppMode.HUB }
                    )

                    AppMode.PROFILE -> ProfileScreen(
                        onBackToHub = { mode = AppMode.HUB }
                    )
                }
            }
        }
    }

    override fun onWindowFocusChanged(hasFocus: Boolean) {
        super.onWindowFocusChanged(hasFocus)
        if (hasFocus) {
            val controller = WindowInsetsControllerCompat(window, window.decorView)
            controller.hide(WindowInsetsCompat.Type.systemBars())
            controller.systemBarsBehavior =
                WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE
        }
    }
}
