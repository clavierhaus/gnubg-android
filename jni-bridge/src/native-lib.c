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

#define LOG_TAG "gnubg-jni"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

pthread_mutex_t gnubg_lock = PTHREAD_MUTEX_INITIALIZER;
static int             gnubg_initialised = 0;

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
 * Engine.commandDouble(): void -- human offers cube
 */
JNIEXPORT void JNICALL
Java_com_clavierhaus_gnubg_Engine_commandDouble(JNIEnv *env, jobject thiz) {
    (void)env;
    (void)thiz;
    (void)gnubg_mobile_command_double();
}

/*
 * Engine.canDouble(): Boolean -- 1 iff a CommandDouble would succeed in
 * the current matchstate. Mirrors CommandDouble's preconditions (play.c:2369)
 * minus the desktop-only move_not_last_in_match_ok prompt. See
 * engine-core/play.c gnubg_can_double() + PROVENANCE.md.
 */
JNIEXPORT jboolean JNICALL
Java_com_clavierhaus_gnubg_Engine_canDouble(JNIEnv *env, jobject thiz) {
    (void)env;
    (void)thiz;
    if (!gnubg_initialised) return JNI_FALSE;
    return gnubg_mobile_can_double() ? JNI_TRUE : JNI_FALSE;
}
JNIEXPORT void JNICALL
Java_com_clavierhaus_gnubg_Engine_commandTake(JNIEnv *env, jobject thiz) {
    (void)env;
    (void)thiz;
    (void)gnubg_mobile_command_take();
}

/*
 * Engine.commandDrop(): void -- human drops engine's double
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
 * Returns dice from plLastMove record -- persists after TurnDone clears ms.anDice.
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
    /* Display-layer cache: copy ms.anDice before TurnDone clears it, so the
     * UI can show what gnubg just rolled. Never written back into ms.anDice
     * (traced in audit V7). Player-type check replaces previous hardcoded
     * fTurn==1 -- mirrors gnubg pattern at play.c:1316. */
    if (ap[ms.fTurn].pt != PLAYER_HUMAN && ms.anDice[0] > 0) {
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

/* getLegalMoves: wraps gnubg_mobile_get_legal_moves. Returns a flat int array of
 * n*8 ints (each move is 8 ints: 4 (src,dst) sub-move pairs, per gnubg anMove).
 * A position can have a few hundred legal moves at most; MAX_LEGAL_MOVES*8
 * bounds the buffer. */
JNIEXPORT jintArray JNICALL
Java_com_clavierhaus_gnubg_Engine_getLegalMoves(JNIEnv *env, jobject thiz,
                                                  jintArray jboard,
                                                  jint die0, jint die1,
                                                  jint fPartial) {
    (void)thiz;
    if (!gnubg_initialised) return (*env)->NewIntArray(env, 0);
    jint inBuf[50];
    (*env)->GetIntArrayRegion(env, jboard, 0, 50, inBuf);
    int in[50]; for (int i = 0; i < 50; i++) in[i] = (int)inBuf[i];
    /* generous cap: gnubg never exceeds a few thousand sub-moves */
    enum { CAP = 8 * 4096 };
    int *moves = (int *)malloc(CAP * sizeof(int));
    if (!moves) return (*env)->NewIntArray(env, 0);
    int nMoves = gnubg_mobile_get_legal_moves(in, (int)die0, (int)die1,
                                              (int)fPartial, moves, CAP);
    if (nMoves < 0) nMoves = 0;
    int nInts = nMoves * 8;
    jintArray result = (*env)->NewIntArray(env, nInts);
    if (nInts > 0) {
        jint *buf = (jint *)malloc(nInts * sizeof(jint));
        for (int i = 0; i < nInts; i++) buf[i] = (jint)moves[i];
        (*env)->SetIntArrayRegion(env, result, 0, nInts, buf);
        free(buf);
    }
    free(moves);
    return result;
}

JNIEXPORT jboolean JNICALL
Java_com_clavierhaus_gnubg_Engine_initialise(JNIEnv *env, jobject thiz,
                                              jstring jWeightsPath) {
    (void)thiz;
    pthread_mutex_lock(&gnubg_lock);
    if (gnubg_initialised) {
        pthread_mutex_unlock(&gnubg_lock);
        return JNI_TRUE;
    }
    pthread_mutex_unlock(&gnubg_lock);

    const char *weightsPath = (*env)->GetStringUTFChars(env, jWeightsPath, NULL);
    gnubg_mobile_initialise(weightsPath);
    (*env)->ReleaseStringUTFChars(env, jWeightsPath, weightsPath);

    pthread_mutex_lock(&gnubg_lock);
    gnubg_initialised = 1;
    pthread_mutex_unlock(&gnubg_lock);

    LOGI("Engine initialised");
    return JNI_TRUE;
}

JNIEXPORT jintArray JNICALL
Java_com_clavierhaus_gnubg_Engine_newGame(JNIEnv *env, jobject thiz, jint matchLength) {
    (void)thiz;
    (void)gnubg_mobile_start_match((int)matchLength);

    int b[50];
    gnubg_mobile_get_board(b);
    jintArray result = (*env)->NewIntArray(env, 50);
    jint buf[50];
    for (int i = 0; i < 50; i++) buf[i] = (jint)b[i];
    (*env)->SetIntArrayRegion(env, result, 0, 50, buf);

    return result;
}


JNIEXPORT jintArray JNICALL
Java_com_clavierhaus_gnubg_Engine_nextGame(JNIEnv *env, jobject thiz) {
    (void)thiz;
    (void)gnubg_mobile_next_game();

    int b[50];
    gnubg_mobile_get_board(b);
    jintArray result = (*env)->NewIntArray(env, 50);
    jint buf[50];
    for (int i = 0; i < 50; i++) buf[i] = (jint)b[i];
    (*env)->SetIntArrayRegion(env, result, 0, 50, buf);

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
Java_com_clavierhaus_gnubg_Engine_commandPrevious(JNIEnv *env, jobject thiz, jstring argument) {
    (void)thiz;
    char *sz = copy_jstring_or_empty(env, argument);
    (void)gnubg_mobile_command_previous(sz);
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

    int d[2];
    gnubg_mobile_get_dice(d);
    jint dice[2] = { (jint)d[0], (jint)d[1] };
    jintArray result = (*env)->NewIntArray(env, 2);
    (*env)->SetIntArrayRegion(env, result, 0, 2, dice);

    return result;
}


JNIEXPORT jintArray JNICALL
Java_com_clavierhaus_gnubg_Engine_applyMoveString(JNIEnv *env, jobject thiz,
                                                    jstring jmoveStr) {
    (void)thiz;
    const char *moveStr = (*env)->GetStringUTFChars(env, jmoveStr, NULL);
    (void)gnubg_mobile_command_move(moveStr);

    int b[50];
    gnubg_mobile_get_board(b);
    jintArray result = (*env)->NewIntArray(env, 50);
    jint buf[50];
    for (int i = 0; i < 50; i++) buf[i] = (jint)b[i];
    (*env)->SetIntArrayRegion(env, result, 0, 50, buf);

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
 * Pure board geometry -- no match record, no TurnDone.
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
    (void)thiz;
    jint oldBuf[50], curBuf[50];
    (*env)->GetIntArrayRegion(env, joldBoard, 0, 50, oldBuf);
    (*env)->GetIntArrayRegion(env, jcurBoard, 0, 50, curBuf);
    int oldB[50], curB[50];
    for (int i = 0; i < 50; i++) { oldB[i] = (int)oldBuf[i]; curB[i] = (int)curBuf[i]; }
    char sz[64] = {0};
    gnubg_mobile_find_move(oldB, curB, (int)die0, (int)die1, sz, (int)sizeof(sz));
    return (*env)->NewStringUTF(env, sz);
}

/*
 * Engine.setEngineStrength(idx): void
 * idx: 0=Beginner 1=Casual play 2=Intermediate 3=Advanced (gnubg SETTINGS_*).
 */
JNIEXPORT void JNICALL
Java_com_clavierhaus_gnubg_Engine_setEngineStrength(JNIEnv *env, jobject thiz, jint idx) {
    (void)env; (void)thiz;
    gnubg_mobile_set_engine_strength((int)idx);
}

JNIEXPORT void JNICALL
Java_com_clavierhaus_gnubg_Engine_setAutoCrawford(JNIEnv *env, jobject thiz, jboolean on) {
    (void)env; (void)thiz;
    gnubg_mobile_set_auto_crawford(on ? 1 : 0);
}

JNIEXPORT void JNICALL
Java_com_clavierhaus_gnubg_Engine_setJacoby(JNIEnv *env, jobject thiz, jboolean on) {
    (void)env; (void)thiz;
    gnubg_mobile_set_jacoby(on ? 1 : 0);
}

JNIEXPORT void JNICALL
Java_com_clavierhaus_gnubg_Engine_setCubeUse(JNIEnv *env, jobject thiz, jboolean on) {
    (void)env; (void)thiz;
    gnubg_mobile_set_cube_use(on ? 1 : 0);
}

JNIEXPORT void JNICALL
Java_com_clavierhaus_gnubg_Engine_setMet(JNIEnv *env, jobject thiz, jstring jpath) {
    (void)thiz;
    const char *path = (*env)->GetStringUTFChars(env, jpath, NULL);
    gnubg_mobile_set_met(path);
    (*env)->ReleaseStringUTFChars(env, jpath, path);
}

JNIEXPORT void JNICALL
Java_com_clavierhaus_gnubg_Engine_setAutoDoubles(JNIEnv *env, jobject thiz, jint n) {
    (void)env; (void)thiz;
    gnubg_mobile_set_auto_doubles((int)n);
}

JNIEXPORT void JNICALL
Java_com_clavierhaus_gnubg_Engine_setBeavers(JNIEnv *env, jobject thiz, jint n) {
    (void)env; (void)thiz;
    gnubg_mobile_set_beavers((int)n);
}

/*
 * Engine.tutorAnalyze(oldBoard): IntArray
 * Wraps gnubg_mobile_tutor_analyze. Call AFTER applyMoveString.
 * Returns IntArray[52]: [0]=played equity bits, [1]=best equity bits,
 * [2..51]=best-move board. Empty array if no analyzable move.
 */
JNIEXPORT jfloatArray JNICALL
Java_com_clavierhaus_gnubg_Engine_analyzePlayedMove(JNIEnv *env, jobject thiz,
                                                    jintArray joldBoard) {
    (void)thiz;
    jint oldBuf[50];
    (*env)->GetIntArrayRegion(env, joldBoard, 0, 50, oldBuf);
    int oldB[50];
    for (int i = 0; i < 50; i++) oldB[i] = (int)oldBuf[i];
    float out[7] = {0};
    int rc = gnubg_mobile_analyze_played_move(oldB, out);
    if (rc < 7) return (*env)->NewFloatArray(env, 0);
    jfloatArray result = (*env)->NewFloatArray(env, 7);
    (*env)->SetFloatArrayRegion(env, result, 0, 7, out);
    return result;
}

JNIEXPORT jintArray JNICALL
Java_com_clavierhaus_gnubg_Engine_reviewVerdict(JNIEnv *env, jobject thiz) {
    (void)thiz;
    int out[71] = {0};
    int rc = gnubg_mobile_review_verdict(out);
    if (rc < 1) return (*env)->NewIntArray(env, 0);
    jintArray result = (*env)->NewIntArray(env, 71);
    jint buf[71];
    for (int i = 0; i < 71; i++) buf[i] = (jint)out[i];
    (*env)->SetIntArrayRegion(env, result, 0, 71, buf);
    return result;
}

JNIEXPORT jintArray JNICALL
Java_com_clavierhaus_gnubg_Engine_tutorAnalyze(JNIEnv *env, jobject thiz,
                                                jintArray joldBoard) {
    (void)thiz;
    jint oldBuf[50];
    (*env)->GetIntArrayRegion(env, joldBoard, 0, 50, oldBuf);
    int oldB[50];
    for (int i = 0; i < 50; i++) oldB[i] = (int)oldBuf[i];
    int out[52] = {0};
    int rc = gnubg_mobile_tutor_analyze(oldB, out);
    if (rc < 1) return (*env)->NewIntArray(env, 0);
    jintArray result = (*env)->NewIntArray(env, 52);
    jint buf[52];
    for (int i = 0; i < 52; i++) buf[i] = (jint)out[i];
    (*env)->SetIntArrayRegion(env, result, 0, 52, buf);
    return result;
}

JNIEXPORT jint JNICALL
Java_com_clavierhaus_gnubg_Engine_skill(JNIEnv *env, jobject thiz, jfloat equityDelta) {
    (void)env; (void)thiz;
    return (jint) gnubg_mobile_skill((float) equityDelta);
}

JNIEXPORT jfloatArray JNICALL
Java_com_clavierhaus_gnubg_Engine_positionFeatures(JNIEnv *env, jobject thiz,
                                                    jintArray jboard) {
    (void)thiz;
    jint inBuf[50];
    (*env)->GetIntArrayRegion(env, jboard, 0, 50, inBuf);
    int b[50];
    for (int i = 0; i < 50; i++) b[i] = (int)inBuf[i];
    float out[2 * MORE_INPUTS];
    int rc = gnubg_mobile_position_features(b, out);
    if (rc < 1) return (*env)->NewFloatArray(env, 0);
    jfloatArray result = (*env)->NewFloatArray(env, rc);
    jfloat fbuf[2 * MORE_INPUTS];
    for (int i = 0; i < rc; i++) fbuf[i] = (jfloat)out[i];
    (*env)->SetFloatArrayRegion(env, result, 0, rc, fbuf);
    return result;
}

/*
 * Engine.getMatchWinner(): Int
 * Returns 0 if human (ap[0]) won, 1 if engine (ap[1]) won, -1 if game still playing.
 * Reads ms.anScore -- updated by ApplyGameOver in play.c.
 */
/*
 * Engine.getMatchScore(): IntArray[3] -- [humanScore, engineScore, matchLength]
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
    int b[50];
    gnubg_mobile_get_board(b);
    jintArray result = (*env)->NewIntArray(env, 50);
    jint buf[50];
    for (int i = 0; i < 50; i++) buf[i] = (jint)b[i];
    (*env)->SetIntArrayRegion(env, result, 0, 50, buf);
    return result;
}


/* Board in a stable human (player-0) frame, orientation resolved atomically
 * under the engine lock. Use this for DISPLAY -- it never flips mid-game. */
JNIEXPORT jintArray JNICALL
Java_com_clavierhaus_gnubg_Engine_getMatchBoardHuman(JNIEnv *env, jobject thiz) {
    (void)thiz;
    int b[50];
    gnubg_mobile_get_board_human(b);
    jintArray result = (*env)->NewIntArray(env, 50);
    jint buf[50];
    for (int i = 0; i < 50; i++) buf[i] = (jint)b[i];
    (*env)->SetIntArrayRegion(env, result, 0, 50, buf);
    return result;
}

JNIEXPORT jintArray JNICALL
Java_com_clavierhaus_gnubg_Engine_getMatchDice(JNIEnv *env, jobject thiz) {
    (void)thiz;
    int d[2];
    gnubg_mobile_get_dice(d);
    jintArray result = (*env)->NewIntArray(env, 2);
    jint dice[2] = { (jint)d[0], (jint)d[1] };
    (*env)->SetIntArrayRegion(env, result, 0, 2, dice);
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
    (void)thiz;
    if (!gnubg_initialised) return NULL;
    jint inBuf[50];
    (*env)->GetIntArrayRegion(env, jboard, 0, 50, inBuf);
    int in[50]; for (int i = 0; i < 50; i++) in[i] = (int)inBuf[i];
    float arOutput[NUM_OUTPUTS] = {0};
    if (gnubg_mobile_evaluate(in, arOutput, NUM_OUTPUTS) < 0) return NULL;
    jfloatArray result = (*env)->NewFloatArray(env, NUM_OUTPUTS);
    (*env)->SetFloatArrayRegion(env, result, 0, NUM_OUTPUTS, arOutput);
    return result;
}

JNIEXPORT jint JNICALL
Java_com_clavierhaus_gnubg_Engine_classifyPosition(JNIEnv *env, jobject thiz,
                                                    jintArray jboard) {
    (void)thiz;
    if (!gnubg_initialised) return -1;
    jint inBuf[50];
    (*env)->GetIntArrayRegion(env, jboard, 0, 50, inBuf);
    int in[50]; for (int i = 0; i < 50; i++) in[i] = (int)inBuf[i];
    return (jint)gnubg_mobile_classify(in);
}

JNIEXPORT jintArray JNICALL
Java_com_clavierhaus_gnubg_Engine_cubeDecision(JNIEnv *env, jobject thiz,
                                                jintArray jboard) {
    (void)thiz;
    if (!gnubg_initialised) return NULL;
    jint inBuf[50];
    (*env)->GetIntArrayRegion(env, jboard, 0, 50, inBuf);
    int in[50]; for (int i = 0; i < 50; i++) in[i] = (int)inBuf[i];
    float out[18] = {0};
    int decision = 0;
    if (gnubg_mobile_cube_decision(in, out, 18, &decision) < 0)
        return NULL;
    jintArray result = (*env)->NewIntArray(env, 20);
    jint buf[20];
    for (int i = 0; i < 7; i++) {
        int bits;
        memcpy(&bits, &out[i],     sizeof(int)); buf[i]     = bits;
        memcpy(&bits, &out[7 + i], sizeof(int)); buf[7 + i] = bits;
    }
    /* arDouble[0..3] (OPTIMAL, NODOUBLE, TAKE, DROP) at indices 14..17 */
    for (int i = 0; i < 4; i++) {
        int bits;
        memcpy(&bits, &out[14 + i], sizeof(int)); buf[14 + i] = bits;
    }
    buf[18] = (jint)decision; buf[19] = 0;
    (*env)->SetIntArrayRegion(env, result, 0, 20, buf);
    return result;
}

JNIEXPORT jfloatArray JNICALL
Java_com_clavierhaus_gnubg_Engine_rollout(JNIEnv *env, jobject thiz,
                                           jintArray jboard, jint trials) {
    (void)thiz;
    if (!gnubg_initialised) return NULL;
    jint inBuf[50];
    (*env)->GetIntArrayRegion(env, jboard, 0, 50, inBuf);
    int in[50]; for (int i = 0; i < 50; i++) in[i] = (int)inBuf[i];
    float out[14] = {0};
    if (gnubg_mobile_rollout(in, (int)trials, out, 14) < 0) return NULL;
    jfloatArray result = (*env)->NewFloatArray(env, 14);
    (*env)->SetFloatArrayRegion(env, result, 0, 14, out);
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

/* ---------------------------------------------------------------------------
 * Position entry (Analyse Position). Marshalling only; all behaviour lives in
 * the facade, which wraps gnubg's SetGNUbgID / CommandSwapPlayers / PositionID
 * / MatchIDFromMatchState.
 * ------------------------------------------------------------------------- */

/* Engine.setGnubgId(id): Int -- gnubg's own return code, passed through.
 * 0 installed, 1 no valid IDs found, 2 installed but the player on roll is on
 * top (the UI must offer a swap), -1 bad argument. */
JNIEXPORT jstring JNICALL
Java_com_clavierhaus_gnubg_Engine_idsFromState(JNIEnv *env, jobject thiz,
        jintArray jboard, jint d0, jint d1, jint turn,
        jint scoreH, jint scoreE, jint matchTo,
        jint cube, jint cubeOwner, jint crawford) {
    (void)thiz;
    jint inBuf[50];
    int in[50], i;
    char out[64];
    (*env)->GetIntArrayRegion(env, jboard, 0, 50, inBuf);
    for (i = 0; i < 50; i++) in[i] = (int) inBuf[i];
    if (gnubg_mobile_ids_from_state(in, (int) d0, (int) d1, (int) turn,
                                    (int) scoreH, (int) scoreE, (int) matchTo,
                                    (int) cube, (int) cubeOwner, (int) crawford,
                                    out, (int) sizeof(out)) < 0)
        return NULL;
    return (*env)->NewStringUTF(env, out);
}

JNIEXPORT jstring JNICALL
Java_com_clavierhaus_gnubg_Engine_cubeRecommendation(JNIEnv *env, jobject thiz, jint cd) {
    (void)thiz;
    char out[96];
    if (gnubg_mobile_cube_recommendation((int) cd, out, (int) sizeof(out)) < 0)
        return NULL;
    return (*env)->NewStringUTF(env, out);
}

JNIEXPORT jint JNICALL
Java_com_clavierhaus_gnubg_Engine_setGnubgId(JNIEnv *env, jobject thiz, jstring jid) {
    (void) thiz;
    const char *id;
    jint rc;
    if (!jid) return (jint) -1;
    id = (*env)->GetStringUTFChars(env, jid, NULL);
    if (!id) return (jint) -1;
    rc = (jint) gnubg_mobile_set_gnubg_id(id);
    (*env)->ReleaseStringUTFChars(env, jid, id);
    return rc;
}

/* Engine.swapPlayers(): Int -- the user's yes to the swap offered on rc == 2. */
JNIEXPORT jint JNICALL
Java_com_clavierhaus_gnubg_Engine_swapPlayers(JNIEnv *env, jobject thiz) {
    (void) env; (void) thiz;
    return (jint) gnubg_mobile_swap_players();
}

/* Engine.currentIds(): Array<String>? -- [0] = Position ID, [1] = Match ID.
 * Both are gnubg's own renderings of the current state. L_POSITIONID is 14 and
 * L_MATCHID is 12; the buffers are generous. Returns null on failure. */
JNIEXPORT jobjectArray JNICALL
Java_com_clavierhaus_gnubg_Engine_currentIds(JNIEnv *env, jobject thiz) {
    (void) thiz;
    char pos[64] = {0};
    char match[64] = {0};
    jclass strCls;
    jobjectArray arr;

    if (gnubg_mobile_current_ids(pos, (int) sizeof(pos),
                                 match, (int) sizeof(match)) < 0)
        return NULL;

    strCls = (*env)->FindClass(env, "java/lang/String");
    if (!strCls) return NULL;
    arr = (*env)->NewObjectArray(env, 2, strCls, NULL);
    if (!arr) return NULL;
    (*env)->SetObjectArrayElement(env, arr, 0, (*env)->NewStringUTF(env, pos));
    (*env)->SetObjectArrayElement(env, arr, 1, (*env)->NewStringUTF(env, match));
    return arr;
}

/* Engine.hintMoves(maxN, outEquity, outMoves): Int
 * Marshalling only. outEquity must hold maxN floats, outMoves maxN*8 ints.
 * Returns the number of candidates written, 0 when the position has no dice,
 * or -1 on error. gnubg ranks them; the order is its own. */
JNIEXPORT jint JNICALL
Java_com_clavierhaus_gnubg_Engine_hintMoves(JNIEnv *env, jobject thiz, jint maxN,
                                           jfloatArray jEquity, jintArray jMoves) {
    (void) thiz;
    float *eq;
    int *mv;
    jint *jbuf;
    int n, i;

    if (maxN < 1 || !jEquity || !jMoves) return (jint) -1;
    if ((*env)->GetArrayLength(env, jEquity) < maxN) return (jint) -1;
    if ((*env)->GetArrayLength(env, jMoves) < maxN * 8) return (jint) -1;

    eq = (float *) malloc(sizeof(float) * (size_t) maxN);
    mv = (int *) malloc(sizeof(int) * (size_t) maxN * 8);
    if (!eq || !mv) { free(eq); free(mv); return (jint) -1; }

    n = gnubg_mobile_hint_moves((int) maxN, eq, mv);

    if (n > 0) {
        (*env)->SetFloatArrayRegion(env, jEquity, 0, n, (const jfloat *) eq);
        jbuf = (jint *) malloc(sizeof(jint) * (size_t) n * 8);
        if (jbuf) {
            for (i = 0; i < n * 8; i++) jbuf[i] = (jint) mv[i];
            (*env)->SetIntArrayRegion(env, jMoves, 0, n * 8, jbuf);
            free(jbuf);
        } else {
            n = -1;
        }
    }

    free(eq);
    free(mv);
    return (jint) n;
}

/* Engine.getMatchState(): IntArray(13) -- one consistent snapshot under one
 * lock, rather than four separate getters that can tear. Marshalling only.
 * Layout is the facade's: [0] gs, [1] fTurn, [2] fMove, [3] dice0, [4] dice1,
 * [5] fDoubled, [6] fCubeOwner, [7] nCube, [8] fCrawford, [9] fCubeUse,
 * [10] score0, [11] score1, [12] nMatchTo. */
JNIEXPORT jintArray JNICALL
Java_com_clavierhaus_gnubg_Engine_getMatchState(JNIEnv *env, jobject thiz) {
    (void) thiz;
    int st[13];
    jint buf[13];
    int i;
    jintArray result;
    gnubg_mobile_get_match_state(st);
    for (i = 0; i < 13; i++) buf[i] = (jint) st[i];
    result = (*env)->NewIntArray(env, 13);
    if (!result) return NULL;
    (*env)->SetIntArrayRegion(env, result, 0, 13, buf);
    return result;
}

/* Resignation. Marshalling only. */
JNIEXPORT jint JNICALL
Java_com_clavierhaus_gnubg_Engine_getResignation(JNIEnv *env, jobject thiz) {
    (void) env; (void) thiz;
    return (jint) gnubg_mobile_get_resignation();
}
