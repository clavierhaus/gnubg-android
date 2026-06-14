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
}
