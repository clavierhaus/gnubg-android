#!/bin/sh
# Clock harness gate. Exit code is the verdict.
# The module must compile clean as strict C99 with all warnings fatal --
# that IS part of the promise.
set -u
cd "$(dirname "$0")"
mkdir -p tmp
gcc -std=c99 -pedantic -Wall -Wextra -Werror -O2 \
    -I ../../engine-core \
    ../../engine-core/timecontrol.c harness.c -o tmp/clock_harness
rc=$?
if [ $rc -ne 0 ]; then
    echo "clock harness: BUILD FAILED" >&2
    exit $rc
fi
./tmp/clock_harness
