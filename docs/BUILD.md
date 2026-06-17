# Build Notes

These notes reflect the local development setup used during the 0.8.9 milestone.

## Paths

Project root:

```sh
/home/erweitert/gnubg-android
```

Android app:

```sh
/home/erweitert/gnubg-android/gnubg-app
```

Android SDK:

```sh
/home/erweitert/android-sdk
```

NDK toolchain:

```sh
/home/erweitert/android-sdk/ndk/27.0.11718014/build/cmake/android.toolchain.cmake
```

Java:

```sh
/home/peter/.sdkman/candidates/java/21.0.7-tem/bin/java
```

## App build

Use this for Kotlin/UI-only work:

```sh
cd /home/erweitert/gnubg-android/gnubg-app || return

OUT="/home/erweitert/gnubg-android/gnubg-app/tmp/app_build.txt"
{
  set -e
  ./gradlew assembleDebug
} > "$OUT" 2>&1

RC=$?
printf 'exit code: %s\n' "$RC"
printf 'wrote: %s\n' "$OUT"
tail -n 120 "$OUT"
```

## Install and start

```sh
cd /home/erweitert/gnubg-android/gnubg-app || return

OUT="/home/erweitert/gnubg-android/gnubg-app/tmp/app_install_start.txt"
{
  set -e
  adb install -r app/build/outputs/apk/debug/app-debug.apk
  adb shell am force-stop com.clavierhaus.gnubg
  adb shell logcat -c
  adb shell am start -n com.clavierhaus.gnubg/.MainActivity
} > "$OUT" 2>&1

RC=$?
printf 'exit code: %s\n' "$RC"
printf 'wrote: %s\n' "$OUT"
tail -n 120 "$OUT"
```

## Native rebuild

Only use this when native bridge or engine code changed:

```sh
cd /home/erweitert/gnubg-android || return

OUT="/home/erweitert/gnubg-android/gnubg-app/tmp/native_app_rebuild.txt"
{
  set -e
  rm -rf jni-bridge/build
  cmake -S jni-bridge -B jni-bridge/build \
    -DCMAKE_TOOLCHAIN_FILE=/home/erweitert/android-sdk/ndk/27.0.11718014/build/cmake/android.toolchain.cmake \
    -DANDROID_ABI=arm64-v8a \
    -DANDROID_PLATFORM=android-23
  cmake --build jni-bridge/build -j"$(nproc)"
  cp jni-bridge/build/libgnubg-engine.so \
    gnubg-app/app/src/main/jniLibs/arm64-v8a/libgnubg-engine.so
  nm -D gnubg-app/app/src/main/jniLibs/arm64-v8a/libgnubg-engine.so \
    | grep -E 'Java_com_clavierhaus_gnubg_Engine_applyHumanDoubleTake|Java_com_clavierhaus_gnubg_Engine_cubeDecision|Java_com_clavierhaus_gnubg_Engine_getCubeDebugState'
  cd gnubg-app
  ./gradlew assembleDebug
} > "$OUT" 2>&1

RC=$?
printf 'exit code: %s\n' "$RC"
printf 'wrote: %s\n' "$OUT"
tail -n 140 "$OUT"
```

## Important shell rule

Do not use `exit` in pasted Konsole blocks. Use `|| return` for directory changes and print return codes instead.
