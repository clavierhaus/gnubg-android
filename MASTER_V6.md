# GNU Backgammon Android Port — MASTER V6

**GNU Backgammon by clavierhaus.at**  
clavierhaus.at · Vienna · Austria  
June 2026

---

## 1. Introduction & Project Objective

The objective of this project is to port the GNU Backgammon evaluation engine to the Android platform, making the full evaluation strength of gnubg — one of the world's strongest backgammon programs — available as a native Android shared library.

The primary constraint is the monolithic nature of the original gnubg desktop codebase, which is tightly coupled with GTK/GNOME UI components, file I/O, global configuration state, and platform-specific threading primitives. The project goal is to isolate the mathematical evaluation engine and expose it to Kotlin/Java via JNI (Java Native Interface) without compromising its correctness or playing strength.

### 1.1 Scope

This document covers the complete build history, all architectural decisions, every obstacle encountered and its resolution, and the exact current state of the project as of Version 6. It is intended as a working reference for any software engineer picking up or contributing to this project.

### 1.2 App Identity

| Property | Value |
|---|---|
| App name | GNU Backgammon by clavierhaus.at |
| Android package | `com.clavierhaus.gnubg` |
| Minimum Android API | 28 (Android 9.0, Pie) |
| Target ABI | `arm64-v8a` |
| Engine source | GNU Backgammon upstream (gnubg.org) |
| Repository | https://github.com/clavierhaus/gnubg-android |

---

## 2. Narrative of Progress

### Phase 1: Monolithic Analysis & Prototype

Initial efforts focused on analysing the upstream GNU Backgammon source. Attempts to build the full desktop environment proved unsustainable due to deep dependencies on GTK, GLib, and global UI state variables (`ms`, `ap`, `positions`). A "Frankenstein" build approach — attempting to stub these dependencies incrementally — established that the mathematical evaluation logic was sound in isolation, but produced a fragile, non-portable build environment that could not be taken further.

### Phase 2: Redesign — Deterministic Engineering

Phase 2 pivoted to a "Clean-Room" architecture. The attempt to port the desktop environment was discarded entirely. The "Core Engine" was isolated: a strict build hierarchy was established in which the upstream source acts as a read-only reference, and the `engine-core/` directory contains only the verified, essential mathematical components.

This phase established the orchestration scripts (`pull_dependencies.sh`, `patch_engine.sh`, `build_all.sh`) and the Patch Registry: surgical, idempotent fixes for POSIX/Android compliance.

### Phase 3: The Pristine Headless Build (Completed)

Phase 3 achieved a fully hermetic host build of the mathematical core, confirming that the engine compiles and links cleanly on Linux when detached from the GTK/GNOME infrastructure.

When cross-compilation for Android NDK was attempted, the shim approach collapsed. The root cause: gnubg uses GLib for threading primitives (`GMutex`, `GCond`, `GThread`), dynamic data structures (`GList`, `GArray`, `GHashTable`), and character encoding conversion. Shimming these by hand created an infinite chain of type collisions. This is not a sustainable path; it is an attempt to reconstruct the Linux runtime inside the Android NDK.

### Phase 4: GLib Cross-Compilation for Android (Completed)

The architectural decision was made to cross-compile a minimal but real GLib 2.88.1 for `arm64-v8a` Android API 28, rather than shimming GLib. This provides the full, tested GLib API that gnubg requires, with no behavioral deviations from upstream.

The GLib source was obtained from the Fedora 44 SRPM (`glib2-2.88.1-1.fc44`), ensuring Fedora's tested patches were applied. The result is `libglib-2.0.so` verified as ELF 64-bit ARM aarch64 for Android 28.

### Phase 5: JNI Bridge & libgnubg-engine.so (Completed)

Phase 5 constructed the JNI bridge layer (`native-lib.c`, `stubs.c`, `Engine.kt`) and CMake build pipeline producing `libgnubg-engine.so`, verified as ELF 64-bit ARM aarch64 for Android 28.

### Phase 6: NEON Acceleration & Device Verification (Completed)

Phase 6 enabled ARM NEON SIMD acceleration and verified the engine running correctly on a Pixel 8 Pro (aarch64, Android 16) via adb test harness. Two critical bugs were discovered and fixed during device testing; see §5.5 and §5.6.

**Verified results on device (opening position, 1-ply cubeless):**

| Output | Value | Expected |
|---|---|---|
| `ClassifyPosition` | 10 (CLASS_CONTACT) | correct |
| Win probability | 0.5269 | ~0.50 |
| Win gammon | 0.1478 | ~0.12 |
| Win backgammon | 0.0085 | ~0.01 |
| Lose gammon | 0.1285 | ~0.12 |
| Lose backgammon | 0.0049 | ~0.01 |
| `FindBestMove` 3-1 | 8/5 6/5 | correct (textbook) |

---

## 3. Architecture

### 3.1 Directory Structure

All project artefacts reside under `/home/erweitert/gnubg-android/`:

```
/home/erweitert/gnubg-android/
  android-sdk/               <- Android SDK & NDK (NDK 27.0.11718014)
  engine-core/               <- gnubg source (config.h is Android-patched)
    lib/                     <- gnubg math library (neuralnet, SFMT, cache...)
    config.h                 <- PATCHED: host-generated flags removed
    lib/config.h             <- Wrapper: #include "../config.h"
  upstream-source/gnubg/     <- Unmodified upstream reference
  glib-2.88.1/               <- GLib source (Fedora SRPM, patched)
  glib-android-build/        <- Meson build directory (out-of-tree)
  jni-bridge/                <- JNI bridge layer
    CMakeLists.txt           <- NDK CMake build
    config.h                 <- Shadow config.h for engine-core/*.c
    external/glib/           <- Installed GLib (headers + .so files)
    src/native-lib.c         <- JNI entry points
    src/stubs.c              <- UI/threading layer stubs + TLD init
    src/com/clavierhaus/gnubg/Engine.kt  <- Kotlin JNI declarations
    build/libgnubg-engine.so <- Final deliverable
  test-harness/              <- Android adb test harness
    CMakeLists.txt           <- Builds standalone Android executable
    src/main.c               <- Test entry point
    src/stubs.c              <- Same stubs as jni-bridge
  android-arm64.cross        <- Meson cross-file for GLib build
  build_glib_android.sh      <- GLib cross-build script
  doc/                       <- Documentation
    Makefile                 <- make / make pdf / make tex
    gnubg.tex                <- XeLaTeX template
```

### 3.2 Build Toolchain

| Component | Version / Detail |
|---|---|
| Host OS | Fedora 44, KDE Plasma 6/Wayland |
| Compiler (host) | GCC 16.1.1 (Red Hat) |
| Compiler (Android) | Clang 18.0.1 (NDK r27-beta1, build 11718014) |
| NDK | 27.0.11718014 at `android-sdk/ndk/27.0.11718014/` |
| Meson | 1.11.1 (Fedora package) |
| Ninja | 1.13.2 (Fedora package) |
| CMake | 4.3.0 (Fedora package) |
| GLib | 2.88.1 (Fedora SRPM `glib2-2.88.1-1.fc44`) |
| Target ABI | `arm64-v8a` (`aarch64-linux-android28`) |
| Android API level | 28 (Android 9.0 Pie) |
| Test device | Pixel 8 Pro (aarch64, Android 16) |

---

## 4. GLib Cross-Compilation for Android

### 4.1 Rationale

gnubg's engine uses GLib for: threading primitives (`GMutex`, `GCond`, `GThread`); dynamic data structures (`GList`, `GArray`, `GHashTable`); character encoding conversion (iconv wrapper); memory allocation wrappers; assertion macros; and logging. The engine cannot function without these. Shimming them by hand is not viable: each shim requires further GLib internals, leading to infinite type collision regression.

Cross-compiling real GLib for Android provides the full, tested implementation that gnubg was written against, with no behavioral deviation. The investment is a one-time build; the resulting `.so` files are reused for all future engine builds.

### 4.2 Meson Cross-File (android-arm64.cross)

The cross-file specifies the NDK toolchain binaries, target machine description, and platform properties that Meson cannot auto-detect in a cross build. Critical design points:

- **Do NOT define `__ANDROID_API__` in `c_args`:** The compiler binary `aarch64-linux-android28-clang` already encodes API level 28. NDK clang 18 defines it internally; a redefinition causes `-Werror` failures in Meson's size detection probes.
- **Explicit `sizeof_*` properties:** Provided to bypass Meson's cross-detection probes, which fail because they attempt to run target binaries on the host.
- **`pkg-config` disabled:** Set to `false` to prevent Meson from accidentally finding host-side `.pc` files.

### 4.3 Key Build Obstacles & Resolutions

| Obstacle | Resolution |
|---|---|
| Unknown option `iconv` | GLib 2.88 removed `-Diconv` as a Meson option. iconv detection is now via `dependency('iconv')` internally. |
| `size_t` detection failure | Caused by `-D__ANDROID_API__=26` in `c_args` conflicting with the compiler's built-in definition. Removed the redundant define. |
| iconv not found (API 26) | Bionic declares iconv under `#if __ANDROID_API__ >= 28`. Bumped target from API 26 to API 28, which is the correct minimum for this project. |
| `dependency('iconv')` fails on Android | GLib's Meson uses `clang++` to probe `iconv_open()`, but Bionic's `iconv.h` lacks `extern "C"` linkage for C++ probing. Patched GLib's `meson.build` to use `declare_dependency()` on Android since iconv is in Bionic libc with no separate library. |
| `libintl.h` not found | GLib's `gi18n.h` requires `libintl.h`. The GLib build's `proxy-libintl` subproject provides both the header and `libintl.so`. Added `external/glib/include` to the CMake include path. |

### 4.4 Installed Artefacts

The GLib build installs to `jni-bridge/external/glib/` and provides:

- `lib/libglib-2.0.so` — core GLib (the primary dependency)
- `lib/libintl.so` — proxy-libintl stub (required by `gi18n.h` / gettext macros)
- `lib/libffi.so` — libffi (GLib closure support)
- `lib/libgio-2.0.so`, `libgobject-2.0.so`, `libgmodule-2.0.so`, `libgthread-2.0.so` — GLib sublibraries
- `include/glib-2.0/` — GLib headers
- `include/libintl.h` — proxy-libintl header
- `lib/glib-2.0/include/glibconfig.h` — platform-specific GLib configuration

---

## 5. JNI Bridge Construction

### 5.1 Source File Inventory

| File | Purpose |
|---|---|
| `jni-bridge/CMakeLists.txt` | NDK CMake build definition. Lists all compiled source files, include paths, compile definitions, and linked libraries. |
| `jni-bridge/config.h` | Shadow `config.h` for `engine-core/*.c` files. Includes `../engine-core/config.h` then removes Android-incompatible flags and adds NEON defines. See §5.3. |
| `engine-core/config.h` | Host-generated autotools config, patched to remove flags invalid on Android. Canonical config for the build. |
| `engine-core/lib/config.h` | Single-line wrapper: `#include "../config.h"`. Intercepts quoted `#include "config.h"` from files in `engine-core/lib/`. |
| `engine-core/lib/neuralnetsse.c` | Modified: `NeuralNetEvaluateSSE` VLA replaced with `posix_memalign`. See §5.6. |
| `jni-bridge/src/stubs.c` | Provides storage, stub implementations, and thread-local data initialisation. See §5.4 and §5.5. |
| `jni-bridge/src/native-lib.c` | JNI entry points. Exposes `EvalInitialise`, `EvaluatePosition`, `FindBestMove`, `ClassifyPosition`, and `ApplyMove` to Kotlin. |
| `jni-bridge/src/com/clavierhaus/gnubg/Engine.kt` | Kotlin object declaring all external (JNI) functions. Loads `libgnubg-engine` via `System.loadLibrary`. |

### 5.2 Engine Source Files in Build

The following `engine-core` source files are compiled into `libgnubg-engine.so`:

| File | Role |
|---|---|
| `eval.c` | Evaluation engine core, neural network inference, move generation |
| `dice.c` | Dice RNG (SFMT/Mersenne Twister; GMP/BBS and random.org disabled) |
| `bearoff.c`, `bearoffgammon.c` | Bearoff database |
| `positionid.c` | Position ID encoding/decoding |
| `matchequity.c`, `mec.c` | Match equity tables |
| `util.c`, `file.c`, `boardpos.c` | Utility functions |
| `multithread.c` | Threading infrastructure (single-threaded on Android; `USE_MULTITHREAD` disabled) |
| `lib/output.c` | Output formatting |
| `lib/SFMT.c` | SIMD-oriented Fast Mersenne Twister (primary RNG) |
| `lib/neuralnet.c` | Neural network evaluation — plain C path at 0-ply, NEON path at 1-ply |
| `lib/neuralnetsse.c` | NEON neural net implementation (patched; see §5.6) |
| `lib/cache.c`, `lib/md5.c`, `lib/isaac.c`, `lib/list.c` | Supporting data structures |
| `lib/inputs.c` | Neural net input feature computation (`baseInputs()`) |

### 5.3 The config.h Shadow Mechanism

This is the most architecturally significant decision in the JNI bridge build.

The upstream `engine-core/config.h` is generated by GNU autotools `./configure` on the Fedora host. It contains `#define` flags for features present on Fedora that do not exist on Android:

```c
#define HAVE_LIBGMP 1            // GMP (dice.c BBS RNG)
#define USE_SIMD_INSTRUCTIONS 1  // x86 SSE/AVX — invalid on aarch64
#define HAVE_SSE 1
#define USE_SSE2 1
#define USE_AVX 1
#define LIBCURL_PROTOCOL_HTTPS 1 // randomorg.c
#define HAVE_LIBCURL 1
#define USE_MULTITHREAD 1        // GLib-based thread pool
```

These flags cannot be overridden via `-U` on the compiler command line because the C standard specifies that `#include "file.h"` (quoted include) searches the source file's own directory before any `-I` paths. This means `eval.c`'s `#include "config.h"` resolves to `engine-core/config.h` regardless of any `-I` flags pointing elsewhere.

**The solution — the shadow file mechanism:** a file named `config.h` is placed in each directory from which source files perform a quoted `config.h` include. Each shadow file includes the real `config.h` via a relative path, undefines the invalid flags, and adds Android-specific defines.

Two shadow files are required:

- `jni-bridge/config.h` — intercepts includes from `engine-core/*.c`
- `engine-core/lib/config.h` — intercepts includes from `engine-core/lib/*.c`

The current shadow config adds NEON defines after stripping x86 SIMD:

```c
#include "../engine-core/config.h"
#undef HAVE_LIBGMP
#undef LIBCURL_PROTOCOL_HTTPS
#undef HAVE_LIBCURL
#undef USE_SIMD_INSTRUCTIONS   // strip x86 SIMD first
#undef HAVE_SSE
#undef USE_SSE2
#undef USE_AVX
// ARM NEON — mandatory on aarch64, replaces x86 SIMD:
#define USE_SIMD_INSTRUCTIONS 1
#define HAVE_NEON 1
#define USE_NEON 1
```

The real `engine-core/config.h` is also regenerated without the invalid flags using `grep -v`, making the shadow undefines redundant but retained as defense-in-depth. **This approach requires no modification to any upstream source file** (except `neuralnetsse.c`; see §5.6).

### 5.4 stubs.c Design

#### 5.4.1 Global Variable Storage

Several gnubg globals are declared `extern` in headers but their storage is defined in desktop-layer source files not in the Android build. `stubs.c` provides the storage with correct types:

- `matchstate ms` — the global match state struct
- `player ap[2]` — the two player structs
- `int positions[2][30][3]` — board position geometry array (from `boardpos.h`)
- `ThreadData td` — the thread pool state struct (zero-initialised at declaration; populated by `gnubg_init_tld()`)
- `const char *szHomeDirectory`, `char *szCurrentFileName` — path globals

#### 5.4.2 Function Stubs

Functions belonging to the GTK/UI/desktop layer are stubbed with signatures matching the header declarations exactly:

- **Threading:** `Mutex_Lock`, `Mutex_Release`, `ResetManualEvent`, `SetManualEvent`, `WaitForManualEvent`, `InitManualEvent`, `FreeManualEvent`, `InitMutex`, `CloseThread`, `TLSSetValue`, `MT_CreateThreadLocalData`
- **Locking wrappers (EXP_LOCK_FUN):** `EvaluatePositionWithLocking`, `FindBestMoveWithLocking`, `FindnSaveBestMovesWithLocking`, `GeneralCubeDecisionEWithLocking`, `GeneralEvaluationEWithLocking`, `ScoreMoveWithLocking`, `BasicCubefulRolloutWithLocking` — each delegates to the corresponding `NoLocking` variant (single-threaded evaluation)
- **UI/progress:** `ProcessEvents`, `ProgressValue`, `ProgressStart`, `ProgressEnd`, `ProgressValueAdd`
- **Callbacks:** `LogCube`, `SetRNG`, `ChangeGame`, `FormatMove`, `get_current_moverecord`
- **Network dice:** `RandomorgDice`, `NetworkDice` (returns -1; requires libcurl)
- **Match state:** `save_autosave`, `msBoard`

#### 5.4.3 Disabled Features

| Feature | Reason disabled | Re-enablement |
|---|---|---|
| GMP/BBS RNG (`dice.c`) | Requires `libgmp`, unavailable on Android | Add libgmp cross-build; restore `HAVE_LIBGMP` in `config.h` |
| x86 SIMD | `HAVE_SSE`, `USE_SSE2`, `USE_AVX` invalid on aarch64 | N/A — replaced by NEON |
| GNU Multithread pool | `USE_MULTITHREAD` requires GLib thread pool integration | Re-enable `USE_MULTITHREAD`; extend `gnubg_init_tld()` accordingly |
| libcurl / random.org | `LIBCURL_PROTOCOL_HTTPS` / `HAVE_LIBCURL` removed; `randomorg.c` excluded | Cross-compile libcurl for Android; restore flags and add `randomorg.c` to build |
| Rollout (`rollout.c`) | Not included in build | Add `rollout.c` to `CMakeLists.txt`; implement required stubs |

### 5.5 Thread-Local Data Initialisation (Critical)

**Background:** When `USE_MULTITHREAD` is disabled, gnubg's move generation and scoring macros expand as follows:

```c
#define MT_Get_aMoves()   td.tld->aMoves
#define MT_Get_nnState()  td.tld->pnnState
```

Both dereference `td.tld`, a `ThreadLocalData *` inside the global `ThreadData td`. At program start, `td` is zero-initialised, making `td.tld = NULL`. The first call to 1-ply evaluation triggers `GenerateMoves` → `MT_Get_aMoves()` → null dereference at offset 0x8. Similarly, `ScoreMoves` calls `MT_Get_nnState()` → `td.tld->pnnState`, also null, causing a write through null at offset 0x40 into the NNState array.

**The fix — `gnubg_init_tld()`:** A function in `stubs.c` that must be called once after `EvalInitialise()` and before any evaluation:

```c
static ThreadLocalData gnubg_tld;
static NNState         gnubg_nn_states[3];
static move            gnubg_moves[MAX_MOVES];

void gnubg_init_tld(void) {
    unsigned int i;

    gnubg_tld.aMoves    = gnubg_moves;
    gnubg_tld.pnnState  = gnubg_nn_states;
    td.tld              = &gnubg_tld;

    /* Allocate savedBase (cHidden floats) and savedIBase (cInput floats)
     * for each NNState entry. Sizes are read from nnContact after weight load.
     * Three entries cover CLASS_RACE, CLASS_CRASHED, CLASS_CONTACT offsets. */
    for (i = 0; i < 3; i++) {
        gnubg_nn_states[i].state     = NNSTATE_NONE;
        gnubg_nn_states[i].savedBase  = g_malloc(nnContact.cHidden * sizeof(float));
        gnubg_nn_states[i].savedIBase = g_malloc(nnContact.cInput  * sizeof(float));
    }
}
```

**Why 3 NNState entries:** `ScoreMoves` accesses `nnStates[0..2]` directly, and `EvaluatePositionFull` indexes by `pc - CLASS_RACE` where `pc` ranges from `CLASS_RACE` (8) to `CLASS_CONTACT` (10), giving offsets 0, 1, 2.

**Call site in native-lib.c:** `gnubg_init_tld()` must be called inside `Java_com_clavierhaus_gnubg_Engine_initialise` after `EvalInitialise()` returns, while `gnubg_lock` is held.

**Call site in test harness:** Called immediately after `EvalInitialise()` in `main()`.

### 5.6 NeuralNetEvaluateSSE VLA Alignment Patch

**Background:** `NeuralNetEvaluateSSE` in `neuralnetsse.c` declares its hidden-layer activation buffer as a Variable Length Array with an alignment attribute:

```c
SSE_ALIGN(float ar[pnn->cHidden]);
// expands to: float ar[pnn->cHidden] __attribute__((aligned(16)));
```

On aarch64 with Clang 18, the `__attribute__((aligned(16)))` on a VLA is not guaranteed to be honoured. The NEON intrinsics (`vld1q_f32`, `vst1q_f32`) then operate on a potentially misaligned buffer, causing a SIGSEGV.

**The fix:** Replace the VLA with a heap-allocated aligned buffer:

```c
float *ar = NULL;
if (posix_memalign((void **)&ar, ALIGN_SIZE, pnn->cHidden * sizeof(float)) != 0)
    return -1;
EvaluateSSE(pnn, arInput, ar, arOutput);
free(ar);
return 0;
```

`posix_memalign` guarantees `ALIGN_SIZE` (16 bytes for NEON) alignment. This is the only modification made to a file in `engine-core/lib/` beyond the shadow `config.h`. It is documented in `PROVENANCE.md`.

---

## 6. JNI API Reference

### 6.1 Board Encoding

The board is passed as a `jintArray` of 50 elements encoding `TanBoard anBoard[2][25]`:

- Elements 0–24: `anBoard[0][i]` — opponent's checker counts
- Elements 25–49: `anBoard[1][i]` — current player's checker counts

### 6.2 Kotlin API (Engine object)

| Method | Description |
|---|---|
| `initialise(weightsPath: String): Boolean` | Must be called once before any evaluation. Pass the absolute path to `gnubg.weights` extracted to internal storage. Calls `EvalInitialise()`, `SetCubeInfo()`, and `gnubg_init_tld()`. |
| `evaluatePosition(board: IntArray): FloatArray?` | Returns `FloatArray[5]`: `[winNormal, winGammon, winBackgammon, loseGammon, loseBackgammon]`. Null on engine error. Uses 1-ply cubeless `evalcontext`. |
| `findBestMove(board: IntArray, die0: Int, die1: Int): IntArray?` | Returns `IntArray[8]` encoding `anMove[8]`; unused slots are -1. Null on error. |
| `classifyPosition(board: IntArray): Int` | Returns the `positionclass` integer (race, contact, bearoff etc). Returns -1 if not initialised. |
| `applyMove(board: IntArray, move: IntArray): IntArray` | Applies a move to a board. Returns the resulting 50-element board encoding. |

### 6.3 Thread Safety

All JNI entry points acquire a static `pthread_mutex_t` (`gnubg_lock`) before calling any engine function and release it on return. The gnubg evaluation engine uses global state (NNState, bearoff database handles, weight arrays) that is not safe to access concurrently.

### 6.4 Initialisation & Data Files

The engine requires the following files at runtime, extracted from app assets to internal storage before calling `Engine.initialise()`:

- `gnubg.weights` — trained neural network weights (~6 MB). Primary source of playing strength.
- `gnubg_os0.bd` — small one-sided bearoff database (~1.4 MB). Loaded into `pbc1`.
- `gnubg_ts0.bd` — small two-sided bearoff database (~6.8 MB). Loaded into `pbc2`.
- `gnubg_os.bd` — large one-sided bearoff database. Loaded into `pbcOS`. **Not yet deployed.**
- `gnubg_ts.bd` — large two-sided bearoff database. Loaded into `pbcTS`. **Not yet deployed.**

The `AC_PKGDATADIR` path is compiled in as `/data/data/com.clavierhaus.gnubg/files` and must match the actual extraction location.

**Note on bearoff databases:** `pbcOS` and `pbcTS` are currently null (the large databases are absent). gnubg falls back gracefully to the neural net for bearoff positions, but accuracy in late-game positions is reduced. The large databases are generated with `makebearoff` or obtained from the gnubg distribution.

---

## 7. Build Instructions

### 7.1 Prerequisites (Fedora 44)

All prerequisites are available via dnf. No pip, npm, or external package managers are required:

```bash
sudo dnf install meson ninja-build cmake glib2-devel
```

NDK location: `/home/erweitert/gnubg-android/android-sdk/ndk/27.0.11718014/`

### 7.2 Build GLib for Android (one-time, ~3–5 minutes)

```bash
cd /home/erweitert/gnubg-android
./build_glib_android.sh 2>&1 | tee glib-build.log
```

On success: `jni-bridge/external/glib/lib/libglib-2.0.so` is created (ELF 64-bit ARM aarch64, Android 28).

### 7.3 Build libgnubg-engine.so

```bash
cd /home/erweitert/gnubg-android/jni-bridge
rm -rf build && mkdir build && cd build
cmake .. \
  -DCMAKE_TOOLCHAIN_FILE=/home/erweitert/gnubg-android/android-sdk/ndk/27.0.11718014/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-28 \
  -DCMAKE_BUILD_TYPE=Release
cmake --build . 2>&1 | tee /home/erweitert/gnubg-android/jni-build.log
```

### 7.4 Verification

```bash
file jni-bridge/build/libgnubg-engine.so
# Expected: ELF 64-bit LSB shared object, ARM aarch64, for Android 28

nm jni-bridge/build/libgnubg-engine.so | grep "T Java_"
# Expected (5 JNI entry points):
#   T Java_com_clavierhaus_gnubg_Engine_applyMove
#   T Java_com_clavierhaus_gnubg_Engine_classifyPosition
#   T Java_com_clavierhaus_gnubg_Engine_evaluatePosition
#   T Java_com_clavierhaus_gnubg_Engine_findBestMove
#   T Java_com_clavierhaus_gnubg_Engine_initialise

nm jni-bridge/build/libgnubg-engine.so | grep "NeuralNetEvaluateSSE"
# Expected: T NeuralNetEvaluateSSE  (confirms NEON path active)
```

### 7.5 Android adb Test Harness

The test harness in `test-harness/` builds as a standalone Android executable and exercises the engine end-to-end on a connected device.

```bash
# Build
cd /home/erweitert/gnubg-android/test-harness
rm -rf build && mkdir build && cd build
cmake .. \
  -DCMAKE_TOOLCHAIN_FILE=/home/erweitert/gnubg-android/android-sdk/ndk/27.0.11718014/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-28 \
  -DCMAKE_BUILD_TYPE=Release
cmake --build .

# Push to device
adb push test_runner /data/local/tmp/gnubg-test_runner
adb push /path/to/gnubg.weights   /data/local/tmp/gnubg.weights
adb push /path/to/gnubg_os0.bd   /data/local/tmp/gnubg_os0.bd
adb push /path/to/gnubg_ts0.bd   /data/local/tmp/gnubg_ts0.bd
adb push jni-bridge/external/glib/lib/libglib-2.0.so /data/local/tmp/
adb push jni-bridge/external/glib/lib/libintl.so     /data/local/tmp/
adb shell chmod +x /data/local/tmp/gnubg-test_runner

# Run
adb shell "LD_LIBRARY_PATH=/data/local/tmp \
    /data/local/tmp/gnubg-test_runner /data/local/tmp/gnubg.weights"
```

Expected output:
```
=== GNU Backgammon Android Test Harness ===
[1] Initialising engine... OK
[2] ClassifyPosition = 10 (expected 10 = CLASS_CONTACT)
[3] EvaluatePosition 1-ply cubeless...
    Win prob:        0.5269
    Win gammon:      0.1478
    ...
[4] FindBestMove for 3-1...
    Move: 8/5 6/5 (expected 8/5 6/5)
=== All tests passed ===
```

---

## 8. Open Items & Roadmap

### 8.1 Features

These are the capabilities that define gnubg's world-class playing strength and analytical depth, and the primary motivation for this port.

- **Rollout:** Adding `rollout.c` to the build and implementing its required stubs would enable full rollout-based evaluation — gnubg's strongest analysis mode, using Monte Carlo simulation over thousands of games to produce statistically precise equity estimates.
- **Cube decisions:** `GeneralCubeDecisionE` is already exported and functional. A higher-level Kotlin API for cube decision evaluation (take/drop/double) should be added to `Engine.kt`.
- **SGF import/export:** gnubg's SGF layer is not in the build. Adding it would enable match file import/export, allowing games to be analysed, shared, and replayed.
- **SQLite:** `HAVE_SQLITE` is disabled. Enabling it would allow relational database features: match statistics, player tracking, historical analysis.

### 8.2 Immediate Next Steps

- **Large bearoff databases:** Obtain or generate `gnubg_os.bd` and `gnubg_ts.bd` to populate `pbcOS` and `pbcTS`. This improves accuracy in late-game bearoff positions.
- **`gnubg_init_tld()` in native-lib.c:** The JNI bridge `initialise()` function must call `gnubg_init_tld()` after `EvalInitialise()`. Currently only the test harness does this.
- **Android Studio project:** Create an Android app project, add `libgnubg-engine.so` and `libglib-2.0.so`/`libintl.so` as prebuilt native libraries, add `Engine.kt` to the source set.
- **Asset packaging:** Add `gnubg.weights` and bearoff databases to the app's assets. Implement extraction to `context.filesDir` on first launch.
- **Strip debug info:** `libgnubg-engine.so` currently carries `debug_info`. Build with `Release` and strip to reduce `.so` size.

### 8.3 Performance

- **ARM NEON SIMD:** Enabled. `NeuralNetEvaluateSSE` confirmed exported and active. `CheckNEON()` always returns 1 on aarch64 since NEON is mandatory in the architecture spec.
- **Multi-threading:** The GLib threading layer is present and functional. Re-enabling `USE_MULTITHREAD` would allow gnubg's thread pool to distribute rollout work across cores. Requires extending `gnubg_init_tld()` to handle the full `ThreadData` struct initialisation.

### 8.4 Known Warnings

- **`multithread.c`:** 6 instances of incompatible pointer types passing `pthread_mutex_t *` to `Mutex *`. Harmless: `USE_MULTITHREAD` is disabled and this code path is never executed.
- **CMake deprecation warnings** from the NDK toolchain file regarding `cmake_minimum_required VERSION < 3.10`. In the NDK's own toolchain file; cannot be fixed from the project.
- **`pbcOS=NULL`, `pbcTS=NULL`:** Large bearoff databases not deployed. gnubg falls back to neural net evaluation for bearoff positions; accuracy slightly reduced.

---

## 9. Key File Reference

### build_glib_android.sh

Orchestrates the complete GLib cross-build: applies Fedora SRPM patches, patches `meson.build` for Android iconv-in-libc, runs `meson setup` with `android-arm64.cross`, builds with ninja, installs to `jni-bridge/external/glib/`. Safe to re-run.

### android-arm64.cross

Meson cross-file for the GLib build. Specifies NDK 27 toolchain binaries (`aarch64-linux-android28-clang`), target machine (android/aarch64/little-endian), compile flags (`-DANDROID -fPIC -Os`), and platform size properties. API level 28 is encoded in the compiler binary name, not as a `-D` flag.

### jni-bridge/CMakeLists.txt

NDK CMake build file for `libgnubg-engine.so`. Enumerates all compiled source files, sets include paths (`jni-bridge/` first for shadow `config.h` interception, then `engine-core/`, `engine-core/lib/`, GLib headers), defines `AC_DATADIR`/`AC_PKGDATADIR`/`AC_DOCDIR` for Android, and links against `libglib-2.0.so`, `libintl.so`, `liblog`, and `libm`.

### jni-bridge/src/stubs.c

Provides global variable storage, function stubs for the GTK/UI layer, and the critical `gnubg_init_tld()` function. Must be kept in sync with `test-harness/src/stubs.c` — they are identical files serving the same purpose in different build contexts.

### engine-core/config.h

Host-generated autotools `config.h`, regenerated by `grep -v` to strip Android-incompatible flags. **Important:** if `engine-core/` is refreshed from upstream, this file must be regenerated and re-patched using the `grep -v` pipeline documented in §5.3.

### engine-core/lib/neuralnetsse.c

Modified from upstream: `NeuralNetEvaluateSSE` function has its VLA replaced with `posix_memalign` allocation for correct 16-byte alignment on aarch64. See §5.6 and `PROVENANCE.md` for full documentation of this change.

---

*GNU Backgammon Android Port — MASTER V6 — clavierhaus.at — June 2026*
