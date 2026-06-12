#!/bin/bash
ENGINE_ROOT="/home/erweitert/gnubg-android/engine-core"
UPSTREAM="/home/erweitert/gnubg-android/upstream-source/gnubg"
INCLUDE_DIR="$ENGINE_ROOT/include"
PROCESSED_HEADERS="/tmp/gnubg_processed.txt"
touch "$PROCESSED_HEADERS"

make -C "$ENGINE_ROOT/build" > build.log 2>&1
while grep -q "fatal error: .* No such file or directory" build.log; do
    HEADER_PATH=$(grep "fatal error:" build.log | head -n 1 | sed 's/.*fatal error: \(.*\):.*/\1/')
    HEADER=$(basename "$HEADER_PATH")
    if [[ "$HEADER" == "glib.h" ]]; then
        touch "$INCLUDE_DIR/$HEADER"; else
        FOUND=$(find "$UPSTREAM" -name "$HEADER" | head -n 1)
        [ -n "$FOUND" ] && mkdir -p "$(dirname "$INCLUDE_DIR/$HEADER_PATH")" && cp "$FOUND" "$(dirname "$INCLUDE_DIR/$HEADER_PATH")/"
    fi
    make -C "$ENGINE_ROOT/build" > build.log 2>&1
done
