#!/usr/bin/env bash
set -euo pipefail

NDK_VERSION="${NDK_VERSION:-27.0.11718014}"
ANDROID_ABI="${ANDROID_ABI:-arm64-v8a}"
ANDROID_PLATFORM="${ANDROID_PLATFORM:-android-28}"
BUILD_TYPE="${BUILD_TYPE:-Release}"

SCRIPT_DIR="$(
    cd "$(dirname "${BASH_SOURCE[0]}")"
    pwd
)"
ROOT="$SCRIPT_DIR"

CMAKE_BUILD="$ROOT/jni-bridge/build-android-arm64"
GLIB_INSTALL="$ROOT/jni-bridge/external/glib"
JNILIBS="$ROOT/gnubg-app/app/src/main/jniLibs/$ANDROID_ABI"

die() {
    printf 'ERROR: %s\n' "$*" >&2
    exit 1
}

find_sdk_root() {
    local candidate

    for candidate in \
        "${ANDROID_SDK_ROOT:-}" \
        "${ANDROID_HOME:-}" \
        "$HOME/Android/Sdk" \
        "/home/erweitert/android-sdk"
    do
        [ -n "$candidate" ] || continue

        # accept the SDK if it has the pinned NDK OR any NDK at all (F-Droid
        # installs an r27 point release whose version string may differ)
        if [ -d "$candidate/ndk/$NDK_VERSION" ] || \
           [ -n "$(find "$candidate/ndk" -maxdepth 1 -mindepth 1 -type d 2>/dev/null | head -n1)" ]; then
            printf '%s\n' "$candidate"
            return 0
        fi
    done

    return 1
}

command -v cmake >/dev/null 2>&1 ||
    die "cmake is required"

command -v file >/dev/null 2>&1 ||
    die "file is required"

find_ndk_root() {
    # 1. explicit env (F-Droid sets ANDROID_NDK_HOME / ANDROID_NDK_ROOT)
    for candidate in "${ANDROID_NDK_HOME:-}" "${ANDROID_NDK_ROOT:-}"; do
        [ -n "$candidate" ] && [ -d "$candidate" ] || continue
        printf '%s\n' "$candidate"; return 0
    done
    # 2. the exact pinned version under the SDK
    if [ -d "$SDK_ROOT/ndk/$NDK_VERSION" ]; then
        printf '%s\n' "$SDK_ROOT/ndk/$NDK_VERSION"; return 0
    fi
    # 3. any installed NDK under the SDK (F-Droid installs an r27 point
    #    release whose exact version string need not match NDK_VERSION)
    if [ -d "$SDK_ROOT/ndk" ]; then
        local d
        d="$(find "$SDK_ROOT/ndk" -maxdepth 1 -mindepth 1 -type d | sort -V | tail -n1)"
        [ -n "$d" ] && { printf '%s\n' "$d"; return 0; }
    fi
    return 1
}

SDK_ROOT="$(find_sdk_root)" ||
    die "Android SDK with NDK not found (set ANDROID_SDK_ROOT or ANDROID_HOME)"

NDK_ROOT="$(find_ndk_root)" ||
    die "Android NDK not found under $SDK_ROOT/ndk (set ANDROID_NDK_HOME)"
TOOLCHAIN="$NDK_ROOT/build/cmake/android.toolchain.cmake"

[ -f "$TOOLCHAIN" ] ||
    die "Android CMake toolchain not found: $TOOLCHAIN"

echo "==> Building Android dependencies"
"$ROOT/build_glib_android.sh"

required_glib=(
    "$GLIB_INSTALL/include/glib-2.0/glib.h"
    "$GLIB_INSTALL/include/libintl.h"
    "$GLIB_INSTALL/lib/glib-2.0/include/glibconfig.h"
    "$GLIB_INSTALL/lib/libglib-2.0.so"
    "$GLIB_INSTALL/lib/libgobject-2.0.so"
    "$GLIB_INSTALL/lib/libintl.so"
)

for file_path in "${required_glib[@]}"; do
    [ -s "$file_path" ] ||
        die "Missing GLib build output: $file_path"
done

echo
echo "==> Configuring GNUbg native engine"

rm -rf "$CMAKE_BUILD"

# Reproducible-build flags: strip absolute build paths from the binary so the
# same source yields the same bytes regardless of where it is built
# (F-Droid builds under /home/vagrant, the maintainer under /home/erweitert).
# -ffile-prefix-map rewrites both debug info and __FILE__; mapping the repo
# root and the NDK root to fixed tokens removes the machine-specific paths.
REPRO_CFLAGS="-ffile-prefix-map=$ROOT=. -ffile-prefix-map=$NDK_ROOT=/ndk -Wno-builtin-macro-redefined -D__DATE__= -D__TIME__= -D__TIMESTAMP__="

cmake \
    -S "$ROOT/jni-bridge" \
    -B "$CMAKE_BUILD" \
    -DANDROID_ABI="$ANDROID_ABI" \
    -DANDROID_PLATFORM="$ANDROID_PLATFORM" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_C_FLAGS="$REPRO_CFLAGS" \
    -DCMAKE_CXX_FLAGS="$REPRO_CFLAGS" \
    -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN"

echo
echo "==> Building GNUbg native engine"

cmake \
    --build "$CMAKE_BUILD" \
    --parallel "$(nproc)"

engine="$CMAKE_BUILD/libgnubg-engine.so"

[ -s "$engine" ] ||
    die "GNUbg engine was not produced: $engine"

echo
echo "==> Preparing Android jniLibs"

rm -rf "$ROOT/gnubg-app/app/src/main/jniLibs"
mkdir -p "$JNILIBS"

cp "$engine" "$JNILIBS/libgnubg-engine.so"

mapfile -t glib_libraries < <(
    find "$GLIB_INSTALL/lib" \
        -maxdepth 1 \
        -type f \
        -name 'lib*.so' \
        -print \
        | sort
)

if [ "${#glib_libraries[@]}" -eq 0 ]; then
    die "No Android GLib shared libraries were produced"
fi

for library in "${glib_libraries[@]}"; do
    cp "$library" "$JNILIBS/"
done

echo
echo "==> Packaged native libraries"

find "$JNILIBS" \
    -maxdepth 1 \
    -type f \
    -name '*.so' \
    -printf '%f\n' \
    | sort

echo
echo "==> Verifying architecture"

while IFS= read -r library; do
    description="$(file "$library")"
    printf '%s\n' "$description"

    case "$description" in
        *ARM\ aarch64*) ;;
        *)
            die "Non-ARM64 library detected: $library"
            ;;
    esac
done < <(
    find "$JNILIBS" \
        -maxdepth 1 \
        -type f \
        -name '*.so' \
        -print \
        | sort
)

echo
echo "==> Complete Android native build finished"
