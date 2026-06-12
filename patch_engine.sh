#!/bin/bash
# Apply surgical fixes to engine-core sources
cd /home/erweitert/gnubg-android/engine-core/src
sed -i '1i #include "android_compat.h"' output.c dice.c eval.c 2>/dev/null || true
awk '/#include "config.h"/ {print "#define VERSION \"1.0\"\n#define PACKAGE \"gnubg\""} 1' eval.c > temp.c && mv temp.c eval.c
