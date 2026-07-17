import java.io.FileInputStream
import java.util.Properties

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

android {
    namespace = "com.clavierhaus.gnubg"
    compileSdk = 35

    defaultConfig {
        applicationId = "com.clavierhaus.gnubg"
        minSdk = 31
        targetSdk = 35
        versionCode = 18
        versionName = "0.22.0"
        ndk {
            abiFilters += "arm64-v8a"
        }
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
