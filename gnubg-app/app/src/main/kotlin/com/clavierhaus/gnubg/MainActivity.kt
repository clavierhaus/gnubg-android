package com.clavierhaus.gnubg

import android.content.pm.ActivityInfo
import android.os.Bundle
import android.view.WindowManager
import android.widget.Toast
import androidx.activity.ComponentActivity
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.compose.setContent
import androidx.activity.result.contract.ActivityResultContracts
import androidx.activity.viewModels
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import androidx.compose.ui.platform.LocalContext
import androidx.core.view.WindowCompat
import androidx.core.view.WindowInsetsCompat
import androidx.core.view.WindowInsetsControllerCompat
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import com.clavierhaus.gnubg.analyse.AnalyseScreen
import com.clavierhaus.gnubg.engine.GameViewModel
import com.clavierhaus.gnubg.coach.CoachScreen
import com.clavierhaus.gnubg.hub.HomeHubScreen
import com.clavierhaus.gnubg.learn.LearnScreen
import com.clavierhaus.gnubg.options.OptionsModeScreen
import com.clavierhaus.gnubg.play.GameLayout
import com.clavierhaus.gnubg.shared.AppMode
import com.clavierhaus.gnubg.review.ReviewScreen
import com.clavierhaus.gnubg.ui.theme.GnubgTheme
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import java.io.File
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

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

                // ONE settings overlay, owned here, opened by the gear every
                // screen carries. It renders over the current mode and returns
                // to it on dismiss -- Settings is an overlay, not a place, so
                // no screen loses its state by opening it.
                var showSettings by remember { mutableStateOf(false) }
                val settings by viewModel.settings.collectAsStateWithLifecycle()

                val context = LocalContext.current
                val scope = rememberCoroutineScope()

                // Feature [2]: save the match file. gnubg's CommandSaveMatch writes
                // to a filesystem path, so the engine writes to a private cache file
                // first, and the bytes are then copied into whatever destination the
                // user picked. The Storage Access Framework is what puts the file
                // somewhere the user can actually reach it -- Downloads, Drive, a USB
                // transfer to a desktop -- which is the reason the feature was asked
                // for: "go through the match on a bigger screen, or just catalog them".
                //
                // The cache path must be free of whitespace: CommandSaveMatch runs
                // NextToken() on it and would truncate at the first space.
                //
                // STORAGE LAW (docs/PLUS_STRATEGY, settled 2026-07-20): saving
                // goes through ONE user-granted CBG folder (SAF tree grant),
                // not a per-save picker. The first tap of Save shows a short
                // explanation and the folder picker, once; every save after
                // that is silent. The grant is the system's own persisted-
                // permission record -- see CbgFolder.
                fun defaultMatchFilename(): String {
                    val stamp = SimpleDateFormat("yyyyMMdd-HHmm", Locale.US).format(Date())
                    return "gnubg-match-" + stamp + ".sgf"
                }

                var showStorageDialog by remember { mutableStateOf(false) }

                // Writes the current match into the granted folder. On a stale
                // grant (folder deleted, permission revoked) it falls back to
                // asking again -- the dialog is the recovery path too.
                suspend fun saveIntoCbgFolder(tree: android.net.Uri) {
                    val tmp = File(context.cacheDir, "gnubg-match-export.sgf")
                    val written = viewModel.saveMatchToFile(tmp)
                    if (!written) {
                        Toast.makeText(
                            context,
                            "Nothing to save yet -- start a game first.",
                            Toast.LENGTH_SHORT
                        ).show()
                        return
                    }
                    val name = defaultMatchFilename()
                    val ok = withContext(Dispatchers.IO) {
                        com.clavierhaus.gnubg.storage.CbgFolder.saveInto(context, tree, name, tmp)
                    }
                    tmp.delete()
                    if (ok) {
                        Toast.makeText(context, "Saved $name to your CBG folder.", Toast.LENGTH_SHORT).show()
                    } else {
                        Toast.makeText(context, "Your CBG folder is no longer reachable -- please choose it again.", Toast.LENGTH_SHORT).show()
                        showStorageDialog = true
                    }
                }

                val pickCbgFolder = rememberLauncherForActivityResult(
                    ActivityResultContracts.OpenDocumentTree()
                ) { uri ->
                    if (uri == null) return@rememberLauncherForActivityResult
                    com.clavierhaus.gnubg.storage.CbgFolder.take(context, uri)
                    scope.launch { saveIntoCbgFolder(uri) }
                }

                fun onSaveTapped() {
                    val tree = com.clavierhaus.gnubg.storage.CbgFolder.grantedTree(context)
                    if (tree == null) showStorageDialog = true
                    else scope.launch { saveIntoCbgFolder(tree) }
                }

                if (showStorageDialog) {
                    androidx.compose.material3.AlertDialog(
                        onDismissRequest = { showStorageDialog = false },
                        title = { androidx.compose.material3.Text("Your matches, your folder") },
                        text = {
                            androidx.compose.material3.Text(
                                "CBG stores matches as plain gnubg files (.sgf) in a folder " +
                                "you pick -- browse them, back them up, open them anywhere " +
                                "gnubg runs. The same folder works in both CBG and CBG Plus, " +
                                "so upgrading never locks in or loses a match. Choose the " +
                                "folder once; every save after that is silent."
                            )
                        },
                        confirmButton = {
                            androidx.compose.material3.TextButton(onClick = {
                                showStorageDialog = false
                                pickCbgFolder.launch(
                                    com.clavierhaus.gnubg.storage.CbgFolder.documentsInitialUri()
                                )
                            }) { androidx.compose.material3.Text("Choose folder") }
                        },
                        dismissButton = {
                            androidx.compose.material3.TextButton(onClick = { showStorageDialog = false }) {
                                androidx.compose.material3.Text("Not now")
                            }
                        }
                    )
                }

                // Feature [3]: open a saved match. gnubg's SGF reader takes a
                // filesystem path, so the picked document is copied to a private
                // cache file first -- and that path must be free of whitespace,
                // since CommandLoadMatch tokenizes it exactly as CommandSaveMatch
                // does. loadMatch replaces the engine's match, so a game in
                // progress is discarded: the screen warns before this runs.
                var reviewPath by remember { mutableStateOf<String?>(null) }
                val openMatch = rememberLauncherForActivityResult(
                    ActivityResultContracts.OpenDocument()
                ) { uri ->
                    if (uri == null) return@rememberLauncherForActivityResult
                    scope.launch {
                        // A fresh name each time. Assigning null and then the same path
                        // in one coroutine can collapse into a single recomposition, and
                        // ReviewScreen's LaunchedEffect would not re-run for the same
                        // file chosen twice. The name carries no whitespace.
                        val tmp = File(context.cacheDir, "review-" + System.currentTimeMillis() + ".sgf")
                        val ok = withContext(Dispatchers.IO) {
                            runCatching {
                                context.contentResolver.openInputStream(uri)?.use { input ->
                                    tmp.outputStream().use { out -> input.copyTo(out) }
                                } ?: throw java.io.IOException("no input stream")
                            }.isSuccess && tmp.length() > 0L
                        }
                        if (ok) {
                            withContext(Dispatchers.IO) {
                                context.cacheDir.listFiles()
                                    ?.filter { it.name.startsWith("review-") && it != tmp }
                                    ?.forEach { it.delete() }
                            }
                            reviewPath = tmp.absolutePath
                        } else {
                            tmp.delete()
                            Toast.makeText(context, "Could not read that file.", Toast.LENGTH_SHORT).show()
                        }
                    }
                }

                androidx.compose.foundation.layout.Box(
                    modifier = androidx.compose.ui.Modifier.fillMaxSize()
                ) {
                when (mode) {
                    AppMode.HUB -> HomeHubScreen(
                        onPlay = { mode = AppMode.PLAY },
                        onCoach = { mode = AppMode.COACH },
                        onAnalysePosition = { mode = AppMode.ANALYSE },
                        onReviewMatch = { mode = AppMode.REVIEW },
                        onOptions = { showSettings = true },
                    )

                    AppMode.PLAY -> GameLayout(
                        viewModel = viewModel,
                        onReturnToHub = { mode = AppMode.HUB },
                        onSaveMatch = { onSaveTapped() },
                        onOpenSettings = { showSettings = true }
                    )

                    AppMode.COACH -> CoachScreen(
                        viewModel = viewModel,
                        settings = settings,
                        onReturnToHub = { mode = AppMode.HUB },
                        onOpenSettings = { showSettings = true }
                    )

                    AppMode.LEARN -> LearnScreen(
                        onBackToHub = { mode = AppMode.HUB }
                    )

                    AppMode.ANALYSE -> AnalyseScreen(
                        settings = settings,
                        onBackToHub = { mode = AppMode.HUB },
                        onOpenSettings = { showSettings = true }
                    )

                    AppMode.REVIEW -> ReviewScreen(
                        settings = settings,
                        onOpenMatch = { openMatch.launch(arrayOf("*/*")) },
                        matchPath = reviewPath,
                        onReturnToHub = { mode = AppMode.HUB },
                        onOpenSettings = { showSettings = true }
                    )

                    AppMode.OPTIONS -> OptionsModeScreen(
                        settings = settings,
                        viewModel = viewModel,
                        onBackToHub = { mode = AppMode.HUB }
                    )

                }

                if (showSettings) {
                    // OptionsModeScreen provides the themed palette and fills the
                    // screen; the callback is a dismiss, back to whatever mode is
                    // underneath. AppMode.OPTIONS is now unreachable -- the hub
                    // gear opens this overlay instead -- and its route above is
                    // kept only because the when must stay exhaustive.
                    OptionsModeScreen(
                        settings = settings,
                        viewModel = viewModel,
                        onBackToHub = { showSettings = false }
                    )
                }
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
