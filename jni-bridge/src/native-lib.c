/*
 * native-lib.c — JNI bridge for GNU Backgammon engine
 * Package: com.clavierhaus.gnubg
 * App:     GNU Backgammon by clavierhaus.at
 *
 * Exposes the gnubg evaluation engine to Kotlin/Java via JNI.
 * All calls are serialised through gnubg_lock to protect global engine state.
 *
 * Board encoding (JNI convention):
 *   jintArray of 50 ints: anBoard[0][0..24] followed by anBoard[1][0..24]
 *   i.e. element[i] = anBoard[i/25][i%25]
 */

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
 *
 * Must be called once before any evaluation. Pass the path to gnubg.weights
 * extracted into internal storage, e.g.:
 *   context.filesDir.absolutePath + "/gnubg.weights"
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
                /*nCube*/    1,
                /*fOwner*/  -1,
                /*fMove*/    0,
                /*nMatchTo*/ 0,
                /*anScore*/  (int[]){0, 0},
                /*fCrawford*/0,
                /*fJacoby*/  0,
                /*fBeavers*/ 0,
                /*bgv*/      VARIATION_STANDARD);

    EvalInitialise((char *)weightsPath, NULL, 0, NULL);

    (*env)->ReleaseStringUTFChars(env, jWeightsPath, weightsPath);

    gnubg_init_tld();
    gnubg_initialised = 1;
    pthread_mutex_unlock(&gnubg_lock);

    LOGI("Engine initialised");
    return JNI_TRUE;
}

/*
 * Engine.evaluatePosition(board: IntArray): FloatArray
 *
 * board: 50-element IntArray encoding TanBoard (see file header)
 * Returns: 5-element FloatArray [winNorm, winGammon, winBG, losGammon, losBG]
 *          or null on error.
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
 * Returns: 8-element IntArray encoding the best move (anMove[8]),
 *          or null on error. Unused move slots are -1.
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

    int rc = FindBestMove(anMove, (int)die0, (int)die1,
                          anBoard, &ci_default, &ec_default,
                          defaultFilters);

    pthread_mutex_unlock(&gnubg_lock);

    if (rc != 0) {
        LOGE("FindBestMove failed: %d", rc);
        return NULL;
    }

    jintArray result = (*env)->NewIntArray(env, 8);
    (*env)->SetIntArrayRegion(env, result, 0, 8, (jint *)anMove);
    return result;
}

/*
 * Engine.classifyPosition(board: IntArray): Int
 *
 * Returns the positionclass enum value (race, contact, bearoff, etc.)
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
