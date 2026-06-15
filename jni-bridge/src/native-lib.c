#include <jni.h>
#include <android/log.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>

#include "config.h"
#include "eval.h"
#include "positionid.h"
#include "backgammon.h"
#include "dice.h"
/* FormatMove declared directly to avoid shadow header conflict */
extern char *FormatMove(char *sz, const TanBoard anBoard, const int anMove[8]);
extern void gnubg_init_tld(void);
extern void gnubg_init_rollout(void);
extern int gnubg_rollout(const TanBoard anBoard, float arOutput[], float arStdDev[], const cubeinfo *pci, rolloutcontext *prc);
extern void CommandNewGame(char *);
extern void CommandRoll(char *);
extern void CommandMove(char *);
extern void ClearMatch(void);
extern int NextTurn(int fPlayNext);
extern int ListCreate(listOLD *pl);
extern listOLD lMatch;
extern rng rngCurrent;
extern rngcontext *rngctxCurrent;

#define LOG_TAG "gnubg-jni"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

/* ── Global serialisation lock ───────────────────────────────────────────── */
static pthread_mutex_t gnubg_lock = PTHREAD_MUTEX_INITIALIZER;
static int             gnubg_initialised = 0;

/* ── Default eval context (1-ply, cubeless) ──────────────────────────────── */
static evalcontext ec_default = {
    .fCubeful  = 0,
    .nPlies    = 1,
    .fUsePrune = 0,
    .fDeterministic = 1,
    .rNoise    = 0.0f
};

/* ── Default cubeless cubeinfo ───────────────────────────────────────────── */
static cubeinfo ci_default;

/* ── Cached board state from gnubg_on_board_changed callback ─────────────── */
static int cached_board[50];
static int cached_dice[2];
static int cached_turn;  /* ms.fTurn */
static int cached_gs;    /* ms.gs */

/* ── Strong implementation of gnubg_on_board_changed ─────────────────────── */
void gnubg_on_board_changed(void) {
    /* Cache ms state for JNI retrieval */
    for (int i = 0; i < 25; i++) {
        cached_board[i]      = (int)ms.anBoard[0][i];
        cached_board[25 + i] = (int)ms.anBoard[1][i];
    }
    cached_dice[0] = ms.anDice[0];
    cached_dice[1] = ms.anDice[1];
    cached_turn    = ms.fTurn;
    cached_gs      = ms.gs;
    LOGI("board_changed: gs=%d fTurn=%d dice=%d,%d", ms.gs, ms.fTurn, ms.anDice[0], ms.anDice[1]);
}

/* ── Helper: pack TanBoard → jintArray ───────────────────────────────────── */
static jintArray pack_board(JNIEnv *env, const unsigned int anBoard[2][25]) {
    jintArray result = (*env)->NewIntArray(env, 50);
    jint buf[50];
    for (int i = 0; i < 25; i++) {
        buf[i]      = (jint)anBoard[0][i];
        buf[25 + i] = (jint)anBoard[1][i];
    }
    (*env)->SetIntArrayRegion(env, result, 0, 50, buf);
    return result;
}

/* ── Helper: unpack jintArray → TanBoard ─────────────────────────────────── */
static void unpack_board(JNIEnv *env, jintArray jboard, TanBoard anBoard) {
    jint buf[50];
    (*env)->GetIntArrayRegion(env, jboard, 0, 50, buf);
    for (int i = 0; i < 25; i++) {
        anBoard[0][i] = (unsigned int)buf[i];
        anBoard[1][i] = (unsigned int)buf[25 + i];
    }
}

/*
 * Engine.initialise(weightsPath: String): Boolean
 */
JNIEXPORT jboolean JNICALL
Java_com_clavierhaus_gnubg_Engine_initialise(JNIEnv *env, jobject thiz,
                                              jstring jWeightsPath) {
    pthread_mutex_lock(&gnubg_lock);

    if (gnubg_initialised) {
        pthread_mutex_unlock(&gnubg_lock);
        return JNI_TRUE;
    }

    const char *weightsPath = (*env)->GetStringUTFChars(env, jWeightsPath, NULL);

    SetCubeInfo(&ci_default,
                1, -1, 0, 0, (int[]){0, 0}, 0, 0, 0, VARIATION_STANDARD);

    EvalInitialise((char *)weightsPath, NULL, 0, NULL);
    (*env)->ReleaseStringUTFChars(env, jWeightsPath, weightsPath);

    /* Initialise gnubg RNG */
    rngctxCurrent = InitRNG(NULL, NULL, TRUE, rngCurrent);

    gnubg_init_tld();
    gnubg_init_rollout();

    /* Set up match state for human vs GNU */
    ListCreate(&lMatch);
    ClearMatch();
    ap[0].pt = PLAYER_HUMAN;
    ap[1].pt = PLAYER_GNU;
    strcpy(ap[0].szName, "Human");
    strcpy(ap[1].szName, "GNU");
    ms.nMatchTo = 1;  /* 1-point match */
    ms.fJacoby  = FALSE;

    gnubg_initialised = 1;
    pthread_mutex_unlock(&gnubg_lock);

    LOGI("Engine initialised");
    return JNI_TRUE;
}

/*
 * Engine.newGame(): IntArray
 * Starts a new game via CommandNewGame, returns ms.anBoard as 50-element array.
 */
JNIEXPORT jintArray JNICALL
Java_com_clavierhaus_gnubg_Engine_newGame(JNIEnv *env, jobject thiz) {
    pthread_mutex_lock(&gnubg_lock);
    ListCreate(&lMatch);
    ClearMatch();
    CommandNewMatch("1");
    CommandNewGame(NULL);
    jintArray result = pack_board(env, ms.anBoard);
    pthread_mutex_unlock(&gnubg_lock);
    return result;
}

/*
 * Engine.rollDice(): IntArray[2]
 * Calls CommandRoll, returns ms.anDice.
 */
JNIEXPORT jintArray JNICALL
Java_com_clavierhaus_gnubg_Engine_rollDice(JNIEnv *env, jobject thiz) {
    pthread_mutex_lock(&gnubg_lock);
    CommandRoll(NULL);
    jint dice[2] = { (jint)ms.anDice[0], (jint)ms.anDice[1] };
    jintArray result = (*env)->NewIntArray(env, 2);
    (*env)->SetIntArrayRegion(env, result, 0, 2, dice);
    pthread_mutex_unlock(&gnubg_lock);
    return result;
}

/*
 * Engine.getLegalMoves(board: IntArray, die0: Int, die1: Int, fPartial: Int): IntArray
 * Returns flat IntArray of all legal moves from GenerateMoves.
 */
JNIEXPORT jintArray JNICALL
Java_com_clavierhaus_gnubg_Engine_getLegalMoves(JNIEnv *env, jobject thiz,
                                                  jintArray jboard,
                                                  jint die0, jint die1,
                                                  jint fPartial) {
    pthread_mutex_lock(&gnubg_lock);

    if (!gnubg_initialised) {
        pthread_mutex_unlock(&gnubg_lock);
        return (*env)->NewIntArray(env, 0);
    }

    TanBoard anBoard;
    unpack_board(env, jboard, anBoard);

    movelist ml;
    memset(&ml, 0, sizeof(ml));
    GenerateMoves(&ml, (ConstTanBoard)anBoard, (int)die0, (int)die1, (int)fPartial);

    int nMoves = ml.cMoves;
    jintArray result = (*env)->NewIntArray(env, nMoves * 8);
    if (nMoves > 0) {
        jint *buf = (jint *)malloc(nMoves * 8 * sizeof(jint));
        for (int i = 0; i < nMoves; i++)
            for (int j = 0; j < 8; j++)
                buf[i * 8 + j] = ml.amMoves[i].anMove[j];
        (*env)->SetIntArrayRegion(env, result, 0, nMoves * 8, buf);
        free(buf);
    }

    pthread_mutex_unlock(&gnubg_lock);
    return result;
}

/*
 * Engine.applyMoveString(moveStr: String): IntArray
 * Calls CommandMove with gnubg point notation, returns updated ms.anBoard.
 */
JNIEXPORT jintArray JNICALL
Java_com_clavierhaus_gnubg_Engine_applyMoveString(JNIEnv *env, jobject thiz,
                                                    jstring jmoveStr) {
    const char *moveStr = (*env)->GetStringUTFChars(env, jmoveStr, NULL);
    pthread_mutex_lock(&gnubg_lock);
    char *szCopy = strdup(moveStr);
    CommandMove(szCopy);
    free(szCopy);
    /* CommandMove calls TurnDone() which sets fNextTurn=TRUE.
     * NextTurn() advances ms.fTurn and triggers ComputerTurn if needed. */
    NextTurn(TRUE);
    jintArray result = pack_board(env, ms.anBoard);
    pthread_mutex_unlock(&gnubg_lock);
    (*env)->ReleaseStringUTFChars(env, jmoveStr, moveStr);
    return result;
}

/*
 * Engine.formatMove(board: IntArray, move: IntArray): String
 * Converts anMove[8] to gnubg point notation string using FormatMove.
 */
JNIEXPORT jstring JNICALL
Java_com_clavierhaus_gnubg_Engine_formatMove(JNIEnv *env, jobject thiz,
                                              jintArray jboard,
                                              jintArray jmove) {
    TanBoard anBoard;
    unpack_board(env, jboard, anBoard);
    jint moveBuf[8];
    (*env)->GetIntArrayRegion(env, jmove, 0, 8, moveBuf);
    int anMove[8];
    for (int i = 0; i < 8; i++) anMove[i] = (int)moveBuf[i];
    char sz[64] = {0};
    FormatMove(sz, (ConstTanBoard)anBoard, anMove);
    return (*env)->NewStringUTF(env, sz);
}

/*
 * Engine.getMatchBoard(): IntArray
 * Returns current ms.anBoard as 50-element array.
 */
JNIEXPORT jintArray JNICALL
Java_com_clavierhaus_gnubg_Engine_getMatchBoard(JNIEnv *env, jobject thiz) {
    pthread_mutex_lock(&gnubg_lock);
    jintArray result = pack_board(env, ms.anBoard);
    pthread_mutex_unlock(&gnubg_lock);
    return result;
}

/*
 * Engine.getMatchDice(): IntArray[2]
 * Returns current ms.anDice.
 */
JNIEXPORT jintArray JNICALL
Java_com_clavierhaus_gnubg_Engine_getMatchDice(JNIEnv *env, jobject thiz) {
    pthread_mutex_lock(&gnubg_lock);
    jint dice[2] = { (jint)ms.anDice[0], (jint)ms.anDice[1] };
    jintArray result = (*env)->NewIntArray(env, 2);
    (*env)->SetIntArrayRegion(env, result, 0, 2, dice);
    pthread_mutex_unlock(&gnubg_lock);
    return result;
}

/*
 * Engine.getMatchTurn(): Int
 * Returns ms.fTurn (0=player0/human, 1=player1/GNU).
 */
JNIEXPORT jint JNICALL
Java_com_clavierhaus_gnubg_Engine_getMatchTurn(JNIEnv *env, jobject thiz) {
    return (jint)ms.fTurn;
}

/*
 * Engine.getMatchStatus(): Int
 * Returns ms.gs (game status).
 */
JNIEXPORT jint JNICALL
Java_com_clavierhaus_gnubg_Engine_getMatchStatus(JNIEnv *env, jobject thiz) {
    return (jint)ms.gs;
}

/*
 * Engine.isGameOver(board: IntArray): Int
 * Uses gnubg GameStatus().
 */
JNIEXPORT jint JNICALL
Java_com_clavierhaus_gnubg_Engine_isGameOver(JNIEnv *env, jobject thiz,
                                               jintArray jboard) {
    pthread_mutex_lock(&gnubg_lock);
    TanBoard anBoard;
    unpack_board(env, jboard, anBoard);
    int status = GameStatus((ConstTanBoard)anBoard, VARIATION_STANDARD);
    pthread_mutex_unlock(&gnubg_lock);
    return (jint)status;
}

/*
 * Engine.pipCount(board: IntArray): IntArray[2]
 */
JNIEXPORT jintArray JNICALL
Java_com_clavierhaus_gnubg_Engine_pipCount(JNIEnv *env, jobject thiz,
                                            jintArray jboard) {
    TanBoard anBoard;
    unpack_board(env, jboard, anBoard);
    unsigned int anPip[2];
    PipCount((ConstTanBoard)anBoard, anPip);
    jintArray result = (*env)->NewIntArray(env, 2);
    jint buf[2] = { (jint)anPip[0], (jint)anPip[1] };
    (*env)->SetIntArrayRegion(env, result, 0, 2, buf);
    return result;
}

/*
 * Engine.swapBoard(board: IntArray): IntArray
 */
JNIEXPORT jintArray JNICALL
Java_com_clavierhaus_gnubg_Engine_swapBoard(JNIEnv *env, jobject thiz,
                                             jintArray jboard) {
    TanBoard anBoard;
    unpack_board(env, jboard, anBoard);
    SwapSides(anBoard);
    return pack_board(env, anBoard);
}

/*
 * Engine.evaluatePosition(board: IntArray): FloatArray
 */
JNIEXPORT jfloatArray JNICALL
Java_com_clavierhaus_gnubg_Engine_evaluatePosition(JNIEnv *env, jobject thiz,
                                                    jintArray jboard) {
    pthread_mutex_lock(&gnubg_lock);
    if (!gnubg_initialised) {
        pthread_mutex_unlock(&gnubg_lock);
        return NULL;
    }
    TanBoard anBoard;
    unpack_board(env, jboard, anBoard);
    float arOutput[NUM_OUTPUTS] = {0};
    int rc = EvaluatePosition(NULL, (ConstTanBoard)anBoard, arOutput,
                               &ci_default, &ec_default);
    pthread_mutex_unlock(&gnubg_lock);
    if (rc != 0) return NULL;
    jfloatArray result = (*env)->NewFloatArray(env, NUM_OUTPUTS);
    (*env)->SetFloatArrayRegion(env, result, 0, NUM_OUTPUTS, arOutput);
    return result;
}

/*
 * Engine.classifyPosition(board: IntArray): Int
 */
JNIEXPORT jint JNICALL
Java_com_clavierhaus_gnubg_Engine_classifyPosition(JNIEnv *env, jobject thiz,
                                                    jintArray jboard) {
    pthread_mutex_lock(&gnubg_lock);
    if (!gnubg_initialised) { pthread_mutex_unlock(&gnubg_lock); return -1; }
    TanBoard anBoard;
    unpack_board(env, jboard, anBoard);
    positionclass pc = ClassifyPosition((ConstTanBoard)anBoard, VARIATION_STANDARD);
    pthread_mutex_unlock(&gnubg_lock);
    return (jint)pc;
}

/*
 * Engine.cubeDecision(...): IntArray
 */
JNIEXPORT jintArray JNICALL
Java_com_clavierhaus_gnubg_Engine_cubeDecision(JNIEnv *env, jobject thiz,
                                                jintArray jboard,
                                                jint cubeValue, jint cubeOwner,
                                                jint matchTo,
                                                jint score0, jint score1,
                                                jint crawford) {
    pthread_mutex_lock(&gnubg_lock);
    if (!gnubg_initialised) { pthread_mutex_unlock(&gnubg_lock); return NULL; }
    TanBoard anBoard;
    unpack_board(env, jboard, anBoard);
    cubeinfo ci;
    int anScore[2] = { (int)score0, (int)score1 };
    SetCubeInfo(&ci, (int)cubeValue, (int)cubeOwner, 0, (int)matchTo,
                anScore, (int)crawford, 0, 0, VARIATION_STANDARD);
    evalsetup es;
    memset(&es, 0, sizeof(es));
    es.et = EVAL_EVAL;
    es.ec = ec_default;
    float aarOutput[2][NUM_ROLLOUT_OUTPUTS];
    memset(aarOutput, 0, sizeof(aarOutput));
    int rc = GeneralCubeDecisionENoLocking(aarOutput, (ConstTanBoard)anBoard,
                                            &ci, &ec_default, &es);
    if (rc != 0) { pthread_mutex_unlock(&gnubg_lock); return NULL; }
    float arDouble[4];
    cubedecision cd = FindCubeDecision(arDouble, aarOutput, &ci);
    pthread_mutex_unlock(&gnubg_lock);
    jintArray result = (*env)->NewIntArray(env, 16);
    jint buf[16];
    for (int i = 0; i < 7; i++) {
        int bits;
        memcpy(&bits, &aarOutput[0][i], sizeof(int)); buf[i] = bits;
        memcpy(&bits, &aarOutput[1][i], sizeof(int)); buf[7 + i] = bits;
    }
    buf[14] = (jint)cd; buf[15] = 0;
    (*env)->SetIntArrayRegion(env, result, 0, 16, buf);
    return result;
}

/*
 * Engine.rollout(board: IntArray, trials: Int): FloatArray
 */
JNIEXPORT jfloatArray JNICALL
Java_com_clavierhaus_gnubg_Engine_rollout(JNIEnv *env, jobject thiz,
                                           jintArray jboard, jint trials) {
    pthread_mutex_lock(&gnubg_lock);
    if (!gnubg_initialised) { pthread_mutex_unlock(&gnubg_lock); return NULL; }
    TanBoard anBoard;
    unpack_board(env, jboard, anBoard);
    rolloutcontext rc_ro = rcRollout;
    rc_ro.nTrials  = (unsigned int)(trials > 0 ? trials : 144);
    rc_ro.fCubeful = 1;
    rc_ro.fVarRedn = 1;
    float arOutput[NUM_ROLLOUT_OUTPUTS] = {0};
    float arStdDev[NUM_ROLLOUT_OUTPUTS] = {0};
    int ret = gnubg_rollout((ConstTanBoard)anBoard, arOutput, arStdDev,
                             &ci_default, &rc_ro);
    pthread_mutex_unlock(&gnubg_lock);
    if (ret != 0) return NULL;
    jfloatArray result = (*env)->NewFloatArray(env, 14);
    jfloat buf[14];
    for (int i = 0; i < 7; i++) { buf[i] = arOutput[i]; buf[7+i] = arStdDev[i]; }
    (*env)->SetFloatArrayRegion(env, result, 0, 14, buf);
    return result;
}

/*
 * Engine.applySubMove(board, iSrc, nRoll): IntArray
 * Wraps gnubg eval.c: ApplySubMove(TanBoard, int iSrc, int nRoll, int fCheckLegal)
 * Returns updated board, or empty array if move is illegal.
 */
extern int ApplySubMove(TanBoard anBoard, int iSrc, int nRoll, int fCheckLegal);

JNIEXPORT jintArray JNICALL
Java_com_clavierhaus_gnubg_Engine_applySubMove(JNIEnv *env, jobject thiz,
                                                jintArray jboard,
                                                jint iSrc, jint nRoll) {
    TanBoard anBoard;
    unpack_board(env, jboard, anBoard);
    if (ApplySubMove(anBoard, (int)iSrc, (int)nRoll, TRUE) != 0)
        return (*env)->NewIntArray(env, 0);
    return pack_board(env, anBoard);
}

/*
 * Engine.findMove(oldBoard, curBoard, die0, die1): String
 * Mirrors update_move() in gtkboard.c: finds the complete move that transforms
 * oldBoard into curBoard by comparing position keys against the movelist.
 * Returns gnubg point notation for CommandMove, or empty string if no match.
 */
JNIEXPORT jstring JNICALL
Java_com_clavierhaus_gnubg_Engine_findMove(JNIEnv *env, jobject thiz,
                                            jintArray joldBoard,
                                            jintArray jcurBoard,
                                            jint die0, jint die1) {
    TanBoard oldBoard, curBoard;
    unpack_board(env, joldBoard, oldBoard);
    unpack_board(env, jcurBoard, curBoard);

    movelist ml;
    memset(&ml, 0, sizeof(ml));
    GenerateMoves(&ml, (ConstTanBoard)oldBoard, (int)die0, (int)die1, FALSE);

    positionkey curKey;
    PositionKey((ConstTanBoard)curBoard, &curKey);

    char sz[64] = {0};
    for (unsigned int i = 0; i < ml.cMoves; i++) {
        if (EqualKeys(ml.amMoves[i].key, curKey)) {
            FormatMove(sz, (ConstTanBoard)oldBoard, ml.amMoves[i].anMove);
            break;
        }
    }
    return (*env)->NewStringUTF(env, sz);
}

/* ── SGF ─────────────────────────────────────────────────────────────────── */
extern void CommandLoadMatch(char *sz);
extern void CommandSaveMatch(char *sz);

JNIEXPORT jboolean JNICALL
Java_com_clavierhaus_gnubg_Engine_loadSGF(JNIEnv *env, jobject thiz, jstring path) {
    const char *szPath = (*env)->GetStringUTFChars(env, path, 0);
    if (!szPath) return JNI_FALSE;
    pthread_mutex_lock(&gnubg_lock);
    char *szCopy = strdup(szPath);
    CommandLoadMatch(szCopy);
    free(szCopy);
    pthread_mutex_unlock(&gnubg_lock);
    (*env)->ReleaseStringUTFChars(env, path, szPath);
    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_clavierhaus_gnubg_Engine_saveSGF(JNIEnv *env, jobject thiz, jstring path) {
    const char *szPath = (*env)->GetStringUTFChars(env, path, 0);
    if (!szPath) return JNI_FALSE;
    pthread_mutex_lock(&gnubg_lock);
    char *szCopy = strdup(szPath);
    CommandSaveMatch(szCopy);
    free(szCopy);
    pthread_mutex_unlock(&gnubg_lock);
    (*env)->ReleaseStringUTFChars(env, path, szPath);
    return JNI_TRUE;
}
