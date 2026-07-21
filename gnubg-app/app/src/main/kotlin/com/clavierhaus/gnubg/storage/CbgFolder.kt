package com.clavierhaus.gnubg.storage

import android.content.Context
import android.content.Intent
import android.net.Uri
import android.provider.DocumentsContract
import java.io.File

/**
 * The CBG folder: one user-chosen directory, granted once through the
 * Storage Access Framework, holding plain gnubg-format files the user can
 * browse, back up, and open anywhere gnubg runs. The same folder can be
 * granted to more than one edition of the app, so match data is never locked
 * to an installation (docs/PLUS_STRATEGY: the settled storage mechanism).
 *
 * Persistence is the SYSTEM's: takePersistableUriPermission() makes the
 * grant survive reboots, and ContentResolver.persistedUriPermissions is the
 * durable record of it. The app takes exactly one tree grant, ever, so that
 * list IS the store -- no preference key, nothing to migrate, nothing to go
 * stale separately from the grant itself. A revoked or deleted folder simply
 * disappears from the list and the app asks again at the next save.
 *
 * Design law (maintainer): the folder is requested at the FIRST tap of Save,
 * never at launch -- the question makes sense exactly when it is asked.
 */
object CbgFolder {

    /** The granted CBG folder, or null if none (never granted, or revoked). */
    fun grantedTree(context: Context): Uri? =
        context.contentResolver.persistedUriPermissions
            .firstOrNull { it.isWritePermission && it.isReadPermission }
            ?.uri

    /** Persist a fresh grant from the tree picker. */
    fun take(context: Context, uri: Uri) {
        context.contentResolver.takePersistableUriPermission(
            uri,
            Intent.FLAG_GRANT_READ_URI_PERMISSION or Intent.FLAG_GRANT_WRITE_URI_PERMISSION
        )
    }

    /** Best-effort initial location for the tree picker: Documents/ on the
     *  primary volume. Pickers that don't honour it fall back gracefully. */
    fun documentsInitialUri(): Uri =
        DocumentsContract.buildDocumentUri(
            "com.android.externalstorage.documents", "primary:Documents"
        )

    /**
     * Create [displayName] inside the granted tree and fill it with [tmp]'s
     * bytes. Returns true on success. The document provider uniquifies a
     * colliding name itself (name (1).sgf), so no pre-check is needed.
     * A false return usually means the grant went stale (folder deleted,
     * permission revoked) -- the caller should fall back to asking again.
     */
    fun saveInto(context: Context, tree: Uri, displayName: String, tmp: File): Boolean =
        runCatching {
            val parent = DocumentsContract.buildDocumentUriUsingTree(
                tree, DocumentsContract.getTreeDocumentId(tree)
            )
            val child = DocumentsContract.createDocument(
                context.contentResolver, parent, "application/octet-stream", displayName
            ) ?: return false
            context.contentResolver.openOutputStream(child)?.use { out ->
                tmp.inputStream().use { input -> input.copyTo(out) }
            } ?: return false
            true
        }.getOrDefault(false)
}
