#include <jni.h>
#include <android/log.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include "gnubg_mobile.h"

#include "config.h"
#include "eval.h"
#include "positionid.h"
#include "backgammon.h"
#include "dice.h"
extern char *FormatMove(char *sz, const TanBoard anBoard, const int anMove[8]);
extern void gnubg_init_tld(void);
extern void gnubg_init_rollout(void);
extern int gnubg_rollout(const TanBoard anBoard, float arOutput[], float arStdDev[], const cubeinfo *pci, rolloutcontext *prc);
extern void ClearMatch(void);
extern int NextTurn(int fPlayNext);
extern int ListCreate(listOLD *pl);
extern int ApplySubMove(TanBoard anBoard, int iSrc, int nRoll, int fCheckLegal);
extern listOLD lMatch;
extern rng rngCurrent;
extern rngcontext *rngctxCurrent;
extern void CommandLoadMatch(char *sz);
extern void CommandSaveMatch(char *sz);
extern void CommandLoadGame(char *sz);
extern void CommandSaveGame(char *sz);
extern void CommandLoadPosition(char *sz);
extern void CommandSavePosition(char *sz);

#define LOG_TAG "gnubg-jni"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

pthread_mutex_t gnubg_lock = PTHREAD_MUTEX_INITIALIZER;
static int             gnubg_initialised = 0;

static evalcontext ec_default = {
    .fCubeful  = 0,
    .nPlies    = 1,
    .fUsePrune = 0,
    .fDeterministic = 1,
    .rNoise    = 0.0f
};

static cubeinfo ci_default;

static int last_engine_dice[2] = {0, 0};

/*
 * Engine.getMatchCubeInfo(): IntArray[3]
 * Returns [fDoubled, fCubeOwner, nCube]
 * fDoubled: 1=cube offered, 0=not
 * fCubeOwner: -1=centred, 0=human, 1=engine
 * nCube: current cube value (1,2,4,8...)
 */
JNIEXPORT jintArray JNICALL
Java_com_clavierhaus_gnubg_Engine_getMatchCubeInfo(JNIEnv *env, jobject thiz) {
    (void)thiz;
    jintArray result = (*env)->NewIntArray(env, 3);
    int c[3];
    gnubg_mobile_get_cube_info(c);
    jint buf[3] = { (jint)c[0], (jint)c[1], (jint)c[2] };
    (*env)->SetIntArrayRegion(env, result, 0, 3, buf);
    return result;
}

JNIEXPORT jintArray JNICALL
Java_com_clavierhaus_gnubg_Engine_getCubeDebugState(JNIEnv *env, jobject thiz) {
    (void)thiz;
    jintArray result = (*env)->NewIntArray(env, 13);
    int s[13];
    gnubg_mobile_get_match_state(s);
    jint buf[13];
    for (int i = 0; i < 13; i++) buf[i] = (jint)s[i];
    (*env)->SetIntArrayRegion(env, result, 0, 13, buf);
    return result;
}



/*
 * Engine.commandDouble(): void — human offers cube
 */
JNIEXPORT void JNICALL
Java_com_clavierhaus_gnubg_Engine_commandDouble(JNIEnv *env, jobject thiz) {
    (void)env;
    (void)thiz;
    (void)gnubg_mobile_command_double();
}
JNIEXPORT jintArray JNICALL
Java_com_clavierhaus_gnubg_Engine_applyHumanDoubleTake(JNIEnv *env, jobject thiz) {
    (void)thiz;

    pthread_mutex_lock(&gnubg_lock);

    if (ms.gs == GAME_PLAYING &&
        ms.fTurn == 0 &&
        ms.fMove == 0 &&
        ms.anDice[0] == 0 &&
        ms.fDoubled == 0 &&
        ms.fCrawford == 0 &&
        ms.fCubeUse &&
        (ms.fCubeOwner < 0 || ms.fCubeOwner == 0)) {

        ms.nCube *= 2;
        ms.fCubeOwner = 1;
        ms.fDoubled = 0;
        ms.fTurn = 0;
        ms.fMove = 0;
        ms.anDice[0] = 0;
        ms.anDice[1] = 0;
    }

    jintArray result = (*env)->NewIntArray(env, 10);
    jint buf[10] = {
        (jint)ms.gs,
        (jint)ms.fTurn,
        (jint)ms.fMove,
        (jint)ms.anDice[0],
        (jint)ms.anDice[1],
        (jint)ms.fDoubled,
        (jint)ms.fCubeOwner,
        (jint)ms.nCube,
        (jint)ms.anScore[0],
        (jint)ms.anScore[1]
    };
    (*env)->SetIntArrayRegion(env, result, 0, 10, buf);

    pthread_mutex_unlock(&gnubg_lock);
    return result;
}

JNIEXPORT void JNICALL
Java_com_clavierhaus_gnubg_Engine_commandTake(JNIEnv *env, jobject thiz) {
    (void)env;
    (void)thiz;
    (void)gnubg_mobile_command_take();
}

/*
 * Engine.commandDrop(): void — human drops engine's double
 */
JNIEXPORT void JNICALL
Java_com_clavierhaus_gnubg_Engine_commandDrop(JNIEnv *env, jobject thiz) {
    (void)env;
    (void)thiz;
    (void)gnubg_mobile_command_drop();
}

/*
 * Engine.getGameResult(): IntArray[2]
 * Returns [fWinner, nPoints] from the game record (plGame->plNext->p).
 * nPoints = nCube * GameStatus: 1=win, 2=gammon, 3=backgammon.
 * fWinner: 0=human, 1=engine. Returns [-1,0] if no result yet.
 */
JNIEXPORT jintArray JNICALL
Java_com_clavierhaus_gnubg_Engine_getGameResult(JNIEnv *env, jobject thiz) {
    (void)thiz;
    jintArray result = (*env)->NewIntArray(env, 2);
    int r[2];
    gnubg_mobile_get_game_result(r);
    jint buf[2] = { (jint)r[0], (jint)r[1] };
    (*env)->SetIntArrayRegion(env, result, 0, 2, buf);
    return result;
}

/*
 * Engine.getMoveRecordDice(): IntArray[2]
 * Returns dice from plLastMove record — persists after TurnDone clears ms.anDice.
 * Mirrors what gnubg GTK reads for board display.
 */
JNIEXPORT jintArray JNICALL
Java_com_clavierhaus_gnubg_Engine_getMoveRecordDice(JNIEnv *env, jobject thiz) {
    (void)thiz;
    jintArray result = (*env)->NewIntArray(env, 2);
    int d[2];
    gnubg_mobile_get_move_record_dice(d);
    jint buf[2] = { (jint)d[0], (jint)d[1] };
    (*env)->SetIntArrayRegion(env, result, 0, 2, buf);
    return result;
}

void gnubg_on_board_changed(void) {
    /* Cache engine dice when engine rolls — ms.anDice cleared by TurnDone after move */
    if (ms.fTurn == 1 && ms.anDice[0] > 0) {
        last_engine_dice[0] = ms.anDice[0];
        last_engine_dice[1] = ms.anDice[1];
        LOGI("cached engine dice: %d,%d", last_engine_dice[0], last_engine_dice[1]);
    }
    LOGI("board_changed: gs=%d fTurn=%d dice=%d,%d",
         ms.gs, ms.fTurn, ms.anDice[0], ms.anDice[1]);
}

/*
 * Engine.getLastEngineDice(): IntArray[2]
 * Returns the dice from the engine's most recent roll, for display purposes.
 */
JNIEXPORT jintArray JNICALL
Java_com_clavierhaus_gnubg_Engine_getLastEngineDice(JNIEnv *env, jobject thiz) {
    jintArray result = (*env)->NewIntArray(env, 2);
    jint buf[2] = { (jint)last_engine_dice[0], (jint)last_engine_dice[1] };
    (*env)->SetIntArrayRegion(env, result, 0, 2, buf);
    return result;
}

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

static void unpack_board(JNIEnv *env, jintArray jboard, TanBoard anBoard) {
    jint buf[50];
    (*env)->GetIntArrayRegion(env, jboard, 0, 50, buf);
    for (int i = 0; i < 25; i++) {
        anBoard[0][i] = (unsigned int)buf[i];
        anBoard[1][i] = (unsigned int)buf[25 + i];
    }
}

JNIEXPORT jboolean JNICALL
Java_com_clavierhaus_gnubg_Engine_initialise(JNIEnv *env, jobject thiz,
                                              jstring jWeightsPath) {
    pthread_mutex_lock(&gnubg_lock);
    if (gnubg_initialised) {
        pthread_mutex_unlock(&gnubg_lock);
        return JNI_TRUE;
    }
    const char *weightsPath = (*env)->GetStringUTFChars(env, jWeightsPath, NULL);
    SetCubeInfo(&ci_default, 1, -1, 0, 0, (int[]){0, 0}, 0, 0, 0, VARIATION_STANDARD);
    EvalInitialise((char *)weightsPath, NULL, 0, NULL);
    (*env)->ReleaseStringUTFChars(env, jWeightsPath, weightsPath);
    rngctxCurrent = InitRNG(NULL, NULL, TRUE, rngCurrent);
    gnubg_init_tld();
    gnubg_init_rollout();
    ListCreate(&lMatch);
    ClearMatch();
    ap[0].pt = PLAYER_HUMAN;
    ap[1].pt = PLAYER_GNU;
    strcpy(ap[0].szName, "Human");
    strcpy(ap[1].szName, "GNU");
    ms.nMatchTo = 1;
    ms.fJacoby  = FALSE;
    fAutoRoll   = FALSE;  /* human rolls manually via UI */
    gnubg_initialised = 1;
    pthread_mutex_unlock(&gnubg_lock);
    LOGI("Engine initialised");
    return JNI_TRUE;
}

JNIEXPORT jintArray JNICALL
Java_com_clavierhaus_gnubg_Engine_newGame(JNIEnv *env, jobject thiz, jint matchLength) {
    (void)thiz;
    (void)gnubg_mobile_start_match((int)matchLength);

    pthread_mutex_lock(&gnubg_lock);
    jintArray result = pack_board(env, ms.anBoard);
    pthread_mutex_unlock(&gnubg_lock);

    return result;
}


JNIEXPORT jintArray JNICALL
Java_com_clavierhaus_gnubg_Engine_nextGame(JNIEnv *env, jobject thiz) {
    (void)thiz;
    (void)gnubg_mobile_next_game();

    pthread_mutex_lock(&gnubg_lock);
    jintArray result = pack_board(env, ms.anBoard);
    pthread_mutex_unlock(&gnubg_lock);

    return result;
}


static char *copy_jstring_or_empty(JNIEnv *env, jstring js) {
    if (!js) return strdup("");
    const char *raw = (*env)->GetStringUTFChars(env, js, 0);
    if (!raw) return strdup("");
    char *copy = strdup(raw);
    (*env)->ReleaseStringUTFChars(env, js, raw);
    return copy ? copy : strdup("");
}

JNIEXPORT void JNICALL
Java_com_clavierhaus_gnubg_Engine_commandNewGame(JNIEnv *env, jobject thiz) {
    (void)env;
    (void)thiz;
    (void)gnubg_mobile_command_new_game();
}

JNIEXPORT void JNICALL
Java_com_clavierhaus_gnubg_Engine_commandNewMatch(JNIEnv *env, jobject thiz, jint matchLength) {
    (void)env;
    (void)thiz;
    (void)gnubg_mobile_command_new_match((int)matchLength);
}

JNIEXPORT void JNICALL
Java_com_clavierhaus_gnubg_Engine_commandNewSession(JNIEnv *env, jobject thiz, jint games) {
    (void)env;
    (void)thiz;
    (void)gnubg_mobile_command_new_session((int)games);
}

JNIEXPORT void JNICALL
Java_com_clavierhaus_gnubg_Engine_commandEndGame(JNIEnv *env, jobject thiz) {
    (void)env;
    (void)thiz;
    (void)gnubg_mobile_command_end_game();
}

JNIEXPORT void JNICALL
Java_com_clavierhaus_gnubg_Engine_commandResign(JNIEnv *env, jobject thiz, jstring value) {
    (void)thiz;
    char *sz = copy_jstring_or_empty(env, value);
    (void)gnubg_mobile_command_resign(sz);
    free(sz);
}

JNIEXPORT void JNICALL
Java_com_clavierhaus_gnubg_Engine_commandNext(JNIEnv *env, jobject thiz, jstring argument) {
    (void)thiz;
    char *sz = copy_jstring_or_empty(env, argument);
    (void)gnubg_mobile_command_next(sz);
    free(sz);
}

JNIEXPORT void JNICALL
Java_com_clavierhaus_gnubg_Engine_commandAccept(JNIEnv *env, jobject thiz) {
    (void)env;
    (void)thiz;
    (void)gnubg_mobile_command_accept();
}

JNIEXPORT void JNICALL
Java_com_clavierhaus_gnubg_Engine_commandReject(JNIEnv *env, jobject thiz) {
    (void)env;
    (void)thiz;
    (void)gnubg_mobile_command_reject();
}

JNIEXPORT void JNICALL
Java_com_clavierhaus_gnubg_Engine_commandDecline(JNIEnv *env, jobject thiz) {
    (void)env;
    (void)thiz;
    (void)gnubg_mobile_command_decline();
}

JNIEXPORT void JNICALL
Java_com_clavierhaus_gnubg_Engine_commandAgree(JNIEnv *env, jobject thiz) {
    (void)env;
    (void)thiz;
    (void)gnubg_mobile_command_agree();
}

JNIEXPORT void JNICALL
Java_com_clavierhaus_gnubg_Engine_commandRedouble(JNIEnv *env, jobject thiz) {
    (void)env;
    (void)thiz;
    (void)gnubg_mobile_command_redouble();
}
JNIEXPORT jintArray JNICALL
Java_com_clavierhaus_gnubg_Engine_rollDice(JNIEnv *env, jobject thiz) {
    (void)thiz;
    (void)gnubg_mobile_command_roll();

    pthread_mutex_lock(&gnubg_lock);
    jint dice[2] = { (jint)ms.anDice[0], (jint)ms.anDice[1] };
    jintArray result = (*env)->NewIntArray(env, 2);
    (*env)->SetIntArrayRegion(env, result, 0, 2, dice);
    pthread_mutex_unlock(&gnubg_lock);

    return result;
}

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

JNIEXPORT jintArray JNICALL
Java_com_clavierhaus_gnubg_Engine_applyMoveString(JNIEnv *env, jobject thiz,
                                                    jstring jmoveStr) {
    (void)thiz;
    const char *moveStr = (*env)->GetStringUTFChars(env, jmoveStr, NULL);
    (void)gnubg_mobile_command_move(moveStr);

    pthread_mutex_lock(&gnubg_lock);
    jintArray result = pack_board(env, ms.anBoard);
    pthread_mutex_unlock(&gnubg_lock);

    (*env)->ReleaseStringUTFChars(env, jmoveStr, moveStr);
    return result;
}

JNIEXPORT jstring JNICALL
Java_com_clavierhaus_gnubg_Engine_formatMove(JNIEnv *env, jobject thiz,
                                              jintArray jboard,
                                              jintArray jmove) {
    (void)thiz;
    jint inBuf[50];
    (*env)->GetIntArrayRegion(env, jboard, 0, 50, inBuf);
    int in[50]; for (int i = 0; i < 50; i++) in[i] = (int)inBuf[i];
    jint moveBuf[8];
    (*env)->GetIntArrayRegion(env, jmove, 0, 8, moveBuf);
    int anMove[8]; for (int i = 0; i < 8; i++) anMove[i] = (int)moveBuf[i];
    char sz[64] = {0};
    gnubg_mobile_format_move(in, anMove, sz, (int)sizeof(sz));
    return (*env)->NewStringUTF(env, sz);
}

/*
 * Engine.applySubMove(board, iSrc, nRoll): IntArray
 * Wraps ApplySubMove(TanBoard, iSrc, nRoll, fCheckLegal) from eval.c.
 * Pure board geometry — no match record, no TurnDone.
 * Returns updated board, or empty array if move is illegal.
 */
JNIEXPORT jintArray JNICALL
Java_com_clavierhaus_gnubg_Engine_applySubMove(JNIEnv *env, jobject thiz,
                                                jintArray jboard,
                                                jint iSrc, jint nRoll) {
    (void)thiz;
    jint inBuf[50];
    (*env)->GetIntArrayRegion(env, jboard, 0, 50, inBuf);
    int in[50], out[50];
    for (int i = 0; i < 50; i++) in[i] = (int)inBuf[i];
    if (gnubg_mobile_apply_sub_move(in, (int)iSrc, (int)nRoll, out) != 1)
        return (*env)->NewIntArray(env, 0);
    jint outBuf[50];
    for (int i = 0; i < 50; i++) outBuf[i] = (jint)out[i];
    jintArray result = (*env)->NewIntArray(env, 50);
    (*env)->SetIntArrayRegion(env, result, 0, 50, outBuf);
    return result;
}

/*
 * Engine.findMove(oldBoard, curBoard, die0, die1): String
 * Mirrors update_move() + Confirm(bd) in gtkboard.c:
 * GenerateMoves on oldBoard, find move whose position key matches curBoard,
 * accept only if cMoves==cMaxMoves && cPips==cMaxPips (maximum dice used).
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
        if (EqualKeys(ml.amMoves[i].key, curKey) &&
            ml.amMoves[i].cMoves == ml.cMaxMoves &&
            ml.amMoves[i].cPips  == ml.cMaxPips) {
            FormatMove(sz, (ConstTanBoard)oldBoard, ml.amMoves[i].anMove);
            break;
        }
    }
    return (*env)->NewStringUTF(env, sz);
}

/*
 * Engine.getMatchWinner(): Int
 * Returns 0 if human (ap[0]) won, 1 if engine (ap[1]) won, -1 if game still playing.
 * Reads ms.anScore — updated by ApplyGameOver in play.c.
 */
/*
 * Engine.getMatchScore(): IntArray[3] — [humanScore, engineScore, matchLength]
 */
JNIEXPORT jintArray JNICALL
Java_com_clavierhaus_gnubg_Engine_getMatchScore(JNIEnv *env, jobject thiz) {
    jintArray result = (*env)->NewIntArray(env, 3);
    int s[3]; gnubg_mobile_get_match_score(s);
    jint buf[3] = { (jint)s[0], (jint)s[1], (jint)s[2] };
    (*env)->SetIntArrayRegion(env, result, 0, 3, buf);
    return result;
}

JNIEXPORT jint JNICALL
Java_com_clavierhaus_gnubg_Engine_getMatchLength(JNIEnv *env, jobject thiz) {
    (void)env; (void)thiz;
    int s[13]; gnubg_mobile_get_match_state(s);
    return (jint)s[12];
}

JNIEXPORT jint JNICALL
Java_com_clavierhaus_gnubg_Engine_getMatchWinner(JNIEnv *env, jobject thiz) {
    (void)env; (void)thiz;
    return (jint)gnubg_mobile_get_match_winner();
}

JNIEXPORT jintArray JNICALL
Java_com_clavierhaus_gnubg_Engine_getMatchBoard(JNIEnv *env, jobject thiz) {
    pthread_mutex_lock(&gnubg_lock);
    jintArray result = pack_board(env, ms.anBoard);
    pthread_mutex_unlock(&gnubg_lock);
    return result;
}

JNIEXPORT jintArray JNICALL
Java_com_clavierhaus_gnubg_Engine_getMatchDice(JNIEnv *env, jobject thiz) {
    pthread_mutex_lock(&gnubg_lock);
    jint dice[2] = { (jint)ms.anDice[0], (jint)ms.anDice[1] };
    jintArray result = (*env)->NewIntArray(env, 2);
    (*env)->SetIntArrayRegion(env, result, 0, 2, dice);
    pthread_mutex_unlock(&gnubg_lock);
    return result;
}

JNIEXPORT jint JNICALL
Java_com_clavierhaus_gnubg_Engine_getMatchTurn(JNIEnv *env, jobject thiz) {
    (void)env; (void)thiz;
    int s[13]; gnubg_mobile_get_match_state(s);
    return (jint)s[1];
}

JNIEXPORT jint JNICALL
Java_com_clavierhaus_gnubg_Engine_getMatchStatus(JNIEnv *env, jobject thiz) {
    (void)env; (void)thiz;
    int s[13]; gnubg_mobile_get_match_state(s);
    return (jint)s[0];
}

JNIEXPORT jintArray JNICALL
Java_com_clavierhaus_gnubg_Engine_pipCount(JNIEnv *env, jobject thiz,
                                            jintArray jboard) {
    (void)thiz;
    jint inBuf[50];
    (*env)->GetIntArrayRegion(env, jboard, 0, 50, inBuf);
    int in[50]; for (int i = 0; i < 50; i++) in[i] = (int)inBuf[i];
    int pips[2];
    gnubg_mobile_pip_count(in, pips);
    jintArray result = (*env)->NewIntArray(env, 2);
    jint buf[2] = { (jint)pips[0], (jint)pips[1] };
    (*env)->SetIntArrayRegion(env, result, 0, 2, buf);
    return result;
}

JNIEXPORT jintArray JNICALL
Java_com_clavierhaus_gnubg_Engine_swapBoard(JNIEnv *env, jobject thiz,
                                             jintArray jboard) {
    (void)thiz;
    jint inBuf[50];
    (*env)->GetIntArrayRegion(env, jboard, 0, 50, inBuf);
    int in[50], out[50];
    for (int i = 0; i < 50; i++) in[i] = (int)inBuf[i];
    gnubg_mobile_swap_board(in, out);
    jint outBuf[50];
    for (int i = 0; i < 50; i++) outBuf[i] = (jint)out[i];
    jintArray result = (*env)->NewIntArray(env, 50);
    (*env)->SetIntArrayRegion(env, result, 0, 50, outBuf);
    return result;
}

JNIEXPORT jfloatArray JNICALL
Java_com_clavierhaus_gnubg_Engine_evaluatePosition(JNIEnv *env, jobject thiz,
                                                    jintArray jboard) {
    pthread_mutex_lock(&gnubg_lock);
    if (!gnubg_initialised) { pthread_mutex_unlock(&gnubg_lock); return NULL; }
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

JNIEXPORT jintArray JNICALL
Java_com_clavierhaus_gnubg_Engine_cubeDecision(JNIEnv *env, jobject thiz,
                                                jintArray jboard,
                                                jint cubeValue, jint cubeOwner,
                                                jint fMove, jint matchTo,
                                                jint score0, jint score1,
                                                jint crawford) {
    pthread_mutex_lock(&gnubg_lock);
    if (!gnubg_initialised) { pthread_mutex_unlock(&gnubg_lock); return NULL; }
    TanBoard anBoard;
    unpack_board(env, jboard, anBoard);
    cubeinfo ci;
    int anScore[2] = { (int)score0, (int)score1 };
    SetCubeInfo(&ci, (int)cubeValue, (int)cubeOwner, (int)fMove, (int)matchTo,
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


JNIEXPORT jboolean JNICALL
Java_com_clavierhaus_gnubg_Engine_loadGame(JNIEnv *env, jobject thiz, jstring path) {
    (void)thiz;
    const char *p = (*env)->GetStringUTFChars(env, path, 0);
    jboolean ok = (p && gnubg_mobile_load_game(p)) ? JNI_TRUE : JNI_FALSE;
    if (p) (*env)->ReleaseStringUTFChars(env, path, p);
    return ok;
}

JNIEXPORT jboolean JNICALL
Java_com_clavierhaus_gnubg_Engine_saveGame(JNIEnv *env, jobject thiz, jstring path) {
    (void)thiz;
    const char *p = (*env)->GetStringUTFChars(env, path, 0);
    jboolean ok = (p && gnubg_mobile_save_game(p)) ? JNI_TRUE : JNI_FALSE;
    if (p) (*env)->ReleaseStringUTFChars(env, path, p);
    return ok;
}

JNIEXPORT jboolean JNICALL
Java_com_clavierhaus_gnubg_Engine_loadMatch(JNIEnv *env, jobject thiz, jstring path) {
    (void)thiz;
    const char *p = (*env)->GetStringUTFChars(env, path, 0);
    jboolean ok = (p && gnubg_mobile_load_match(p)) ? JNI_TRUE : JNI_FALSE;
    if (p) (*env)->ReleaseStringUTFChars(env, path, p);
    return ok;
}

JNIEXPORT jboolean JNICALL
Java_com_clavierhaus_gnubg_Engine_saveMatch(JNIEnv *env, jobject thiz, jstring path) {
    (void)thiz;
    const char *p = (*env)->GetStringUTFChars(env, path, 0);
    jboolean ok = (p && gnubg_mobile_save_match(p)) ? JNI_TRUE : JNI_FALSE;
    if (p) (*env)->ReleaseStringUTFChars(env, path, p);
    return ok;
}

JNIEXPORT jboolean JNICALL
Java_com_clavierhaus_gnubg_Engine_loadPosition(JNIEnv *env, jobject thiz, jstring path) {
    (void)thiz;
    const char *p = (*env)->GetStringUTFChars(env, path, 0);
    jboolean ok = (p && gnubg_mobile_load_position(p)) ? JNI_TRUE : JNI_FALSE;
    if (p) (*env)->ReleaseStringUTFChars(env, path, p);
    return ok;
}

JNIEXPORT jboolean JNICALL
Java_com_clavierhaus_gnubg_Engine_savePosition(JNIEnv *env, jobject thiz, jstring path) {
    (void)thiz;
    const char *p = (*env)->GetStringUTFChars(env, path, 0);
    jboolean ok = (p && gnubg_mobile_save_position(p)) ? JNI_TRUE : JNI_FALSE;
    if (p) (*env)->ReleaseStringUTFChars(env, path, p);
    return ok;
}


JNIEXPORT jboolean JNICALL
Java_com_clavierhaus_gnubg_Engine_loadSGF(JNIEnv *env, jobject thiz, jstring path) {
    (void)thiz;
    /* SGF load currently aliases to match load (unchanged behaviour). */
    const char *p = (*env)->GetStringUTFChars(env, path, 0);
    jboolean ok = (p && gnubg_mobile_load_match(p)) ? JNI_TRUE : JNI_FALSE;
    if (p) (*env)->ReleaseStringUTFChars(env, path, p);
    return ok;
}

JNIEXPORT jboolean JNICALL
Java_com_clavierhaus_gnubg_Engine_saveSGF(JNIEnv *env, jobject thiz, jstring path) {
    (void)thiz;
    /* SGF save currently aliases to match save (unchanged behaviour). */
    const char *p = (*env)->GetStringUTFChars(env, path, 0);
    jboolean ok = (p && gnubg_mobile_save_match(p)) ? JNI_TRUE : JNI_FALSE;
    if (p) (*env)->ReleaseStringUTFChars(env, path, p);
    return ok;
}

static int android_gnubg_command_allowed(const char *cmd) {
    if (!cmd || !*cmd) return 0;

    /*
     * Android Settings bridge, deliberately conservative.
     *
     * This is not a public command shell. It only permits GNUbg configuration
     * commands we intentionally translate from Kotlin settings controls.
     * Game/session actions stay on dedicated JNI wrappers.
     */
    static const char *prefixes[] = {
        "set analysis ",
        "set automatic ",
        "set beavers ",
        "set crawford ",
        "set cube use ",
        "set evaluation ",
        "set gui ",
        "set invert met ",
        "set jacoby ",
        "set match length ",
        "set output ",
        "set player ",
        "set rollout ",
        "set seed ",
        "set tutor ",
        "set warning ",
        "show analysis",
        "show automatic",
        "show crawford",
        "show cube",
        "show evaluation",
        "show gui",
        "show jacoby",
        "show output",
        "show player",
        "show rollout",
        "show seed",
        "show tutor",
        "show warning"
    };

    for (unsigned i = 0; i < sizeof(prefixes) / sizeof(prefixes[0]); ++i) {
        const char *prefix = prefixes[i];
        size_t n = strlen(prefix);
        if (strncmp(cmd, prefix, n) == 0) return 1;
    }

    return 0;
}

extern void HandleCommand(char *sz, command *ac);
extern command acTop[];

JNIEXPORT jboolean JNICALL
Java_com_clavierhaus_gnubg_Engine_runCommand(JNIEnv *env, jobject thiz, jstring command) {
    (void) thiz;

    if (!command) return JNI_FALSE;

    const char *utf = (*env)->GetStringUTFChars(env, command, NULL);
    if (!utf) return JNI_FALSE;

    if (!android_gnubg_command_allowed(utf)) {
        __android_log_print(ANDROID_LOG_WARN, "gnubg-engine",
                            "runCommand rejected: %s", utf);
        (*env)->ReleaseStringUTFChars(env, command, utf);
        return JNI_FALSE;
    }

    size_t len = strlen(utf);
    char *copy = (char *) malloc(len + 1);
    if (!copy) {
        (*env)->ReleaseStringUTFChars(env, command, utf);
        return JNI_FALSE;
    }

    memcpy(copy, utf, len + 1);

    __android_log_print(ANDROID_LOG_INFO, "gnubg-engine",
                        "runCommand: %s", copy);

    HandleCommand(copy, acTop);

    free(copy);
    (*env)->ReleaseStringUTFChars(env, command, utf);
    return JNI_TRUE;
}
