/*
 * stubs.c — Android JNI stub layer for GNU Backgammon engine
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
#include "lib/isaac.h"

/* ── Global state variables ─────────────────────────────────────────────── */

/* Defined in backgammon.h as extern matchstate ms — provide the storage */
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

/* ── ThreadData td ───────────────────────────────────────────────────────── */
ThreadData td;

/* ── msBoard — returns current board position ────────────────────────────── */
ConstTanBoard msBoard(void) {
    return NULL;
}

/* ── save_autosave ───────────────────────────────────────────────────────── */
gboolean save_autosave(gpointer unused) {
    return FALSE;
}

/* ── Threading primitives ────────────────────────────────────────────────── */
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

/* ── EXP_LOCK_FUN function pointer variables ─────────────────────────────── 
 * EXP_LOCK_FUN declares: typedef ret (*f_name)(...); extern f_name name;
 * We provide the storage for the function pointer and point it at the
 * NoLocking variant which is the real implementation in eval.c
 */

/* ── WithLocking variants — single-threaded: just call NoLocking ─────────── */
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

/* ── UI / progress stubs ─────────────────────────────────────────────────── */
void ProcessEvents(void)                 {}
void progress(void)                      {}
void ProgressValue(int val)              {}
void ProgressStart(const char *sz)       {}
void ProgressEnd(void)                   {}
void ProgressValueAdd(int val)           {}

/* ── Misc engine callbacks ───────────────────────────────────────────────── */
/* LogCube: provided by set.c */
int GetManualDice(unsigned int anDice[2]) { return 0; }
/* SetRNG: provided by set.c */
/* ChangeGame: provided by play.c */
double get_time(void) { return 0.0; }
/* FormatMove: provided by format.c */
/* get_current_moverecord: provided by play.c */

/* ── randomorg — network dice unavailable on Android ─────────────────────── */
void RandomorgDice(void)                               {}
int  NetworkDice(unsigned int *pdice, int ndice)       { return -1; }

/* ── Thread-local data initialisation ───────────────────────────────────────
 * MT_Get_aMoves() expands to td.tld->aMoves when USE_MULTITHREAD is off.
 * td.tld must point to a valid ThreadLocalData with an allocated aMoves buffer
 * before any move generation occurs (i.e. before any 1-ply evaluation).
 * Called once from Engine.initialise() via gnubg_init_tld().
 */

/* ── Rollout global state ────────────────────────────────────────────────────
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

/* ── QuasiRandomSeed — copied from rollout.c (static there) ─────────────────
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

/* ── gnubg_init_rollout ──────────────────────────────────────────────────────
 * Allocates and seeds the rollout RNG context.
 * Called after EvalInitialise().
 */

/* ── gnubg_rollout ───────────────────────────────────────────────────────────
 * Synchronous rollout bypassing MT task queue.
 * Calls BasicCubefulRolloutNoLocking directly for nTrials games.
 */

/* MT_WaitForTasks: provided by multithread.c */


#include <unistd.h>

/* ── Thread-Local Storage (TLS) Allocator ───────────────────────────────── */

#include <unistd.h>

/* ── Thread-Local Storage (TLS) Allocator ───────────────────────────────── */
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

/* ── Rollout Infrastructure ─────────────────────────────────────────────── */
static GThreadPool *rollout_pool = NULL;

typedef struct {
    float arOutput[NUM_ROLLOUT_OUTPUTS];
    float arStdDev[NUM_ROLLOUT_OUTPUTS]; 
} __attribute__((aligned(64))) RolloutResult;

typedef struct {
    GMutex mutex;
    GCond cond;
    gint tasks_remaining;
    RolloutResult *results;
    const cubeinfo *pci;
    rolloutcontext *prc;
    const unsigned int (*anBoard)[25]; /* Matches the decayed pointer type */
} RolloutBarrier;

static void rollout_worker_func(gpointer data, gpointer user_data) {
    int task_index = GPOINTER_TO_INT(data);
    RolloutBarrier *barrier = (RolloutBarrier *)user_data;

    /* Per-thread RNG context — copied lazily on first use */
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

    perArray dicePerms;
    dicePerms.nPermutationSeed = -1;
    QuasiRandomSeed(&dicePerms, (int)barrier->prc->nSeed + task_index);

    BasicCubefulRolloutNoLocking(aanBoard, aarOutput,
                                  0, task_index,
                                  aci, afCubeDecTop, 1,
                                  barrier->prc, aarsStats, 0,
                                  &dicePerms, local_rng, NULL);

    memcpy(barrier->results[task_index].arOutput, aarOutput[0],
           NUM_ROLLOUT_OUTPUTS * sizeof(float));

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

int gnubg_rollout(const TanBoard anBoard, float arOutput[NUM_ROLLOUT_OUTPUTS], float arStdDev[NUM_ROLLOUT_OUTPUTS], const cubeinfo *pci, rolloutcontext *prc) {
    if (!rollout_pool) gnubg_init_rollout();
    
    RolloutBarrier barrier;
    g_mutex_init(&barrier.mutex);
    g_cond_init(&barrier.cond);
    barrier.tasks_remaining = prc->nTrials;
    barrier.pci = pci;
    barrier.prc = prc;
    barrier.anBoard = anBoard; /* Decays cleanly into const unsigned int (*)[25] */
    
    /* Allocate cache-aligned results array */
    if (posix_memalign((void**)&barrier.results, 64, prc->nTrials * sizeof(RolloutResult)) != 0) {
        return -1;
    }
    
    /* Initialize the results array to zero */
    memset(barrier.results, 0, prc->nTrials * sizeof(RolloutResult));
    
    /* Dispatch tasks */
    for (int i = 0; i < prc->nTrials; i++) {
        g_thread_pool_push(rollout_pool, GINT_TO_POINTER(i), NULL);
    }
    
    /* Wait for completion */
    g_mutex_lock(&barrier.mutex);
    while (barrier.tasks_remaining > 0) {
        g_cond_wait(&barrier.cond, &barrier.mutex);
    }
    g_mutex_unlock(&barrier.mutex);
    
    /* Accumulate results (scatter-gather merge) */
    for (int j = 0; j < NUM_ROLLOUT_OUTPUTS; j++) {
        arOutput[j] = 0.0f;
        arStdDev[j] = 0.0f;
    }
    
    double adSum[NUM_ROLLOUT_OUTPUTS]  = {0};
    double adSum2[NUM_ROLLOUT_OUTPUTS] = {0};
    for (unsigned int i = 0; i < prc->nTrials; i++) {
        for (int j = 0; j < NUM_ROLLOUT_OUTPUTS; j++) {
            double v = barrier.results[i].arOutput[j];
            adSum[j]  += v;
            adSum2[j] += v * v;
        }
    }
    for (int j = 0; j < NUM_ROLLOUT_OUTPUTS; j++) {
        arOutput[j] = (float)(adSum[j] / prc->nTrials);
        if (prc->nTrials > 1) {
            double var = (adSum2[j] - adSum[j]*adSum[j]/prc->nTrials)
                         / (prc->nTrials - 1);
            arStdDev[j] = (float)(var > 0 ? sqrt(var/prc->nTrials) : 0.0);
        } else {
            arStdDev[j] = 0.0f;
        }
    }
    
    free(barrier.results);
    g_mutex_clear(&barrier.mutex);
    g_cond_clear(&barrier.cond);
    
    return 0;
}
