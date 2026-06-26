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
extern void gnubg_set_computer_decision(int f);  /* play.c seam: lets CommandTake/Drop run for the engine player */
extern void CommandRoll(char *);
extern void CommandMove(char *);
extern int NextTurn(int fPlayNext);
extern void ClearMatch(void);
extern int ListCreate(listOLD *pl);
extern listOLD lMatch;

/* Engine initialisation symbols (Tier C: initialise extraction) */
extern void  EvalInitialise(char *szWeights, char *szWeightsBinary, int fNoBearoff, void (*pfProgress)(unsigned int));
extern void *InitRNG(unsigned long *pnSeed, int *pfInitFrom, int fSet, rng rngx);
extern void  gnubg_init_tld(void);
extern void  gnubg_init_rollout(void);
extern rng           rngCurrent;
extern rngcontext   *rngctxCurrent;
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

/*
 * Engine cube response to a double already on the table.
 * Precondition: a human double has been registered (ms.fDoubled == TRUE) and it
 * is now the engine player's turn. gnubg's CommandTake/CommandDrop refuse to act
 * for a non-human player unless fComputerDecision is set, so we raise that flag
 * around the call (mirroring play.c's ComputerTurn cube block), then drain.
 *   take != 0 -> CommandTake (engine accepts; play continues)
 *   take == 0 -> CommandDrop (engine passes; game ends, doubler awarded points)
 */
int gnubg_mobile_engine_cube_response(int take) {
    pthread_mutex_lock(&gnubg_lock);
    gnubg_set_computer_decision(1);
    if (take)
        CommandTake(NULL);
    else
        CommandDrop(NULL);
    gnubg_set_computer_decision(0);
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
static cubeinfo fac_ci_default;   /* set via gnubg_mobile_set_default_cubeinfo() */

/* Initialise the facade's default cubeinfo. Call once at engine init, with the
 * same arguments native-lib.c uses for its ci_default (money play, centred cube). */
void gnubg_mobile_set_default_cubeinfo(void) {
    int anScore[2] = {0, 0};
    SetCubeInfo(&fac_ci_default, 1, -1, 0, 0, anScore, 0, 0, 0, VARIATION_STANDARD);
}

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

/* Tutor analysis in one gnubg call.
 *
 * Identifies the played move by PositionKey (same as find_move), evaluates
 * every legal move using the same 1-ply path as get_candidates, and returns
 * the played equity, the best equity, and the best-move board. Everything
 * stays inside gnubg -- no move encodings cross the JNI boundary.
 *
 * out[52] layout (caller provides):
 *   out[0]     = Float.floatToRawIntBits(played_equity)
 *   out[1]     = Float.floatToRawIntBits(best_equity)
 *   out[2..51] = best-move board in player-on-roll frame (flat 50 ints)
 *
 * Returns  1 if the played move was found in the legal move list (normal).
 *          0 if the played move was not found (dance / forced / mismatch).
 *         -1 on parameter error.
 *
 * Must be called BEFORE gnubg advances match state (before applyMoveString).
 */
int gnubg_mobile_tutor_analyze(const int old_board[50], const int cur_board[50],
                                int d0, int d1, int out[52]) {
    TanBoard oldB, curB;
    movelist ml;
    positionkey curKey;
    float arOutput[NUM_OUTPUTS];
    float played_equity = 0.0f, best_equity = -2.0f;
    int   best_idx = 0, found = 0;
    unsigned int i;
    union { float f; unsigned int bits; } u;

    if (!out) return -1;

    facade_unpack_board(old_board, oldB);
    facade_unpack_board(cur_board, curB);

    pthread_mutex_lock(&gnubg_lock);

    memset(&ml, 0, sizeof(ml));
    GenerateMoves(&ml, (ConstTanBoard) oldB, d0, d1, FALSE);
    PositionKey((ConstTanBoard) curB, &curKey);

    for (i = 0; i < ml.cMoves; i++) {
        TanBoard boardAfter;
        float eq;

        /* Evaluate this candidate: apply, swap, evaluate, negate. */
        memcpy(boardAfter, oldB, sizeof(TanBoard));
        ApplyMove(boardAfter, ml.amMoves[i].anMove, FALSE);
        SwapSides(boardAfter);
        memset(arOutput, 0, sizeof(arOutput));
        if (EvaluatePosition(NULL, (ConstTanBoard) boardAfter, arOutput,
                             &fac_ci_default, &fac_ec_default) == 0) {
            eq = -(arOutput[0] + arOutput[1] + arOutput[2]
                  -arOutput[3] - arOutput[4]);
        } else {
            eq = 0.0f;
        }

        /* Is this the played move? Same check as find_move. */
        if (!found &&
            EqualKeys(ml.amMoves[i].key, curKey) &&
            ml.amMoves[i].cMoves == ml.cMaxMoves &&
            ml.amMoves[i].cPips  == ml.cMaxPips) {
            played_equity = eq;
            found = 1;
        }

        /* Track best move. */
        if (eq > best_equity) {
            best_equity = eq;
            best_idx = (int) i;
        }
    }

    /* Build best-move board: apply best move, swap back to player frame. */
    {
        TanBoard bestBoard;
        memcpy(bestBoard, oldB, sizeof(TanBoard));
        if (ml.cMoves > 0) {
            ApplyMove(bestBoard, ml.amMoves[best_idx].anMove, FALSE);
            SwapSides(bestBoard);
        }
        facade_pack_board((ConstTanBoard) bestBoard, out + 2);
    }

    pthread_mutex_unlock(&gnubg_lock);

    u.f = played_equity; out[0] = (int) u.bits;
    u.f = best_equity;   out[1] = (int) u.bits;

    return found ? 1 : 0;
}

/* Return ranked move candidates with cubeless 1-ply equity.
 *
 * Design notes:
 *   - GenerateMoves already ranks by 0-ply evaluation; we re-evaluate each
 *     candidate at 1-ply (fac_ec_default.nPlies == 1) for better accuracy.
 *   - ApplyMove modifies a local copy of the board -- the original is untouched.
 *   - Cubeless equity: negate the opponent-perspective result to get the
 *     current player's equity (higher = better for the player).
 *   - The lock is held for the entire enumeration so the engine state
 *     (particularly the position cache) remains consistent.
 *   - EvaluatePosition failure on a single candidate is non-fatal: we record
 *     equity 0.0 and continue so the caller still gets a ranked list.
 */
int gnubg_mobile_get_candidates(const int board[50], int d0, int d1,
                                int *out_moves, float *out_equities,
                                int n_max) {
    TanBoard anBoard;
    movelist ml;
    int i, j, n_written;

    if (!out_moves || !out_equities || n_max <= 0) return -1;

    facade_unpack_board(board, anBoard);

    pthread_mutex_lock(&gnubg_lock);

    memset(&ml, 0, sizeof(ml));
    GenerateMoves(&ml, (ConstTanBoard) anBoard, d0, d1, FALSE);

    n_written = (int) ml.cMoves < n_max ? (int) ml.cMoves : n_max;

    for (i = 0; i < n_written; i++) {
        TanBoard boardAfter;
        float arOutput[NUM_OUTPUTS] = {0};
        float equity = 0.0f;

        /* Copy the pre-move board and apply this candidate move to it. */
        memcpy(boardAfter, anBoard, sizeof(TanBoard));
        ApplyMove(boardAfter, ml.amMoves[i].anMove, FALSE);

        /* After the player moves it is the opponent's turn: swap sides so
         * EvaluatePosition sees the board from the next player's perspective,
         * then negate to recover the current player's cubeless equity. */
        SwapSides(boardAfter);

        if (EvaluatePosition(NULL, (ConstTanBoard) boardAfter, arOutput,
                             &fac_ci_default, &fac_ec_default) == 0) {
            /* arOutput: [0]=W [1]=WG [2]=WBG [3]=L [4]=LG [5]=LBG
             * Opponent-perspective equity = W+WG+WBG - L-LG-LBG.
             * Negate for current player's equity. */
            /* NUM_OUTPUTS == 5: indices 0=W 1=WG 2=WBG 3=L 4=LG.
             * Cubeless equity from current player's perspective. */
            equity = -(arOutput[0] + arOutput[1] + arOutput[2]
                      -arOutput[3] - arOutput[4]);
        }

        /* Write the 8-int move encoding (anMove[8] convention). */
        for (j = 0; j < 8; j++)
            out_moves[i * 8 + j] = ml.amMoves[i].anMove[j];

        out_equities[i] = equity;
    }

    pthread_mutex_unlock(&gnubg_lock);
    return n_written;
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

/* ===========================================================================
 * Tier C: engine initialisation, relocated from native-lib.c's initialise().
 * Platform-neutral: takes the weights path as a plain C string, runs the full
 * gnubg engine startup, sets both the engine and facade default cube info, and
 * configures the human-vs-GNU player setup. The JNI wrapper keeps only the
 * jstring round-trip and the gnubg_initialised guard (a static it owns).
 * Returns 1 on success.
 * =========================================================================== */
int gnubg_mobile_initialise(const char *weights_path) {
    pthread_mutex_lock(&gnubg_lock);

    /* Default cube info: money play, centred cube (mirrors native-lib ci_default). */
    gnubg_mobile_set_default_cubeinfo();

    EvalInitialise((char *) weights_path, NULL, 0, NULL);

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
    fAutoRoll   = FALSE;   /* human rolls manually via UI */

    pthread_mutex_unlock(&gnubg_lock);
    return 1;
}
