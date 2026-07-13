#!/usr/bin/env bash
set -euo pipefail

GLIB_VERSION="2.88.1"
GLIB_ARCHIVE="glib-${GLIB_VERSION}.tar.xz"
GLIB_SHA256="51ab804c56f6eab3e5045c774d1290ac5e4c923d4f9a3d8e33123bee45c1840e"
GLIB_URL="https://download.gnome.org/sources/glib/2.88/${GLIB_ARCHIVE}"

NDK_VERSION="${NDK_VERSION:-27.0.11718014}"
ANDROID_API="${ANDROID_API:-28}"

SCRIPT_DIR="$(
    cd "$(dirname "${BASH_SOURCE[0]}")"
    pwd
)"
PROJECT_ROOT="$SCRIPT_DIR"

DEPS_DIR="$PROJECT_ROOT/.deps"
ARCHIVE_PATH="$DEPS_DIR/$GLIB_ARCHIVE"
GLIB_SRC="$DEPS_DIR/glib-${GLIB_VERSION}"
BUILD_DIR="$PROJECT_ROOT/glib-android-build"
INSTALL_DIR="$PROJECT_ROOT/jni-bridge/external/glib"
CROSS_FILE="$DEPS_DIR/android-arm64.cross"

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

        if [ -d "$candidate/ndk/$NDK_VERSION" ]; then
            printf '%s\n' "$candidate"
            return 0
        fi
    done

    return 1
}

command -v curl >/dev/null 2>&1 ||
    die "curl is required"

command -v sha256sum >/dev/null 2>&1 ||
    die "sha256sum is required"

command -v tar >/dev/null 2>&1 ||
    die "tar is required"

command -v patch >/dev/null 2>&1 ||
    die "patch is required"

command -v meson >/dev/null 2>&1 ||
    die "meson is required"

command -v ninja >/dev/null 2>&1 ||
    die "ninja is required"

SDK_ROOT="$(find_sdk_root)" ||
    die "Android SDK with NDK $NDK_VERSION not found"

NDK_ROOT="$SDK_ROOT/ndk/$NDK_VERSION"
HOST_TAG="linux-x86_64"
TOOLCHAIN_BIN="$NDK_ROOT/toolchains/llvm/prebuilt/$HOST_TAG/bin"

CC="$TOOLCHAIN_BIN/aarch64-linux-android${ANDROID_API}-clang"
CXX="$TOOLCHAIN_BIN/aarch64-linux-android${ANDROID_API}-clang++"

for tool in \
    "$CC" \
    "$CXX" \
    "$TOOLCHAIN_BIN/llvm-ar" \
    "$TOOLCHAIN_BIN/llvm-strip" \
    "$TOOLCHAIN_BIN/llvm-ranlib" \
    "$TOOLCHAIN_BIN/llvm-nm"
do
    [ -x "$tool" ] ||
        die "Required NDK tool not found: $tool"
done

mkdir -p "$DEPS_DIR"

echo "==> Project root: $PROJECT_ROOT"
echo "==> Android SDK:  $SDK_ROOT"
echo "==> Android NDK:  $NDK_ROOT"
echo "==> Android API:  $ANDROID_API"
echo "==> GLib:         $GLIB_VERSION"

echo
echo "==> Fetching official GLib source archive"

if [ ! -f "$ARCHIVE_PATH" ]; then
    curl \
        --fail \
        --location \
        --retry 3 \
        --output "$ARCHIVE_PATH.part" \
        "$GLIB_URL"

    mv "$ARCHIVE_PATH.part" "$ARCHIVE_PATH"
fi

actual_sha="$(
    sha256sum "$ARCHIVE_PATH" |
        awk '{print $1}'
)"

if [ "$actual_sha" != "$GLIB_SHA256" ]; then
    printf 'Expected: %s\n' "$GLIB_SHA256" >&2
    printf 'Actual:   %s\n' "$actual_sha" >&2
    die "GLib source archive checksum mismatch"
fi

echo "==> GLib archive checksum verified"

echo
echo "==> Extracting pristine GLib source"

rm -rf "$GLIB_SRC"

tar \
    -xJf "$ARCHIVE_PATH" \
    -C "$DEPS_DIR"

[ -f "$GLIB_SRC/meson.build" ] ||
    die "Extracted GLib source is incomplete"

echo
echo "==> Applying Android iconv patch"

python3 - "$GLIB_SRC/meson.build" <<'PY'
from pathlib import Path
import sys

path = Path(sys.argv[1])
text = path.read_text()

old = "  libiconv = dependency('iconv')"
new = """  # Android: iconv is provided by Bionic libc.
  if host_machine.system() == 'android'
    libiconv = declare_dependency()
  else
    libiconv = dependency('iconv')
  endif"""

if new in text:
    print("meson.build iconv patch already present")
elif old in text:
    path.write_text(text.replace(old, new, 1))
    print("meson.build iconv patch applied")
else:
    raise SystemExit(
        "ERROR: Expected GLib iconv dependency line was not found"
    )
PY

echo
echo "==> Ensuring gconvert.c includes iconv.h"

python3 - "$GLIB_SRC/glib/gconvert.c" <<'PY'
from pathlib import Path
import sys

path = Path(sys.argv[1])
text = path.read_text()

include = "#include <iconv.h>\n"

if include not in text:
    path.write_text(include + text)
    print("gconvert.c iconv include added")
else:
    print("gconvert.c already includes iconv.h")
PY

echo
echo "==> Generating Meson Android cross-file"

cat > "$CROSS_FILE" <<CROSS
[binaries]
c = '$CC'
cpp = '$CXX'
ar = '$TOOLCHAIN_BIN/llvm-ar'
strip = '$TOOLCHAIN_BIN/llvm-strip'
ranlib = '$TOOLCHAIN_BIN/llvm-ranlib'
nm = '$TOOLCHAIN_BIN/llvm-nm'
pkg-config = 'false'

[host_machine]
system = 'android'
cpu_family = 'aarch64'
cpu = 'aarch64'
endian = 'little'

[built-in options]
c_args = ['-DANDROID', '-fPIC', '-Os']
c_link_args = ['-fPIC']

[properties]
sizeof_size_t = 8
sizeof_int = 4
sizeof_long = 8
sizeof_long_long = 8
sizeof_void_p = 8
sizeof_wchar_t = 4

have_posix_threads = true
have_libelf = false
have_libmount = false
have_selinux = false
have_xattr = false
CROSS

echo
echo "==> Removing previous GLib build and install output"

rm -rf \
    "$BUILD_DIR" \
    "$INSTALL_DIR"

mkdir -p "$INSTALL_DIR"

echo
echo "==> Configuring GLib"

meson setup \
    "$BUILD_DIR" \
    "$GLIB_SRC" \
    --cross-file "$CROSS_FILE" \
    --prefix "$INSTALL_DIR" \
    --libdir lib \
    --includedir include \
    --buildtype release \
    --default-library shared \
    -Dglib_debug=disabled \
    -Dtests=false \
    -Dinstalled_tests=false \
    -Ddocumentation=false \
    -Dman-pages=disabled \
    -Dintrospection=disabled \
    -Dgio_module_dir="$INSTALL_DIR/gio-modules" \
    -Dselinux=disabled \
    -Dxattr=false \
    -Dlibelf=disabled \
    -Dlibmount=disabled \
    -Ddtrace=disabled \
    -Dsystemtap=disabled \
    -Dsysprof=disabled \
    -Dglib_assert=false \
    -Dglib_checks=false \
    -Dnls=disabled \
    --wrap-mode=forcefallback

echo
echo "==> Building GLib"

ninja \
    -C "$BUILD_DIR" \
    -j"$(nproc)"

echo
echo "==> Installing GLib"

DESTDIR="" ninja \
    -C "$BUILD_DIR" \
    install

required_files=(
    "$INSTALL_DIR/include/glib-2.0/glib.h"
    "$INSTALL_DIR/include/libintl.h"
    "$INSTALL_DIR/lib/glib-2.0/include/glibconfig.h"
    "$INSTALL_DIR/lib/libglib-2.0.so"
    "$INSTALL_DIR/lib/libgobject-2.0.so"
    "$INSTALL_DIR/lib/libintl.so"
)

for file in "${required_files[@]}"; do
    [ -s "$file" ] ||
        die "Required GLib output is missing: $file"
done

echo
echo "==> Installed Android libraries"

for file in \
    "$INSTALL_DIR/lib/libglib-2.0.so" \
    "$INSTALL_DIR/lib/libgobject-2.0.so" \
    "$INSTALL_DIR/lib/libintl.so"
do
    file "$file"
done

echo
echo "==> GLib Android build complete"
