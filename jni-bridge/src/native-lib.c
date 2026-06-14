#include <jni.h>
#include <android/log.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>

#include "config.h"
#include "eval.h"
#include "positionid.h"
#include "backgammon.h"
extern void gnubg_init_tld(void);
extern void gnubg_init_rollout(void);
extern int gnubg_rollout(const TanBoard anBoard, float arOutput[], float arStdDev[], const cubeinfo *pci, rolloutcontext *prc);

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

/* ── Helper: unpack jintArray → TanBoard ─────────────────────────────────── */
static void unpack_board(JNIEnv *env, jintArray jboard, TanBoard anBoard) {
    jint buf[50];
    (*env)->GetIntArrayRegion(env, jboard, 0, 50, buf);
    for (int i = 0; i < 25; i++) {
        anBoard[0][i] = (unsigned int)buf[i];
        anBoard[1][i] = (unsigned int)buf[25 + i];
    }
}

/* ── Helper: pack TanBoard → jintArray ───────────────────────────────────── */
static jintArray pack_board(JNIEnv *env, TanBoard anBoard) {
    jintArray result = (*env)->NewIntArray(env, 50);
    jint buf[50];
    for (int i = 0; i < 25; i++) {
        buf[i]      = (jint)anBoard[0][i];
        buf[25 + i] = (jint)anBoard[1][i];
    }
    (*env)->SetIntArrayRegion(env, result, 0, 50, buf);
    return result;
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

    gnubg_init_tld();
    gnubg_init_rollout();
    gnubg_initialised = 1;
    pthread_mutex_unlock(&gnubg_lock);

    LOGI("Engine initialised");
    return JNI_TRUE;
}

/*
 * Engine.evaluatePosition(board: IntArray): FloatArray
 */
JNIEXPORT jfloatArray JNICALL
Java_com_clavierhaus_gnubg_Engine_evaluatePosition(JNIEnv *env, jobject thiz,
                                                    jintArray jboard) {
    pthread_mutex_lock(&gnubg_lock);

    if (!gnubg_initialised) {
        LOGE("evaluatePosition called before initialise()");
        pthread_mutex_unlock(&gnubg_lock);
        return NULL;
    }

    TanBoard anBoard;
    unpack_board(env, jboard, anBoard);

    float arOutput[NUM_OUTPUTS] = {0};
    int rc = EvaluatePosition(NULL, (ConstTanBoard)anBoard, arOutput,
                               &ci_default, &ec_default);

    pthread_mutex_unlock(&gnubg_lock);

    if (rc != 0) {
        LOGE("EvaluatePosition failed: %d", rc);
        return NULL;
    }

    jfloatArray result = (*env)->NewFloatArray(env, NUM_OUTPUTS);
    (*env)->SetFloatArrayRegion(env, result, 0, NUM_OUTPUTS, arOutput);
    return result;
}

/*
 * Engine.findBestMove(board: IntArray, die0: Int, die1: Int): IntArray
 *
 * board must have anBoard[1] = moving player's checkers (call swapBoard first
 * if it is the engine's turn).
 * Returns 8-element IntArray encoding the best move, or null on error.
 */
JNIEXPORT jintArray JNICALL
Java_com_clavierhaus_gnubg_Engine_findBestMove(JNIEnv *env, jobject thiz,
                                                jintArray jboard,
                                                jint die0, jint die1) {
    pthread_mutex_lock(&gnubg_lock);

    if (!gnubg_initialised) {
        LOGE("findBestMove called before initialise()");
        pthread_mutex_unlock(&gnubg_lock);
        return NULL;
    }

    TanBoard anBoard;
    unpack_board(env, jboard, anBoard);

    int anMove[8];
    memset(anMove, -1, sizeof(anMove));

    int rc = FindBestMoveNoLocking(anMove, (int)die0, (int)die1,
                                    anBoard, &ci_default, &ec_default,
                                    defaultFilters);

    pthread_mutex_unlock(&gnubg_lock);

    if (rc < 0) {
        LOGE("FindBestMove failed: %d", rc);
        return NULL;
    }

    jintArray result = (*env)->NewIntArray(env, 8);
    (*env)->SetIntArrayRegion(env, result, 0, 8, (jint *)anMove);
    return result;
}

/*
 * Engine.swapBoard(board: IntArray): IntArray
 *
 * Swaps anBoard[0] and anBoard[1] — use before/after engine turn calls.
 * gnubg always operates from anBoard[1]'s perspective (the moving player).
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
 * Engine.classifyPosition(board: IntArray): Int
 */
JNIEXPORT jint JNICALL
Java_com_clavierhaus_gnubg_Engine_classifyPosition(JNIEnv *env, jobject thiz,
                                                    jintArray jboard) {
    pthread_mutex_lock(&gnubg_lock);

    if (!gnubg_initialised) {
        pthread_mutex_unlock(&gnubg_lock);
        return -1;
    }

    TanBoard anBoard;
    unpack_board(env, jboard, anBoard);

    positionclass pc = ClassifyPosition((ConstTanBoard)anBoard,
                                         VARIATION_STANDARD);

    pthread_mutex_unlock(&gnubg_lock);
    return (jint)pc;
}

/*
 * Engine.applyMove(board: IntArray, move: IntArray): IntArray
 *
 * Applies anMove[8] to a board and returns the resulting 50-element board.
 * board must have anBoard[1] = moving player's checkers.
 */
JNIEXPORT jintArray JNICALL
Java_com_clavierhaus_gnubg_Engine_applyMove(JNIEnv *env, jobject thiz,
                                             jintArray jboard,
                                             jintArray jmove) {
    pthread_mutex_lock(&gnubg_lock);

    TanBoard anBoard;
    unpack_board(env, jboard, anBoard);

    jint moveBuf[8];
    (*env)->GetIntArrayRegion(env, jmove, 0, 8, moveBuf);

    ApplyMove(anBoard, (const int *)moveBuf, 0);

    jintArray result = pack_board(env, anBoard);
    pthread_mutex_unlock(&gnubg_lock);
    return result;
}

/*
 * Engine.getLegalMoves(board: IntArray, die0: Int, die1: Int): IntArray
 *
 * Returns flat IntArray of all legal moves (nMoves * 8).
 * board must have anBoard[1] = moving player's checkers.
 */
JNIEXPORT jintArray JNICALL
Java_com_clavierhaus_gnubg_Engine_getLegalMoves(JNIEnv *env, jobject thiz,
                                                  jintArray jboard,
                                                  jint die0, jint die1) {
    pthread_mutex_lock(&gnubg_lock);

    if (!gnubg_initialised) {
        pthread_mutex_unlock(&gnubg_lock);
        return (*env)->NewIntArray(env, 0);
    }

    TanBoard anBoard;
    unpack_board(env, jboard, anBoard);

    movelist ml;
    memset(&ml, 0, sizeof(ml));
    GenerateMoves(&ml, (ConstTanBoard)anBoard, (int)die0, (int)die1, FALSE);

    int nMoves = ml.cMoves;

    jintArray result = (*env)->NewIntArray(env, nMoves * 8);
    if (nMoves > 0) {
        jint *buf = (jint *)malloc(nMoves * 8 * sizeof(jint));
        for (int i = 0; i < nMoves; i++) {
            for (int j = 0; j < 8; j++) {
                buf[i * 8 + j] = ml.amMoves[i].anMove[j];
            }
        }
        (*env)->SetIntArrayRegion(env, result, 0, nMoves * 8, buf);
        free(buf);
    }

    pthread_mutex_unlock(&gnubg_lock);
    return result;
}

/*
 * Engine.isGameOver(board: IntArray): Int
 * Returns: 0 = in progress, 1 = anBoard[1] wins, 2 = anBoard[0] wins
 */
JNIEXPORT jint JNICALL
Java_com_clavierhaus_gnubg_Engine_isGameOver(JNIEnv *env, jobject thiz,
                                               jintArray jboard) {
    pthread_mutex_lock(&gnubg_lock);

    TanBoard anBoard;
    unpack_board(env, jboard, anBoard);

    int total1 = 0;
    for (int i = 0; i < 25; i++) total1 += anBoard[1][i];
    if (total1 == 0) { pthread_mutex_unlock(&gnubg_lock); return 1; }

    int total0 = 0;
    for (int i = 0; i < 25; i++) total0 += anBoard[0][i];
    if (total0 == 0) { pthread_mutex_unlock(&gnubg_lock); return 2; }

    pthread_mutex_unlock(&gnubg_lock);
    return 0;
}

/*
 * Engine.pipCount(board: IntArray): IntArray[2]
 * Returns [pipCountPlayer1, pipCountPlayer0] using gnubg's PipCount().
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
 * Engine.rollDice(): IntArray[2]
 * Returns two random dice values (1-6).
 */
JNIEXPORT jintArray JNICALL
Java_com_clavierhaus_gnubg_Engine_rollDice(JNIEnv *env, jobject thiz) {
    jintArray result = (*env)->NewIntArray(env, 2);
    unsigned int dice[2];
    dice[0] = (unsigned int)(rand() % 6) + 1;
    dice[1] = (unsigned int)(rand() % 6) + 1;
    jint jdice[2] = { (jint)dice[0], (jint)dice[1] };
    (*env)->SetIntArrayRegion(env, result, 0, 2, jdice);
    return result;
}

/*
 * Engine.newGame(): IntArray
 * Returns the standard starting position as a 50-element board.
 */
JNIEXPORT jintArray JNICALL
Java_com_clavierhaus_gnubg_Engine_newGame(JNIEnv *env, jobject thiz) {
    TanBoard anBoard;
    InitBoard(anBoard, VARIATION_STANDARD);
    return pack_board(env, anBoard);
}

/* ── SGF Import / Export ─────────────────────────────────────────────────── */
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

/* ── cubeDecision ─────────────────────────────────────────────────────────── */
JNIEXPORT jintArray JNICALL
Java_com_clavierhaus_gnubg_Engine_cubeDecision(JNIEnv *env, jobject thiz,
                                                jintArray jboard,
                                                jint cubeValue, jint cubeOwner,
                                                jint matchTo,
                                                jint score0, jint score1,
                                                jint crawford) {
    pthread_mutex_lock(&gnubg_lock);

    if (!gnubg_initialised) {
        LOGE("cubeDecision called before initialise()");
        pthread_mutex_unlock(&gnubg_lock);
        return NULL;
    }

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
    if (rc != 0) {
        LOGE("GeneralCubeDecisionE failed: %d", rc);
        pthread_mutex_unlock(&gnubg_lock);
        return NULL;
    }

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
    buf[14] = (jint)cd;
    buf[15] = 0;
    (*env)->SetIntArrayRegion(env, result, 0, 16, buf);
    return result;
}

/* ── rollout ──────────────────────────────────────────────────────────────── */
JNIEXPORT jfloatArray JNICALL
Java_com_clavierhaus_gnubg_Engine_rollout(JNIEnv *env, jobject thiz,
                                           jintArray jboard, jint trials) {
    pthread_mutex_lock(&gnubg_lock);

    if (!gnubg_initialised) {
        LOGE("rollout called before initialise()");
        pthread_mutex_unlock(&gnubg_lock);
        return NULL;
    }

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

    if (ret != 0) {
        LOGE("gnubg_rollout failed: %d", ret);
        return NULL;
    }

    jfloatArray result = (*env)->NewFloatArray(env, 14);
    jfloat buf[14];
    for (int i = 0; i < 7; i++) {
        buf[i] = arOutput[i]; buf[7 + i] = arStdDev[i];
    }
    (*env)->SetFloatArrayRegion(env, result, 0, 14, buf);
    return result;
}
