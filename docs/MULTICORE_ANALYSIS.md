# Multi-Core Single-Move Analysis -- Design Note

Status: design spike / proposal. Not yet implemented. This note maps the exact
data dependencies in gnubg's move-scoring path, scopes a parallelization of the
single-move candidate loop, and lays out the path to offer it upstream. It is
grounded in the actual engine-core code (line references are to the vendored
GNU Backgammon 1.08.003 in engine-core/); no design choice here is speculative
about what gnubg does -- each rests on a named function.

Everything proposed keeps gnubg as the sole authority for evaluation. The work
is concurrency plumbing around gnubg's own ScoreMove, producing bit-identical
results to the serial path; it is not new backgammon logic.


## 1. What is and isn't already threaded in gnubg

gnubg is not single-threaded in general. It already parallelizes two things,
both over the existing multithread.c task pool (which this port extended for
mobile in Phase 9, commits ee38afa / 462aca7, and made TLS-safe in 99836e0):

- Rollouts -- rollout.c:1480 dispatches RolloutLoopMT across
  MT_GetNumThreads() workers.
- Batch session analysis -- analysis.c:1034 and :1275 call
  MT_WaitForTasks; the AnalyseMoveTask struct (multithread.h:55-60) is one
  task per move record in a whole-game/match analysis. Different moves in the
  game are analysed on different cores.

What gnubg does not thread is the candidate loop inside a single move
evaluation -- the for loop in ScoreMoves (eval.c:5479) that evaluates
each legal move for one dice roll. On the desktop this was an acceptable choice:
a single hint returns fast enough, and the batch case (analysing a saved match)
was the one worth parallelizing.

On mobile the felt path is the opposite one. A live tutor hint is exactly a
single-move evaluation: the player moves, and we score every candidate for that
one roll, now, while they wait. That is the loop upstream left serial. The
phone typically has 6-8 cores sitting idle during the turn. This is the gap the
mobile port is positioned to close -- and, done cleanly, to give back upstream.


## 2. The parallelization target: ScoreMoves

ScoreMoves (eval.c:5466) is the function. Each ScoreMove call in its loop is the
unit of work. The candidates are independent: ScoreMove reads the shared
board/cubeinfo/evalcontext and writes only into its own pml->amMoves[i] slot.
That independence is what makes the loop a parallelization candidate. Three
dependencies constrain how.


## 3. The three data dependencies (and how each is handled)

### D1 -- Per-thread NNState (already solved)

ScoreMove takes NNState *nnStates, gnubg's scratch state for neural-net
evaluation. MT_Get_nnState() (multithread.h:152) returns a thread-local
NNState via TLSGet. This port already fixed that TLS lazy-init for rollouts
(commit 99836e0). So each worker thread already has its own NNState by
construction -- no sharing, no locking needed on the hot path. This is the
dependency that is already clean, and the reason this is feasible rather than a
rewrite.

### D2 -- The incremental-evaluation optimization (the real tradeoff)

At nPlies == 0, ScoreMoves sets the three NNStates to NNSTATE_INCREMENTAL. This
is a serial optimization -- consecutive candidates often share board structure,
and incremental mode reuses part of the previous move's neural-net computation
instead of recomputing from scratch. It assumes the candidates are evaluated in
order, on one NNState.

Parallelizing breaks that assumption: independent workers cannot share an
incremental chain. A threaded path must run NNSTATE_NONE (full evaluation per
candidate). So the tradeoff is concrete and must be measured, not assumed:

- Serial 0-ply: cheap per-move (incremental) x N moves, one core.
- Parallel 0-ply: full per-move (no incremental) x N moves / K cores.

Whether parallel wins at 0-ply depends on N (candidate count) and K (cores) vs
the incremental speedup factor. At 2-ply and deeper the picture is clearer:
each candidate expands into 21 opponent-roll sub-evaluations, the per-candidate
cost dominates, incremental's relative benefit shrinks, and parallelism wins
comfortably. Implication for the tutor: the win is largest exactly where we
now operate -- fixed 2-ply analysis (commit 32a7c91) -- and larger still if we
ever offer 3/4-ply or a 21-roll temperature map.

Design consequence: keep the serial incremental path for nPlies == 0 and the
single-thread case; take the parallel path only when nPlies >= 1 and
MT_GetNumThreads() > 1 and the candidate count justifies dispatch overhead.
This preserves today's fast 0-ply behaviour untouched.

### D3 -- Deterministic reduction (non-negotiable for a tutor)

The loop also picks the best move, with a tie-break on rScore2 and a fixed
iteration order. This must produce bit-identical results to the serial path --
same iMoveBest, same equities, same ordering -- or the port has forked gnubg's
judgment rather than ported it, which violates THE ONE RULE and makes the tutor
a liar.

Therefore the reduction stays serial and ordered: workers only fill
pml->amMoves[i].rScore in parallel (disjoint slots, no contention); the
best-move selection runs afterward as the existing serial loop over the now-
filled scores, preserving the exact tie-break and order. fDeterministic in the
evalcontext must be honoured; determinism is verified by asserting the parallel
result equals the serial result across a position test suite before the path is
trusted.


## 4. Proposed design

A threaded ScoreMoves variant, dispatched only when it pays:

1. Guard. If MT_GetNumThreads() == 1, or nPlies == 0, or pml->cMoves < threshold,
   call the existing serial ScoreMoves unchanged. (Single-thread builds and the
   incremental 0-ply path keep today's behaviour.)
2. Dispatch. Otherwise partition the cMoves candidates across MT_GetNumThreads()
   workers using the existing mt_add_tasks / MT_WaitForTasks machinery
   (multithread.c:257, :297) -- the same pool rollouts and batch analysis
   already use. Each task scores a disjoint slice of the movelist; each worker
   uses its own MT_Get_nnState() (D1) in NNSTATE_NONE mode (D2).
3. Join + reduce. After MT_WaitForTasks, run the serial best-move selection over
   the filled scores (D3) for a deterministic result.
4. Interrupt. Honour MT_SafeGet(&fInterrupt) (already used in the eval paths at
   eval.c:5320, :5921) so a long analysis stays cancelable -- which the tutor
   vision requires for the "bounded, cancelable deeper analysis" rule.

The facade and tutor need no change: they call FindnSaveBestMoves, which calls
ScoreMoves. The parallelization is entirely inside the engine, below the facade
seam. That is the right layer -- it makes every gnubg evaluation path faster
(play, hint, tutor), not just ours.


## 5. The upstream contribution path

This is a general improvement to gnubg that happens to matter most on many-core
consumer hardware -- which is now universal, desktop included. It is a candidate
to contribute back, not a mobile-only fork.

Why upstream might want it: it removes the one remaining single-threaded hot
path (live single-move hint/eval), using gnubg's own task pool, with no change
to results. The documented criticism that gnubg is single-threaded, so you
can't do other things while it analyzes a move, is partly about the desktop UI
thread (a UI-architecture issue this port already avoids) and partly about this
compute path (which this contribution addresses).

What a clean contribution requires:

- Bit-identical results. A determinism test proving parallel == serial across a
  broad position suite. Without this it will (rightly) be rejected.
- Preserved single-thread path. MT_GetNumThreads() == 1 must behave exactly as
  today, including the 0-ply incremental optimization. The parallel path is
  purely additive.
- Honour existing settings. fDeterministic, the interrupt flag, and the
  move-filter semantics all carry through unchanged.
- FSF copyright assignment. gnubg requires contributors assign copyright to the
  Free Software Foundation. This must be in place before a patch is accepted.
- The gnubg process. Discussion on the bug-gnubg list / tracker first (design
  agreement before code), then a patch against upstream, not a drop of our
  vendored tree.

Honest risks. (1) Upstream may prefer their current architecture or judge the
maintenance cost too high; an offer is not an acceptance. (2) The shared
position cache (EvaluatePositionCache, eval.c:96) is a real concurrency concern
-- the NoLocking vs WithLocking function-pointer split (eval.c:71-90, :5129-5132)
already exists precisely because the cache is not thread-safe without locking; a
parallel ScoreMoves must use the locking variants or per-thread cache
partitions, and getting that wrong corrupts results silently. (3) Concurrency
bugs are the hardest class to get right; the determinism suite is the gate that
makes them visible.


## 6. Sequencing

1. Prerequisite (done): tutor at fixed 2-ply (commit 32a7c91) -- establishes the
   depth where parallelism pays.
2. Spike: a threaded ScoreMoves behind a guard, measured against serial on the
   Pixel 8 Pro across candidate counts and plies, with the determinism assertion
   wired in from the first line.
3. Decide: keep as a mobile-local engine divergence (recorded in PROVENANCE.md
   like the other seams) if it wins, and only then
4. Offer upstream: open the bug-gnubg discussion, arrange FSF assignment, submit
   against upstream.

Step 2 is where the real work and the real risk live. It should not begin until
there is a determinism test harness, because "faster but occasionally different"
is worse than "slower" for a tutor whose entire value is speaking gnubg's exact
judgment.


## Reference: functions named in this note

- ScoreMoves -- eval.c:5466 (the target loop)
- ScoreMove -- eval.c:5428 (per-candidate unit of work)
- FindnSaveBestMoves -- eval.c:5588 (caller; what the facade uses)
- MT_Get_nnState -- multithread.h:152 (per-thread NNState)
- mt_add_tasks / MT_WaitForTasks -- multithread.c:257 / :297 (task pool)
- MT_GetNumThreads -- multithread.h:148
- AnalyseMoveTask -- multithread.h:55-60 (existing per-move-record analysis task)
- EvaluatePositionCache -- eval.c:96 (shared cache; the locking concern)
- RolloutLoopMT -- rollout.c:1480 (existing MT usage pattern to mirror)

---

## Part 2 -- Enabling USE_MULTITHREAD for on-device rollouts (the verified path)

Added 2026-08-01. Part 1 scoped a future parallelization gnubg does not have
(the single-move candidate loop). Part 2 is the nearer, smaller prize:
rollouts are ALREADY multithreaded in gnubg's own design -- the port merely
compiles with the switch off. Every claim below was read from the vendored
sources on this date; a future session can start from here without
re-deriving anything. Realm: FOSS first, Plus inherits (maintainer ruling).

### 2.1 The two configurations, precisely

- **MT-off (the port today).** `jni-bridge/src/stubs.c` supplies the entire
  threading substrate as no-ops (Mutex_*, ManualEvent_*, TLS*, CloseThread)
  PLUS serial bodies under the `*WithLocking` names, and `gnubg_init_tld()`
  hand-builds the single ThreadLocalData. `multithread.h:181` falls back to
  `MT_GetNumThreads() == 1`. No build flag defines USE_MULTITHREAD anywhere.
- **MT-on (gnubg's design).** `eval.c:5129-5134` renames the canonical
  implementations to the `*WithLocking` symbols via macro; the plain names
  (`EvaluatePosition`, `ScoreMove`, `FindnSaveBestMoves`, ...) become
  FUNCTION POINTERS assigned at init (`multithread.c:210-216`), and workers
  get per-thread state via `MT_CreateThreadLocalData` (`multithread.c:171`).
  Consequence one: stubs.c's serial WithLocking bodies would collide at
  link. Consequence two: **MT init is mandatory** -- with the pointers
  unassigned, the first evaluation in the app dereferences null.

### 2.2 The change, minimal and reversible

1. **stubs.c** gains `#ifndef USE_MULTITHREAD` guards around exactly three
   regions: the threading substrate, the WithLocking bodies, and the tld
   hand-init. Nothing is deleted; the file becomes explicitly the MT-off
   shim, and BOTH configurations build forever (which is also what makes the
   enablement upstreamable evidence for Part 1's ambition).
   **The guard must NOT cover** the rollout docking points: `rcRollout` and
   friends live in stubs.c too and are configuration-independent.
2. **Build flag**: `-DUSE_MULTITHREAD` (and `MAX_NUMTHREADS`) ride the
   existing `CMAKE_C_FLAGS` path in `build_native_android.sh` (:111/:120).
   Reproducible; the owed F-Droid 0.22.8 reproducibility re-proof covers it.
3. **Facade init** (`gnubg_mobile.c:1363`): under MT, `MT_InitThreads()` +
   `MT_SetNumThreads(N)` take the slot `gnubg_init_tld()` holds today.
   N = device performance cores, conservatively capped (thermals), shown in
   gnubg's own term ("Threads"), never a user knob in v1.

### 2.3 Why rollouts then need no new threading work

`rollout.c:1480`: `RolloutGeneral` itself dispatches
`mt_add_tasks(MT_GetNumThreads(), RolloutLoopMT, ...)` and blocks in
`MT_WaitForTasks(UpdateProgress, 2000, ...)` -- the progress callback runs
ON THE CALLING THREAD every 2 s. That is the app's proven poll-snapshot
pattern with no adaptation: the facade's rollout call sits on the engine
thread, fills a snapshot the UI polls, and cancellation is `fInterrupt`
(read `MT_SafeGet`-safe, cross-thread by design).

### 2.4 Determinism, the verify-line's foundation

`RolloutDice(iTurn, iGame, ...)` (`rollout.c:223`) derives each trial's dice
from the trial and turn indices under a once-seeded quasi-random permutation
(`QuasiRandomSeed`, `rollout.c:190`). Whichever worker plays game 517 rolls
game 517's dice; aggregates are therefore identical at any thread count for
a given seed -- which is what makes "reproduce this rollout in desktop
gnubg: same seed, same numbers" true by construction. Gate B confirms it
empirically anyway.

### 2.5 The reads happened -- and rewrote the init story (2026-08-01)

The three planned reads were done, plus the follow-ups they forced. The
findings, each load-bearing:

1. **`MT_InitThreads` is a phantom.** Declared (`multithread.h:112`),
   defined nowhere in the vendored engine, called nowhere: on desktop it
   lives in gnubg's main program, which this port replaced with the facade.
   The real init is `MT_SetNumThreads(N)` (`multithread.c:194`): it creates
   the glib worker threads, gives each its TLD, and assigns the function
   pointers -- including a third family, `*NoLocking`, used when N == 1
   (which hands Gate A its ideal configuration: MT-on at one thread is the
   nearest cousin of today's serial build).
2. **The engine defines NO threading primitives at all.** The complete
   definition inventory of `multithread.c` shows it USES Mutex_*,
   ManualEvent_*, TLS*, `CloseThread`, `MT_CreateThreadLocalData`, and the
   `td` object -- and defines none of them. `ThreadData td` itself lives in
   OUR `stubs.c` (:47). Consequence: stubs.c is not a shim over an
   engine-provided layer; **it is the port's only provider of that layer**,
   and under MT-on its no-ops must become real glib-backed implementations
   (a new `mt_glue.c`), while the config-independent globals -- `td`,
   `rcRollout`, and friends -- stay unguarded in both configurations.
3. **glib threading is core glib** (`g_thread_try_new` with a pre-2.32
   fallback branch already in the source); the Android glib this port
   builds carries it.
4. **The facade owns the whole init sequence** under MT-on: primitive-layer
   init (queue mutex, activity event, TLS key), the CALLING thread's own
   TLD (the engine thread runs evaluations directly and needs its aMoves),
   then `MT_SetNumThreads(N)`.

5. **The header's type layer is half-migrated** (found by the first
   compile probe, 2026-08-01): `ThreadData` declares raw `pthread_mutex_t`
   queue/multi locks and `ManualEvent`'s NEWER-glib branch is
   `pthread_cond_t` (inverted from upstream logic), while `Mutex` still
   typedefs to `GMutex` -- the port trimmed the primitive implementations
   out of multithread.c and drifted the header toward pthread to keep the
   MT-off stubs compiling, then stopped halfway. The port owns this layer
   completely. **Design consequence:** `mt_glue.c` finishes the migration
   in ONE coherent direction -- pthread throughout (the direction already
   begun; native to bionic; worker creation moves from `g_thread_try_new`
   to `pthread_create`, removing the glib-thread dependency from the MT
   story entirely). The header's `Mutex`/`TLSItem` typedefs move with it;
   the compiler re-probes after, and only a clean ledger proceeds.

**Method ruling for the next step:** no further archaeology -- the compiler
builds the symbol ledger. A host-side link probe of `multithread.c` +
`stubs.c` under `-DUSE_MULTITHREAD` (host glib, same as the C syntax gate)
enumerates every missing and duplicate symbol mechanically; that list IS
the specification for `mt_glue.c` and for the exact `#ifndef` guard
boundaries in stubs.c. Flawless by compiler, not by grep.

### 2.6 The gates, each blocking the next

- **Gate A -- behavior unchanged:** one evaluation and one full match
  analysis, MT-on vs MT-off, bit-identical numbers (Part 1's own standard).
- **Gate B -- rollout determinism:** same seed, threads 1 vs 4, identical
  outputs, trial for trial.
- Only then: the rollout facade (new functions parallel to the validated
  ones, driving `ScoreMoveRollout` over gnubg's own candidate list with the
  `rcRollout` docking points), and the Analyse-screen UI (live candidate
  list, seed displayed, foreground-service continuation).

### 2.7 What the feature inherits for free

The regulation preset is already codified in the tree: `rcRollout` in
stubs.c (cubeful, variance reduction, rotation, 1296 trials, Mersenne
dice, 144-game minimum) under the comment "docking points ... set by the
Kotlin UI layer" -- written for exactly this feature, waiting.

### 2.9 The discovered engine, and the pivot (2026-08-01, second session)

Preparing mt_glue.c, the full read of stubs.c found what no grep had
surfaced: **the port already contains a parallel rollout engine**
(stubs.c, "Rollout Infrastructure"), initialized at startup
(gnubg_init_rollout, called from the facade's init since day one) and
never yet called. Inventory:

- GThreadPool sized to all online cores; one task per trial.
- Per-thread lazy TLS -- and this is ALSO how all current evaluation
  works: the real TLSGet lives here, building each thread's
  ThreadLocalData (aMoves + three NNStates) on first touch.
- Per-thread RNG contexts (CopyRNGContext of rngctxRollout).
- Each trial runs **gnubg's own BasicCubefulRolloutNoLocking** -- the
  genuine trial core; the custom code is scheduling and mean/SD
  accumulation only. Part-1-class plumbing around gnubg's logic.
- Barrier wait, cache-aligned per-trial results, scatter-gather merge.

**The pivot:** rollouts do NOT need USE_MULTITHREAD. Sections 2.1-2.3
stay as the documented path for Part 1's candidate-loop future, but the
rollout feature rides the engine that exists. The nightmare sections
(mt_glue.c, the half-migrated header, the stubs guards, the native-wide
risk) are SHELVED, not deleted -- they are the map for a different
mountain.

**Fidelity findings the pivot must fix first (sole-authority guards):**

1. **Seed scheme diverges from desktop gnubg.** The pool reseeds the
   quasi-random permutation PER TRIAL (nSeed + task_index); gnubg seeds
   ONE permutation array and indexes dice by (iTurn, iGame)
   (rollout.c:223). Same seed would not reproduce desktop numbers --
   and same-seed desktop reproduction is the feature's headline. Fix:
   one shared read-only QuasiRandomSeed(nSeed) array across workers,
   trial index as gnubg's iGame. Beyond the permutation depth gnubg
   falls to sequential RNG draws; Gate B decides empirically what the
   verify-line may honestly claim there.
2. **Hand-rolled mean/SD.** Near-certainly gnubg's basic-output math;
   Gate B's field-for-field desktop comparison covers it, because
   near-certainly is not the standard.

**Additions the feature needs (port-owned, zero gnubg divergence):**
a mutex-guarded progress snapshot over the filling results array
(running equity +/- CI for the live candidate list) with cancel; and a
per-candidate driver (the existing entry rolls one position; the
feature rolls each candidate's post-move board).

**Revised gates:** Gate A is unnecessary (nothing about the existing
engine changes -- no flag, no header, no stubs edits). Gate B becomes
the whole test: this pool vs desktop gnubg, same position, same seed,
same trials -- dice-identical through the permutation depth,
statistics field-for-field.
