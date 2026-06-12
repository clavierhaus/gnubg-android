#!/bin/bash
set -euo pipefail

PROJECT_ROOT="/home/erweitert/gnubg-android"
GLIB_SRC="${PROJECT_ROOT}/glib-2.88.1"
BUILD_DIR="${PROJECT_ROOT}/glib-android-build"
INSTALL_DIR="${PROJECT_ROOT}/jni-bridge/external/glib"
CROSS_FILE="${PROJECT_ROOT}/android-arm64.cross"

[ ! -d "${GLIB_SRC}" ]  && echo "ERROR: GLib source not found" && exit 1
[ ! -f "${CROSS_FILE}" ] && echo "ERROR: Cross-file not found" && exit 1

echo "==> Applying Fedora patches..."
cd "${GLIB_SRC}"
for p in default-terminal.patch gnutls-hmac.patch; do
    [ -f "${PROJECT_ROOT}/${p}" ] && \
        patch -p1 --forward --reject-file=/dev/null < "${PROJECT_ROOT}/${p}" || true
done

echo "==> Patching GLib meson.build for Android iconv-in-libc..."
if ! grep -q "android.*iconv" "${GLIB_SRC}/meson.build"; then
    sed -i "s/^\(  libiconv = dependency('iconv')\)$/  # Android: iconv is in Bionic libc\n  if host_machine.system() == 'android'\n    libiconv = declare_dependency()\n  else\n    libiconv = dependency('iconv')\n  endif/" \
        "${GLIB_SRC}/meson.build"
fi

echo "==> Patching gconvert.c to include iconv.h..."
if ! grep -q '#include <iconv.h>' "${GLIB_SRC}/glib/gconvert.c"; then
    sed -i '1s/^/#include <iconv.h>\n/' "${GLIB_SRC}/glib/gconvert.c"
fi

echo "==> Configuring..."
rm -rf "${BUILD_DIR}"

meson setup "${BUILD_DIR}" "${GLIB_SRC}"          \
    --cross-file "${CROSS_FILE}"                   \
    --prefix     "${INSTALL_DIR}"                  \
    --libdir     "lib"                             \
    --includedir "include"                         \
    --buildtype  release                           \
    --default-library shared                       \
    -D glib_debug=disabled                         \
    -D tests=false                                 \
    -D installed_tests=false                       \
    -D documentation=false                         \
    -D man-pages=disabled                          \
    -D introspection=disabled                      \
    -D gio_module_dir="${INSTALL_DIR}/gio-modules" \
    -D selinux=disabled                            \
    -D xattr=false                                 \
    -D libelf=disabled                             \
    -D libmount=disabled                           \
    -D dtrace=disabled                             \
    -D systemtap=disabled                          \
    -D sysprof=disabled                            \
    -D glib_assert=false                           \
    -D glib_checks=false                           \
    -D nls=disabled

echo "==> Building..."
ninja -C "${BUILD_DIR}" -j$(nproc)

echo "==> Installing..."
ninja -C "${BUILD_DIR}" install

echo ""
file "${INSTALL_DIR}/lib/libglib-2.0.so" && \
    echo "==> SUCCESS: libglib-2.0.so installed at ${INSTALL_DIR}/lib/"
