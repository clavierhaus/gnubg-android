#include <pthread.h>
#include <stdio.h>
#include <string.h>

#include "gnubg_mobile.h"

#include "config.h"
#include "eval.h"
#include "positionid.h"
#include "backgammon.h"

/* Tier 1/2 engine entry points (declared in the engine headers above; listed
 * here for the ones native-lib.c also forward-declared). */
extern char *FormatMove(char *sz, const TanBoard anBoard, const int anMove[8]);
extern int   ApplySubMove(TanBoard anBoard, int iSrc, int nRoll, int fCheckLegal);
extern void  SwapSides(TanBoard anBoard);
extern void  CommandLoadGame(char *sz);
extern void  CommandSaveGame(char *sz);
extern void  CommandLoadMatch(char *sz);
extern void  CommandSaveMatch(char *sz);
extern void  CommandLoadPosition(char *sz);
extern void  CommandSavePosition(char *sz);
extern int   gnubg_rollout(const TanBoard anBoard, float arOutput[],
                           float arStdDev[], const cubeinfo *pci,
                           rolloutcontext *prc);

extern void CommandNewGame(char *);
extern void CommandNewMatch(char *);
extern void CommandNewSession(char *);
extern void CommandEndGame(char *);
extern void CommandResign(char *);
extern void CommandNext(char *);
extern void CommandAccept(char *);
extern void CommandReject(char *);
extern void CommandDecline(char *);
extern void CommandAgree(char *);
extern void CommandRedouble(char *);
extern void CommandDouble(char *);
extern void CommandTake(char *);
extern void CommandDrop(char *);
extern void CommandRoll(char *);
extern void CommandMove(char *);
extern int NextTurn(int fPlayNext);
extern void ClearMatch(void);
extern int ListCreate(listOLD *pl);
extern listOLD lMatch;
extern int fNextTurn;
extern pthread_mutex_t gnubg_lock;

const char *gnubg_mobile_facade_version(void) {
    return "gnubg-mobile-facade-v0";
}

static void gnubg_mobile_drain_next_turns(void) {
    while (fNextTurn) {
        NextTurn(TRUE);
    }
}

int gnubg_mobile_command_new_game(void) {
    pthread_mutex_lock(&gnubg_lock);
    CommandNewGame(NULL);
    gnubg_mobile_drain_next_turns();
    pthread_mutex_unlock(&gnubg_lock);

    return 1;
}

int gnubg_mobile_command_new_match(int match_length) {
    char sz_match[16];

    snprintf(sz_match, sizeof(sz_match), "%d", match_length);

    pthread_mutex_lock(&gnubg_lock);
    CommandNewMatch(sz_match);
    gnubg_mobile_drain_next_turns();
    pthread_mutex_unlock(&gnubg_lock);

    return 1;
}

int gnubg_mobile_command_new_session(int games) {
    char sz_games[16];

    snprintf(sz_games, sizeof(sz_games), "%d", games);

    pthread_mutex_lock(&gnubg_lock);
    CommandNewSession(sz_games);
    gnubg_mobile_drain_next_turns();
    pthread_mutex_unlock(&gnubg_lock);

    return 1;
}

int gnubg_mobile_command_end_game(void) {
    pthread_mutex_lock(&gnubg_lock);
    CommandEndGame(NULL);
    gnubg_mobile_drain_next_turns();
    pthread_mutex_unlock(&gnubg_lock);

    return 1;
}

int gnubg_mobile_command_resign(const char *value) {
    pthread_mutex_lock(&gnubg_lock);
    CommandResign((char *)(value ? value : ""));
    gnubg_mobile_drain_next_turns();
    pthread_mutex_unlock(&gnubg_lock);

    return 1;
}

int gnubg_mobile_command_next(const char *argument) {
    pthread_mutex_lock(&gnubg_lock);
    CommandNext((char *)(argument ? argument : ""));
    gnubg_mobile_drain_next_turns();
    pthread_mutex_unlock(&gnubg_lock);

    return 1;
}

int gnubg_mobile_command_accept(void) {
    pthread_mutex_lock(&gnubg_lock);
    CommandAccept(NULL);
    gnubg_mobile_drain_next_turns();
    pthread_mutex_unlock(&gnubg_lock);

    return 1;
}

int gnubg_mobile_command_reject(void) {
    pthread_mutex_lock(&gnubg_lock);
    CommandReject(NULL);
    gnubg_mobile_drain_next_turns();
    pthread_mutex_unlock(&gnubg_lock);

    return 1;
}

int gnubg_mobile_command_decline(void) {
    pthread_mutex_lock(&gnubg_lock);
    CommandDecline(NULL);
    gnubg_mobile_drain_next_turns();
    pthread_mutex_unlock(&gnubg_lock);

    return 1;
}

int gnubg_mobile_command_agree(void) {
    pthread_mutex_lock(&gnubg_lock);
    CommandAgree(NULL);
    gnubg_mobile_drain_next_turns();
    pthread_mutex_unlock(&gnubg_lock);

    return 1;
}

int gnubg_mobile_command_redouble(void) {
    pthread_mutex_lock(&gnubg_lock);
    CommandRedouble(NULL);
    gnubg_mobile_drain_next_turns();
    pthread_mutex_unlock(&gnubg_lock);

    return 1;
}

int gnubg_mobile_command_double(void) {
    pthread_mutex_lock(&gnubg_lock);
    CommandDouble(NULL);
    gnubg_mobile_drain_next_turns();
    pthread_mutex_unlock(&gnubg_lock);

    return 1;
}

int gnubg_mobile_command_roll(void) {
    pthread_mutex_lock(&gnubg_lock);
    CommandRoll(NULL);
    gnubg_mobile_drain_next_turns();
    pthread_mutex_unlock(&gnubg_lock);

    return 1;
}

int gnubg_mobile_command_move(const char *move) {
    pthread_mutex_lock(&gnubg_lock);
    CommandMove((char *)(move ? move : ""));
    NextTurn(TRUE);
    pthread_mutex_unlock(&gnubg_lock);

    return 1;
}

int gnubg_mobile_command_take(void) {
    pthread_mutex_lock(&gnubg_lock);
    CommandTake(NULL);
    gnubg_mobile_drain_next_turns();
    pthread_mutex_unlock(&gnubg_lock);

    return 1;
}

int gnubg_mobile_command_drop(void) {
    pthread_mutex_lock(&gnubg_lock);
    CommandDrop(NULL);
    gnubg_mobile_drain_next_turns();
    pthread_mutex_unlock(&gnubg_lock);

    return 1;
}

int gnubg_mobile_start_match(int match_length) {
    char szMatch[16];

    snprintf(szMatch, sizeof(szMatch), "%d", match_length);

    pthread_mutex_lock(&gnubg_lock);
    ListCreate(&lMatch);
    ClearMatch();
    CommandNewMatch(szMatch);
    CommandNewGame(NULL);
    gnubg_mobile_drain_next_turns();
    pthread_mutex_unlock(&gnubg_lock);

    return 1;
}

int gnubg_mobile_next_game(void) {
    pthread_mutex_lock(&gnubg_lock);
    CommandNext("");
    gnubg_mobile_drain_next_turns();
    pthread_mutex_unlock(&gnubg_lock);

    return 1;
}


/* ===========================================================================
 * Tier 1 — board (un)packing + state readers + board utils + file ops
 * Relocated from native-lib.c so the JNI layer becomes pure marshalling.
 * =========================================================================== */

static void facade_unpack_board(const int in[50], TanBoard anBoard) {
    for (int i = 0; i < 25; i++) {
        anBoard[0][i] = (unsigned int) in[i];
        anBoard[1][i] = (unsigned int) in[25 + i];
    }
}

static void facade_pack_board(ConstTanBoard anBoard, int out[50]) {
    for (int i = 0; i < 25; i++) {
        out[i]      = (int) anBoard[0][i];
        out[25 + i] = (int) anBoard[1][i];
    }
}

int gnubg_mobile_get_board(int out_board[50]) {
    pthread_mutex_lock(&gnubg_lock);
    facade_pack_board((ConstTanBoard) ms.anBoard, out_board);
    pthread_mutex_unlock(&gnubg_lock);
    return 50;
}

int gnubg_mobile_get_match_state(int out_state[13]) {
    pthread_mutex_lock(&gnubg_lock);
    out_state[0]  = (int) ms.gs;
    out_state[1]  = (int) ms.fTurn;
    out_state[2]  = (int) ms.fMove;
    out_state[3]  = (int) ms.anDice[0];
    out_state[4]  = (int) ms.anDice[1];
    out_state[5]  = (int) ms.fDoubled;
    out_state[6]  = (int) ms.fCubeOwner;
    out_state[7]  = (int) ms.nCube;
    out_state[8]  = (int) ms.fCrawford;
    out_state[9]  = (int) ms.fCubeUse;
    out_state[10] = (int) ms.anScore[0];
    out_state[11] = (int) ms.anScore[1];
    out_state[12] = (int) ms.nMatchTo;
    pthread_mutex_unlock(&gnubg_lock);
    return 13;
}

int gnubg_mobile_get_cube_info(int out_cube[3]) {
    pthread_mutex_lock(&gnubg_lock);
    out_cube[0] = (int) ms.fDoubled;
    out_cube[1] = (int) ms.fCubeOwner;
    out_cube[2] = (int) ms.nCube;
    pthread_mutex_unlock(&gnubg_lock);
    return 3;
}

int gnubg_mobile_get_dice(int out_dice[2]) {
    pthread_mutex_lock(&gnubg_lock);
    out_dice[0] = (int) ms.anDice[0];
    out_dice[1] = (int) ms.anDice[1];
    pthread_mutex_unlock(&gnubg_lock);
    return 2;
}

int gnubg_mobile_get_move_record_dice(int out_dice[2]) {
    out_dice[0] = 0;
    out_dice[1] = 0;
    pthread_mutex_lock(&gnubg_lock);
    if (plLastMove && plLastMove->p) {
        moverecord *pmr = (moverecord *) plLastMove->p;
        if (pmr->mt == MOVE_SETDICE || pmr->mt == MOVE_NORMAL) {
            out_dice[0] = (int) pmr->anDice[0];
            out_dice[1] = (int) pmr->anDice[1];
        }
    }
    pthread_mutex_unlock(&gnubg_lock);
    return 2;
}

int gnubg_mobile_get_game_result(int out_result[2]) {
    out_result[0] = -1;
    out_result[1] = 0;
    pthread_mutex_lock(&gnubg_lock);
    if (plGame && plGame->plNext && plGame->plNext->p) {
        moverecord *pmr = (moverecord *) plGame->plNext->p;
        xmovegameinfo *pmgi = &pmr->g;
        if (pmgi->fWinner >= 0) {
            out_result[0] = (int) pmgi->fWinner;
            out_result[1] = (int) pmgi->nPoints;
        }
    }
    pthread_mutex_unlock(&gnubg_lock);
    return 2;
}

int gnubg_mobile_get_match_score(int out_score[3]) {
    pthread_mutex_lock(&gnubg_lock);
    out_score[0] = (int) ms.anScore[0];
    out_score[1] = (int) ms.anScore[1];
    out_score[2] = (int) ms.nMatchTo;
    pthread_mutex_unlock(&gnubg_lock);
    return 3;
}

int gnubg_mobile_get_match_winner(void) {
    int w;
    pthread_mutex_lock(&gnubg_lock);
    if (ms.gs < GAME_OVER)                  w = -1;
    else if (ms.anScore[0] > ms.anScore[1]) w = 0;
    else if (ms.anScore[1] > ms.anScore[0]) w = 1;
    else                                    w = -1;
    pthread_mutex_unlock(&gnubg_lock);
    return w;
}

int gnubg_mobile_swap_board(const int in_board[50], int out_board[50]) {
    TanBoard anBoard;
    facade_unpack_board(in_board, anBoard);
    SwapSides(anBoard);
    facade_pack_board((ConstTanBoard) anBoard, out_board);
    return 50;
}

int gnubg_mobile_pip_count(const int in_board[50], int out_pips[2]) {
    TanBoard anBoard;
    unsigned int anPips[2];
    facade_unpack_board(in_board, anBoard);
    PipCount((ConstTanBoard) anBoard, anPips);
    out_pips[0] = (int) anPips[0];
    out_pips[1] = (int) anPips[1];
    return 2;
}

int gnubg_mobile_apply_sub_move(const int in_board[50], int i_src, int n_roll,
                                int out_board[50]) {
    TanBoard anBoard;
    int rc;
    facade_unpack_board(in_board, anBoard);
    pthread_mutex_lock(&gnubg_lock);
    rc = ApplySubMove(anBoard, i_src, n_roll, TRUE);
    pthread_mutex_unlock(&gnubg_lock);
    facade_pack_board((ConstTanBoard) anBoard, out_board);
    return (rc == 0) ? 1 : 0;
}

int gnubg_mobile_format_move(const int in_board[50], const int in_move[8],
                             char *out_text, int out_capacity) {
    TanBoard anBoard;
    char sz[64] = {0};
    if (out_capacity <= 0) return -1;
    facade_unpack_board(in_board, anBoard);
    FormatMove(sz, (ConstTanBoard) anBoard, in_move);
    snprintf(out_text, (size_t) out_capacity, "%s", sz);
    return (int) strlen(out_text);
}

#define FACADE_FILE_OP(fn, cmd)                  \
    int fn(const char *path) {                   \
        if (!path) return 0;                     \
        pthread_mutex_lock(&gnubg_lock);         \
        cmd((char *) path);                      \
        pthread_mutex_unlock(&gnubg_lock);       \
        return 1;                                \
    }
FACADE_FILE_OP(gnubg_mobile_load_game,     CommandLoadGame)
FACADE_FILE_OP(gnubg_mobile_save_game,     CommandSaveGame)
FACADE_FILE_OP(gnubg_mobile_load_match,    CommandLoadMatch)
FACADE_FILE_OP(gnubg_mobile_save_match,    CommandSaveMatch)
FACADE_FILE_OP(gnubg_mobile_load_position, CommandLoadPosition)
FACADE_FILE_OP(gnubg_mobile_save_position, CommandSavePosition)
#undef FACADE_FILE_OP

/* ===========================================================================
 * Tier 2 — engine algorithms relocated whole from native-lib.c.
 * Defaults mirror native-lib.c's ec_default/ci_default; set ci in initialise().
 * =========================================================================== */

static evalcontext fac_ec_default = {
    .fCubeful = 0, .nPlies = 1, .fUsePrune = 0, .fDeterministic = 1, .rNoise = 0.0f
};
static cubeinfo fac_ci_default;   /* set in gnubg_mobile_initialise() */

int gnubg_mobile_get_legal_moves(const int board[50], int d0, int d1,
                                 int f_partial, int *out_moves, int out_cap) {
    TanBoard anBoard;
    movelist ml;
    int n, i, j, written = 0;
    facade_unpack_board(board, anBoard);
    pthread_mutex_lock(&gnubg_lock);
    memset(&ml, 0, sizeof(ml));
    GenerateMoves(&ml, (ConstTanBoard) anBoard, d0, d1, f_partial);
    n = (int) ml.cMoves;
    for (i = 0; i < n && (written + 8) <= out_cap; i++)
        for (j = 0; j < 8; j++)
            out_moves[written++] = ml.amMoves[i].anMove[j];
    pthread_mutex_unlock(&gnubg_lock);
    return i;
}

int gnubg_mobile_find_move(const int old_board[50], const int cur_board[50],
                           int d0, int d1, char *out_text, int out_cap) {
    TanBoard oldB, curB;
    movelist ml;
    positionkey curKey;
    char sz[64] = {0};
    unsigned int i;
    if (out_cap <= 0) return -1;
    facade_unpack_board(old_board, oldB);
    facade_unpack_board(cur_board, curB);
    pthread_mutex_lock(&gnubg_lock);
    memset(&ml, 0, sizeof(ml));
    GenerateMoves(&ml, (ConstTanBoard) oldB, d0, d1, FALSE);
    PositionKey((ConstTanBoard) curB, &curKey);
    for (i = 0; i < ml.cMoves; i++) {
        if (EqualKeys(ml.amMoves[i].key, curKey) &&
            ml.amMoves[i].cMoves == ml.cMaxMoves &&
            ml.amMoves[i].cPips  == ml.cMaxPips) {
            FormatMove(sz, (ConstTanBoard) oldB, ml.amMoves[i].anMove);
            break;
        }
    }
    pthread_mutex_unlock(&gnubg_lock);
    snprintf(out_text, (size_t) out_cap, "%s", sz);
    return (int) strlen(out_text);
}

int gnubg_mobile_evaluate(const int board[50], float *out, int out_cap) {
    TanBoard anBoard;
    float arOutput[NUM_OUTPUTS] = {0};
    int rc, i;
    if (out_cap < NUM_OUTPUTS) return -1;
    facade_unpack_board(board, anBoard);
    pthread_mutex_lock(&gnubg_lock);
    rc = EvaluatePosition(NULL, (ConstTanBoard) anBoard, arOutput,
                          &fac_ci_default, &fac_ec_default);
    pthread_mutex_unlock(&gnubg_lock);
    if (rc != 0) return -1;
    for (i = 0; i < NUM_OUTPUTS; i++) out[i] = arOutput[i];
    return NUM_OUTPUTS;
}

int gnubg_mobile_classify(const int board[50]) {
    TanBoard anBoard;
    positionclass pc;
    facade_unpack_board(board, anBoard);
    pthread_mutex_lock(&gnubg_lock);
    pc = ClassifyPosition((ConstTanBoard) anBoard, VARIATION_STANDARD);
    pthread_mutex_unlock(&gnubg_lock);
    return (int) pc;
}

int gnubg_mobile_cube_decision(const int board[50], int cube_value,
                               int cube_owner, int f_move, int match_to,
                               int score0, int score1, int crawford,
                               float *out, int out_cap, int *out_decision) {
    TanBoard anBoard;
    cubeinfo ci;
    int anScore[2];
    evalsetup es;
    float aarOutput[2][NUM_ROLLOUT_OUTPUTS];
    float arDouble[4];
    cubedecision cd;
    int rc, i;
    if (out_cap < 14) return -1;
    facade_unpack_board(board, anBoard);
    anScore[0] = score0; anScore[1] = score1;
    pthread_mutex_lock(&gnubg_lock);
    SetCubeInfo(&ci, cube_value, cube_owner, f_move, match_to,
                anScore, crawford, 0, 0, VARIATION_STANDARD);
    memset(&es, 0, sizeof(es));
    es.et = EVAL_EVAL;
    es.ec = fac_ec_default;
    memset(aarOutput, 0, sizeof(aarOutput));
    rc = GeneralCubeDecisionENoLocking(aarOutput, (ConstTanBoard) anBoard,
                                       &ci, &fac_ec_default, &es);
    if (rc != 0) { pthread_mutex_unlock(&gnubg_lock); return -1; }
    cd = FindCubeDecision(arDouble, aarOutput, &ci);
    pthread_mutex_unlock(&gnubg_lock);
    for (i = 0; i < 7; i++) { out[i] = aarOutput[0][i]; out[7 + i] = aarOutput[1][i]; }
    if (out_decision) *out_decision = (int) cd;
    return 14;
}

int gnubg_mobile_rollout(const int board[50], int trials,
                         float *out, int out_cap) {
    TanBoard anBoard;
    rolloutcontext rc_ro;
    float arOutput[NUM_ROLLOUT_OUTPUTS] = {0};
    float arStdDev[NUM_ROLLOUT_OUTPUTS] = {0};
    int ret, i;
    if (out_cap < 14) return -1;
    facade_unpack_board(board, anBoard);
    pthread_mutex_lock(&gnubg_lock);
    rc_ro = rcRollout;
    rc_ro.nTrials  = (unsigned int) (trials > 0 ? trials : 144);
    rc_ro.fCubeful = 1;
    rc_ro.fVarRedn = 1;
    ret = gnubg_rollout((ConstTanBoard) anBoard, arOutput, arStdDev,
                        &fac_ci_default, &rc_ro);
    pthread_mutex_unlock(&gnubg_lock);
    if (ret != 0) return -1;
    for (i = 0; i < 7; i++) { out[i] = arOutput[i]; out[7 + i] = arStdDev[i]; }
    return 14;
}
