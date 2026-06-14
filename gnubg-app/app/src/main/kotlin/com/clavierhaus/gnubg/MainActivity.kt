package com.clavierhaus.gnubg

import android.content.pm.ActivityInfo
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import com.clavierhaus.gnubg.ui.GameLayout
import com.clavierhaus.gnubg.ui.theme.GnubgTheme

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        requestedOrientation = ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE
        setContent {
            GnubgTheme {
                GameLayout()
            }
        }
    }
}
