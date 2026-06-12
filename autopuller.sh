#!/bin/bash
ENGINE_ROOT="/home/erweitert/gnubg-android/engine-core"
UPSTREAM="/home/erweitert/gnubg-android/upstream-source/gnubg"
INCLUDE_DIR="$ENGINE_ROOT/include"

# Use a temporary file to keep track of what we've already copied to avoid loops
PROCESSED_HEADERS="/tmp/gnubg_processed_headers.txt"
touch "$PROCESSED_HEADERS"

make -C "$ENGINE_ROOT/build" > build.log 2>&1

while grep -q "fatal error: .* No such file or directory" build.log; do
    ERROR_LINE=$(grep "fatal error:" build.log | head -n 1)
    MISSING_HEADER_PATH=$(echo "$ERROR_LINE" | sed 's/.*fatal error: \(.*\):.*/\1/')
    MISSING_HEADER=$(basename "$MISSING_HEADER_PATH")
    
    # Avoid infinite loop
    if grep -q "$MISSING_HEADER" "$PROCESSED_HEADERS"; then
        echo "🛑 ERROR: Loop detected. $MISSING_HEADER was already copied but is still not found."
        echo "Check if the source file expects a subdirectory (e.g., lib/simd.h)."
        exit 1
    fi

    if [[ "$MISSING_HEADER" == "glib.h" ]] || [[ "$MISSING_HEADER" == *"gobject"* ]]; then
        echo "⚠️ Skipping system header: $MISSING_HEADER"
        touch "$INCLUDE_DIR/$MISSING_HEADER"
    else
        # Search for the file in upstream
        FOUND_PATH=$(find "$UPSTREAM" -name "$MISSING_HEADER" | head -n 1)
        
        if [ -n "$FOUND_PATH" ]; then
            DEST_DIR=$(dirname "$INCLUDE_DIR/$MISSING_HEADER_PATH")
            mkdir -p "$DEST_DIR"
            echo "✅ Copying: $MISSING_HEADER_PATH to $DEST_DIR"
            cp "$FOUND_PATH" "$DEST_DIR/"
            echo "$MISSING_HEADER" >> "$PROCESSED_HEADERS"
        else
            echo "🛑 FAILED: Could not locate $MISSING_HEADER."
            exit 1
        fi
    fi
    
    make -C "$ENGINE_ROOT/build" > build.log 2>&1
done

echo "✅ All headers resolved."