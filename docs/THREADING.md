# Threading

> **Read this before any conversation about "making it faster with threads."**
> Every claim below was checked against the source in the session that wrote it;
> where a file is named, it was read, not recalled. Do the same before acting on
> it — the code may have moved. If you cannot verify a claim here, treat it as
> unverified and say so, rather than building on it.

## The current reality (verified)

**The app is single-threaded for play.** `multithread.c` compiles, but its real
machinery is behind `#if defined(USE_MULTITHREAD)`, and that macro is **not**
defined in this build — it is in neither `jni-bridge/CMakeLists.txt`
(`target_compile_definitions`) nor `jni-bridge/config.h`. So the `#else` branch of
`multithread.c` is what runs: `MT_WaitForTasks` simply loops the task list and
calls each `task->fun` inline, on the calling thread.

**The per-move evaluation is already NEON-vectorised.** The neural-net inner loop
goes through `engine-core/lib/simd.h`, whose guard is
`#if defined(USE_SIMD_INSTRUCTIONS)` → `#elif defined(HAVE_NEON)` →
`float_vector = float32x4_t`. The build passes both `-DUSE_SIMD_INSTRUCTIONS` and
`-DHAVE_NEON`, so on arm64 the evaluation runs four lanes wide. (`config.h` carries
a stale `#undef USE_SIMD_INSTRUCTIONS` for the x86 path; the `-D` flags are what
the ARM build compiles with.) This is the single largest per-position speedup
available, and it is already in.

**The named strength levels already prune.** The 2-ply and 3-ply presets
(`aecSettings`, `eval.c:187`) set `fUsePrune = TRUE` and use the Normal move
filter, so gnubg is searching a pruned tree with pruning nets at interior nodes,
not brute-forcing every move to full depth.

## Why a single move cannot be sped up by threading

gnubg parallelises **across independent positions**, not **within one position's
search**. Confirmed by where the task calls live: `mt_add_tasks` / `MT_AddTask`
appear only in `rollout.c` (rollout trials) and `analysis.c` (per-move analysis of
a saved game). `FindnSaveBestMoves` and the ply search in `eval.c` add no tasks;
they call `MT_Get_aMoves()` and `MT_Get_nnState()` only for **thread-local scratch
buffers**, then evaluate serially.

So a single live 3-ply decision — the 7–9 second Grandmaster move — is one serial
tree search. Enabling `USE_MULTITHREAD` would not touch it, because gnubg never
decomposed that search into tasks. Capturing its parallelism (the top-level
candidate moves are independent subtrees) would mean threading work gnubg does
**not** thread for you, on the interactive path. That is the highest-risk,
lowest-obligation place to add concurrency, and it is off the table until there is
a concrete, measured reason and a safe design.

**Therefore the honest position on move latency:** 7–9 seconds at 3-ply on one
phone core is the fundamental cost. Pruning, pruning nets, and NEON are all already
applied. Any app running the gnubg engine — or an equally strong net — at the same
depth on the same silicon pays the same price. It is not a defect in this port; it
is the arithmetic of the search. We live with it, and it is fine, because it is
the shared constraint.

## Where threading IS the right tool (later, and carefully)

Match analysis (feature [3]'s per-move blunder marks) and rollouts (the wired but
unused `Engine.rollout`) are **embarrassingly parallel** and gnubg **already**
decomposes them into MT tasks. There, enabling `USE_MULTITHREAD` is the correct
move and near-linear speedup is realistic. It is also off the interactive path, so
a slow or contended pool degrades a progress bar, not a move.

When that day comes, the non-negotiables — each a way this could break something:

1. **`mtsupport.c` and `evallock.c` must be un-ignored.** They are gnubg's task
   support and evaluation locking, currently in `.gitignore`. `USE_MULTITHREAD`
   pulls them into the build; a fresh clone without them fails.
   `tools/check_buildable_clone.sh` will force this the moment they are compiled.
2. **The facade lock and the thread pool must not deadlock.** The JNI facade
   serialises engine entry under `gnubg_lock`. `gnubg_lock` must **not** be held
   across an `MT_WaitForTasks` that the worker threads also need to enter, or the
   pool serialises on the very lock meant to protect a single call — or deadlocks.
   The pool belongs *inside* one facade call, released to the workers, rejoined
   before return.
3. **Thread count is capped for a phone**, not set to the core count blindly:
   thermal throttling and battery make "all nine cores" a poor default. Measure.
4. **It ships behind analysis/rollout, never behind a live move**, until and
   unless a separate, measured decision says otherwise.

## The rule for this document

Whenever threading is raised — "can we thread the move," "why not use all the
cores," "enable multithreading" — return here and re-verify against the source
before answering:

- Is `USE_MULTITHREAD` still undefined? (`grep` the CMake defines and config.h.)
- Does the operation in question decompose into MT tasks, or is it one serial
  search? (`grep mt_add_tasks / MT_AddTask` in the relevant file.)
- Is it on the interactive path or behind a progress bar?

If the answer is "single serial search on the interactive path," the answer to
"thread it" is no, and the reason is above. Do not re-litigate it from optimism;
re-litigate it only from a new measurement.
