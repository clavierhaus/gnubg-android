package com.clavierhaus.gnubg.debug

import android.content.Context
import android.net.Uri
import android.provider.DocumentsContract
import com.clavierhaus.gnubg.storage.CbgFolder
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale
import java.util.concurrent.Executors
import java.util.concurrent.LinkedBlockingQueue

/**
 * Plus-only field debugging trace. Always on -- there is no toggle, by
 * design (docs/PLUS_STRATEGY: this file exists ONLY on the plus branch, so
 * FOSS has no logger at all; a trace that exists is a Plus trace, and that
 * is the edition marker -- every line also carries edition=PLUS so an
 * uploaded log is self-labelling). A bug reproduced here is logged as PLUS;
 * whether it ALSO affects FOSS is the maintainer's separate call, made by
 * playing FOSS, never claimed by this log.
 *
 * Storage: a `debug/` subfolder inside the user's granted CBG folder (the
 * same tree as saved matches -- see CbgFolder), so the file is browsable in
 * any file manager and uploadable from the phone. CbgFolder.saveInto neither
 * appends nor makes subfolders, so the find-or-create-subdir and append-to-
 * existing-document logic lives here, on gnubg's own DocumentsContract idiom.
 *
 * Discipline: creates the file when missing, ALWAYS appends, NEVER purges.
 * Past 10 MB it emits one WARN line per process and keeps logging. All I/O
 * runs on a single background thread; callers never block the game thread.
 * Lines are batched and flushed on a short cadence and on every explicit
 * flush(), because SAF writes are heavier than a plain file handle.
 */
object DebugTrace {

    private const val SUBDIR = "debug"
    private const val FILENAME = "cbg-debug.log"
    private const val WARN_BYTES = 10L * 1024 * 1024
    private const val FLUSH_MS = 1500L

    private val stamp = SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss.SSS", Locale.US)

    private val queue = LinkedBlockingQueue<String>()
    private val worker = Executors.newSingleThreadExecutor { r ->
        Thread(r, "cbg-debug-trace").apply { isDaemon = true }
    }

    @Volatile private var appContext: Context? = null
    @Volatile private var started = false
    @Volatile private var warned = false

    /** Idempotent. Call once with any context (application context is taken). */
    fun init(context: Context) {
        appContext = context.applicationContext
        if (!started) {
            started = true
            worker.execute { drainLoop() }
        }
        record("TRACE", "event" to "init")
    }

    /**
     * Queue one line: ISO timestamp, edition marker, tag, then key=value
     * pairs. Non-blocking; the actual SAF write happens on the worker.
     * Values have newlines and '=' softened so one event is always one line.
     */
    fun record(tag: String, vararg fields: Pair<String, Any?>) {
        val sb = StringBuilder(64)
        sb.append(stamp.format(Date())).append(' ')
            .append("edition=PLUS").append(' ')
            .append(tag)
        for ((k, v) in fields) {
            sb.append(' ').append(k).append('=').append(sanitize(v))
        }
        queue.offer(sb.toString())
    }

    /** Ask the worker to flush the current batch promptly (e.g. at match end). */
    fun flush() {
        queue.offer(FLUSH_SENTINEL)
    }

    // --- worker thread ------------------------------------------------------

    private const val FLUSH_SENTINEL = "\u0000FLUSH"

    private fun drainLoop() {
        val batch = ArrayList<String>(64)
        while (true) {
            try {
                // Block for the first line, then coalesce whatever else is
                // waiting into one SAF append.
                val first = queue.take()
                batch.clear()
                if (first != FLUSH_SENTINEL) batch.add(first)
                // Brief settle window so bursts (action + resulting state)
                // land in a single write.
                Thread.sleep(FLUSH_MS)
                queue.drainTo(batch)
                val lines = batch.filter { it != FLUSH_SENTINEL }
                if (lines.isNotEmpty()) appendLines(lines)
            } catch (ie: InterruptedException) {
                Thread.currentThread().interrupt()
                return
            } catch (t: Throwable) {
                // A logger must never crash the app. Drop the batch, keep going.
                android.util.Log.w("cbg-debug-trace", "append failed: ${t.message}")
            }
        }
    }

    private fun appendLines(lines: List<String>) {
        val ctx = appContext ?: return
        val tree = CbgFolder.grantedTree(ctx) ?: return  // no grant yet: nothing to write to
        val logDoc = ensureLogDocument(ctx, tree) ?: return
        val payload = (lines.joinToString("\n") + "\n").toByteArray(Charsets.UTF_8)

        // "wa" = write-append; the document provider positions at EOF, so we
        // never read-modify-write the whole file. Always append, never purge.
        ctx.contentResolver.openOutputStream(logDoc, "wa")?.use { out ->
            out.write(payload)
            out.flush()
        }

        maybeWarnSize(ctx, logDoc)
    }

    private fun maybeWarnSize(ctx: Context, logDoc: Uri) {
        if (warned) return
        val size = runCatching {
            ctx.contentResolver.query(
                logDoc, arrayOf(DocumentsContract.Document.COLUMN_SIZE), null, null, null
            )?.use { c -> if (c.moveToFirst()) c.getLong(0) else -1L } ?: -1L
        }.getOrDefault(-1L)
        if (size >= WARN_BYTES) {
            warned = true
            // Logged into the trace itself so it's visible on upload; logging
            // does NOT stop (maintainer's instruction: warn, keep going).
            record("WARN", "size_bytes" to size, "note" to "exceeds10MB_still_logging")
        }
    }

    // --- SAF subdir + document resolution -----------------------------------

    private fun ensureLogDocument(ctx: Context, tree: Uri): Uri? = runCatching {
        val treeDoc = DocumentsContract.buildDocumentUriUsingTree(
            tree, DocumentsContract.getTreeDocumentId(tree)
        )
        val debugDir = findOrCreateChild(
            ctx, tree, treeDoc, SUBDIR, DocumentsContract.Document.MIME_TYPE_DIR
        ) ?: return null
        findOrCreateChild(
            ctx, tree, debugDir, FILENAME, "text/plain"
        )
    }.getOrNull()

    /**
     * Return the child of [parentDoc] named [displayName], creating it with
     * [mime] if absent. gnubg's own createDocument idiom; the query walks the
     * parent's children because there is no name-lookup in DocumentsContract.
     */
    private fun findOrCreateChild(
        ctx: Context, tree: Uri, parentDoc: Uri, displayName: String, mime: String
    ): Uri? {
        val parentDocId = DocumentsContract.getDocumentId(parentDoc)
        val childrenUri = DocumentsContract.buildChildDocumentsUriUsingTree(tree, parentDocId)
        ctx.contentResolver.query(
            childrenUri,
            arrayOf(
                DocumentsContract.Document.COLUMN_DOCUMENT_ID,
                DocumentsContract.Document.COLUMN_DISPLAY_NAME
            ),
            null, null, null
        )?.use { c ->
            val idCol = c.getColumnIndexOrThrow(DocumentsContract.Document.COLUMN_DOCUMENT_ID)
            val nameCol = c.getColumnIndexOrThrow(DocumentsContract.Document.COLUMN_DISPLAY_NAME)
            while (c.moveToNext()) {
                if (c.getString(nameCol) == displayName) {
                    return DocumentsContract.buildDocumentUriUsingTree(tree, c.getString(idCol))
                }
            }
        }
        return DocumentsContract.createDocument(ctx.contentResolver, parentDoc, mime, displayName)
    }

    private fun sanitize(v: Any?): String =
        v.toString().replace('\n', ' ').replace('=', ':')
}
