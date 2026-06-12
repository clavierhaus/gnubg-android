# GNU Backgammon for Android

**GNU Backgammon by clavierhaus.at**

A port of the [GNU Backgammon](https://www.gnu.org/software/gnubg/) evaluation
engine to Android as a native shared library (`libgnubg-engine.so`), exposing
the full playing strength of gnubg to Kotlin/Java via JNI.

> **Status:** Engine builds and runs on Android API 28+ (arm64-v8a).
> Android Studio project integration in progress.

---

## What this is

GNU Backgammon (gnubg) is one of the world's strongest backgammon programs,
built around a neural-network evaluation engine trained to superhuman strength.
This project isolates that engine from gnubg's GTK/GNOME desktop infrastructure
and makes it available as a self-contained Android native library.

The result is `libgnubg-engine.so` — a verified ELF 64-bit ARM aarch64 shared
object for Android 28 — exposing five JNI entry points:

| Method | Description |
|---|---|
| `initialise(weightsPath)` | Load neural network weights and bearoff databases |
| `evaluatePosition(board)` | 5-output equity evaluation (1-ply cubeless) |
| `findBestMove(board, die0, die1)` | Best move for a given dice roll |
| `classifyPosition(board)` | Position class (race, contact, bearoff…) |
| `applyMove(board, move)` | Apply a move and return the resulting position |

---

## Repository structure

```
gnubg-android/
  engine-core/               # gnubg source, Android-patched (see PROVENANCE.md)
    lib/                     # gnubg math library (neuralnet, SFMT, cache…)
    config.h                 # Patched autotools config (Android-incompatible flags removed)
    lib/config.h             # Shadow wrapper: #include "../config.h"
  upstream-source/gnubg/     # Unmodified upstream reference snapshot
  jni-bridge/                # JNI bridge layer
    CMakeLists.txt           # NDK CMake build
    config.h                 # Shadow config.h for engine-core/*.c
    src/native-lib.c         # JNI entry points
    src/stubs.c              # UI/threading layer stubs
    src/com/clavierhaus/gnubg/Engine.kt
  android-arm64.cross        # Meson cross-file for GLib cross-compilation
  build_glib_android.sh      # GLib cross-build script (one-time)
  PROVENANCE.md              # Upstream source documentation and licence compliance
  MASTER_V6.md               # Full technical reference document
```

---

## Prerequisites

| Tool | Version | Install |
|---|---|---|
| Fedora | 44 | — |
| Meson | 1.11.1 | `sudo dnf install meson` |
| Ninja | 1.13.2 | `sudo dnf install ninja-build` |
| CMake | 4.3.0 | `sudo dnf install cmake` |
| Android NDK | 27.0.11718014 | Android Studio SDK Manager |

NDK must be placed at (or symlinked from):
```
gnubg-android/android-sdk/ndk/27.0.11718014/
```

GLib source is **not** tracked in this repository. It is downloaded and built
by `build_glib_android.sh` (see below).

---

## Build

### Step 1 — Cross-compile GLib for Android (one-time, ~5 minutes)

```bash
cd gnubg-android
sudo dnf install meson ninja-build cmake glib2-devel
./build_glib_android.sh 2>&1 | tee glib-build.log
```

On success: `jni-bridge/external/glib/lib/libglib-2.0.so`
(ELF 64-bit ARM aarch64, Android 28)

### Step 2 — Build libgnubg-engine.so

```bash
cd jni-bridge
rm -rf build && mkdir build && cd build
cmake .. \
  -DCMAKE_TOOLCHAIN_FILE=../android-sdk/ndk/27.0.11718014/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-28 \
  -DCMAKE_BUILD_TYPE=Release
cmake --build . 2>&1 | tee ../../jni-build.log
```

On success: `jni-bridge/build/libgnubg-engine.so`

### Verification

```bash
file jni-bridge/build/libgnubg-engine.so
# ELF 64-bit LSB shared object, ARM aarch64, for Android 28

nm jni-bridge/build/libgnubg-engine.so | grep "T Java_"
# T Java_com_clavierhaus_gnubg_Engine_applyMove
# T Java_com_clavierhaus_gnubg_Engine_classifyPosition
# T Java_com_clavierhaus_gnubg_Engine_evaluatePosition
# T Java_com_clavierhaus_gnubg_Engine_findBestMove
# T Java_com_clavierhaus_gnubg_Engine_initialise
```

---

## Runtime data files

The engine requires these files extracted to internal storage before calling
`Engine.initialise()`:

| File | Size | Purpose |
|---|---|---|
| `gnubg.weights` | ~6 MB | Trained neural network weights |
| `gnubg_os0.bd` | ~4 MB | One-sided bearoff database |
| `gnubg_ts0.bd` | ~7 MB | Two-sided bearoff database |

These files are part of the standard gnubg distribution and are **not**
included in this repository. Obtain them from
https://www.gnu.org/software/gnubg/

The compiled-in data path is `/data/data/com.clavierhaus.gnubg/files`.
Extract assets there on first launch.

---

## Roadmap

### Features (highest impact)
- [ ] **Rollout** — synchronous single-threaded `Engine.runRollout(board, iterations)`
- [ ] **Cube decisions** — Kotlin API wrapping `GeneralCubeDecisionE` (already exported)
- [ ] **SGF import/export** — add `sgf.c`/`sgf.h` to build
- [ ] **SQLite** — link against Android's bundled `libsqlite3`

### Performance
- [ ] **ARM NEON SIMD** — add `-DHAVE_NEON` to `CMakeLists.txt`; `lib/neuralnet.c` path exists
- [ ] **Multi-threaded rollout** — re-enable `USE_MULTITHREAD` after synchronous rollout verified

### Integration
- [ ] Android Studio project skeleton
- [ ] Asset extraction on first launch
- [ ] Integration test against known gnubg reference outputs
- [ ] Strip debug info from release build

---

## Licence

GNU Backgammon is licensed under the
[GNU General Public License v3.0 or later](https://www.gnu.org/licenses/gpl-3.0.html).
This port is likewise GPL-3.0-or-later.

See `PROVENANCE.md` for full documentation of which upstream files are included,
what modifications were made, and licence compliance requirements.

---

*Clavierhaus Vienna GmbH · clavierhaus.at · June 2026*
