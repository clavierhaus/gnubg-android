package com.clavierhaus.gnubg.career

import android.content.Context
import android.util.Base64
import com.clavierhaus.gnubg.plusstore.SafDocs
import com.clavierhaus.gnubg.storage.CbgFolder
import java.security.KeyFactory
import java.security.MessageDigest
import java.security.PublicKey
import java.security.Signature
import java.security.spec.X509EncodedKeySpec

/**
 * On-device verification of the career chain -- the "Check my record" action
 * of the Your-career surface (docs/CAREER_UI.md). Runs the SAME checks as
 * the public tools/verify_career.py, against the SAME artifacts, and its
 * honest status is stated wherever it is shown: this is reassurance for the
 * owner, not proof for a skeptic (docs/CRYPTOGRAPHY.md section 6.6) -- the
 * independent script exists for that.
 *
 * Verdicts carry the event copy from the design table verbatim; the UI
 * renders them as given and invents nothing.
 */
object CareerVerify {

    data class Result(
        val entries: Int,
        /** Event sentences, design-table copy: the EARLIEST finding, or empty. */
        val findings: List<String>,
        /** Additional findings beyond the first (design: collapsed to a count). */
        val findingsExtra: Int,
        /** Informational notes (damage, never tamper): design-table copy. */
        val notes: List<String>,
        /** Key fingerprint from the folder's pem, "SHA256:xxxx…xxxx". */
        val fingerprint: String?,
        /** True when the career folder or ledger does not exist yet. */
        val empty: Boolean
    ) {
        val intact: Boolean get() = findings.isEmpty() && !empty
    }

    fun run(ctx: Context): Result {
        val tree = CbgFolder.grantedTree(ctx)
            ?: return Result(0, emptyList(), 0, emptyList(), null, empty = true)
        val root = SafDocs.treeRootDoc(tree)
        val dir = SafDocs.findChild(ctx, tree, root, "career")
            ?: return Result(0, emptyList(), 0, emptyList(), null, empty = true)
        val ledgerDoc = SafDocs.findChild(ctx, tree, dir, "career-ledger.jsonl")
            ?: return Result(0, emptyList(), 0, emptyList(), null, empty = true)
        val pubPemBytes = SafDocs.findChild(ctx, tree, dir, "career-pubkey.pem")
            ?.let { SafDocs.readAll(ctx, it) }
        val pub = pubPemBytes?.let(::parsePem)
        val fingerprint = pubPemBytes?.let(::fingerprintOf)

        val raw = SafDocs.readAll(ctx, ledgerDoc)
            ?: return Result(0, emptyList(), 0, emptyList(), fingerprint, empty = true)

        var firstFinding: String? = null
        var extra = 0
        fun found(sentence: String) {
            if (firstFinding == null) firstFinding = sentence else extra++
        }
        val notes = ArrayList<String>()

        val text = raw.toString(Charsets.UTF_8)
        val completePart = text.substringBeforeLast('\n', "")
        if (!text.endsWith("\n") && text.isNotEmpty()) {
            notes.add(
                "The last entry wasn't fully written — likely an interruption, " +
                "not a change. Your next match continues the record."
            )
        }
        val lines = if (completePart.isEmpty()) emptyList()
                    else completePart.split('\n').filter { it.isNotBlank() }

        var prevExpected = "genesis"
        val referenced = HashSet<String>()
        var entryNo = 0

        for (line in lines) {
            entryNo++
            val tabIdx = line.indexOf('\t')
            if (tabIdx < 0) {
                found("One entry was rewritten after it was recorded (entry $entryNo). " +
                      "Everything before it still checks out.")
                prevExpected = sha256Hex(line.toByteArray(Charsets.UTF_8))
                continue
            }
            val entryBytes = line.substring(0, tabIdx).toByteArray(Charsets.UTF_8)
            val sigOk = pub != null && runCatching {
                val der = Base64.decode(line.substring(tabIdx + 1), Base64.DEFAULT)
                Signature.getInstance("SHA256withECDSA").run {
                    initVerify(pub); update(entryBytes); verify(der)
                }
            }.getOrDefault(false)
            if (!sigOk) {
                found("One entry was rewritten after it was recorded (entry $entryNo). " +
                      "Everything before it still checks out.")
            }

            val prevInEntry = jsonString(entryBytes, "prev")
            if (prevInEntry != prevExpected) {
                found("The record's order was changed after entry ${entryNo - 1}. " +
                      "Everything up to there still checks out.")
            }

            val matchName = jsonString(entryBytes, "match") ?: ""
            referenced.add(matchName)
            val sgfDoc = SafDocs.findChild(ctx, tree, dir, matchName)
            if (sgfDoc == null) {
                found("One recorded match file is missing ($matchName).")
            } else {
                val sgfBytes = SafDocs.readAll(ctx, sgfDoc)
                val recorded = jsonString(entryBytes, "sha256")
                if (sgfBytes == null || sha256Hex(sgfBytes) != recorded) {
                    found("One match file was changed after it was recorded ($matchName). " +
                          "Everything before it still checks out.")
                }
            }
            prevExpected = sha256Hex(line.toByteArray(Charsets.UTF_8))
        }

        strayNote(ctx, tree, dir, referenced)?.let(notes::add)

        return Result(
            entries = entryNo,
            findings = firstFinding?.let { listOf(it) } ?: emptyList(),
            findingsExtra = extra,
            notes = notes,
            fingerprint = fingerprint,
            empty = entryNo == 0
        )
    }

    private fun strayNote(
        ctx: Context, tree: android.net.Uri, dir: android.net.Uri, referenced: Set<String>
    ): String? {
        var stray: String? = null
        runCatching {
            val parentId = android.provider.DocumentsContract.getDocumentId(dir)
            val children = android.provider.DocumentsContract
                .buildChildDocumentsUriUsingTree(tree, parentId)
            ctx.contentResolver.query(
                children,
                arrayOf(android.provider.DocumentsContract.Document.COLUMN_DISPLAY_NAME),
                null, null, null
            )?.use { c ->
                while (c.moveToNext()) {
                    val name = c.getString(0)
                    if (name.endsWith(".sgf") && name !in referenced) { stray = name; break }
                }
            }
        }
        return stray?.let {
            "One match was saved but never entered the record — likely an interruption ($it)."
        }
    }

    // -- helpers --------------------------------------------------------------

    private fun spkiDer(pemBytes: ByteArray): ByteArray? = runCatching {
        Base64.decode(
            pemBytes.toString(Charsets.UTF_8)
                .replace("-----BEGIN PUBLIC KEY-----", "")
                .replace("-----END PUBLIC KEY-----", "")
                .replace("\n", "").trim(),
            Base64.DEFAULT
        )
    }.getOrNull()

    private fun parsePem(pemBytes: ByteArray): PublicKey? = runCatching {
        KeyFactory.getInstance("EC").generatePublic(X509EncodedKeySpec(spkiDer(pemBytes)))
    }.getOrNull()

    private fun fingerprintOf(pemBytes: ByteArray): String? = spkiDer(pemBytes)?.let {
        val h = sha256Hex(it)
        "SHA256:${h.take(4)}…${h.takeLast(4)}"
    }

    /** Minimal extraction of a top-level string field from the entry's exact
     *  bytes. The entry JSON is produced by CareerLedger with known escaping;
     *  the bytes are the authority (never re-serialized), so a targeted scan
     *  is sufficient and avoids a parser that normalizes. */
    private fun jsonString(entry: ByteArray, field: String): String? {
        val s = entry.toString(Charsets.UTF_8)
        val k = "\"$field\":\""
        val i = s.indexOf(k)
        if (i < 0) return null
        val sb = StringBuilder()
        var j = i + k.length
        while (j < s.length) {
            val ch = s[j]
            if (ch == '\\' && j + 1 < s.length) { sb.append(s[j + 1]); j += 2; continue }
            if (ch == '"') break
            sb.append(ch); j++
        }
        return sb.toString()
    }

    private fun sha256Hex(b: ByteArray): String =
        MessageDigest.getInstance("SHA-256").digest(b)
            .joinToString("") { "%02x".format(it) }
}
