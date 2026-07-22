import java.io.FileInputStream
import java.util.Properties
import java.text.SimpleDateFormat
import java.util.Date
import java.util.TimeZone

plugins {
    alias(libs.plugins.android.application)
    alias(libs.plugins.kotlin.android)
    alias(libs.plugins.kotlin.compose)
}

// Release signing.
//
// Secrets live in keystore.properties, which is gitignored. When that file is
// available, local release builds are signed with the project's release key.
// In a clean clone, the release APK remains unsigned so that distributors such
// as F-Droid can apply their own signature.

val keystorePropertiesFile = rootProject.file("keystore.properties")
val keystoreProperties = Properties().apply {
    if (keystorePropertiesFile.exists()) {
        load(FileInputStream(keystorePropertiesFile))
    }
}
val hasReleaseKey = keystorePropertiesFile.exists()

// Build stamp read from git at configure time, so it can never drift from the
// binary (field lesson 2026-07-20: a device ran a build the repo no longer
// had; a visible stamp turns that from forensics into a glance).
fun gitOut(vararg args: String): String = try {
    val p = ProcessBuilder(listOf("git", *args)).redirectErrorStream(true).start()
    p.inputStream.bufferedReader().readText().trim().also { p.waitFor() }
} catch (e: Exception) { "" }
val gitCommit = gitOut("rev-parse", "--short", "HEAD").ifEmpty { "unknown" }
val gitDirty = gitOut("status", "--porcelain").isNotEmpty()
val buildStampUtc = SimpleDateFormat("yyyy-MM-dd HH:mm 'UTC'").apply {
    timeZone = TimeZone.getTimeZone("UTC")
}.format(Date(gitOut("log", "-1", "--format=%ct").toLongOrNull()?.times(1000) ?: 0L))

android {
    // Match the NDK the native libs are built with (recipe: r27); without
    // this AGP defaults to 26.1.x and skips stripping with a warning.
    ndkVersion = "27.0.12077973"
    namespace = "com.clavierhaus.gnubg"
    compileSdk = 35

    defaultConfig {
        applicationId = "com.clavierhaus.gnubg"
        minSdk = 31
        targetSdk = 35
        versionCode = 24
        versionName = "0.22.6"
        ndk {
            abiFilters += "arm64-v8a"
        }
        buildConfigField("String", "GIT_COMMIT", "\"$gitCommit${if (gitDirty) "+dirty" else ""}\"")
        buildConfigField("String", "BUILD_STAMP_UTC", "\"$buildStampUtc\"")
    }

    buildFeatures {
        buildConfig = true
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_21
        targetCompatibility = JavaVersion.VERSION_21
    }

    signingConfigs {
        if (hasReleaseKey) {
            create("release") {
                storeFile = file(
                    keystoreProperties.getProperty("storeFile")
                )
                storePassword = keystoreProperties.getProperty(
                    "storePassword"
                )
                keyAlias = keystoreProperties.getProperty("keyAlias")
                keyPassword = keystoreProperties.getProperty(
                    "keyPassword"
                )
            }
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = false

            if (hasReleaseKey) {
                signingConfig = signingConfigs.getByName("release")
            }
        }
    }
}

dependencies {
    implementation(libs.androidx.core.ktx)
    implementation(libs.androidx.lifecycle.runtime.ktx)
    implementation(libs.androidx.lifecycle.viewmodel.compose)
    implementation(libs.androidx.activity.compose)
    implementation(platform(libs.compose.bom))
    implementation(libs.compose.ui)
    implementation(libs.compose.ui.graphics)
    implementation(libs.compose.material3)
    implementation(libs.compose.ui.tooling.preview)
    debugImplementation(libs.compose.ui.tooling)
    implementation("androidx.datastore:datastore-preferences:1.1.1")
}
