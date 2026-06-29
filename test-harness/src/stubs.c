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
    return NULL;
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
void LogCube(void)                       {}
int GetManualDice(unsigned int anDice[2]) { return 0; }
void SetRNG(rng *prng, rngcontext *rngctx, rng rngNew, char *szSeed) {}
void ChangeGame(listOLD *plGameNew) {}
double get_time(void) { return 0.0; }
void FormatMove(void)                    {}
moverecord *get_current_moverecord(int *pfHistory) { return NULL; }

/* -- randomorg -- network dice unavailable on Android ----------------------- */
void RandomorgDice(void)                               {}
int  NetworkDice(unsigned int *pdice, int ndice)       { return -1; }

/* -- Thread-local data initialisation ---------------------------------------
 * MT_Get_aMoves() expands to td.tld->aMoves when USE_MULTITHREAD is off.
 * td.tld must point to a valid ThreadLocalData with an allocated aMoves buffer
 * before any move generation occurs (i.e. before any 1-ply evaluation).
 * Called once from Engine.initialise() via gnubg_init_tld().
 */
static ThreadLocalData gnubg_tld;
static NNState gnubg_nn_states[3];
static move gnubg_moves[MAX_MOVES];

void gnubg_init_tld(void) {
    unsigned int i;

    gnubg_tld.aMoves = gnubg_moves;
    gnubg_tld.pnnState = gnubg_nn_states;
    td.tld = &gnubg_tld;

    /* Allocate savedBase (cHidden floats) and savedIBase (cInput floats)
     * for each NNState entry. Sizes come from the loaded networks:
     *   [0] = nnContact/nnCrashed: cHidden, cInput
     *   [1] = nnRace
     *   [2] = nnCrashed
     * Use the largest network dimensions to cover all cases safely.
     * nnContact has the most inputs (NUM_INPUTS > NUM_RACE_INPUTS).
     */
    for (i = 0; i < 3; i++) {
        gnubg_nn_states[i].state     = NNSTATE_NONE;
        gnubg_nn_states[i].savedBase  = (float *)g_malloc(nnContact.cHidden * sizeof(float));
        gnubg_nn_states[i].savedIBase = (float *)g_malloc(nnContact.cInput  * sizeof(float));
    }
}

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
int fOutputMWC       = 0;
int fOutputWinPC     = 0;
int fOutputMatchPC   = 0;

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
void gnubg_init_rollout(void) {
    if (!rngctxRollout && rngctxCurrent)
        rngctxRollout = CopyRNGContext(rngctxCurrent);
}

/* -- gnubg_rollout -----------------------------------------------------------
 * Synchronous rollout bypassing MT task queue.
 * Calls BasicCubefulRolloutNoLocking directly for nTrials games.
 */
int gnubg_rollout(const TanBoard anBoard,
                  float arOutput[NUM_ROLLOUT_OUTPUTS],
                  float arStdDev[NUM_ROLLOUT_OUTPUTS],
                  const cubeinfo *pci, rolloutcontext *prc) {
    unsigned int i;
    unsigned int nTrials = prc->nTrials;

    double adSum[NUM_ROLLOUT_OUTPUTS]  = {0};
    double adSum2[NUM_ROLLOUT_OUTPUTS] = {0};

    perArray dicePerms;
    dicePerms.nPermutationSeed = -1;
    QuasiRandomSeed(&dicePerms, (int)prc->nSeed);

    cubeinfo aci[1];
    memcpy(&aci[0], pci, sizeof(cubeinfo));
    int afCubeDecTop[1] = {0};

    unsigned int aanBoard[1][2][25];
    float aarOutput[1][NUM_ROLLOUT_OUTPUTS];
    rolloutstat aarsStats[1][2];
    memset(aarsStats, 0, sizeof(aarsStats));

    for (i = 0; i < nTrials; i++) {
        memcpy(aanBoard[0], anBoard, 25 * 2 * sizeof(unsigned int));
        memset(aarOutput[0], 0, sizeof(aarOutput[0]));

        if (BasicCubefulRolloutNoLocking(aanBoard, aarOutput,
                                          0, (int)i, aci, afCubeDecTop, 1,
                                          prc, aarsStats, 0,
                                          &dicePerms, rngctxRollout, NULL) < 0)
            return -1;

        unsigned int j;
        for (j = 0; j < NUM_ROLLOUT_OUTPUTS; j++) {
            adSum[j]  += aarOutput[0][j];
            adSum2[j] += aarOutput[0][j] * aarOutput[0][j];
        }
    }

    unsigned int j;
    for (j = 0; j < NUM_ROLLOUT_OUTPUTS; j++) {
        arOutput[j] = (float)(adSum[j] / nTrials);
        if (nTrials > 1) {
            double var = (adSum2[j] - adSum[j]*adSum[j]/nTrials) / (nTrials-1);
            arStdDev[j] = (float)(var > 0 ? sqrt(var) / sqrt((double)nTrials) : 0);
        } else {
            arStdDev[j] = 0.0f;
        }
    }
    return 0;
}
