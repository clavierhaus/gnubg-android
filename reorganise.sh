#!/bin/bash
# reorganise.sh
# Run once from /home/erweitert/gnubg-android/
# Moves documentation files to doc/, cleans up root clutter.

set -euo pipefail
cd /home/erweitert/gnubg-android

echo "==> Creating doc/ directory..."
mkdir -p doc

echo "==> Moving doc files to doc/..."
mv Makefile   doc/
mv gnubg.tex  doc/
mv MASTER_V6.pdf doc/ 2>/dev/null || true   # may not exist yet; make will generate it

echo "==> Moving MASTER_V6.md to repo root (if not already there)..."
# MASTER_V6.md belongs at repo root, referenced as ../MASTER_V6.md from doc/Makefile
ls MASTER_V6.md 2>/dev/null && echo "  MASTER_V6.md already at root" || echo "  WARNING: MASTER_V6.md not found -- copy it here"

echo "==> Cleaning editor backups..."
rm -f init_repo.sh~ autopuller.sh~

echo "==> Cleaning SRPM extraction artefacts (not tracked)..."
rm -f glib2.spec default-terminal.patch gnutls-hmac.patch

echo "==> Updating .gitignore to cover new layout..."
cat > .gitignore << 'EOF'
# --- Build output -------------------------------------------------------------
jni-bridge/build/
glib-android-build/
*.o
*.a
*.so
*.log

# --- GLib source tree (reproducible via build_glib_android.sh) ----------------
glib-2.88.1/

# --- GLib SRPM extraction artefacts -------------------------------------------
glib2-*.src.rpm
*.patch
glib2.spec

# --- Android SDK / NDK (large; installed separately) -------------------------
android-sdk/

# --- Installed GLib binaries (reproducible via build_glib_android.sh) ---------
jni-bridge/external/

# --- CMake artefacts ----------------------------------------------------------
CMakeCache.txt
CMakeFiles/
cmake_install.cmake

# --- Meson artefacts ----------------------------------------------------------
.mesonpy/
meson-info/
meson-logs/
meson-private/

# --- Doc build output (generated; not tracked) --------------------------------
doc/MASTER_V6.pdf
doc/MASTER_V6.tex

# --- Misc ---------------------------------------------------------------------
*.zip
*~
.DS_Store
*.swp
.idea/
.vscode/
files.zip
EOF

echo ""
echo "==> Result:"
echo ""
echo "--- Root ---"
ls -1 /home/erweitert/gnubg-android/
echo ""
echo "--- doc/ ---"
ls -1 /home/erweitert/gnubg-android/doc/
echo ""
echo "==> Done. Next: update init_repo.sh REMOTE_URL then run it."