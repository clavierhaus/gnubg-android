#include <pthread.h>
#include <stdio.h>
#include <string.h>

#include <android/log.h>
#include "gnubg_mobile.h"

#include "config.h"
#include "eval.h"
#include "positionid.h"
/* NOTE: jni-bridge/matchid.h is an empty stub (it exists so rollout.c can
 * include matchid.h without pulling in symbols), and jni-bridge/ precedes
 * engine-core/ on the include path -- so a plain #include "matchid.h" here
 * silently resolves to the stub and MatchIDFromMatchState goes undeclared.
 * Reach the real header explicitly. Do NOT reorder the include path: six
 * other headers in jni-bridge/ shadow GTK-dependent engine headers on
 * purpose. */
#include "../../engine-core/matchid.h"
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
extern void CommandPrevious(char *);
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
extern void CommandSetCubeUse(char *);       /* set.c -- safe global toggle (fCubeUse) */
extern int  gnubg_can_double(void);
extern void gnubg_set_suppress_auto_forfeit(int);
extern void InitMatchEquity(const char *szFileName);
extern int  gnubg_legal_sub_move(const TanBoard anBoard, int iSrc, int nPips);  /* seam in engine-core/eval.c -- bear-off + opponent-block rule */   /* seam in engine-core/play.c -- see PROVENANCE */
extern void CommandTake(char *);
extern void CommandDrop(char *);
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

/* Cube use on/off. CommandSetCubeUse is a safe global SetToggle (set.c:893),
 * same class as Jacoby/Beavers -- pure fCubeUse flag, no match-state assertion. */
int gnubg_mobile_set_cube_use(int on) {
    pthread_mutex_lock(&gnubg_lock);
    CommandSetCubeUse(on ? (char *) "on" : (char *) "off");
    pthread_mutex_unlock(&gnubg_lock);
    return 1;
}

/* Load a match equity table by file path. Mirrors CommandSetMET (set.c:3133):
 * InitMatchEquity loads the XML (or falls back to the built-in Zadeh default if
 * the path is empty/unreadable), then the eval cache must be flushed because
 * cubeful evaluations cached under the previous table would be stale. Safe: it
 * touches only the MET tables and cache, not live match state. */
extern void EvalCacheFlush(void);
int gnubg_mobile_set_met(const char *path) {
    pthread_mutex_lock(&gnubg_lock);
    InitMatchEquity(path ? path : "");
    EvalCacheFlush();
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

/* Step backwards through the game record. PORT: CommandPrevious (play.c).
 *
 * Takes the same argument grammar as CommandNext: an empty string for one move
 * record, or "game" / "roll" / "rolled" / "marked", or a count.
 *
 * Navigation only walks the record. The sole writer of fNextTurn is TurnDone()
 * (play.c:1663), so neither CommandNext nor CommandPrevious schedules a turn and
 * the drain below is a no-op while reviewing. It is kept so every verb leaves the
 * engine in the same state.
 *
 * Both refuse when plGame is NULL, printing to the log and returning void, so a
 * caller cannot distinguish "moved" from "refused" -- read the match state after.
 */
int gnubg_mobile_command_previous(const char *argument) {
    pthread_mutex_lock(&gnubg_lock);
    CommandPrevious((char *)(argument ? argument : ""));
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
    /* Human offers the cube, then we drive gnubg's own turn loop. gnubg's
     * ComputerTurn (the `else if (ms.fDoubled)` branch) evaluates the cube and
     * decides take/drop/beaver ITSELF -- that decision is gnubg's, not the
     * port's. Draining here is what lets gnubg make it. (Previously the port
     * stopped after CommandDouble and had Kotlin decide take/drop via a forced
     * CommandTake/CommandDrop, overriding gnubg's cube judgment.) */
    CommandDouble(NULL);
    gnubg_mobile_drain_next_turns();
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

/* Why CommandRoll is about to refuse, if it is.
 *
 * CommandRoll (play.c:4048) returns void and reports its five refusals through
 * outputl(), which on Android goes to the log and nowhere the UI can see. The
 * facade then returned 1 unconditionally, so a refused roll was indistinguishable
 * from a successful one and the UI simply re-entered WAITING_FOR_ROLL -- a silent
 * loop that looks like a stuck game.
 *
 * These are gnubg's own guards, read in gnubg's own order. Nothing is decided
 * here and no behaviour is duplicated: CommandRoll still makes every decision.
 * This only reports which of its conditions currently holds.
 *
 * The state must be read BEFORE the call: drain_next_turns() afterwards runs the
 * computer's turn, so ms.anDice no longer describes the human's roll.
 *
 * 0 = no refusal expected. 1..5 map to CommandRoll's guards in order.
 */
static int roll_refusal_reason(void) {
    if (ms.gs != GAME_PLAYING)              return 1;  /* no game in progress */
    if (ap[ms.fTurn].pt != PLAYER_HUMAN)    return 2;  /* it is the computer's turn */
    if (ms.fDoubled)                        return 3;  /* cube must be answered */
    if (ms.fResigned)                       return 4;  /* resignation must be resolved */
    if (ms.anDice[0])                       return 5;  /* already rolled */
    return 0;
}

/* Returns 1 when the roll was attempted with every precondition met, 0 when
 * gnubg was always going to refuse. The reason is logged under gnubg-roll. */
int gnubg_mobile_command_roll(void) {
    int reason;

    pthread_mutex_lock(&gnubg_lock);

    reason = roll_refusal_reason();
    if (reason) {
        __android_log_print(ANDROID_LOG_WARN, "gnubg-roll",
            "CommandRoll refused: reason=%d gs=%d fTurn=%d pt=%d fDoubled=%d "
            "fResigned=%d dice=%u,%u",
            reason, (int) ms.gs, ms.fTurn, (int) ap[ms.fTurn].pt,
            ms.fDoubled, ms.fResigned,
            ms.anDice[0], ms.anDice[1]);
    }

    CommandRoll(NULL);
    gnubg_mobile_drain_next_turns();

    pthread_mutex_unlock(&gnubg_lock);

    return reason == 0;
}

/* The resignation currently on the table: 0 none, 1 normal, 2 gammon,
 * 3 backgammon. This is gnubg's own ms.fResigned.
 *
 * GNU resigns by itself. ComputerTurn calls getResignation() and, when the
 * position is lost badly enough, CommandResign("n"|"g"|"b") (play.c:1327-1335),
 * which sets ms.fResigned. gnubg then waits for the human to answer, and
 * CommandRoll refuses with "Please resolve the resignation first" until they do.
 * A port that never asks leaves the game unable to proceed. */
int gnubg_mobile_get_resignation(void) {
    int r;
    pthread_mutex_lock(&gnubg_lock);
    r = ms.fResigned;
    pthread_mutex_unlock(&gnubg_lock);
    return r;
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
/* Shared analysis replay (used by tutor_analyze and analyze_played_move).
 * Finds the human's last MOVE_NORMAL, reconstructs the pre-move matchstate by
 * replaying the game exactly as gnubg's AnalyzeGame does, scores all legal moves
 * with FindnSaveBestMoves at the fixed 2-ply analysis context, and locates the
 * played move's index. On success returns 1 and fills *pml (caller MUST g_free
 * pml->amMoves), *piPlayed, and *pmsAnalyse. Returns 0 if no analyzable move,
 * -1 on error. Caller holds gnubg_lock. */
static int analyze_replay(movelist *pml, unsigned int *piPlayed,
                          matchstate *pmsAnalyse) {
    listOLD *pl;
    moverecord *pmr, *pmrLast;
    statcontext *psc;

    if (!plGame || plGame->plNext == plGame) return 0;

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
    if (!pmrLast) return 0;

    pl  = plGame->plNext;
    pmr = (moverecord *) pl->p;
    if (!pmr || pmr->mt != MOVE_GAMEINFO) return 0;
    psc = &pmr->g.sc;
    if (AnalyzeMove(pmr, pmsAnalyse, plGame, psc,
                    &esAnalysisChequer, &esAnalysisCube, aamfAnalysis, NULL, NULL) < 0) {
        return 0;
    }
    for (pl = pl->plNext; pl != plGame && pl->p != pmrLast; pl = pl->plNext) {
        pmr = (moverecord *) pl->p;
        FixMatchState(pmsAnalyse, pmr);
        if (pmr->fPlayer != pmsAnalyse->fMove) {
            SwapSides(pmsAnalyse->anBoard);
            pmsAnalyse->fMove = pmr->fPlayer;
        }
        ApplyMoveRecord(pmsAnalyse, plGame, pmr);
    }

    FixMatchState(pmsAnalyse, pmrLast);
    if (pmrLast->fPlayer != pmsAnalyse->fMove) {
        SwapSides(pmsAnalyse->anBoard);
        pmsAnalyse->fMove = pmrLast->fPlayer;
    }
    {
        cubeinfo ci;
        positionkey key;
        TanBoard anBoardMove;
        unsigned int j, iPlayed;

        memcpy(anBoardMove, pmsAnalyse->anBoard, sizeof(anBoardMove));
        ApplyMove(anBoardMove, pmrLast->n.anMove, FALSE);
        PositionKey((ConstTanBoard) anBoardMove, &key);

        GetMatchStateCubeInfo(&ci, pmsAnalyse);

        memset(pml, 0, sizeof(*pml));
        /* PORT: fixed 2-ply analysis context (esAnalysisChequer), independent of
         * the opponent-strength selector -- see the tutor-ply fix commit. */
        if (FindnSaveBestMoves(pml, pmrLast->anDice[0], pmrLast->anDice[1],
                               (ConstTanBoard) pmsAnalyse->anBoard, &key, TRUE,
                               arSkillLevel[SKILL_DOUBTFUL], &ci,
                               &esAnalysisChequer.ec, aamfAnalysis) < 0) {
            g_free(pml->amMoves); pml->amMoves = NULL; return -1;
        }
        if (pml->cMoves == 0 || !pml->amMoves) {
            g_free(pml->amMoves); pml->amMoves = NULL; return 0;
        }

        iPlayed = pml->cMoves;
        for (j = 0; j < pml->cMoves; j++)
            if (EqualKeys(key, pml->amMoves[j].key)) { iPlayed = j; break; }
        if (iPlayed >= pml->cMoves) {
            g_free(pml->amMoves); pml->amMoves = NULL; return 0;
        }
        *piPlayed = iPlayed;
    }
    return 1;
}

int gnubg_mobile_tutor_analyze(const int old_board[50], int out[52]) {
    matchstate msAnalyse;
    movelist ml;
    unsigned int iPlayed;
    union { float f; unsigned int bits; } u;
    int rc;

    if (!out) return -1;

    pthread_mutex_lock(&gnubg_lock);
    rc = analyze_replay(&ml, &iPlayed, &msAnalyse);
    if (rc < 1) { pthread_mutex_unlock(&gnubg_lock); return rc; }

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

    pthread_mutex_unlock(&gnubg_lock);
    return 1;
}

/* Analysis-mode detail for the played move: gnubg's Hint-window probability
 * breakdown + equities, all in the mover's frame (ScoreMove applies
 * InvertEvaluationR, so arEvalMove is the mover's perspective, matching rScore).
 * out[7]: [0]=Win [1]=WinGammon [2]=WinBackgammon [3]=LoseGammon
 *         [4]=LoseBackgammon [5]=cubeful equity (rScore) [6]=cubeless (rScore2).
 * Win/gammon/bg are cumulative exactly as gnubg reports them (Win includes
 * gammon+bg; WinGammon includes bg). Returns 7 on success, 0/-1 otherwise. */
int gnubg_mobile_analyze_played_move(const int old_board[50], float out[7]) {
    matchstate msAnalyse;
    movelist ml;
    unsigned int iPlayed;
    int rc;
    (void) old_board;

    if (!out) return -1;

    pthread_mutex_lock(&gnubg_lock);
    rc = analyze_replay(&ml, &iPlayed, &msAnalyse);
    if (rc < 1) { pthread_mutex_unlock(&gnubg_lock); return rc; }

    {
        const move *pm = &ml.amMoves[iPlayed];
        out[0] = pm->arEvalMove[OUTPUT_WIN];
        out[1] = pm->arEvalMove[OUTPUT_WINGAMMON];
        out[2] = pm->arEvalMove[OUTPUT_WINBACKGAMMON];
        out[3] = pm->arEvalMove[OUTPUT_LOSEGAMMON];
        out[4] = pm->arEvalMove[OUTPUT_LOSEBACKGAMMON];
        out[5] = pm->rScore;
        out[6] = pm->rScore2;
    }
    g_free(ml.amMoves);

    pthread_mutex_unlock(&gnubg_lock);
    return 7;
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


/* ===========================================================================
 * Tier 5 -- position entry (Analyse Position)
 *
 * PORT: gnubg's own entry point is SetGNUbgID (backgammon.h:519, set.c).
 * It accepts a GNU BG ID ("PositionID:MatchID") or an XGID and discriminates
 * the dialects itself: SetXGID first, otherwise base64 tokens keyed on their
 * known lengths (L_MATCHID / L_POSITIONID). It then installs the match context
 * BEFORE the board --
 *
 *     if (matchid) SetMatchID(matchid);
 *     if (posid)   SetBoard(posid);
 *
 * -- which is precisely why SetBoard's "there must be a game in progress"
 * requirement is satisfied: SetMatchID has already set ms.gs from the Match
 * ID's gamestate field. The ordering is gnubg's, and it is deliberate.
 *
 * We call SetGNUbgID and NOT CommandSetGNUbgID. The Command wrapper answers
 * the "player on roll appears on top -- swap?" question by calling GetInputYN,
 * and this port's GetInputYN (android-app.c:854) always returns TRUE. Routing
 * through it would swap the players silently, every time. gnubg hands that
 * decision back to the caller as return code 2, and the UI must ask.
 *
 * Return value is SetGNUbgID's own, unchanged:
 *    0  IDs accepted; board and match context installed
 *    1  no valid IDs found in the string
 *    2  installed, but the player on roll appears on top (offer a swap)
 *   -1  bad argument (facade level only; gnubg never returns this)
 * =========================================================================== */
/* Encode an edited position + match context as "PositionID:MatchID", using
 * gnubg's OWN encoders -- PositionID (positionid.c) and MatchIDFromMatchState
 * (matchid.c). Nothing is computed here; a matchstate is filled in and gnubg
 * writes both tokens. The caller then installs the string through
 * gnubg_mobile_set_gnubg_id, i.e. gnubg's SetGNUbgID -- the same proven path a
 * pasted ID takes, with the same validation (CheckPosition inside SetBoard).
 *
 * Frames: the int[50] board is the HUMAN frame ([25..49] = human, player 0).
 * gnubg's ms.anBoard[1] is the player ON ROLL (see get_board_human above, which
 * swaps when fMove==1). So when the engine is on roll the board is SwapSides'd
 * before PositionID.
 *
 * Dice 0,0 means "not rolled": MatchID encodes it, and a position installed
 * without dice is exactly gnubg's setup for a CUBE decision rather than a
 * chequer decision (the desktop edit mode works the same way: setting the turn
 * removes the dice).
 *
 * cube_owner: -1 centred, 0 human, 1 engine -- gnubg's own fCubeOwner values;
 * MatchID masks with 0x3 and MatchFromKey restores -1.
 *
 * match_to 0 = money game. crawford is meaningful only in match play.
 * fJacoby is taken from the live ms so the encoded ID matches the app's rules.
 */
int gnubg_mobile_ids_from_state(const int board[50], int d0, int d1, int turn,
                                int score_h, int score_e, int match_to,
                                int cube, int cube_owner, int crawford,
                                char *out, int out_cap) {
    TanBoard anBoard;
    matchstate ms2;
    char *pos, *mat;

    if (out_cap < 32) return -1;
    if (turn != 0 && turn != 1) return -1;

    facade_unpack_board(board, anBoard);
    if (turn == 1)
        SwapSides(anBoard);         /* anBoard[1] must be the player on roll */

    memset(&ms2, 0, sizeof(ms2));
    memcpy(ms2.anBoard, anBoard, sizeof(TanBoard));
    ms2.anDice[0]  = (unsigned int) d0;
    ms2.anDice[1]  = (unsigned int) d1;
    ms2.fTurn      = turn;
    ms2.fMove      = turn;
    ms2.fCubeOwner = cube_owner;
    ms2.fCrawford  = crawford ? TRUE : FALSE;
    ms2.nMatchTo   = match_to;
    ms2.anScore[0] = score_h;       /* human is player 0 in this port */
    ms2.anScore[1] = score_e;
    ms2.nCube      = cube;
    ms2.fResigned  = 0;
    ms2.fDoubled   = FALSE;
    ms2.gs         = GAME_PLAYING;

    pthread_mutex_lock(&gnubg_lock);
    ms2.fJacoby = ms.fJacoby;
    ms2.fCubeUse = ms.fCubeUse;
    pos = PositionID((ConstTanBoard) anBoard);
    /* PositionID returns a static buffer; copy before the next gnubg call. */
    snprintf(out, (size_t) out_cap, "%s:", pos);
    mat = MatchIDFromMatchState(&ms2);
    strncat(out, mat, (size_t) out_cap - strlen(out) - 1);
    pthread_mutex_unlock(&gnubg_lock);

    return (int) strlen(out);
}

/* gnubg's own words for a cube decision -- GetCubeRecommendation (eval.c:2999).
 * The cubedecision value comes from gnubg_mobile_cube_decision's out_decision.
 * Mapping the enum to text in Kotlin would duplicate gnubg's classification. */
int gnubg_mobile_cube_recommendation(int cd, char *out, int out_cap) {
    const char *sz;
    if (out_cap < 8) return -1;
    pthread_mutex_lock(&gnubg_lock);
    sz = GetCubeRecommendation((cubedecision) cd);
    snprintf(out, (size_t) out_cap, "%s", sz ? sz : "");
    pthread_mutex_unlock(&gnubg_lock);
    return (int) strlen(out);
}

int gnubg_mobile_set_gnubg_id(const char *id) {
    char *buf;
    int rc;

    if (!id || !*id) return -1;

    /* SetGNUbgID walks its argument (get_base64(sz, &sz)), so it needs a
     * mutable, owned copy. */
    buf = g_strdup(id);
    if (!buf) return -1;

    pthread_mutex_lock(&gnubg_lock);
    rc = SetGNUbgID(buf);
    /* PORT: CommandSetGNUbgID calls ShowBoard() on success. In this port
     * ShowBoard (android-app.c:543) raises the gnubg_on_board_changed()
     * callback. rc == 2 also means the board was installed. */
    if (rc == 0 || rc == 2)
        ShowBoard();
    pthread_mutex_unlock(&gnubg_lock);

    g_free(buf);
    return rc;
}

/* Answer to gnubg_mobile_set_gnubg_id() returning 2. The UI asks the user;
 * only the user's yes reaches here. PORT: CommandSwapPlayers (backgammon.h:1024)
 * is the same routine gnubg's own callers invoke for this. */
int gnubg_mobile_swap_players(void) {
    pthread_mutex_lock(&gnubg_lock);
    CommandSwapPlayers(NULL);
    pthread_mutex_unlock(&gnubg_lock);
    return 1;
}

/* The reverse: gnubg's own renderings of the current state.
 * PositionID (positionid.h:27) and MatchIDFromMatchState (matchid.h:51) both
 * return pointers to static storage, so copy at once under the lock.
 * gnubg prints the pair as "%s:%s" (set.c:4874); the two halves are returned
 * separately here rather than joined, so no format is invented in C.
 * Returns 1 on success, -1 on bad argument. */
int gnubg_mobile_current_ids(char *out_pos, int pos_cap,
                             char *out_match, int match_cap) {
    const char *pos;
    const char *match;

    if (!out_pos || pos_cap < 1 || !out_match || match_cap < 1) return -1;

    pthread_mutex_lock(&gnubg_lock);
    pos = PositionID((ConstTanBoard) ms.anBoard);
    match = MatchIDFromMatchState(&ms);
    if (pos)   snprintf(out_pos, (size_t) pos_cap, "%s", pos);
    else       out_pos[0] = '\0';
    if (match) snprintf(out_match, (size_t) match_cap, "%s", match);
    else       out_match[0] = '\0';
    pthread_mutex_unlock(&gnubg_lock);

    return 1;
}

/* Ranked chequer-play candidates for the CURRENT state.
 *
 * PORT: FindnSaveBestMoves (eval.h:367), called exactly as analyze_replay calls
 * it -- fAnalyse TRUE at the doubtful-skill threshold, the named 2-ply analysis
 * context esAnalysisChequer.ec, and the named filters aamfAnalysis. The cubeinfo
 * is derived by gnubg from the matchstate via GetMatchStateCubeInfo; no private
 * cubeinfo, evalcontext or movefilter is constructed here.
 *
 * The difference from analyze_replay is the question, not the machinery: that
 * verb asks "how good was the move just played", and needs plGame. This one asks
 * "what are the candidates in the position now loaded", which is what ms holds
 * after gnubg_mobile_set_gnubg_id. keyMove is NULL because no move has been
 * played -- gnubg guards it with `if (keyMove)` and its own callers pass NULL
 * (analysis.c:124, eval.c:5558).
 *
 * gnubg returns the list already sorted best-first. out_equity[i] is
 * amMoves[i].rScore and out_moves[i*8 .. i*8+7] is amMoves[i].anMove, both
 * verbatim.
 *
 * Returns the number of candidates written (0 if the position has no dice, so
 * there is no chequer play to rank), or -1 on error. */
int gnubg_mobile_hint_moves(int max_n, float out_equity[], int out_moves[]) {
    movelist ml;
    cubeinfo ci;
    unsigned int i, n;

    if (max_n < 1 || !out_equity || !out_moves) return -1;

    pthread_mutex_lock(&gnubg_lock);

    if (ms.gs != GAME_PLAYING || !ms.anDice[0] || !ms.anDice[1]) {
        pthread_mutex_unlock(&gnubg_lock);
        return 0;
    }

    GetMatchStateCubeInfo(&ci, &ms);
    memset(&ml, 0, sizeof(ml));

    if (FindnSaveBestMoves(&ml, (int) ms.anDice[0], (int) ms.anDice[1],
                           (ConstTanBoard) ms.anBoard, NULL, TRUE,
                           arSkillLevel[SKILL_DOUBTFUL], &ci,
                           &esAnalysisChequer.ec, aamfAnalysis) < 0) {
        g_free(ml.amMoves);
        pthread_mutex_unlock(&gnubg_lock);
        return -1;
    }

    if (ml.cMoves == 0 || !ml.amMoves) {
        g_free(ml.amMoves);
        pthread_mutex_unlock(&gnubg_lock);
        return 0;
    }

    n = ml.cMoves;
    if (n > (unsigned int) max_n) n = (unsigned int) max_n;

    for (i = 0; i < n; i++) {
        out_equity[i] = ml.amMoves[i].rScore;
        memcpy(out_moves + i * 8, ml.amMoves[i].anMove, 8 * sizeof(int));
    }

    g_free(ml.amMoves);
    pthread_mutex_unlock(&gnubg_lock);
    return (int) n;
}
