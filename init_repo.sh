#!/bin/bash
# init_repo.sh
# Run once from /home/erweitert/gnubg-android/ to initialise the git repository
# and push to GitHub (or GitLab -- change the remote URL accordingly).
#
# Prerequisites:
#   git config --global user.name  "Your Name"
#   git config --global user.email "your@email.com"
#
# For GitHub:  create an empty repo at https://github.com/new
# For GitLab:  create an empty repo at https://gitlab.com/projects/new
# Then set REMOTE_URL below.

REMOTE_URL="https://github.com/clavierhaus/gnubg-android.git"
# REMOTE_URL="https://gitlab.com/YOUR_USERNAME/gnubg-android.git"

set -euo pipefail
cd /home/erweitert/gnubg-android

# -- Initialise ----------------------------------------------------------------
git init
git checkout -b main

# -- Stage everything the .gitignore allows ------------------------------------
git add .gitignore
git add README.md
git add PROVENANCE.md
git add MASTER_V6.md
git add COPYING                        # GPL-3.0 licence text from upstream
git add android-arm64.cross
git add build_glib_android.sh
git add jni-bridge/CMakeLists.txt
git add jni-bridge/config.h
git add jni-bridge/src/native-lib.c
git add jni-bridge/src/stubs.c
git add jni-bridge/src/com/clavierhaus/gnubg/Engine.kt
git add engine-core/config.h
git add engine-core/lib/config.h
git add engine-core/*.c engine-core/*.h
git add engine-core/lib/*.c engine-core/lib/*.h
git add upstream-source/

# -- Initial commit ------------------------------------------------------------
git commit -m "Initial commit: GNU Backgammon Android port -- MASTER V6

libgnubg-engine.so builds and runs on Android API 28, arm64-v8a.

Engine: gnubg 1.08.003 (GPL-3.0-or-later), patched for Android.
GLib:   2.88.1 cross-compiled for aarch64-linux-android28 (Meson).
JNI:    5 entry points -- initialise, evaluatePosition, findBestMove,
        classifyPosition, applyMove.

See MASTER_V6.md for full architecture and build documentation.
See PROVENANCE.md for upstream source documentation and licence compliance."

# -- Push ---------------------------------------------------------------------
git remote add origin "$REMOTE_URL"
git push -u origin main

echo ""
echo "Repository initialised and pushed to $REMOTE_URL"
echo "Next: create a GitHub/GitLab issue for each roadmap item in README.md"
