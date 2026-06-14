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
extern void gnubg_init_rollout(void);
extern int gnubg_rollout(const TanBoard anBoard, float arOutput[], float arStdDev[], const cubeinfo *pci, rolloutcontext *prc);

#define LOG_TAG "gnubg-jni"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

/* ── Global serialisation lock ───────────────────────────────────────────── */
static pthread_mutex_t gnubg_lock = PTHREAD_MUTEX_INITIALIZER;
static int             gnubg_initialised = 0;

/* Ensure thread-local neural net cache is initialised for current thread */
static pthread_key_t tld_key;
static pthread_once_t tld_key_once = PTHREAD_ONCE_INIT;

static void make_tld_key(void) {
    pthread_key_create(&tld_key, NULL);
}

static void ensure_tld(void) {
    pthread_once(&tld_key_once, make_tld_key);
    if (pthread_getspecific(tld_key) == NULL) {
        gnubg_init_tld();
        pthread_setspecific(tld_key, (void*)1);
    }
}

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
    gnubg_init_rollout();
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

    ensure_tld();

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

    ensure_tld();

    TanBoard anBoard;
    unpack_board(env, jboard, anBoard);

    int anMove[8];
    memset(anMove, -1, sizeof(anMove));

    int rc = FindBestMove(anMove, (int)die0, (int)die1,
                          anBoard, &ci_default, &ec_default,
                          aamfEval);

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

/*
 * Engine.cubeDecision(board: IntArray, cubeValue: Int, cubeOwner: Int,
 *                     matchTo: Int, score0: Int, score1: Int,
 *                     crawford: Int): IntArray?
 *
 * Returns IntArray[16]:
 *   [0..6]  aarOutput[0] — equity outputs if double
 *   [7..13] aarOutput[1] — equity outputs if no double
 *   [14]    cubedecision enum value
 *   [15]    reserved (0)
 *
 * cubedecision values:
 *   0=DOUBLE_TAKE  1=DOUBLE_PASS  2=NODOUBLE_TAKE  3=TOOGOOD_TAKE
 *   4=TOOGOOD_PASS 5=DOUBLE_BEAVER 6=NODOUBLE_BEAVER 7=REDOUBLE_TAKE
 *   8=REDOUBLE_PASS 9=NO_REDOUBLE_TAKE ... (see eval.h cubedecision enum)
 */
JNIEXPORT jintArray JNICALL
Java_com_clavierhaus_gnubg_Engine_cubeDecision(JNIEnv *env, jobject thiz,
                                                jintArray jboard,
                                                jint cubeValue,
                                                jint cubeOwner,
                                                jint matchTo,
                                                jint score0,
                                                jint score1,
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
    SetCubeInfo(&ci,
                (int)cubeValue,
                (int)cubeOwner,
                0,
                (int)matchTo,
                anScore,
                (int)crawford,
                0, 0,
                VARIATION_STANDARD);

    evalsetup es;
    memset(&es, 0, sizeof(es));
    es.et = EVAL_EVAL;
    es.ec = ec_default;

    float aarOutput[2][NUM_ROLLOUT_OUTPUTS];
    memset(aarOutput, 0, sizeof(aarOutput));

    int rc = GeneralCubeDecisionENoLocking(aarOutput,
                                            (ConstTanBoard)anBoard,
                                            &ci, &ec_default, &es);

    if (rc != 0) {
        LOGE("GeneralCubeDecisionE failed: %d", rc);
        pthread_mutex_unlock(&gnubg_lock);
        return NULL;
    }

    float arDouble[4];
    cubedecision cd = FindCubeDecision(arDouble, aarOutput, &ci);

    pthread_mutex_unlock(&gnubg_lock);

    /* Pack results into IntArray[16] using float bits for equity values */
    jintArray result = (*env)->NewIntArray(env, 16);
    jint buf[16];
    for (int i = 0; i < 7; i++) {
        /* Store float bits as int — Kotlin unpacks with Float.fromBits() */
        int bits;
        memcpy(&bits, &aarOutput[0][i], sizeof(int));
        buf[i] = bits;
        memcpy(&bits, &aarOutput[1][i], sizeof(int));
        buf[7 + i] = bits;
    }
    buf[14] = (jint)cd;
    buf[15] = 0;
    (*env)->SetIntArrayRegion(env, result, 0, 16, buf);
    return result;
}

/*
 * Engine.rollout(board: IntArray, trials: Int): FloatArray?
 *
 * Runs a synchronous cubeful rollout on the given position.
 * Returns FloatArray[14]:
 *   [0..6]  arOutput[0..6]  — equity outputs
 *   [7..13] arStdDev[0..6] — standard deviations
 */
JNIEXPORT jfloatArray JNICALL
Java_com_clavierhaus_gnubg_Engine_rollout(JNIEnv *env, jobject thiz,
                                           jintArray jboard,
                                           jint trials) {
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
        buf[i]     = arOutput[i];
        buf[7 + i] = arStdDev[i];
    }
    (*env)->SetFloatArrayRegion(env, result, 0, 14, buf);
    return result;
}

/* ── SGF Import / Export ─────────────────────────────────────────────────── */
extern void CommandLoadMatch(char *sz);
extern void CommandSaveMatch(char *sz);

JNIEXPORT jboolean JNICALL
Java_com_clavierhaus_gnubg_Engine_loadSGF(JNIEnv *env, jobject thiz, jstring path) {
    const char *szPath = (*env)->GetStringUTFChars(env, path, 0);
    if (!szPath) return JNI_FALSE;

    pthread_mutex_lock(&gnubg_lock);
    
    /* Command handlers tokenize the string in place, so we must pass a mutable copy */
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

/* ── New functions for v0.9.3 one-point game ─────────────────────────────── */

/*
 * Engine.rollDice(): IntArray
 * Returns IntArray[2] with two random dice values (1-6).
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
 * Engine.getLegalMoves(board: IntArray, die0: Int, die1: Int): IntArray
 *
 * Returns a flat IntArray of all legal moves.
 * Each move is 8 ints (anMove[8]); unused slots are -1.
 * Array length = nMoves * 8. Returns empty array if no legal moves.
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

    ensure_tld();

    TanBoard anBoard;
    unpack_board(env, jboard, anBoard);

    /* Use FindBestMove to get the best move — it handles all internal state.
     * We return just this one move as the single legal move for now.
     * Full legal move generation requires match state setup. */
    int anMove[8];
    memset(anMove, -1, sizeof(anMove));

    int rc = FindBestMove(anMove, (int)die0, (int)die1,
                          anBoard, &ci_default, &ec_default,
                          aamfEval);

    pthread_mutex_unlock(&gnubg_lock);

    if (rc != 0) {
        return (*env)->NewIntArray(env, 0);
    }

    /* Return the single best move as an 8-element array */
    jintArray result = (*env)->NewIntArray(env, 8);
    (*env)->SetIntArrayRegion(env, result, 0, 8, (jint *)anMove);
    return result;
}

/*
 * Engine.isGameOver(board: IntArray): Int
 * Returns: 0 = game in progress, 1 = player 1 wins, 2 = player 0 wins
 */
JNIEXPORT jint JNICALL
Java_com_clavierhaus_gnubg_Engine_isGameOver(JNIEnv *env, jobject thiz,
                                               jintArray jboard) {
    pthread_mutex_lock(&gnubg_lock);

    TanBoard anBoard;
    unpack_board(env, jboard, anBoard);

    /* Check if player 1 (anBoard[1]) has all checkers borne off */
    int total1 = 0;
    for (int i = 0; i < 25; i++) total1 += anBoard[1][i];
    if (total1 == 0) {
        pthread_mutex_unlock(&gnubg_lock);
        return 1;  /* player 1 wins */
    }

    /* Check if player 0 (anBoard[0]) has all checkers borne off */
    int total0 = 0;
    for (int i = 0; i < 25; i++) total0 += anBoard[0][i];
    if (total0 == 0) {
        pthread_mutex_unlock(&gnubg_lock);
        return 2;  /* player 0 wins */
    }

    pthread_mutex_unlock(&gnubg_lock);
    return 0;
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
