#!/bin/sh
# The geometry sweep -- THE SCREEN IS ONE PICTURE, tested for the record.
#
# The layout law says a screen that fits on the reference device fits
# everywhere, by arithmetic. This script is the evidence: it walks the test
# device through a matrix of emulated geometries (adb shell wm size / wm
# density), takes a screenshot of whatever the app is showing at each, and
# restores the device. A new device report becomes a ROW in this matrix,
# not a session of hand-typed overrides.
#
#   ./tools/geometry_sweep.sh                 # the built-in matrix
#   ./tools/geometry_sweep.sh 1272x2772:480   # one geometry (WxH portrait px : dpi)
#   ./tools/geometry_sweep.sh 1272x2772:480 1272x2772:540 1080x2340:420
#
# Before running: put the app on the screen you want swept (the hub, a
# finished match, the set-up editor...). The sweep restarts the app at each
# geometry, so it shows what a fresh launch shows -- the hub -- unless the
# state is persisted. For a phase deep in a match, run one geometry, drive
# the app there by hand, and take the screenshot yourself:
#
#   adb shell screencap -p /sdcard/g.png && adb pull /sdcard/g.png tmp/
#
# Screenshots land in $PWD/tmp/geometry/<WxH>_<dpi>.png. The device is
# always restored (wm size reset / wm density reset), including on Ctrl-C.
#
# Reference device, for the record: Pixel 8 Pro, 1344x2992 px at 480 dpi
# = 997x448 dp landscape. Rows below are field reports and common panels;
# add a row when a device report arrives.

set -u
cd "$(dirname "$0")/.."
mkdir -p tmp/geometry

PKG=com.clavierhaus.gnubg
MATRIX="${*:-1272x2772:480 1272x2772:540 1080x2340:420 1080x2400:440 1600x2560:320}"

restore() {
    adb shell wm size reset
    adb shell wm density reset
}
trap 'echo; echo "restoring device"; restore' EXIT INT TERM

adb get-state >/dev/null 2>&1 || { echo "no device: adb get-state failed"; exit 2; }

echo "physical: $(adb shell wm size | sed -n 's/Physical size: //p') @ $(adb shell wm density | sed -n 's/Physical density: //p') dpi"

for row in $MATRIX; do
    size=${row%%:*}
    dpi=${row##*:}
    w=${size%%x*}; h=${size##*x}
    # landscape dp box the app will see, for the log
    dpw=$(( h * 160 / dpi )); dph=$(( w * 160 / dpi ))
    echo "== $size @ $dpi dpi  ->  ${dpw}x${dph} dp landscape"
    adb shell wm size "$size"
    adb shell wm density "$dpi"
    adb shell am force-stop "$PKG"
    adb shell monkey -p "$PKG" -c android.intent.category.LAUNCHER 1 >/dev/null 2>&1
    sleep 4
    out="tmp/geometry/${size}_${dpi}.png"
    adb shell screencap -p /sdcard/geometry_sweep.png
    adb pull /sdcard/geometry_sweep.png "$out" >/dev/null
    adb shell rm /sdcard/geometry_sweep.png
    echo "   $out"
done
