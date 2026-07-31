package com.clavierhaus.gnubg.career

import android.content.Context
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.OutlinedTextField
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
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontStyle
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.input.PasswordVisualTransformation
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.clavierhaus.gnubg.play.LocalBoardPalette
import com.clavierhaus.gnubg.shared.PlusUi
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

/**
 * The "Your career" surface -- docs/CAREER_UI.md, implemented string for
 * string. This pass ships the status block, the Details disclosure, the
 * backup action (pass file, plain or passphrase) and the on-device check.
 * The phone-to-phone move (QR) ships in its own pass; its row appears then,
 * not before -- a dead row would be worse than absence.
 */
@Composable
fun CareerSettingsTab() {
    val ctx = LocalContext.current
    val pal = LocalBoardPalette.current
    val scope = rememberCoroutineScope()

    var result by remember { mutableStateOf<CareerVerify.Result?>(null) }
    var checking by remember { mutableStateOf(false) }
    var detailsOpen by remember { mutableStateOf(false) }
    var backupOpen by remember { mutableStateOf(false) }
    var passphrase by remember { mutableStateOf("") }
    var passphrase2 by remember { mutableStateOf("") }
    var exportNote by remember { mutableStateOf<String?>(null) }
    var pendingEncrypted by remember { mutableStateOf(false) }

    // Status on open: one silent check, off the UI thread.
    LaunchedEffect(Unit) {
        result = withContext(Dispatchers.Default) { CareerVerify.run(ctx) }
    }

    val createPass = rememberLauncherForActivityResult(
        ActivityResultContracts.CreateDocument("application/octet-stream")
    ) { uri ->
        if (uri == null) { exportNote = null; return@rememberLauncherForActivityResult }
        scope.launch(Dispatchers.IO) {
            val bytes = if (pendingEncrypted)
                CareerPass.encryptedPem(ctx, passphrase.toCharArray())
            else CareerPass.plainPem(ctx)
            val ok = bytes != null && runCatching {
                ctx.contentResolver.openOutputStream(uri, "wt")?.use { it.write(bytes); true } ?: false
            }.getOrDefault(false)
            withContext(Dispatchers.Main) {
                exportNote = when {
                    ok -> "Your career pass is saved."
                    bytes == null && pendingEncrypted ->
                        "This phone couldn't produce the protected pass — the plain pass still works."
                    bytes == null ->
                        "There is no career yet — the pass exists once your first match is recorded."
                    else -> "The pass couldn't be written there — try another place."
                }
                passphrase = ""; passphrase2 = ""
            }
        }
    }

    Column(Modifier.padding(horizontal = 14.dp, vertical = 10.dp)) {

        // --- Status: the human sentence first (design 2a) --------------------
        val r = result
        val statusText = when {
            r == null -> "Reading your record…"
            r.empty ->
                "Your record starts with your next finished Play match. Coach " +
                "matches are practice — they are never recorded."
            r.intact ->
                "Your record: ${r.entries} ${if (r.entries == 1) "match" else "matches"}, " +
                "chain intact — nothing has been changed since it was played."
            else -> r.findings.first() +
                if (r.findingsExtra > 0)
                    " …and ${r.findingsExtra} further finding" +
                    (if (r.findingsExtra > 1) "s" else "") +
                    " — the full list is in the computer check."
                else ""
        }
        Text(statusText, color = pal.uiTextPrimary, fontSize = 14.sp)
        r?.notes?.forEach { note ->
            Spacer(Modifier.height(4.dp))
            Text(note, color = pal.uiTextSecondary, fontSize = 12.sp)
        }

        // --- Details disclosure (design 2a, the only mechanism vocabulary) ---
        if (r != null && !r.empty) {
            Spacer(Modifier.height(8.dp))
            Text(
                if (detailsOpen) "Details ▾" else "Details ▸",
                color = pal.uiTextSecondary, fontSize = 13.sp,
                modifier = Modifier.clickable { detailsOpen = !detailsOpen }
            )
            if (detailsOpen) {
                val fp = r.fingerprint ?: "—"
                Text(
                    "Record: ${r.entries} entries in career/career-ledger.jsonl\n" +
                    "Key: P-256, fingerprint $fp\n" +
                    "Public key: career/career-pubkey.pem\n\n" +
                    "Independent verification, on any computer:\n" +
                    "tools/verify_career.py /path/to/your/career",
                    color = pal.uiTextSecondary, fontSize = 12.sp,
                    fontFamily = FontFamily.Monospace,
                    modifier = Modifier.padding(top = 6.dp)
                )
            }
        }

        Spacer(Modifier.height(16.dp))

        // --- Life event: Back up your career pass (design 2b) ----------------
        Text(
            "Back up your career pass",
            color = PlusUi.Interactive, fontSize = 15.sp, fontWeight = FontWeight.Bold,
            modifier = Modifier.clickable { backupOpen = !backupOpen; exportNote = null }
        )
        if (backupOpen) {
            Text(
                "Your career pass lets a future phone continue your record. Keep " +
                "the file somewhere safe — with a passphrase, the file alone is " +
                "useless to anyone who finds it; without one, the file itself " +
                "must stay private.",
                color = pal.uiTextPrimary, fontSize = 13.sp,
                modifier = Modifier.padding(top = 6.dp)
            )
            Spacer(Modifier.height(8.dp))
            OutlinedTextField(
                value = passphrase, onValueChange = { passphrase = it },
                label = { Text("Passphrase (optional)") },
                visualTransformation = PasswordVisualTransformation(),
                singleLine = true, modifier = Modifier.fillMaxWidth()
            )
            if (passphrase.isNotEmpty()) OutlinedTextField(
                value = passphrase2, onValueChange = { passphrase2 = it },
                label = { Text("Repeat passphrase") },
                visualTransformation = PasswordVisualTransformation(),
                singleLine = true, modifier = Modifier.fillMaxWidth()
            )
            Spacer(Modifier.height(8.dp))
            val mismatch = passphrase.isNotEmpty() && passphrase != passphrase2
            Text(
                when {
                    mismatch -> "The passphrases don't match yet."
                    passphrase.isNotEmpty() -> "Save with passphrase"
                    else -> "Save without"
                },
                color = if (mismatch) pal.uiTextSecondary else PlusUi.Interactive,
                fontSize = 14.sp, fontWeight = FontWeight.Bold,
                modifier = Modifier.clickable(enabled = !mismatch) {
                    pendingEncrypted = passphrase.isNotEmpty()
                    createPass.launch("career-pass.cbgkey")
                }
            )
        }
        exportNote?.let {
            Spacer(Modifier.height(6.dp))
            Text(it, color = pal.uiTextPrimary, fontSize = 13.sp)
        }

        Spacer(Modifier.height(16.dp))

        // --- Life event: Check my record (design 2b) -------------------------
        Text(
            if (checking) "Checking your record…" else "Check my record",
            color = PlusUi.Interactive, fontSize = 15.sp, fontWeight = FontWeight.Bold,
            modifier = Modifier.clickable(enabled = !checking) {
                checking = true
                scope.launch {
                    result = withContext(Dispatchers.Default) { CareerVerify.run(ctx) }
                    checking = false
                }
            }
        )
        if (r != null && !r.empty) {
            Text(
                "This check ran on this phone. For proof that needs no one's " +
                "word — including ours — run the same check on any computer:\n" +
                "tools/verify_career.py /path/to/your/career",
                color = pal.uiTextSecondary, fontSize = 12.sp,
                fontFamily = FontFamily.Monospace,
                modifier = Modifier.padding(top = 6.dp)
            )
        }

        Spacer(Modifier.height(18.dp))

        // --- The calm paragraph (design 2c) ----------------------------------
        Text(
            "If this phone is ever lost: install CBG on a new one, point it at " +
            "your folder, and use your career pass — your record continues, " +
            "verified. And if the pass is lost too, nothing you played is " +
            "gone: every match and its whole history stay readable and " +
            "checkable forever. A new pass simply signs from that day on.",
            color = pal.uiTextSecondary, fontSize = 12.sp, fontStyle = FontStyle.Italic
        )
        Spacer(Modifier.height(12.dp))
    }
}
