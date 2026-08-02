/*
 * stubs.c -- Android JNI stub layer for GNU Backgammon engine
 *
 * Provides definitions for global variables and functions belonging to
 * the GTK/UI/desktop/threading layer. All signatures match the header
 * declarations exactly. No symbol defined in a compiled engine-core
 * source file is redefined here.
 */

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <math.h>
#include "dice.h"

#include "backgammon.h"
#include "eval.h"
#include "multithread.h"
#include "rollout.h"
#include "positionid.h"
#include "lib/isaac.h"

/* -- Global state variables ----------------------------------------------- */

/* Defined in backgammon.h as extern matchstate ms -- provide the storage */
matchstate ms;
/* Defined in backgammon.h as extern player ap[2] */
player ap[2];

int  fAnalysisRunning  = 0;
int  fJacoby           = 0;
int  fPostCrawford     = 0;
int  nMatchTo          = 0;
int  fTurn             = 0;
int  fMove             = 0;
int  fCrawford         = 0;
int  fAutoSave         = 0;
int  nAutoSaveTime     = 0;
int  fNextTurn         = 0;
int  fComputing        = 0;

char *szCurrentFileName = NULL;
int positions[2][30][3] = {0};
const char *szHomeDirectory = NULL;

/* -- ThreadData td --------------------------------------------------------- */
ThreadData td;

/* -- msBoard -- returns current board position ------------------------------ */
ConstTanBoard msBoard(void) {
    return (ConstTanBoard)ms.anBoard;
}

/* -- save_autosave --------------------------------------------------------- */
gboolean save_autosave(gpointer unused) {
    return FALSE;
}

/* -- Threading primitives -------------------------------------------------- */
void CloseThread(void *unused)                    {}
void Mutex_Lock(Mutex *mutex)                     {}
void Mutex_Release(Mutex *mutex)                  {}
void ResetManualEvent(ManualEvent ME)             {}
void SetManualEvent(ManualEvent ME)               {}
void WaitForManualEvent(ManualEvent ME)           {}
void InitManualEvent(ManualEvent *pME)            {}
void FreeManualEvent(ManualEvent ME)              {}
void InitMutex(Mutex *pMutex)                     {}
void TLSSetValue(TLSItem pItem, size_t val)       {}

ThreadLocalData *MT_CreateThreadLocalData(int id) { return NULL; }

/* -- EXP_LOCK_FUN function pointer variables ------------------------------- 
 * EXP_LOCK_FUN declares: typedef ret (*f_name)(...); extern f_name name;
 * We provide the storage for the function pointer and point it at the
 * NoLocking variant which is the real implementation in eval.c
 */

/* -- WithLocking variants -- single-threaded: just call NoLocking ----------- */
int EvaluatePositionWithLocking(NNState *nnStates, const TanBoard anBoard,
        float arOutput[], cubeinfo * const pci, const evalcontext *pec) {
    return EvaluatePositionNoLocking(nnStates, anBoard, arOutput, pci, pec);
}

int FindBestMoveWithLocking(int anMove[8], int nDice0, int nDice1,
        TanBoard anBoard, const cubeinfo *pci, evalcontext *pec,
        movefilter aamf[MAX_FILTER_PLIES][MAX_FILTER_PLIES]) {
    return FindBestMoveNoLocking(anMove, nDice0, nDice1, anBoard, pci, pec, aamf);
}

int FindnSaveBestMovesWithLocking(movelist *pml, int nDice0, int nDice1,
        const TanBoard anBoard, positionkey *keyMove, int fAnalyse,
        const float rThr, const cubeinfo *pci, const evalcontext *pec,
        movefilter aamf[MAX_FILTER_PLIES][MAX_FILTER_PLIES]) {
    return FindnSaveBestMovesNoLocking(pml, nDice0, nDice1, anBoard, keyMove,
                                       fAnalyse, rThr, pci, pec, aamf);
}

int GeneralCubeDecisionEWithLocking(float aarOutput[2][NUM_ROLLOUT_OUTPUTS],
        const TanBoard anBoard, cubeinfo * const pci,
        const evalcontext *pec, const evalsetup *pes) {
    return GeneralCubeDecisionENoLocking(aarOutput, anBoard, pci, pec, pes);
}

int GeneralEvaluationEWithLocking(float arOutput[NUM_ROLLOUT_OUTPUTS],
        const TanBoard anBoard, cubeinfo * const pci,
        const evalcontext *pec) {
    return GeneralEvaluationENoLocking(arOutput, anBoard, pci, pec);
}

int ScoreMoveWithLocking(NNState *nnStates, move *pm, const cubeinfo *pci,
        const evalcontext *pec, int nPlies) {
    return ScoreMoveNoLocking(nnStates, pm, pci, pec, nPlies);
}

int BasicCubefulRolloutWithLocking(unsigned int aanBoard[][2][25],
        float aarOutput[][NUM_ROLLOUT_OUTPUTS], int iTurn, int iGame,
        const cubeinfo aci[], int afCubeDecTop[], unsigned int cci,
        rolloutcontext *prc, rolloutstat aarsStatistics[][2],
        int nBasisCube, perArray *dicePerms, rngcontext *rngctxRollout,
        FILE *logfp) { return -1; }

/* -- UI / progress stubs --------------------------------------------------- */
void ProcessEvents(void)                 {}
void progress(void)                      {}
void ProgressValue(int val)              {}
void ProgressStart(const char *sz)       {}
void ProgressEnd(void)                   {}
void ProgressValueAdd(int val)           {}

/* -- Misc engine callbacks ------------------------------------------------- */
/* LogCube: provided by set.c */
int GetManualDice(unsigned int anDice[2]) { return 0; }
/* SetRNG: provided by set.c */
/* ChangeGame: provided by play.c */
double get_time(void) { return 0.0; }
/* FormatMove: provided by format.c */
/* get_current_moverecord: provided by play.c */

/* -- randomorg -- network dice unavailable on Android ----------------------- */
void RandomorgDice(void)                               {}
int  NetworkDice(unsigned int *pdice, int ndice)       { return -1; }

/* -- Thread-local data initialisation ---------------------------------------
 * MT_Get_aMoves() expands to td.tld->aMoves when USE_MULTITHREAD is off.
 * td.tld must point to a valid ThreadLocalData with an allocated aMoves buffer
 * before any move generation occurs (i.e. before any 1-ply evaluation).
 * Called once from Engine.initialise() via gnubg_init_tld().
 */

/* -- Rollout global state ----------------------------------------------------
 * Docking points between the engine and the UI layer.
 * On desktop gnubg these are set by the GTK preferences dialog.
 * On Android they will be set by the Kotlin UI layer.
 */

rolloutcontext rcRollout = {
    .fCubeful       = 1,
    .fVarRedn       = 1,
    .fInitial       = 0,
    .fRotate        = 1,
    .fTruncBearoff2 = 1,
    .fTruncBearoffOS= 1,
    .fLateEvals     = 0,
    .fDoTruncate    = 0,
    .fStopOnSTD     = 0,
    .fStopOnJsd     = 0,
    .fStopMoveOnJsd = 0,
    .nTruncate      = 0,
    .nTrials        = 1296,
    .nLate          = 0,
    .rngRollout     = RNG_MERSENNE,
    .nSeed          = 0,
    .nMinimumGames  = 144,
    .rStdLimit      = 0.01f,
    .nMinimumJsdGames = 144,
    .rJsdLimit      = 2.33f,
    .nGamesDone     = 0,
    .rStoppedOnJSD  = 0.0f,
    .nSkip          = 0,
};

rngcontext *rngctxRollout = NULL;

int fAutoCrawford    = 1;
int fAutoSaveRollout = 0;
int fShowProgress    = 0;
/* fOutputMWC: provided by android-app.c */
/* fOutputWinPC: provided by android-app.c */
/* fOutputMatchPC: provided by android-app.c */

/* -- QuasiRandomSeed -- copied from rollout.c (static there) -----------------
 * Uses irandinit/irand from lib/isaac.c (already in build).
 * Must be defined before gnubg_rollout().
 */
void QuasiRandomSeed(perArray * pArray, int n) {
    int i, j, r;
    unsigned char k, t;
    randctx rc;
    if (pArray->nPermutationSeed == n)
        return;
    for (i = 0; i < RANDSIZ; i++)
        rc.randrsl[i] = (ub4) n;
    irandinit(&rc, TRUE);
    for (i = 0; i < 6; i++)
        for (j = i; j < QRLEN; j++) {
            for (k = 0; k < 36; k++)
                pArray->aaanPermutation[i][j][k] = k;
            for (k = 0; k < 35; k++) {
                r = irand(&rc) % (36 - k);
                t = pArray->aaanPermutation[i][j][k + r];
                pArray->aaanPermutation[i][j][k + r] = pArray->aaanPermutation[i][j][k];
                pArray->aaanPermutation[i][j][k] = t;
            }
        }
    pArray->nPermutationSeed = n;
}

/* -- gnubg_init_rollout ------------------------------------------------------
 * Allocates and seeds the rollout RNG context.
 * Called after EvalInitialise().
 */

/* -- gnubg_rollout -----------------------------------------------------------
 * Synchronous rollout bypassing MT task queue.
 * Calls BasicCubefulRolloutNoLocking directly for nTrials games.
 */

/* MT_WaitForTasks: provided by multithread.c */


#include <unistd.h>

/* -- Thread-Local Storage (TLS) Allocator --------------------------------- */

#include <unistd.h>

/* -- Thread-Local Storage (TLS) Allocator --------------------------------- */
static void gnubg_tls_destructor(gpointer data) {
    ThreadLocalData *tld = (ThreadLocalData *)data;
    if (tld) {
        for (int i = 0; i < 3; i++) {
            g_free(tld->pnnState[i].savedBase);
            g_free(tld->pnnState[i].savedIBase);
        }
        g_free(tld->pnnState);
        g_free(tld->aMoves);
        g_free(tld);
    }
}

GPrivate gnubg_tls_key = G_PRIVATE_INIT(gnubg_tls_destructor);
static void free_rng_ctx(gpointer p) { if (p) free_rngctx((rngcontext *)p); }
GPrivate gnubg_rng_key = G_PRIVATE_INIT(free_rng_ctx);

void *TLSGet(void *item) {
    ThreadLocalData *tld = g_private_get(&gnubg_tls_key);
    if (tld == NULL) {
        tld = g_malloc0(sizeof(ThreadLocalData));
        tld->aMoves = g_malloc0(sizeof(move) * MAX_MOVES);
        tld->pnnState = g_malloc0(sizeof(NNState) * 3);
        
        for (int i = 0; i < 3; i++) {
            tld->pnnState[i].state = NNSTATE_NONE;
            tld->pnnState[i].savedBase = g_malloc(nnContact.cHidden * sizeof(float));
            tld->pnnState[i].savedIBase = g_malloc(nnContact.cInput * sizeof(float));
        }
        g_private_set(&gnubg_tls_key, tld);
        td.tld = tld;
    }
    return tld;
}

void gnubg_init_tld(void) {
    /* With GPrivate, initialization is lazy. No global setup needed. */
}

/* -- Rollout Infrastructure -------------------------------------------------
 * Scheduling is the port's; every number-producing line is gnubg's
 * (maintainer ruling 2026-08-01: "this is a gnubg port, not some
 * improvement ... we deliver what we promise").
 *
 * - Dice: gnubg's scheme exactly. ONE quasi-random permutation array,
 *   seeded once per rollout as gnubg seeds it (rollout.c:1159-1160:
 *   gated on prc->fRotate, QuasiRandomSeed(&perms, (int)prc->nSeed)),
 *   shared read-only across workers; the trial index rides as iGame, so
 *   trial i draws the dice desktop gnubg's trial i draws
 *   (RolloutDice contract, rollout.c:223).
 * - Aggregation: gnubg's incremental update ported VERBATIM from
 *   rollout.c:1196-1219 -- running sum, rMuNew, the variance decay
 *   recursion, the [0,1] clamp on probability outputs (j < OUTPUT_EQUITY),
 *   sigma = sqrtf(variance / n) at every step -- applied in trial-index
 *   order, so the arithmetic and its rounding follow serial gnubg.
 * - Progress and cancel are ADDITIONS, not alterations: a per-trial
 *   completion map lets a poll run the same verbatim update over whatever
 *   has finished; cancel makes unstarted trials no-ops, and a partial
 *   result is labeled by its completed count, never passed off as full.
 * - Per-thread RNG contexts remain for draws beyond the permutation depth
 *   (documented in docs/MULTICORE_ANALYSIS.md 2.9; Gate B against desktop
 *   gnubg adjudicates what the verify-line may claim there).
 *
 * One rollout runs at a time (the facade's engine-thread discipline).
 */
static GThreadPool *rollout_pool = NULL;

typedef struct {
    float arOutput[NUM_ROLLOUT_OUTPUTS];
} __attribute__((aligned(64))) RolloutResult;

typedef struct {
    GMutex mutex;
    GCond cond;
    gint tasks_remaining;
    RolloutResult *results;
    unsigned char *completed;          /* per-trial: 1 = results[i] valid */
    const cubeinfo *pci;
    rolloutcontext *prc;
    const unsigned int (*anBoard)[25]; /* Matches the decayed pointer type */
    perArray *dicePerms;               /* the ONE shared array (gnubg's scheme) */
    int fInvert;                       /* move rollouts: invert EACH trial's
                                        * output (InvertEvaluationR) before
                                        * accumulation -- gnubg's own order,
                                        * rollout.c:1192-1194 */
} RolloutBarrier;

/* Live-progress registration: a single rollout at a time. */
static GMutex rollout_live_mutex;
static RolloutBarrier *rollout_live = NULL;
static gint rollout_cancel_flag = 0;
static gint rollout_done_count = 0;

/* gnubg's aggregation, rollout.c:1196-1219 ported verbatim (one
 * alternative; altGameCount -> n; the fInvert branch does not apply to a
 * single-position rollout -- the caller owns perspective). Walks trials in
 * index order over the completion map; returns how many were counted. */
static unsigned int
rollout_accumulate(const RolloutResult *results, const unsigned char *completed,
                   unsigned int nTrials,
                   float arMu[NUM_ROLLOUT_OUTPUTS], float arSigma[NUM_ROLLOUT_OUTPUTS])
{
    float arResult[NUM_ROLLOUT_OUTPUTS];
    float arVariance[NUM_ROLLOUT_OUTPUTS];
    unsigned int n = 0;

    for (int j = 0; j < NUM_ROLLOUT_OUTPUTS; j++) {
        arResult[j] = arVariance[j] = 0.0f;
        arMu[j] = arSigma[j] = 0.0f;
    }

    for (unsigned int i = 0; i < nTrials; i++) {
        const float *aar;

        if (!__atomic_load_n(&completed[i], __ATOMIC_ACQUIRE))
            continue;
        aar = results[i].arOutput;
        n++;

        /* apply the results */
        for (int j = 0; j < NUM_ROLLOUT_OUTPUTS; j++) {
            float rMuNew;

            arResult[j] += aar[j];
            rMuNew = arResult[j] / (float) n;

            if (n > 1) {        /* for i == 0 aarVariance is not defined */
                float rDelta = rMuNew - arMu[j];

                arVariance[j] =
                    arVariance[j] * (1.0f - 1.0f / (float) (n - 1)) +
                    (float) (n) * rDelta * rDelta;
            }

            arMu[j] = rMuNew;

            if (j < OUTPUT_EQUITY) {
                if (arMu[j] < 0.0f)
                    arMu[j] = 0.0f;
                else if (arMu[j] > 1.0f)
                    arMu[j] = 1.0f;
            }

            arSigma[j] = sqrtf(arVariance[j] / (float) n);
        }                       /* for (j = 0; j < NUM_ROLLOUT_OUTPUTS; j++ ) */
    }
    return n;
}

static void rollout_worker_func(gpointer data, gpointer user_data) {
    /* Indices ride as i+1: GLib's queue rejects NULL data, and trial 0
     * through GINT_TO_POINTER IS NULL -- the harness caught task 0 never
     * being queued (dormant since the engine's first write). */
    int task_index = GPOINTER_TO_INT(data) - 1;
    /* The pool is created ONCE at init with NULL user_data; the live
     * barrier is published (mutex) before any task is pushed and cleared
     * only after the last completes -- one rollout at a time. Reading it
     * here fixes a dormant NULL-user_data crash the harness caught on the
     * engine's first-ever invocation. */
    RolloutBarrier *barrier;
    (void) user_data;
    g_mutex_lock(&rollout_live_mutex);
    barrier = rollout_live;
    g_mutex_unlock(&rollout_live_mutex);
    if (!barrier) return;

    if (!g_atomic_int_get(&rollout_cancel_flag)) {
        /* Per-thread RNG context -- copied lazily on first use */
        rngcontext *local_rng = g_private_get(&gnubg_rng_key);
        if (!local_rng) {
            local_rng = CopyRNGContext(rngctxRollout);
            g_private_set(&gnubg_rng_key, local_rng);
        }

        /* Board copy for this trial */
        unsigned int aanBoard[1][2][25];
        memcpy(aanBoard[0], barrier->anBoard, 25 * 2 * sizeof(unsigned int));

        /* Per-trial output slot */
        float aarOutput[1][NUM_ROLLOUT_OUTPUTS];
        memset(aarOutput[0], 0, sizeof(aarOutput[0]));

        cubeinfo aci[1];
        memcpy(&aci[0], barrier->pci, sizeof(cubeinfo));
        int afCubeDecTop[1] = {0};

        rolloutstat aarsStats[1][2];
        memset(aarsStats, 0, sizeof(aarsStats));

        /* Trial index as gnubg's iGame; dice from the ONE shared array. */
        BasicCubefulRolloutNoLocking(aanBoard, aarOutput,
                                      0, task_index,
                                      aci, afCubeDecTop, 1,
                                      barrier->prc, aarsStats, 0,
                                      barrier->dicePerms, local_rng, NULL);

        if (barrier->fInvert)
            InvertEvaluationR(aarOutput[0], barrier->pci);

        memcpy(barrier->results[task_index].arOutput, aarOutput[0],
               NUM_ROLLOUT_OUTPUTS * sizeof(float));
        __atomic_store_n(&barrier->completed[task_index], 1, __ATOMIC_RELEASE);
        g_atomic_int_inc(&rollout_done_count);
    }

    g_mutex_lock(&barrier->mutex);
    barrier->tasks_remaining--;
    if (barrier->tasks_remaining == 0)
        g_cond_signal(&barrier->cond);
    g_mutex_unlock(&barrier->mutex);
}


void gnubg_init_rollout(void) {
    if (!rngctxRollout && rngctxCurrent)
        rngctxRollout = CopyRNGContext(rngctxCurrent);

    if (!rollout_pool) {
        gint max_threads = sysconf(_SC_NPROCESSORS_ONLN);
        if (max_threads < 1) max_threads = 4;
        rollout_pool = g_thread_pool_new(rollout_worker_func, NULL, max_threads, FALSE, NULL);
    }
}

/* Roll out one position under prc. Returns the number of trials actually
 * completed (== prc->nTrials unless cancelled), or -1 on allocation
 * failure. arOutput/arStdDev carry gnubg's aggregation over the completed
 * trials. */
static int gnubg_rollout_internal(const TanBoard anBoard, float arOutput[NUM_ROLLOUT_OUTPUTS], float arStdDev[NUM_ROLLOUT_OUTPUTS], const cubeinfo *pci, rolloutcontext *prc, int fInvert) {
    static perArray sharedPerms;   /* one rollout at a time; engine-thread owned */
    unsigned int completed_trials;

    if (!rollout_pool) gnubg_init_rollout();

    /* The dice generator, set up as gnubg sets it up (rollout.c:1159-1160). */
    sharedPerms.nPermutationSeed = -1;
    if (prc->fRotate)
        QuasiRandomSeed(&sharedPerms, (int) prc->nSeed);

    RolloutBarrier barrier;
    g_mutex_init(&barrier.mutex);
    g_cond_init(&barrier.cond);
    barrier.tasks_remaining = prc->nTrials;
    barrier.pci = pci;
    barrier.prc = prc;
    barrier.anBoard = anBoard; /* Decays cleanly into const unsigned int (*)[25] */
    barrier.dicePerms = &sharedPerms;
    barrier.fInvert = fInvert;

    /* Allocate cache-aligned results array + completion map */
    if (posix_memalign((void**)&barrier.results, 64, prc->nTrials * sizeof(RolloutResult)) != 0) {
        return -1;
    }
    memset(barrier.results, 0, prc->nTrials * sizeof(RolloutResult));
    barrier.completed = g_malloc0(prc->nTrials);

    g_atomic_int_set(&rollout_cancel_flag, 0);
    g_atomic_int_set(&rollout_done_count, 0);
    g_mutex_lock(&rollout_live_mutex);
    rollout_live = &barrier;
    g_mutex_unlock(&rollout_live_mutex);

    /* Dispatch tasks */
    for (unsigned int i = 0; i < prc->nTrials; i++) {
        g_thread_pool_push(rollout_pool, GINT_TO_POINTER((int) i + 1), NULL);
    }

    /* Wait for completion */
    g_mutex_lock(&barrier.mutex);
    while (barrier.tasks_remaining > 0) {
        g_cond_wait(&barrier.cond, &barrier.mutex);
    }
    g_mutex_unlock(&barrier.mutex);

    g_mutex_lock(&rollout_live_mutex);
    rollout_live = NULL;
    g_mutex_unlock(&rollout_live_mutex);

    /* gnubg's aggregation over the completed trials, in trial order. */
    completed_trials = rollout_accumulate(barrier.results, barrier.completed,
                                          prc->nTrials, arOutput, arStdDev);

    free(barrier.results);
    g_free(barrier.completed);
    g_mutex_clear(&barrier.mutex);
    g_cond_clear(&barrier.cond);

    return (int) completed_trials;
}

int gnubg_rollout(const TanBoard anBoard, float arOutput[NUM_ROLLOUT_OUTPUTS], float arStdDev[NUM_ROLLOUT_OUTPUTS], const cubeinfo *pci, rolloutcontext *prc) {
    return gnubg_rollout_internal(anBoard, arOutput, arStdDev, pci, prc, 0);
}

/* Roll out ONE candidate move -- the per-move setup is ScoreMoveRollout's,
 * mirrored line for line (rollout.c:1819+): position from the move's key,
 * SwapSides, fMove flipped, rolled with per-trial inversion; then gnubg's
 * own score mapping (rScore = mwc2eq of cubeful equity in match play,
 * against the ORIGINAL-orientation cubeinfo; rScore2 = cubeless equity).
 * Results land in the move struct exactly where gnubg puts them
 * (arEvalMove / arEvalStdDev / rScore / rScore2). Returns completed
 * trials, or -1. */
int gnubg_rollout_move(move *pm, const cubeinfo *pciOriginal, rolloutcontext *prc) {
    TanBoard anBoard;
    cubeinfo ci;
    int nDone;

    PositionFromKey(anBoard, &pm->key);
    SwapSides(anBoard);

    memcpy(&ci, pciOriginal, sizeof(cubeinfo));
    /* swap fMove in cubeinfo */
    ci.fMove = !ci.fMove;

    nDone = gnubg_rollout_internal((ConstTanBoard) anBoard,
                                   pm->arEvalMove, pm->arEvalStdDev,
                                   &ci, prc, 1);
    if (nDone <= 0)
        return nDone;

    /* Score for move:
     * rScore is the primary score (cubeful/cubeless)
     * rScore2 is the secondary score (cubeless) */
    if (prc->fCubeful) {
        if (pciOriginal->nMatchTo)
            pm->rScore = mwc2eq(pm->arEvalMove[OUTPUT_CUBEFUL_EQUITY], pciOriginal);
        else
            pm->rScore = pm->arEvalMove[OUTPUT_CUBEFUL_EQUITY];
    } else
        pm->rScore = pm->arEvalMove[OUTPUT_EQUITY];

    pm->rScore2 = pm->arEvalMove[OUTPUT_EQUITY];

    return nDone;
}

/* Live progress for the UI's polled snapshot: fills done/total and the
 * running gnubg aggregation over whatever has completed. Returns 1 while a
 * rollout is live, 0 otherwise. Safe from any thread. */
int gnubg_rollout_poll(int *done, int *total,
                       float arMu[NUM_ROLLOUT_OUTPUTS], float arSigma[NUM_ROLLOUT_OUTPUTS]) {
    int live = 0;
    g_mutex_lock(&rollout_live_mutex);
    if (rollout_live) {
        live = 1;
        *done = g_atomic_int_get(&rollout_done_count);
        *total = (int) rollout_live->prc->nTrials;
        rollout_accumulate(rollout_live->results, rollout_live->completed,
                           rollout_live->prc->nTrials, arMu, arSigma);
    }
    g_mutex_unlock(&rollout_live_mutex);
    return live;
}

/* Cancel the live rollout: unstarted trials become no-ops; the running
 * gnubg_rollout call returns with the completed count. */
void gnubg_rollout_cancel(void) {
    g_atomic_int_set(&rollout_cancel_flag, 1);
}
