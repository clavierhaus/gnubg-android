package com.clavierhaus.gnubg.engine

import android.content.Context
import java.io.File

object AssetExtractor {

    fun extractWeights(context: Context): String {
        val outFile = File(context.filesDir, "gnubg.weights")
        if (!outFile.exists()) {
            context.assets.open("gnubg.weights").use { input ->
                outFile.outputStream().use { output ->
                    input.copyTo(output)
                }
            }
        }
        return outFile.absolutePath
    }

    /**
     * Extract the bundled match-equity-table files (assets/met/*) into
     * filesDir/met and return that directory's path. gnubg's InitMatchEquity
     * loads a table by file path; these are the canonical upstream MET XMLs.
     */
    fun extractMets(context: Context): String {
        val outDir = File(context.filesDir, "met")
        if (!outDir.exists()) outDir.mkdirs()
        val names = context.assets.list("met") ?: emptyArray()
        for (name in names) {
            val outFile = File(outDir, name)
            if (!outFile.exists()) {
                context.assets.open("met/$name").use { input ->
                    outFile.outputStream().use { output ->
                        input.copyTo(output)
                    }
                }
            }
        }
        return outDir.absolutePath
    }
}
