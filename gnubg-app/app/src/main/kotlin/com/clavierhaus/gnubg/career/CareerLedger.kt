package com.clavierhaus.gnubg.career

import android.content.Context
import android.net.Uri
import android.util.Base64
import android.util.Log
import com.clavierhaus.gnubg.engine.MatchReport
import com.clavierhaus.gnubg.plusstore.SafDocs
import com.clavierhaus.gnubg.storage.CbgFolder
import java.io.File
import java.security.KeyFactory
import java.security.KeyPair
import java.security.KeyPairGenerator
import java.security.MessageDigest
import java.security.Signature
import java.security.spec.ECGenParameterSpec
import java.security.spec.PKCS8EncodedKeySpec

/**
 * The career ledger (scope 1): the silent, tamper-evident record of finished
 * Play matches. Design and reasoning are PUBLIC -- FOSS docs/CRYPTOGRAPHY.md
 * (stack, verdicts, GPLv3 defense) and docs/CAREER_AND_STATS.md (the arc).
 * This file implements exactly that spec and nothing beyond it:
 *
 *  - Store: a `career/` subfolder of the user's granted CBG tree.
 *      career/<match>.sgf          plain gnubg match files (plaintext, theirs)
 *      career/career-ledger.jsonl  one line per match: entryJSON \t base64(DER sig)
 *      career/career-pubkey.pem    the user's public key, standard SPKI PEM
 *  - Chain: each entry carries sha256 of its SGF bytes, the analysis figures
 *    as computed (with the app version that computed them), and sha256 of the
 *    PREVIOUS full stored line (bytes before the newline), or "genesis".
 *  - Signature: SHA256withECDSA (P-256) over the entry's EXACT stored bytes
 *    (the JSON before the tab) -- never a re-serialization. Verifiable with
 *    stock openssl; proven in the build gate before this file was written.
 *  - Key: the USER'S property. Generated on device, standard PKCS#8, kept in
 *    the app's private filesDir (never in the shared folder). Extractable by
 *    construction; the export/portability flow is the immediate follow-up and
 *    changes nothing here. No escrow, no copy, no "our key" anywhere.
 *  - Honest limits: tamper-EVIDENT, not unforgeable. The chain proves the
 *    record as kept.
 *
 * Failure discipline: the ledger never throws into the game. Every failure
 * logs and returns; a saved-but-uncollected SGF is a state the FOSS verifier
 * names distinctly from tampering.
 */
object CareerLedger {

    private const val TAG = "cbg-career"
    private const val DIR = "career"
    private const val LEDGER = "career-ledger.jsonl"
    private const val PUBKEY = "career-pubkey.pem"
    private const val KEYFILE = "career-key.p8"
    private const val FORMAT_V = 1

    // ---- key management (user-owned, standard encodings) --------------------

    private fun loadOrCreateKey(ctx: Context): KeyPair? = runCatching {
        val f = File(ctx.filesDir, KEYFILE)
        val kf = KeyFactory.getInstance("EC")
        if (f.exists()) {
            val priv = kf.generatePrivate(PKCS8EncodedKeySpec(f.readBytes()))
            // Public key is re-derivable only via the folder copy; keep both
            // halves together by storing the public alongside on creation and
            // reading it back here.
            val pubPem = File(ctx.filesDir, "$KEYFILE.pub")
            val pub = kf.generatePublic(
                java.security.spec.X509EncodedKeySpec(pubPem.readBytes())
            )
            KeyPair(pub, priv)
        } else {
            val g = KeyPairGenerator.getInstance("EC")
            g.initialize(ECGenParameterSpec("secp256r1"))
            val kp = g.generateKeyPair()
            f.writeBytes(kp.private.encoded)              // PKCS#8
            File(ctx.filesDir, "$KEYFILE.pub").writeBytes(kp.public.encoded) // SPKI
            Log.i(TAG, "career key generated (P-256, PKCS#8 in filesDir)")
            kp
        }
    }.onFailure { Log.w(TAG, "key load/create failed: ${it.message}") }.getOrNull()

    private fun publicKeyPem(kp: KeyPair): String {
        val b64 = Base64.encodeToString(kp.public.encoded, Base64.NO_WRAP)
        val wrapped = b64.chunked(64).joinToString("\n")
        return "-----BEGIN PUBLIC KEY-----\n$wrapped\n-----END PUBLIC KEY-----\n"
    }

    // ---- the collect step ---------------------------------------------------

    /**
     * Silently collect one finished Play match: [sgf] is the match file as
     * written by gnubg (a temp file in app storage; the engine needs a real
     * path), [report] the analysis as computed. Copies the SGF into career/,
     * appends the signed chain entry. Never throws.
     */
    fun collect(ctx: Context, sgf: File, report: MatchReport): Int? {
        val tree = CbgFolder.grantedTree(ctx) ?: run {
            Log.w(TAG, "no folder grant; match not collected"); return null
        }
        val kp = loadOrCreateKey(ctx) ?: return null
        val careerDir = SafDocs.findOrCreateChild(
            ctx, tree, SafDocs.treeRootDoc(tree), DIR,
            android.provider.DocumentsContract.Document.MIME_TYPE_DIR
        ) ?: run { Log.w(TAG, "career/ not creatable"); return null }

        // Public key: present once, written on first collect.
        val pubDoc = SafDocs.findChild(ctx, tree, careerDir, PUBKEY)
        if (pubDoc == null) {
            SafDocs.findOrCreateChild(ctx, tree, careerDir, PUBKEY, "application/x-pem-file")
                ?.let { SafDocs.overwrite(ctx, it, publicKeyPem(kp).toByteArray()) }
        }

        // 1. SGF into career/, recording the name the provider ACTUALLY used.
        val sgfBytes = runCatching { sgf.readBytes() }.getOrNull() ?: run {
            Log.w(TAG, "sgf unreadable; match not collected"); return null
        }
        val requested = sgf.name
        val created = SafDocs.createDocumentReturningName(
            ctx, careerDir, "application/x-gnubg-sgf", requested
        ) ?: run { Log.w(TAG, "sgf copy not creatable"); return null }
        val (sgfDoc, actualName) = created
        if (!SafDocs.overwrite(ctx, sgfDoc, sgfBytes)) {
            Log.w(TAG, "sgf copy write failed"); return null
        }

        // 2. Chain: hash of the previous FULL stored line (or genesis).
        val ledgerDoc = SafDocs.findOrCreateChild(
            ctx, tree, careerDir, LEDGER, "application/json"
        ) ?: run { Log.w(TAG, "ledger not creatable"); return null }
        val priorRaw = SafDocs.readAll(ctx, ledgerDoc)
        val prevHash = lastLineHash(priorRaw) ?: "genesis"
        val priorCount = priorRaw?.toString(Charsets.UTF_8)
            ?.substringBeforeLast('\n', "")?.split('\n')?.count { it.isNotBlank() } ?: 0

        // 3. The entry, hand-assembled so the SIGNED BYTES ARE THE STORED
        //    BYTES -- no serializer between signing and storage.
        val entry = buildString {
            append("{\"v\":").append(FORMAT_V)
            append(",\"ts\":\"").append(nowUtc()).append('"')
            append(",\"app\":\"").append(jsonEsc(appVersion(ctx))).append('"')
            append(",\"match\":\"").append(jsonEsc(actualName)).append('"')
            append(",\"sha256\":\"").append(hex(sha256(sgfBytes))).append('"')
            append(",\"stats\":").append(statsJson(report))
            append(",\"prev\":\"").append(prevHash).append('"')
            append('}')
        }.toByteArray(Charsets.UTF_8)

        // 4. Sign the exact bytes; store: entry \t base64(sig) \n
        val sig = Signature.getInstance("SHA256withECDSA").run {
            initSign(kp.private); update(entry); sign()
        }
        val line = entry + "\t".toByteArray() +
            Base64.encodeToString(sig, Base64.NO_WRAP).toByteArray() + "\n".toByteArray()
        if (!SafDocs.append(ctx, ledgerDoc, line)) {
            Log.w(TAG, "ledger append failed (sgf saved, entry missing -- verifier will name it)")
            return null
        }
        Log.i(TAG, "collected: $actualName (chain prev=$prevHash)")
        return priorCount + 1
    }

    // ---- helpers ------------------------------------------------------------

    /** sha256 (hex) of the last complete line's bytes (before its newline). */
    private fun lastLineHash(ledger: ByteArray?): String? {
        if (ledger == null || ledger.isEmpty()) return null
        // Only COMPLETE lines anchor the chain; a truncated tail (crash mid-
        // append) is the verifier's "incomplete last entry", never our anchor.
        val text = ledger.toString(Charsets.UTF_8)
        val complete = text.substringBeforeLast('\n', "")
        if (complete.isEmpty()) return null
        val last = complete.substringAfterLast('\n')
        if (last.isBlank()) return null
        return hex(sha256(last.toByteArray(Charsets.UTF_8)))
    }

    private fun statsJson(r: MatchReport): String = buildString {
        append("{\"games\":").append(r.games)
        append(",\"perDecision\":[").append(f4(r.perDecisionRate[0])).append(',')
            .append(f4(r.perDecisionRate[1])).append(']')
        append(",\"er\":[").append(f4(r.prPerPlayer[0])).append(',')
            .append(f4(r.prPerPlayer[1])).append(']')
        append(",\"actual\":").append(f4(r.actualResult[0]))
        append(",\"luckAdj\":").append(f4(r.luckAdjResult[0]))
        append('}')
    }

    private fun sha256(b: ByteArray): ByteArray =
        MessageDigest.getInstance("SHA-256").digest(b)

    private fun hex(b: ByteArray): String =
        b.joinToString("") { "%02x".format(it) }

    private fun f4(v: Float): String = String.format(java.util.Locale.ROOT, "%.4f", v)

    private fun jsonEsc(s: String): String =
        s.replace("\\", "\\\\").replace("\"", "\\\"")
            .replace("\n", " ").replace("\t", " ")

    private fun nowUtc(): String =
        java.time.format.DateTimeFormatter.ISO_INSTANT
            .format(java.time.Instant.now().truncatedTo(java.time.temporal.ChronoUnit.SECONDS))

    private fun appVersion(ctx: Context): String = runCatching {
        ctx.packageManager.getPackageInfo(ctx.packageName, 0).versionName ?: "?"
    }.getOrDefault("?")
}
