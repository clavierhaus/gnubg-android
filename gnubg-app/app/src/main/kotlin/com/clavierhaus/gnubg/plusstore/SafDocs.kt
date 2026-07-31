package com.clavierhaus.gnubg.plusstore

import android.content.Context
import android.net.Uri
import android.provider.DocumentsContract

/**
 * Plus-only SAF document helpers, shared by the field debug trace and the
 * career ledger. Kept on the Plus side deliberately: the FOSS CbgFolder is
 * upstream and must not receive code derived from Plus features (firewall
 * order). gnubg's own DocumentsContract idiom throughout; no name-lookup
 * exists in SAF, so child resolution walks the parent's children.
 */
object SafDocs {

    /**
     * Return the child of [parentDoc] named [displayName], creating it with
     * [mime] if absent. Returns null on failure.
     */
    fun findOrCreateChild(
        ctx: Context, tree: Uri, parentDoc: Uri, displayName: String, mime: String
    ): Uri? {
        findChild(ctx, tree, parentDoc, displayName)?.let { return it }
        return runCatching {
            DocumentsContract.createDocument(ctx.contentResolver, parentDoc, mime, displayName)
        }.getOrNull()
    }

    /** Return the child of [parentDoc] named exactly [displayName], or null. */
    fun findChild(ctx: Context, tree: Uri, parentDoc: Uri, displayName: String): Uri? {
        val parentDocId = runCatching {
            DocumentsContract.getDocumentId(parentDoc)
        }.getOrNull() ?: return null
        val childrenUri = DocumentsContract.buildChildDocumentsUriUsingTree(tree, parentDocId)
        runCatching {
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
        }
        return null
    }

    /** The tree's root as a document Uri. */
    fun treeRootDoc(tree: Uri): Uri =
        DocumentsContract.buildDocumentUriUsingTree(
            tree, DocumentsContract.getTreeDocumentId(tree)
        )

    /**
     * Create a document under [parentDoc] and return BOTH its Uri and the
     * display name the provider ACTUALLY gave it (providers may uniquify a
     * colliding name -- "x (1).sgf"). The caller must record the returned
     * name, never the requested one (filename truth for the ledger).
     */
    fun createDocumentReturningName(
        ctx: Context, parentDoc: Uri, mime: String, requestedName: String
    ): Pair<Uri, String>? {
        val uri = runCatching {
            DocumentsContract.createDocument(ctx.contentResolver, parentDoc, mime, requestedName)
        }.getOrNull() ?: return null
        val actual = runCatching {
            ctx.contentResolver.query(
                uri, arrayOf(DocumentsContract.Document.COLUMN_DISPLAY_NAME), null, null, null
            )?.use { c -> if (c.moveToFirst()) c.getString(0) else null }
        }.getOrNull() ?: requestedName
        return uri to actual
    }

    /** Append [payload] to the document at [doc] ("wa" mode -- provider seeks EOF). */
    fun append(ctx: Context, doc: Uri, payload: ByteArray): Boolean = runCatching {
        ctx.contentResolver.openOutputStream(doc, "wa")?.use { out ->
            out.write(payload); out.flush(); true
        } ?: false
    }.getOrDefault(false)

    /** Overwrite the document at [doc] with [payload] ("wt" mode). */
    fun overwrite(ctx: Context, doc: Uri, payload: ByteArray): Boolean = runCatching {
        ctx.contentResolver.openOutputStream(doc, "wt")?.use { out ->
            out.write(payload); out.flush(); true
        } ?: false
    }.getOrDefault(false)

    /** Read the document's full bytes, or null. */
    fun readAll(ctx: Context, doc: Uri): ByteArray? = runCatching {
        ctx.contentResolver.openInputStream(doc)?.use { it.readBytes() }
    }.getOrNull()
}
