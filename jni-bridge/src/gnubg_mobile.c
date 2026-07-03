#include <pthread.h>
#include <stdio.h>
#include <string.h>

#include <android/log.h>
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
extern void CommandSetAutoCrawford(char *);  /* set.c -- safe global toggle (fAutoCrawford) */
extern void CommandSetJacoby(char *);        /* set.c -- safe global toggle (fJacoby/ms.fJacoby) */
extern void CommandSetAutoDoubles(char *);   /* set.c -- safe global (cAutoDoubles) */
extern void CommandSetBeavers(char *);       /* set.c -- safe global (nBeavers) */
extern int  gnubg_can_double(void);
extern void gnubg_set_suppress_auto_forfeit(int);
extern void InitMatchEquity(const char *szFileName);
extern int  gnubg_legal_sub_move(const TanBoard anBoard, int iSrc, int nPips);  /* seam in engine-core/eval.c -- bear-off + opponent-block rule */   /* seam in engine-core/play.c -- see PROVENANCE */
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

/* Tournament rule toggles. These wrap gnubgs own safe CommandSet* commands
 * (set.c), which are pure global-state toggles with no match-state assertions.
 * IMPORTANT: Crawford is auto-managed by gnubg per game (play.c:864 sets
 * ms.fCrawford from score when fAutoCrawford && nMatchTo). The user-facing
 * Crawford rule is therefore fAutoCrawford via CommandSetAutoCrawford -- NOT
 * CommandSetCrawford, which is an in-match per-game command that asserts a
 * player is 1-away and crashes when called as a settings toggle. */
int gnubg_mobile_set_auto_crawford(int on) {
    pthread_mutex_lock(&gnubg_lock);
    CommandSetAutoCrawford(on ? (char *) "on" : (char *) "off");
    pthread_mutex_unlock(&gnubg_lock);
    return 1;
}

int gnubg_mobile_set_jacoby(int on) {
    pthread_mutex_lock(&gnubg_lock);
    CommandSetJacoby(on ? (char *) "on" : (char *) "off");
    pthread_mutex_unlock(&gnubg_lock);
    return 1;
}

int gnubg_mobile_set_auto_doubles(int n) {
    char sz[16];
    snprintf(sz, sizeof(sz), "%d", n);
    pthread_mutex_lock(&gnubg_lock);
    CommandSetAutoDoubles(sz);
    pthread_mutex_unlock(&gnubg_lock);
    return 1;
}

int gnubg_mobile_set_beavers(int n) {
    char sz[16];
    snprintf(sz, sizeof(sz), "%d", n);
    pthread_mutex_lock(&gnubg_lock);
    CommandSetBeavers(sz);
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
    /* Do NOT drain here. CommandDouble offers the cube and hands the turn
     * to the responder, leaving fDoubled and fNextTurn set. Draining would
     * run NextTurn immediately and auto-resolve the pending double before
     * the caller decides take or drop, destroying the two-step offer/respond
     * flow. The response verb, via CommandTake/CommandDrop, is what drains. */
    CommandDouble(NULL);
    pthread_mutex_unlock(&gnubg_lock);

    return 1;
}

/* Facade query: pure read, no side effects, no drain_next_turns. Held under
 * gnubg_lock since the engine seam touches ms / ap / fComputerDecision which
 * other facade verbs may mutate. */
int gnubg_mobile_can_double(void) {
    int r;
    pthread_mutex_lock(&gnubg_lock);
    r = gnubg_can_double();
    pthread_mutex_unlock(&gnubg_lock);
    return r;
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
    /* Drain, exactly as every other turn-driving verb does. gnubgs contract is
     * that NextTurn processes ONE transition per call and sets fNextTurn while
     * work remains (play.c uses while (fNextTurn) NextTurn(TRUE)). A single
     * NextTurn(TRUE) only flipped to the opponent and left the opponents turn
     * pending -- so the engine never rolled/played and, at game end, the game
     * was never scored. Draining completes the turn sequence like the desktop. */
    gnubg_mobile_drain_next_turns();
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
 * Tier 1 -- board (un)packing + state readers + board utils + file ops
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

/* gnubg own position-feature inputs (eval.c CalculateHalfInputs), both sides.
 * Raw normalised; denormalisation deferred. board[0..24]=p0, [25..49]=p1.
 * out[0..MORE_INPUTS-1]=p0, [MORE_INPUTS..]=p1. Returns 2*MORE_INPUTS or -1. */
int gnubg_mobile_position_features(const int board[50], float out[]) {
    TanBoard anBoard;
    if (!board || !out) return -1;
    facade_unpack_board(board, anBoard);
    CalculateHalfInputs(anBoard[0], anBoard[1], out);
    CalculateHalfInputs(anBoard[1], anBoard[0], out + MORE_INPUTS);
    return 2 * MORE_INPUTS;
}


/* Board in a STABLE human (player-0) frame, computed atomically.
 *
 * ms.anBoard is kept oriented so anBoard[1] is the player ON MOVE (fMove); gnubg
 * SwapSides() it whenever fMove changes, so its raw frame ALTERNATES every turn.
 * The UI must always show the human (player 0) at a fixed position. Reading the
 * board and the orientation field in two separate locked calls let the engine
 * thread change fMove between them -- the orientation could then be applied
 * against the wrong board, flipping the display mid-game. Here fMove and anBoard
 * are read under a SINGLE lock, so the packed board is always human-frame:
 *   fMove==0 (human on move): anBoard[1]=human -> pack raw.
 *   fMove==1 (engine on move): anBoard[1]=engine -> SwapSides first. */
int gnubg_mobile_get_board_human(int out_board[50]) {
    TanBoard tmp;
    pthread_mutex_lock(&gnubg_lock);
    if (ms.fMove == 1) {
        memcpy(tmp, ms.anBoard, sizeof(TanBoard));
        SwapSides(tmp);
        facade_pack_board((ConstTanBoard) tmp, out_board);
    } else {
        facade_pack_board((ConstTanBoard) ms.anBoard, out_board);
    }
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
    /* gnubg pattern at engine-core/play.c:2816 -- gate on nMatchTo, then
     * check which score crossed it. ms.gs is GAME state not MATCH state;
     * the previous gate could return a false-positive winner between
     * games of a multi-game match. */
    if (!ms.nMatchTo)                            w = -1;
    else if (ms.anScore[0] >= ms.nMatchTo)       w = 0;
    else if (ms.anScore[1] >= ms.nMatchTo)       w = 1;
    else                                         w = -1;
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
    /* Gate on LegalMove (eval.c:2732) before ApplySubMove. ApplySubMoves
     * own fCheckLegal arg only validates basic sanity (source non-empty,
     * destination not blocked by opponent); the all-chequers-in-home rule
     * for bear-off lives in LegalMove and is NOT applied by ApplySubMove.
     * Routing through LegalMove first makes the facade reject illegal
     * sub-moves at source -- the UI then sees an empty IntArray return
     * (via the JNI wrapper) and leaves the board untouched, without
     * reinventing the bear-off rule in Kotlin. */
    if (!gnubg_legal_sub_move(anBoard, i_src, n_roll)) {
        pthread_mutex_unlock(&gnubg_lock);
        return 0;
    }
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
 * Tier 2 -- engine algorithms relocated whole from native-lib.c.
 * Evalcontexts and cubeinfo are sourced per-call from gnubg's own named
 * instances (GetEvalCube, GetEvalChequer, GetMatchStateCubeInfo); no defaults
 * are cached at this layer (see PORT CHECKPOINT audit V1/V2/V3, closed in
 * MASTER_V0.9.md Phase 11.1).
 * =========================================================================== */

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

/* gnubg own skill classifier (analysis.c:287). Classifies an equity delta
 * (played - best; <= 0, more negative = worse) into a skilltype ordinal using
 * gnubg arSkillLevel thresholds (gnubg.c canonical 0.16/0.08/0.04). The UI
 * must NOT reimplement these bands -- this is the single source of truth.
 * Returns: 0=SKILL_VERYBAD, 1=SKILL_BAD, 2=SKILL_DOUBTFUL, 3=SKILL_NONE. */
extern skilltype Skill(float r);
int gnubg_mobile_skill(float equity_delta) {
    return (int) Skill(equity_delta);
}

/* Tutor analysis using gnubg's own AnalyzeMove on the last played move.
 *
 * Call AFTER applyMoveString: the move record must already be in plGame.
 * gnubg stores everything we need -- the ranked move list (pmr->ml) and the
 * index of the move that was played (pmr->n.iMove). We do NOT search or
 * re-evaluate; we read what gnubg computed, exactly as move_skill() does.
 *
 * old_board: the pre-move board (player-on-roll frame) -- used only to build
 *            the best-move board for feature comparison.
 * out[52]:
 *   out[0]     = Float bits of played equity (ml.amMoves[iMove].rScore)
 *   out[1]     = Float bits of best equity   (ml.amMoves[0].rScore)
 *   out[2..51] = best-move board in player-on-roll frame
 *
 * Returns 1 on success, 0 if no analyzable move record, -1 on error.
 */
int gnubg_mobile_tutor_analyze(const int old_board[50], int out[52]) {
    listOLD *pl;
    moverecord *pmr, *pmrLast;
    matchstate msAnalyse;
    statcontext *psc;
    union { float f; unsigned int bits; } u;

    if (!out) return -1;
    if (!plGame || plGame->plNext == plGame) return 0;

    pthread_mutex_lock(&gnubg_lock);

    /* The move we analyse is the human's -- the last MOVE_NORMAL by a human
     * player (ap[fPlayer].pt == PLAYER_HUMAN). The very last record may be the
     * engine's reply, so walk backward to find the human's move. */
    pmrLast = NULL;
    {
        listOLD *plScan;
        for (plScan = plGame->plPrev; plScan != plGame; plScan = plScan->plPrev) {
            moverecord *pm = (moverecord *) plScan->p;
            if (pm && pm->mt == MOVE_NORMAL && ap[pm->fPlayer].pt == PLAYER_HUMAN) {
                pmrLast = pm; break;
            }
        }
    }
    if (!pmrLast) { pthread_mutex_unlock(&gnubg_lock); return 0; }

    /* Reconstruct the pre-move matchstate by replaying the game exactly as
     * gnubg's AnalyzeGame does: AnalyzeMove on the MOVE_GAMEINFO record inits
     * msAnalyse, then FixMatchState + ApplyMoveRecord walk forward, stopping
     * before pmrLast. */
    pl  = plGame->plNext;
    pmr = (moverecord *) pl->p;
    if (!pmr || pmr->mt != MOVE_GAMEINFO) { pthread_mutex_unlock(&gnubg_lock); return 0; }
    psc = &pmr->g.sc;
    if (AnalyzeMove(pmr, &msAnalyse, plGame, psc,
                    &esAnalysisChequer, &esAnalysisCube, aamfAnalysis, NULL, NULL) < 0) {
        pthread_mutex_unlock(&gnubg_lock); return 0;
    }
    for (pl = pl->plNext; pl != plGame && pl->p != pmrLast; pl = pl->plNext) {
        pmr = (moverecord *) pl->p;
        FixMatchState(&msAnalyse, pmr);
        if (pmr->fPlayer != msAnalyse.fMove) {
            SwapSides(msAnalyse.anBoard);
            msAnalyse.fMove = pmr->fPlayer;
        }
        ApplyMoveRecord(&msAnalyse, plGame, pmr);
    }

    /* Score all legal moves exactly as gnubg's AnalyzeMove does internally
     * (play.c ~65823): FindnSaveBestMoves with fAnalyse=TRUE fills rScore and
     * sorts best-first. Then find the played move by position key. */
    FixMatchState(&msAnalyse, pmrLast);
    if (pmrLast->fPlayer != msAnalyse.fMove) {
        SwapSides(msAnalyse.anBoard);
        msAnalyse.fMove = pmrLast->fPlayer;
    }
    {
        movelist ml;
        cubeinfo ci;
        positionkey key;
        TanBoard anBoardMove;
        unsigned int j, iPlayed;

        memcpy(anBoardMove, msAnalyse.anBoard, sizeof(anBoardMove));
        ApplyMove(anBoardMove, pmrLast->n.anMove, FALSE);
        PositionKey((ConstTanBoard) anBoardMove, &key);

        GetMatchStateCubeInfo(&ci, &msAnalyse);

        memset(&ml, 0, sizeof(ml));
        /* PORT (audit V1 + V2): cubeinfo &ci comes from GetMatchStateCubeInfo
         * (&ci, &msAnalyse) above; evalcontext comes from gnubg's GetEvalChequer()
         * accessor (android-app.c:903), which under fEvalSameAsAnalysis=FALSE
         * returns esEvalChequer at the user's strength preset (aecSettings[idx]
         * after audit C). The 2-ply/prune path historically returned inf on this
         * build; the named presets the UI exposes are 0/1-ply, so the path is
         * non-inf in practice. */
        if (FindnSaveBestMoves(&ml, pmrLast->anDice[0], pmrLast->anDice[1],
                               (ConstTanBoard) msAnalyse.anBoard, &key, TRUE,
                               arSkillLevel[SKILL_DOUBTFUL], &ci,
                               &(GetEvalChequer()->ec), aamfAnalysis) < 0) {
            g_free(ml.amMoves); pthread_mutex_unlock(&gnubg_lock); return 0;
        }
        if (ml.cMoves == 0 || !ml.amMoves) {
            g_free(ml.amMoves);
            pthread_mutex_unlock(&gnubg_lock); return 0;
        }

        iPlayed = ml.cMoves;
        for (j = 0; j < ml.cMoves; j++)
            if (EqualKeys(key, ml.amMoves[j].key)) { iPlayed = j; break; }
        if (iPlayed >= ml.cMoves) {
            g_free(ml.amMoves);
            pthread_mutex_unlock(&gnubg_lock); return 0;
        }


        u.f = ml.amMoves[iPlayed].rScore; out[0] = (int) u.bits;
        u.f = ml.amMoves[0].rScore;       out[1] = (int) u.bits;

        {
            /* Best board in the SAME frame as the caller's played board
             * (state.board): old_board + ApplyMove, NO SwapSides, exactly as
             * applySubMove builds the played board. */
            TanBoard bestBoard;
            facade_unpack_board(old_board, bestBoard);
            ApplyMove(bestBoard, ml.amMoves[0].anMove, FALSE);
            facade_pack_board((ConstTanBoard) bestBoard, out + 2);
        }
        g_free(ml.amMoves);
    }

    pthread_mutex_unlock(&gnubg_lock);
    return 1;
}

/* Return ranked move candidates with cubeless 1-ply equity.
 *
 * Design notes:
 *   - GenerateMoves already ranks by 0-ply evaluation; we re-evaluate each
 *     candidate via GetEvalChequer() -- the player's chequer evalcontext at
 *     the current strength preset (audit V1, closed B.2) -- for better accuracy.
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
    cubeinfo ci_candidates, ci_candidates_opp;
    movelist ml;
    int i, j, n_written;

    if (!out_moves || !out_equities || n_max <= 0) return -1;

    facade_unpack_board(board, anBoard);

    pthread_mutex_lock(&gnubg_lock);

    /* PORT (audit A.2): the loop below SwapSides()'s the post-move board so it
     * can EvaluatePosition from the opponent's perspective, then negates equity
     * to recover the current player's view. gnubg's own swap-aware pattern at
     * engine-core/eval.c:5455 and :6054 builds a flipped cubeinfo for exactly
     * this case: same nCube/owner/score/Crawford/Jacoby/beavers/bgv, !fMove. */
    GetMatchStateCubeInfo(&ci_candidates, &ms);
    SetCubeInfo(&ci_candidates_opp,
                ci_candidates.nCube, ci_candidates.fCubeOwner,
                !ci_candidates.fMove,
                ci_candidates.nMatchTo, ci_candidates.anScore,
                ci_candidates.fCrawford, ci_candidates.fJacoby,
                ci_candidates.fBeavers, ci_candidates.bgv);

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
                             &ci_candidates_opp, &(GetEvalChequer()->ec)) == 0) {
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
    cubeinfo ci_eval;
    float arOutput[NUM_OUTPUTS] = {0};
    int rc, i;
    if (out_cap < NUM_OUTPUTS) return -1;
    facade_unpack_board(board, anBoard);
    pthread_mutex_lock(&gnubg_lock);
    GetMatchStateCubeInfo(&ci_eval, &ms);
    rc = EvaluatePosition(NULL, (ConstTanBoard) anBoard, arOutput,
                          &ci_eval, &(GetEvalChequer()->ec));
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

int gnubg_mobile_cube_decision(const int board[50],
                               float *out, int out_cap, int *out_decision) {
    TanBoard anBoard;
    cubeinfo ci;
    evalsetup *pesCube;
    float aarOutput[2][NUM_ROLLOUT_OUTPUTS];
    float arDouble[4];
    cubedecision cd;
    int rc, i;
    if (out_cap < 18) return -1;
    facade_unpack_board(board, anBoard);
    pthread_mutex_lock(&gnubg_lock);
    GetMatchStateCubeInfo(&ci, &ms);
    /* PORT: evalcontext is gnubg's own -- GetEvalCube() returns
     * &esAnalysisCube if fEvalSameAsAnalysis else &esEvalCube
     * (android-app.c:907). Both globals are seeded with EVALSETUP_2PLY
     * at android-app.c:894-895; setEngineStrength overwrites .ec with
     * aecSettings[idx] for the user-selected level.
     *
     * History: this comment used to warn of Inf in the chequer path.
     * The real cause was that InitMatchEquity was never called at
     * startup -- aafMET[][] was BSS-zero so mwc2eq divided by zero
     * and arDouble came back Inf/NaN. Fixed in V0.9.x by calling
     * InitMatchEquity("") in gnubg_mobile_initialise. */
    pesCube = GetEvalCube();
    memset(aarOutput, 0, sizeof(aarOutput));
    rc = GeneralCubeDecisionENoLocking(aarOutput, (ConstTanBoard) anBoard,
                                       &ci, &pesCube->ec, pesCube);
    if (rc != 0) { pthread_mutex_unlock(&gnubg_lock); return -1; }
    cd = FindCubeDecision(arDouble, aarOutput, &ci);
    pthread_mutex_unlock(&gnubg_lock);
    for (i = 0; i < 7; i++) { out[i] = aarOutput[0][i]; out[7 + i] = aarOutput[1][i]; }
    /* arDouble[0..3]: OPTIMAL, NODOUBLE, TAKE, DROP -- see engine-core/eval.h:56-59.
     * gnubgs cubeful equity comparisons; FindCubeDecision uses them to classify
     * the cube action. Exposing for offerDouble logging + future cube tutor. */
    for (i = 0; i < 4; i++) out[14 + i] = arDouble[i];
    if (out_decision) *out_decision = (int) cd;
    return 18;
}

int gnubg_mobile_rollout(const int board[50], int trials,
                         float *out, int out_cap) {
    TanBoard anBoard;
    cubeinfo ci_rollout;
    rolloutcontext rc_ro;
    float arOutput[NUM_ROLLOUT_OUTPUTS] = {0};
    float arStdDev[NUM_ROLLOUT_OUTPUTS] = {0};
    int ret, i;
    if (out_cap < 14) return -1;
    facade_unpack_board(board, anBoard);
    pthread_mutex_lock(&gnubg_lock);
    GetMatchStateCubeInfo(&ci_rollout, &ms);
    rc_ro = rcRollout;
    rc_ro.nTrials  = (unsigned int) (trials > 0 ? trials : 144);
    rc_ro.fCubeful = 1;
    rc_ro.fVarRedn = 1;
    ret = gnubg_rollout((ConstTanBoard) anBoard, arOutput, arStdDev,
                        &ci_rollout, &rc_ro);
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

    /* Defer the human no-legal-move auto-pass to the UI Continue tap
     * (PROVENANCE Seam 3). */
    gnubg_set_suppress_auto_forfeit(TRUE);

    EvalInitialise((char *) weights_path, NULL, 0, NULL);

    /* Load match equity table; empty path -> getDefaultMET (Zadeh).
     * Without this, aafMET[][] is BSS-zero and mwc2eq divides by zero,
     * poisoning arDouble with Inf/NaN. */
    InitMatchEquity("");

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
    fAutoCrawford = TRUE;  /* gnubg default: auto-manage Crawford game per match
                            * score (play.c:864). Without this, match play never
                            * flags the Crawford game and the cube AI evaluates
                            * under wrong match rules. UI toggles via
                            * gnubg_mobile_set_auto_crawford. */

    pthread_mutex_unlock(&gnubg_lock);
    return 1;
}
