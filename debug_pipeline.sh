#!/bin/bash
set -e

echo "========================================================="
echo " 1. ARCHITECTURE ENTRY POINT (Catching the Obvious)"
echo "========================================================="
find /home/erweitert/gnubg-android/gnubg-app/app/src -name "MainActivity.kt" -exec cat {} \;

echo -e "\n========================================================="
echo " 2. FULL CLEAN & VERBOSE COMPILE/LINK"
echo "========================================================="
cd /home/erweitert/gnubg-android/gnubg-app
# Wipe old compiled binaries and cache
./gradlew clean > /dev/null 2>&1
# Build with info to trace CMake/JNI compilation and linking
./gradlew assembleDebug --info > build_output.log 2>&1 || {
    echo "BUILD FAILED. Extracting JNI/Kotlin errors:"
    grep -E "e: |error:|FAILED" build_output.log | tail -n 30
    exit 1
}
echo "Build successful. CMake and Kotlin compilation linked properly."

echo -e "\n========================================================="
echo " 3. DEVICE WIPE & FRESH DEPLOY"
echo "========================================================="
adb uninstall com.clavierhaus.gnubg > /dev/null 2>&1 || true
./gradlew installDebug --console=plain >> build_output.log 2>&1
adb logcat -c

echo -e "\n========================================================="
echo " 4. EXECUTION & LOG CAPTURE"
echo "========================================================="
adb shell am start -n com.clavierhaus.gnubg/com.clavierhaus.gnubg.MainActivity
sleep 4

# Dynamically filter logs to ONLY our app process, avoiding empty outputs
APP_PID=$(adb shell pidof com.clavierhaus.gnubg)
if [ -n "$APP_PID" ]; then
    echo "App running (PID: $APP_PID). Execution logs:"
    adb logcat -d --pid=$APP_PID | tail -n 50
else
    echo "APP CRASHED OR EXITED. Android Runtime logs:"
    adb logcat -d | grep -E "AndroidRuntime|FATAL|Exception|gnubg" | tail -n 30
fi
