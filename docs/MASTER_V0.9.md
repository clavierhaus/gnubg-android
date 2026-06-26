# GNU Backgammon Android Port — MASTER V0.9

> **Status:** supersedes MASTER V8. The pre-facade foundation chapters (GLib
> cross-compilation, the `config.h` shadow, thread-local-data init, the NEON
> patch, build instructions) are carried forward intact; everything describing
> the JNI/architecture layer has been rewritten to reflect the platform-neutral
> **facade** that now sits between the engine and Android, the three-tier
> de-Androidification of the JNI bridge, and the engine-routed cube handling.
>
> **Verified working on device** (Pixel 8 Pro, Android 16): full game play,
> legal-move generation, evaluation, classification, cube decisions and the
> human↔engine cube exchange routed through the real gnubg engine, rollouts,
> SGF load/save.

---

## 0. Document Maintenance Mandate (read first)

**This document must be kept in lock-step with the code. Updating it is not
optional housekeeping — it is part of completing any change.**

This project has been built across many sessions, much of it with LLM
assistance. The single most common way such a project rots is that the code
moves forward while the documentation silently freezes — and the next
contributor (human or model) then reasons from a false map. That has already
happened once here: MASTER V8 described a pre-facade architecture for several
revisions after the facade existed, and a prior assistant clobbered the `doc/`
tree entirely. Both cost real time to recover from.

Therefore, to **any future LLM or human** working on this repository:

1. **If you change the code, you change this document in the same change.** A
   patch that alters architecture, adds or removes a JNI/facade function,
   modifies the build, or touches the engine is **not complete** until the
   relevant section here is updated. Treat a stale doc as a bug.
2. **Never reconstruct documentation from memory or assumption.** Read the
   actual source. If a fact (a file's role, a function's contract, a build
   step) is not verified against the tree in front of you, do not write it.
3. **Record every divergence from pristine upstream in `PROVENANCE.md`.** The
   engine in `engine-core/` is vendored gnubg 1.08.003. Any local edit to it —
   however small — is a disclosure obligation and a future-re-vendor hazard if
   undocumented. One-line edits count.
4. **Preserve, don't paraphrase, the hard-won specifics.** The GLib build
   obstacles, the `config.h` shadow mechanism, the NEON VLA patch, the exact
   build commands: these encode trial-and-error that is expensive to
   rediscover. When updating, extend and correct — do not lossily rewrite.
5. **When you supersede this file, keep the old one in git history**, bump the
   version, and note what changed at the top. Do not maintain two "current"
   masters in parallel; they drift.

If you are an LLM and you find yourself about to end a working session having
modified code but not this document: stop and update it first. The code and its
description are one artifact.

---

## 1. Introduction & Project Objective

### 1.1 Scope

A native Android port of GNU Backgammon: the full gnubg evaluation/rollout/cube
engine, de-coupled from its GTK desktop shell, compiled to a shared library
(`libgnubg-engine.so`) and driven from a Kotlin/Jetpack-Compose UI through a
JNI bridge and a platform-neutral C facade.

The defining architectural principle: **gnubg source is the only authority for
game logic. No backgammon rule, move, evaluation, or cube decision is ever
re-implemented in Kotlin or in the bridge.** The app marshals data in and reads
results out; the engine decides everything.

### 1.2 App Identity

| Property | Value |
|---|---|
| Application ID | `com.clavierhaus.gnubg` |
| Engine | GNU Backgammon 1.08.003 (vendored, de-GTK'd) |
| UI | Kotlin + Jetpack Compose (Material You) |
| Native ABI | `arm64-v8a` (`aarch64-linux-android28`) |
| Min API | 28 (Android 9.0) |
| Build | Gradle (`gnubg-app/`), CMake/NDK for native |
| Test device | Pixel 8 Pro (aarch64, Android 16) |

### 1.3 Layer Map (the heart of the design)

```
  Jetpack Compose UI            (Kotlin; rewritten per platform)
        |
  GameViewModel / BoardState    (Kotlin; state-holding only, logic-free —
        |                        every decision is an Engine.* call)
  Engine.kt                     (Kotlin object of `external fun` JNI decls —
        |                        the ABI seam)
  ============== JNI boundary (native-lib.c is the ONLY file with <jni.h>) =====
        |
  native-lib.c                  (C; JNI marshalling ONLY — int[]/float[]/jstring
        |                        in and out, no game logic)
  gnubg_mobile.{c,h}            (C; PLATFORM-NEUTRAL FACADE. Plain C, self-
        |                        locking, zero JNI. Boards cross as flat int[50].
        |                        Header forbids exposing JNI/Kotlin/Swift/UI types.)
  android-app.c                 (C; gnubg.c GTK-shell replacement; engine
        |                        callbacks; zero JNI)
  engine-core/ + lib/           (vendored gnubg 1.08.003, de-GTK'd; the authority)
```

The value of this stack: an iOS port re-implements only the top two boxes
(an `Engine.swift` mirroring `Engine.kt` over the same facade, a SwiftUI layer).
Everything from `gnubg_mobile.*` down is already platform-neutral C and moves
across unchanged.

---

## 2. Narrative of Progress

### Phases 1–9 (foundation — completed, see MASTER V8 history for detail)

1. **Monolithic analysis & prototype** — initial feasibility.
2. **Redesign — deterministic engineering** — reproducible build discipline.
3. **Pristine headless build** — gnubg compiling without GTK.
4. **GLib cross-compilation for Android** — real GLib `.so`s (see §4).
5. **JNI bridge & `libgnubg-engine.so`** — first native library on device.
6. **NEON acceleration & device verification** — `NeuralNetEvaluateSSE` active.
7. **Cube decisions & rollout** — equity matrix + `CubeDecision` enum.
8. **Full game logic layer** — `play.c` compiled, match state machine live.
9. **Multi-threaded rollout & SGF JNI API** — `GThreadPool`, SGF load/save.

### Phase 10: The Facade & De-Androidification (completed)

The JNI bridge had grown to carry game logic directly — `native-lib.c` called
`GenerateMoves`, `PositionKey`, `SetCubeInfo`, packed move lists, and read the
`ms` match-state struct inline. That coupled the engine to Android: an iOS port
would have had to re-implement all of it in Swift.

A platform-neutral C facade (`gnubg_mobile.{c,h}`) was introduced as the single
seam, and the engine-touching code was moved out of the JNI layer down into the
facade in three verified tiers:

- **Tier 1** — state readers (board, score, dice, turn, status, cube info,
  game result), board utilities (pip count, swap, apply-sub-move, format-move),
  and file/SGF operations. Mechanical relocation; the JNI wrappers became pure
  marshalling.
- **Tier 2** — the real algorithms: `getLegalMoves`, `findMove`,
  `evaluatePosition`, `classifyPosition`, `cubeDecision`, `rollout`. Their
  bodies (move generation, position-key matching, cube evaluation) moved whole
  into the facade; the JNI side keeps only the JVM transport detail (e.g. the
  16-int bit-packed cube-decision array).
- **Cleanup (Tier A)** — removed the dead `extern` declarations the relocation
  left behind and retired `applyHumanDoubleTake` (see Phase 11).

Each tier was applied as a verified patch (byte-exact baseline, `git apply
--check`, compile-checked, device-tested) and committed separately. After this
work, `native-lib.c` includes `<jni.h>` and is the **only** file that does;
the de-Android invariant holds at the file level.

### Phase 11: Cube handling routed through the engine (completed)

The human↔engine cube exchange had been faked: a JNI function
`applyHumanDoubleTake` hand-mutated the `ms` struct (`ms.nCube *= 2`, etc.),
bypassing gnubg's cube machinery. The symptom was that the engine appeared to
accept every double regardless of position — in fact gnubg's correct *pass*
decision was being computed and then silently discarded by the app.

This was fixed by routing cube actions through the real engine ("Option B"):

- A one-line visibility setter was added to `engine-core/play.c`:
  `void gnubg_set_computer_decision(int f) { fComputerDecision = f; }`. gnubg's
  `CommandTake`/`CommandDrop`/`CommandDouble` refuse to act for a non-human
  player unless the static `fComputerDecision` flag is set; this setter is the
  single documented hook that lets the facade drive them. (This is the **only**
  local modification to the engine — recorded in `PROVENANCE.md`.)
- The facade gained `gnubg_mobile_engine_cube_response(int take)`: it raises the
  flag, calls the real `CommandTake` or `CommandDrop`, clears the flag, drains
  pending turns.
- `GameViewModel.offerDouble` was rewired so both branches register the human
  double through the real `CommandDouble()` and let the engine respond: a take
  continues play; a pass ends the game with gnubg's own point award via
  `getGameResult()`.
- `applyHumanDoubleTake` was retired (removed in Tier A cleanup).

Result: cube decisions, point math, beavers/Crawford handling, and game-over
on a dropped double are all gnubg's, not the app's. (Beaver *UI* is not yet
implemented; beaver decisions currently collapse to take — see §8.)

### Phase 12: Repository completeness & provenance (completed)

An audit found that 10 engine `.c` files compiled into the Android build were
gitignored (swept onto the desktop-denylist during curation), so a fresh clone
would not build. Nine were byte-identical to upstream; only `play.c` carried a
local edit (the cube setter). All 10 sources plus their headers were untracked
from `.gitignore` and committed; generated build directories were ignored;
`PROVENANCE.md` was created recording the single engine divergence. The
repository is now build-complete from a clean clone.

---

## 3. Architecture

### 3.1 Directory Structure

```
/home/erweitert/gnubg-android/
  engine-core/               <- vendored gnubg 1.08.003 (config.h Android-patched)
    lib/                     <- gnubg math library (neuralnet, SFMT, cache...)
    config.h                 <- PATCHED: host-generated flags removed
    lib/config.h             <- wrapper: #include "../config.h"
    play.c                   <- LOCALLY MODIFIED: cube-decision visibility setter
                                (the only engine divergence; see PROVENANCE.md)
  upstream-source/gnubg/     <- unmodified upstream reference (for diff/provenance)
  jni-bridge/                <- JNI bridge + facade layer
    CMakeLists.txt           <- NDK CMake build (lists engine sources, facade, JNI)
    config.h                 <- shadow config.h for engine-core/*.c (see §5.3)
    include/gnubg_mobile.h   <- facade public header (platform-neutral contract)
    external/glib/           <- installed GLib (headers + .so files)
    src/native-lib.c         <- JNI entry points (marshalling ONLY)
    src/gnubg_mobile.c       <- PLATFORM-NEUTRAL FACADE (game logic lives here)
    src/android-app.c        <- gnubg.c GTK-shell replacement; engine callbacks
    src/stubs.c              <- remaining UI/threading stubs, TLD init, rollout infra
  gnubg-app/                 <- Android Studio / Gradle project
    gradlew                  <- build entry (./gradlew assembleDebug)
    app/src/main/kotlin/com/clavierhaus/gnubg/
      Engine.kt              <- Kotlin object: external (JNI) function declarations
      engine/GameViewModel.kt<- state orchestration (logic-free; calls Engine.*)
      engine/BoardState.kt    <- immutable UI state
      ui/...                 <- Jetpack Compose UI
    app/build/outputs/apk/debug/*.apk   <- built APK
  test-harness/              <- standalone portability harness (no JNI)
    CMakeLists.txt
    src/main.c               <- links engine + facade + mobile-app.c, plays headless
  doc/                       <- documentation (this file; Makefile; LaTeX template)
  PROVENANCE.md              <- record of every local engine-core divergence
### 7.6 Build and deploy scripts

Two scripts live at the repo root and own the full build and deploy pipeline.

**`build.sh` -- native build + APK package.** Run whenever C sources change
(`engine-core/`, `jni-bridge/src/`, `jni-bridge/include/`):

```
./build.sh               # cmake --build + copy .so + gradlew assembleDebug
./build.sh --native-only # cmake --build + copy .so only (skip Gradle)
./build.sh --apk-only    # Gradle only (Kotlin-only changes, .so unchanged)
./build.sh --reconfigure # wipe CMake dir and reconfigure from scratch
```

Pipeline: (1) `cmake --build jni-bridge/build-android-arm64` recompiles
`libgnubg-engine.so`; (2) copies it to
`gnubg-app/app/src/main/jniLibs/arm64-v8a/`; (3) `./gradlew assembleDebug`.

**`run_on_device.sh` -- install and launch.** Run after `build.sh`:

```
./run_on_device.sh               # install APK + launch
./run_on_device.sh --no-build    # skip Gradle; install existing APK only
./run_on_device.sh --logcat      # stream app log after launch (Ctrl-C stops)
./run_on_device.sh --reinstall   # uninstall first (clears data), then install
```

`run.sh` is an alias for `run_on_device.sh` kept for convenience.

**Standard workflow after any C source change:**
`./build.sh` followed by `./run_on_device.sh --no-build`

**Kotlin-only change:**
`./build.sh --apk-only` followed by `./run_on_device.sh --no-build`

### 3.2 Build Toolchain

| Component | Version / Detail |
|---|---|
| Host OS | Fedora 44, KDE Plasma 6/Wayland |
| Compiler (Android) | Clang (NDK r27, build 11718014) |
| NDK | 27.0.11718014 |
| Meson / Ninja | 1.11.1 / 1.13.2 (GLib build) |
| CMake | 4.3.0 |
| GLib | 2.88.1 (Fedora SRPM, cross-compiled — see §4) |
| Target ABI | `arm64-v8a` (`aarch64-linux-android28`) |
| Android API | 28 (min); tested on Android 16 device |
| Test device | Pixel 8 Pro (aarch64) |

### 3.3 The Facade Contract (gnubg_mobile.h)

The facade header is the platform-neutral seam. Its rules, which any port relies
on and any contributor must preserve:

- **Plain C only.** No `<jni.h>`, no Kotlin/Swift/Java/Objective-C types, no UI
  types. If a facade signature would need such a type, the design is wrong.
- **Boards cross as flat `int[50]`** — gnubg's `TanBoard` is 2 rows of 25; the
  facade packs/unpacks at its boundary so callers never see engine board types.
- **Self-locking.** Facade functions take `gnubg_lock` themselves; callers do
  not manage engine threading.
- **Return contracts are explicit and stable** — counts, flat arrays, and (for
  cube decisions) a documented packed layout. The JVM-specific transport
  encoding (e.g. float-bits-as-int) stays in `native-lib.c`, not the facade.

This is what makes the facade reusable: an iOS `Engine.swift` calls the same C
functions with the same flat-array contracts; only the marshalling file
(`native-lib.c` ↔ a future `native-ios.m`/Swift-C interop) is platform-specific.

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

---

## 5. Bridge & Facade Construction

### 5.1 Source File Inventory (current)

| File | Role |
|---|---|
| `jni-bridge/CMakeLists.txt` | NDK CMake build: lists engine sources, the facade, and JNI; include paths; compile defs; linked GLib `.so`s. |
| `jni-bridge/config.h` | Shadow `config.h` for `engine-core/*.c`. See §5.3. |
| `jni-bridge/include/gnubg_mobile.h` | **Facade public header** — the platform-neutral contract (§3.3). |
| `jni-bridge/src/gnubg_mobile.c` | **The facade.** All engine-touching game logic: board (un)packing, state readers, board utils, file/SGF ops, legal-move gen, find-move, evaluate, classify, cube decision, rollout, the cube command verbs, and `gnubg_mobile_engine_cube_response`. Plain C, self-locking, zero JNI. |
| `jni-bridge/src/native-lib.c` | JNI entry points. Marshalling ONLY: `int[]`/`float[]`/`jstring` in and out, then a single `gnubg_mobile_*` call. The only file including `<jni.h>`. |
| `jni-bridge/src/android-app.c` | gnubg.c GTK-shell replacement; engine event callbacks (`gnubg_on_*`); zero JNI. |
| `jni-bridge/src/stubs.c` | Remaining UI/threading stubs, thread-local-data init (§5.5), rollout infrastructure. |
| `gnubg-app/app/src/main/kotlin/.../Engine.kt` | Kotlin object: `external fun` JNI declarations. |
| `gnubg-app/app/src/main/kotlin/.../engine/GameViewModel.kt` | State orchestration; logic-free; every decision is an `Engine.*` call. |
| `engine-core/play.c` | Game state machine + the one local edit: `gnubg_set_computer_decision` (§Phase 11, PROVENANCE.md). |

### 5.2 Engine Source Files in the Build

Compiled into `libgnubg-engine.so` from `engine-core/` (all tracked as of
Phase 12). Core: `eval.c`, `dice.c`, `bearoff.c`, `bearoffgammon.c`,
`positionid.c`, `matchequity.c`, `mec.c`, `util.c`, `file.c`, `boardpos.c`,
`multithread.c`, `rollout.c`, `sgf.c`, `sgf_y.c`, `sgf_l.c`, `play.c`,
`analysis.c`, `format.c`, `formatgs.c`, `drawboard.c` (`FormatMove`),
`external.c`, `glib-ext.c`, `matchid.c`, `set.c`, `renderprefs.c`. Library
(`lib/`): `output.c`, `SFMT.c`, `neuralnet.c`, `neuralnetsse.c` (NEON, patched —
§5.6), `cache.c`, `md5.c`, `isaac.c`, `list.c`, `inputs.c`.

The desktop/GTK/UI files (`gtk*.c`, `render*.c`, `html*.c`, `sound.*`, etc.)
are excluded via `.gitignore` and not compiled.
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
| GNU Multithread pool | Previously disabled; `USE_MULTITHREAD` now active | Re-enabled in Phase 9 via `GThreadPool`/`GPrivate` |
| libcurl / random.org | `LIBCURL_PROTOCOL_HTTPS` / `HAVE_LIBCURL` removed; `randomorg.c` excluded | Cross-compile libcurl for Android; restore flags and add `randomorg.c` to build |

### 5.5 Thread-Local Data Initialisation (Critical)

**Background:** When `USE_MULTITHREAD` is **enabled** (Phase 9+), gnubg's move generation and scoring macros expand as follows:

```c
#define MT_Get_aMoves()   ((ThreadLocalData *)TLSGet(td.tlsItem))->aMoves
#define MT_Get_nnState()  ((ThreadLocalData *)TLSGet(td.tlsItem))->pnnState
```

Both call `TLSGet()` which is implemented via `GPrivate` — each thread gets its own `ThreadLocalData` allocated lazily on first access. This replaces the previous single-threaded approach of a single static `gnubg_tld` struct.

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

### 5.7 Rollout Infrastructure in stubs.c

---

## 6. API Reference

### 6.1 Board Encoding

Boards cross every boundary as a flat array of 50 ints: gnubg's `TanBoard` is
two rows of 25 points (`anBoard[0][0..24]`, `anBoard[1][0..24]`), flattened as
`board[0..24]` = row 0, `board[25..49]` = row 1. The facade packs/unpacks at its
edge; neither Kotlin nor the JNI layer ever sees the engine's board type.

### 6.2 Kotlin API (Engine object)

State readers: `getMatchBoard`, `getMatchScore`, `getMatchDice`, `getMatchTurn`,
`getMatchStatus`, `getMatchLength`, `getMatchWinner`, `getMatchCubeInfo`,
`getCubeDebugState`, `getMoveRecordDice`, `getGameResult`.

Board utilities: `pipCount`, `swapBoard`, `applySubMove`, `formatMove`.

Algorithms: `getLegalMoves`, `findMove`, `evaluatePosition`, `classifyPosition`,
`cubeDecision`, `rollout`.

Game/cube commands: `commandDouble`, `commandTake`, `commandDrop`,
`engineCubeResponse(take: Boolean)`, plus new-game/match controls.

File/SGF: `loadGame`/`saveGame`, `loadMatch`/`saveMatch`,
`loadPosition`/`savePosition`, `loadSGF`/`saveSGF`.

Lifecycle: `initialise(weightsPath)`, `runCommand`.

> `cubeDecision` returns a 16-int array: indices 0–6 and 7–13 are two 7-float
> equity rows with each float bit-packed into an int; index 14 is the
> `CubeDecision` enum; index 15 is reserved. The bit-packing is a JVM transport
> detail handled in `native-lib.c`; the facade itself returns real floats.

### 6.3 Facade API (gnubg_mobile.h) — the platform-neutral surface

Mirror of the above in plain C with flat-array contracts: `gnubg_mobile_get_*`
(state), `gnubg_mobile_pip_count`/`_swap_board`/`_apply_sub_move`/`_format_move`
(board), `gnubg_mobile_get_legal_moves`/`_find_move`/`_evaluate`/`_classify`/
`_cube_decision`/`_rollout` (algorithms), `gnubg_mobile_command_double`/`_take`/
`_drop` and `gnubg_mobile_engine_cube_response` (cube), the file/SGF ops,
`gnubg_mobile_set_default_cubeinfo`, and `gnubg_mobile_start_match`/`_next_game`.

An iOS port calls exactly these.

### 6.4 Thread Safety

All facade functions acquire `gnubg_lock` internally. The Kotlin side runs
engine calls on a dedicated single-thread dispatcher (`engineThread`). Rollout
parallelism is internal to the engine (`GThreadPool` + per-thread `rngcontext`
via `GPrivate`); see §5.5 and §5.7.

### 6.5 Initialisation & Data Files

`initialise(weightsPath)` runs `EvalInitialise`, RNG init, thread-local-data
setup, and sets the default cube info (both the engine's `ci_default` and the
facade's `fac_ci_default` via `gnubg_mobile_set_default_cubeinfo`). The
`gnubg.weights` neural-net file and the four bearoff databases must be present;
package them as app assets and extract to `context.filesDir` on first launch,
passing that path to `initialise`.

---

## 7. Build & Deploy Instructions

### 7.1 Prerequisites (Fedora 44)

NDK 27.0.11718014; Meson 1.11.1 + Ninja (GLib build); CMake 4.3.0; a JDK for
Gradle; `adb` on PATH. The GLib cross-build is a one-time step (§4); its `.so`s
live in `jni-bridge/external/glib/`.

### 7.2 Build GLib for Android (one-time)

Run `build_glib_android.sh` (uses `android-arm64.cross`). Installs GLib and
sublibraries into `jni-bridge/external/glib/`. See §4 for rationale and the
build-obstacle resolutions.

### 7.3 Build the native engine + the app

The app build (Gradle) drives the CMake/NDK native build automatically:

```
cd gnubg-app
./gradlew assembleDebug          # or: ./gradlew clean assembleDebug
```

`clean` forces the native library to recompile — use it after any change to
`engine-core/`, the facade, or the JNI bridge, so the new code lands in
`libgnubg-engine.so`. The APK is written to
`gnubg-app/app/build/outputs/apk/debug/`.

### 7.4 Verification

After a native build, sanity-check the library:

```
nm -D <path>/libgnubg-engine.so | grep Java_com_clavierhaus_gnubg_Engine_ | sort
nm -D <path>/libgnubg-engine.so | grep NeuralNetEvaluateSSE   # NEON path active
```

The `Java_...` entry points should match the `external fun` set in `Engine.kt`.

### 7.5 Portability proof (test-harness)

`test-harness/` links the engine + facade + `android-app.c` with **no JNI** and
plays headlessly. Its purpose is to objectively demonstrate the de-Android
invariant: build it, then `nm` the result and confirm there are **no `Java_`
symbols** — i.e. the core plays a full game with no Android in the binary.

> **Status:** the harness exists and links the facade. Engine init and the
> board/dice paths are now fully facade-routed (Tier B/C, §8.2), so the only
> remaining direct engine references in `native-lib.c` are the
> `gnubg_on_board_changed` callback and `runCommand`/`HandleCommand`. Building
> the harness and confirming via `nm` that the engine+facade play a full game
> with no `Java_` symbols is deferred until iOS work begins; it is not on the
> near-term path. See §8.

### 7.6 Deploy to device (run.sh)

`run.sh` in the repo root is the standard deploy:

```
./run.sh                 # build (debug) + install + launch on the Pixel
./run.sh --clean         # clean build first (recompiles native .so)
./run.sh --no-build      # install the existing APK only
./run.sh --logcat        # after launch, stream this app's log (Ctrl-C stops)
./run.sh --reinstall     # uninstall first (clears data) then install
```

It locates the repo root itself, checks `adb`/device state, handles signature
mismatches (directs you to `--reinstall`), and resolves the launcher activity
from the application ID. Pure ASCII, safe to re-run.

---

## 8. Roadmap & Open Items

### 8.1 Completed

- Headless engine, GLib cross-build, JNI bridge, NEON, multi-threaded rollout,
  SGF (Phases 1–9).
- Platform-neutral facade; three-tier de-Androidification (Phase 10).
- Cube handling routed through the real engine; `ms` hand-poke retired
  (Phase 11).
- Repository build-completeness + `PROVENANCE.md` (Phase 12).
- Dead-code cleanup — Tier A (removed unused externs and `applyHumanDoubleTake`).
- Engine-symbol reduction — Tier B/C (extracted `initialise` into
  `gnubg_mobile_initialise`; routed the board and dice readers through the
  facade). `native-lib.c` now reaches the engine directly in only two named,
  intentional places: the `gnubg_on_board_changed` callback and
  `runCommand`/`HandleCommand`.

### 8.2 Completed — engine-symbol reduction (Tier B/C)

`initialise`'s engine startup (`EvalInitialise`, `InitRNG`,
`gnubg_init_tld/rollout`, `ListCreate`, `ClearMatch`, player setup) was
extracted into `gnubg_mobile_initialise(weights)`; the JNI wrapper now keeps
only the jstring round-trip and the `gnubg_initialised` guard. The board and
dice readers (`pack_board` callers, `getMatchDice`, `rollDice`) route through the
existing facade getters. Verified on device: the app initialises and plays.

`native-lib.c` now reaches the engine directly in only **two** intentional,
documented places, neither of which is JNI marshalling:

- **`gnubg_on_board_changed`** — the engine event callback (caches engine dice).
  Belongs to the host-callback / vtable work (§8.3), not to bridge marshalling.
- **`runCommand` → `HandleCommand`/`acTop`** — the arbitrary-gnubg-command path.
  This is the command channel the Settings feature will drive, so it gains its
  own facade verb (`gnubg_mobile_run_command`, with an allowlist) as part of the
  Settings phase, where its policy design belongs.

A fully zero-engine-symbol `native-lib.c` therefore awaits only those two phases;
the de-Android invariant already holds for all gameplay and init paths.

### 8.3 Pending — gameplay features

- **Beaver/raccoon UI and path.** Cube *decisions* for beavers are computed by
  the engine, but there is no beaver UI; beaver decisions currently collapse to
  take. Implementing the UI lets `gnubg_mobile_engine_cube_response` honour
  `CommandRedouble` for the beaver case.
- **iOS adapter.** `Engine.swift` mirroring `Engine.kt` over the existing
  facade; `GameViewModel` → an `ObservableObject`; SwiftUI board. No facade or
  engine changes required — that is the whole point of the architecture.
- **Callback vtable.** Replace the weak/strong `gnubg_on_*` arrangement with a
  registered host-callback table (`gnubg_mobile_set_host_callbacks`) for full
  iOS-readiness.
- **Rename `android-app.c` → `mobile-app.c`** (it is already platform-neutral;
  the name is the last Android-ism in the C layer).

### 8.4 Known Warnings

- `multithread.c`: a few incompatible-pointer warnings in paths compiled but
  not reached on Android; harmless.
- NDK toolchain CMake deprecation warnings (in the NDK's own files; not
  fixable from the project).
- `statusBarColor` deprecation in `Theme.kt` (cosmetic, pre-existing).
- Rollout variance at low trial counts: at `nTrials=144`, win-prob stddev is
  ~0.005; use 1296+ for reliable equity.

---

## 9. Provenance Summary

`engine-core/` is vendored **gnubg 1.08.003**, de-GTK'd. The pristine reference
is `upstream-source/gnubg/`. The **sole** local modification to engine source is
`engine-core/play.c`: a one-line visibility setter
(`gnubg_set_computer_decision`) exposing the static `fComputerDecision` flag to
the facade, with no change to upstream logic. All other compiled engine files
are byte-identical to upstream (verified by diff). See `PROVENANCE.md` for the
authoritative record, and **update it on any future engine edit**.

---

*End of MASTER V0.9. Per §0, keep this document current with the code — a stale
map is a bug.*
