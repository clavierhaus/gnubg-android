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

#include "backgammon.h"
#include "eval.h"
#include "multithread.h"
#include "rollout.h"

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
void LogCube(void)                       {}
int GetManualDice(unsigned int anDice[2]) { return 0; }
void SetRNG(rng *prng, rngcontext *rngctx, rng rngNew, char *szSeed) {}
void ChangeGame(listOLD *plGameNew) {}
double get_time(void) { return 0.0; }
void FormatMove(void)                    {}
moverecord *get_current_moverecord(int *pfHistory) { return NULL; }

/* ── randomorg — network dice unavailable on Android ─────────────────────── */
void RandomorgDice(void)                               {}
int  NetworkDice(unsigned int *pdice, int ndice)       { return -1; }

/* ── Thread-local data initialisation ───────────────────────────────────────
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
