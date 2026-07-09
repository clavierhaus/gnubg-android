/*
 * android-app.c -- Android application shell for GNU Backgammon
 *
 * This file is the Android equivalent of gnubg.c's application layer.
 * It contains portable functions extracted verbatim or adapted from gnubg.c,
 * plus Android-specific callbacks for sound, UI notification, and navigation.
 *
 * On desktop gnubg, these functions live in gnubg.c (the GTK application
 * shell, ~8000 lines). On Android, gnubg.c is NOT compiled -- this file
 * replaces it entirely for the functions play.c and sgf.c require.
 *
 * Extraction sources (all from engine-core/gnubg.c, version 1.08.003):
 *   InitBoard            -- line 1243
 *   NextTokenGeneral     -- line 645
 *   NextToken            -- line 769
 *   DisectPath           -- line 5171
 *   setDefaultFileName   -- line 5156 (GTK window title removed)
 *   UpdateSetting        -- replaced by Android callback
 *   UpdateSettings       -- line 1023 (ShowBoard replaced by callback)
 *   get_input_discard    -- line 5648 (always returns TRUE on Android)
 *   swapGame             -- line 4900
 *   NameIsKey            -- line 4973
 *   AddKeyName           -- line 5011
 *   DeleteKeyName        -- line 4985
 *   CommandSwapPlayers   -- line 5047
 *   CommandFirstGame     -- navigation command (Android callback)
 *   CommandFirstMove     -- navigation command (Android callback)
 *   SmartSit             -- line 5119
 *   playSound            -- replaced by Android audio callback
 *
 * GNU Backgammon by clavierhaus.at
 * GNU Backgammon upstream: GPL-3.0-or-later
 */

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <glib.h>
#include <signal.h>
#include <limits.h>

#include "backgammon.h"
#include "positionid.h"
#include "eval.h"
#include "sound.h"
#include "drawboard.h"

/* -- Navigation event type ---------------------------------------------------
 * Mirrors gnubg command layer navigation, exposed to Android UI layer.
 */
typedef enum {
    GNUBG_NAV_FIRST_GAME,
    GNUBG_NAV_FIRST_MOVE,
    GNUBG_NAV_LAST_GAME,
    GNUBG_NAV_LAST_MOVE,
    GNUBG_NAV_NEXT_GAME,
    GNUBG_NAV_NEXT_MOVE,
    GNUBG_NAV_PREV_GAME,
    GNUBG_NAV_PREV_MOVE,
} GnubgNavEvent;

/* Forward declarations of Android callbacks */
void gnubg_on_sound_event(gnubgsound gs);
void gnubg_on_setting_changed(void *pv);
void gnubg_on_board_changed(void);
void gnubg_on_navigation_event(int ev);
void gnubg_on_filename_changed(const char *sz);

/* -- Globals defined here ----------------------------------------------------
 * Declared extern in backgammon.h / gnubg headers.
 * On desktop they live in gnubg.c. On Android they live here.
 */
int   fGotoFirstGame         = FALSE;
int   fUseKeyNames           = TRUE;
int   fWithinSmartSit        = FALSE;
int   fAutoSaveConfirmDelete = TRUE;
int   fAutoSaveAnalysis      = FALSE;
char *szCurrentFolder        = NULL;

/* Key player names array -- tracks bot/engine player identities */
char keyNames[MAX_KEY_NAMES][MAX_NAME_LEN] = { "" };
int  keyNamesFirstEmpty = 0;

/* Default player names */
char default_names[2][MAX_NAME_LEN] = { "gnubg", "user" };

/* -- InitBoard ---------------------------------------------------------------
 * Source: gnubg.c:1243 -- copied verbatim.
 * Sets up the standard starting position for the given variation.
 */
void InitBoard(TanBoard anBoard, const bgvariation bgv) {
    switch (bgv) {
    case VARIATION_STANDARD:
        PositionFromID(anBoard, "4HPwATDgc/ABMA");
        break;
    case VARIATION_NACKGAMMON:
        PositionFromID(anBoard, "4Dl4ADbgOXgANg");
        break;
    case VARIATION_HYPERGAMMON_1:
        PositionFromID(anBoard, "AACAAAAAAgAAAA");
        break;
    case VARIATION_HYPERGAMMON_2:
        PositionFromID(anBoard, "AABAAQAACgAAAA");
        break;
    case VARIATION_HYPERGAMMON_3:
        PositionFromID(anBoard, "AACgAgAAKgAAAA");
        break;
    default:
        PositionFromID(anBoard, "4HPwATDgc/ABMA");
        break;
    }
}

/* -- NextTokenGeneral --------------------------------------------------------
 * Source: gnubg.c:645 -- copied verbatim (debug outputerrf removed).
 * Tokenises a string on an arbitrary delimiter set, handling quoted strings.
 */
char *NextTokenGeneral(char **ppch, const char *szTokens) {
    char *pchSave, chQuote = 0;
    int fEnd = FALSE;

    if (!*ppch)
        return NULL;

    /* skip leading whitespace */
    while (isspace(**ppch))
        (*ppch)++;

    if (!**ppch) {
        *ppch = NULL;
        return NULL;
    }

    pchSave = *ppch;

    while (!fEnd) {
        if (!**ppch) {
            fEnd = TRUE;
            break;
        }

        if (chQuote) {
            if (**ppch == chQuote) {
                chQuote = 0;
                memmove(*ppch, *ppch + 1, strlen(*ppch));
            } else {
                (*ppch)++;
            }
        } else if (**ppch == '"' || **ppch == '\'') {
            chQuote = **ppch;
            memmove(*ppch, *ppch + 1, strlen(*ppch));
        } else if (strchr(szTokens, **ppch)) {
            **ppch = 0;
            (*ppch)++;
            fEnd = TRUE;
        } else {
            (*ppch)++;
        }
    }

    if (!**ppch)
        *ppch = NULL;

    return pchSave;
}

/* -- NextToken ---------------------------------------------------------------
 * Source: gnubg.c:769 -- copied verbatim.
 * Tokenises on standard whitespace.
 */
char *NextToken(char **ppch) {
    return NextTokenGeneral(ppch, " \t\n\r\v\f");
}

/* -- DisectPath --------------------------------------------------------------
 * Source: gnubg.c:5171 -- copied verbatim.
 * Splits a file path into folder and filename (without extension).
 */
void DisectPath(const char *path, const char *extension,
                char **name, char **folder) {
    char *fnn, *pc;
    if (!path) {
        *folder = NULL;
        *name = NULL;
        return;
    }
    *folder = g_path_get_dirname(path);
    fnn = g_path_get_basename(path);
    pc = strrchr(fnn, '.');
    if (pc)
        *pc = '\0';
    *name = g_strconcat(fnn, extension ? extension : "", NULL);
    g_free(fnn);
}

/* -- setDefaultFileName ------------------------------------------------------
 * Source: gnubg.c:5156 -- adapted.
 * GTK window title update replaced by Android callback.
 */
void setDefaultFileName(char *path) {
    g_free(szCurrentFolder);
    g_free(szCurrentFileName);
    DisectPath(path, NULL, &szCurrentFileName, &szCurrentFolder);
    gnubg_on_filename_changed(szCurrentFileName);
}

/* -- UpdateSetting -----------------------------------------------------------
 * On desktop, updates GTK widgets when a match setting changes.
 * On Android, posts a state-change notification to the Kotlin layer.
 * The pointer identifies which setting changed (same semantics as desktop --
 * callers pass &ms.nCube, &ms.fTurn etc.).
 */
void UpdateSetting(void *pv) {
    gnubg_on_setting_changed(pv);
}

/* -- UpdateSettings ----------------------------------------------------------
 * Source: gnubg.c:1023 -- adapted.
 * ShowBoard() replaced by gnubg_on_board_changed() Android callback.
 */
void UpdateSettings(void) {
    UpdateSetting(&ms.nCube);
    UpdateSetting(&ms.fCubeOwner);
    UpdateSetting(&ms.fTurn);
    UpdateSetting(&ms.nMatchTo);
    UpdateSetting(&ms.fCrawford);
    UpdateSetting(&ms.fJacoby);
    UpdateSetting(&ms.gs);
    gnubg_on_board_changed();
}

/* -- get_input_discard -------------------------------------------------------
 * Source: gnubg.c:5648 -- adapted.
 * On desktop, prompts user in terminal to confirm discarding a match.
 * On Android there is no terminal -- always return TRUE (discard OK).
 * The Android UI layer handles confirmation dialogs at a higher level.
 */
int get_input_discard(void) {
    return TRUE;
}

/* -- swapGame ----------------------------------------------------------------
 * Source: gnubg.c:4900 -- copied verbatim.
 * Swaps player perspective in all move records of a single game.
 */
static void swapGame(listOLD *plGame) {
    listOLD *pl;
    moverecord *pmr;
    int n;
    TanBoard anBoard;

    for (pl = plGame->plNext; pl != plGame; pl = pl->plNext) {
        pmr = pl->p;

        switch (pmr->mt) {
        case MOVE_GAMEINFO:
            n = pmr->g.anScore[0];
            pmr->g.anScore[0] = pmr->g.anScore[1];
            pmr->g.anScore[1] = n;
            if (pmr->g.fWinner > -1)
                pmr->g.fWinner = !pmr->g.fWinner;
            break;

        case MOVE_DOUBLE:
        case MOVE_TAKE:
        case MOVE_DROP:
        case MOVE_NORMAL:
        case MOVE_RESIGN:
        case MOVE_SETDICE:
        case MOVE_SETCUBEVAL:
            pmr->fPlayer = !pmr->fPlayer;
            break;

        case MOVE_SETBOARD:
            PositionFromKey(anBoard, &pmr->sb.key);
            SwapSides(anBoard);
            PositionKey((ConstTanBoard) anBoard, &pmr->sb.key);
            pmr->fPlayer = !pmr->fPlayer;
            break;

        case MOVE_SETCUBEPOS:
            if (pmr->scp.fCubeOwner > -1)
                pmr->scp.fCubeOwner = !pmr->scp.fCubeOwner;
            pmr->fPlayer = !pmr->fPlayer;
            break;
        }
    }
}

/* -- NameIsKey ---------------------------------------------------------------
 * Source: gnubg.c:4973 -- copied verbatim.
 * Returns 1 if the player name is registered as a "key" (bot) name.
 */
int NameIsKey(const char sz[]) {
    for (int i = 0; i < keyNamesFirstEmpty; i++) {
        if (!strcmp(sz, keyNames[i]))
            return 1;
    }
    return 0;
}

/* -- AddKeyName --------------------------------------------------------------
 * Source: gnubg.c:5011 -- copied verbatim.
 * Registers a player name as a key (bot) name.
 */
int AddKeyName(const char sz[]) {
    if (strstr(sz, "\t") != NULL || strstr(sz, "\n") != NULL) {
        outputerrf(_("Player name contains unallowed character"));
        return 0;
    }
    if (strlen(sz) > MAX_NAME_LEN) {
        outputerrf(_("Player name is too long"));
        return 0;
    }
    if (keyNamesFirstEmpty < MAX_KEY_NAMES) {
        for (int i = 0; i < keyNamesFirstEmpty; i++) {
            if (!strcmp(sz, keyNames[i]))
                return 0;
        }
        strncpy(keyNames[keyNamesFirstEmpty], sz, MAX_NAME_LEN - 1);
        keyNames[keyNamesFirstEmpty][MAX_NAME_LEN - 1] = '\0';
        keyNamesFirstEmpty++;
        return 1;
    }
    return 0;
}

/* -- DeleteKeyName -----------------------------------------------------------
 * Source: gnubg.c:4985 -- copied verbatim.
 * Removes a player name from the key names array.
 */
int DeleteKeyName(const char sz[]) {
    for (int i = 0; i < keyNamesFirstEmpty; i++) {
        if (!strcmp(sz, keyNames[i])) {
            if (keyNamesFirstEmpty == (i + 1)) {
                keyNamesFirstEmpty--;
            } else {
                strcpy(keyNames[i], keyNames[keyNamesFirstEmpty - 1]);
                keyNamesFirstEmpty--;
            }
            return 1;
        }
    }
    return 0;
}

/* -- CommandSwapPlayers ------------------------------------------------------
 * Source: gnubg.c:5047 -- adapted.
 * GTK update calls removed; Android layer notified via callback.
 */
void CommandSwapPlayers(char *UNUSED(sz)) {
    listOLD *pl;
    char *pc;
    int n;

    if (!fWithinSmartSit)
        AddKeyName(ap[0].szName);

    /* swap individual move records */
    for (pl = lMatch.plNext; pl != &lMatch; pl = pl->plNext)
        swapGame(pl->p);

    /* swap player names */
    pc = g_strdup(ap[0].szName);
    strcpy(ap[0].szName, ap[1].szName);
    strcpy(ap[1].szName, pc);
    g_free(pc);

    /* swap player types */
    n = ap[0].pt;
    ap[0].pt = ap[1].pt;
    ap[1].pt = (playertype) n;

    /* swap scores */
    n = ms.anScore[0];
    ms.anScore[0] = ms.anScore[1];
    ms.anScore[1] = n;

    gnubg_on_board_changed();
}

/* -- SmartSit ----------------------------------------------------------------
 * Source: gnubg.c:5119 -- copied verbatim.
 * Swaps players if player 0 is a bot and player 1 is human,
 * so the human always plays from the standard position.
 */
void SmartSit(void) {
    if (NameIsKey(ap[0].szName) && !NameIsKey(ap[1].szName)) {
        fWithinSmartSit = TRUE;
        CommandSwapPlayers(NULL);
        fWithinSmartSit = FALSE;
    }
}

/* CommandFirstGame: provided by play.c */

/* CommandFirstMove: provided by play.c */

/* -- playSound ---------------------------------------------------------------
 * Source: sound.c -- replaced by Android audio callback.
 * On desktop, plays a WAV/OGG file via the platform audio system.
 * On Android, posts a sound event to the Kotlin layer for playback
 * via Android's SoundPool or MediaPlayer API.
 *
 * This is the primary audio docking point between the engine and the
 * Android UI layer. The gnubgsound enum values map directly to sound
 * effect slots in the Android app's asset bundle.
 */
void playSound(gnubgsound gs) {
    gnubg_on_sound_event(gs);
}

/* -- Android callback implementations ---------------------------------------
 * Weak default implementations -- no-ops until the Android UI layer
 * registers real implementations in native-lib.c via non-weak definitions.
 *
 * native-lib.c overrides these by defining non-weak versions that call
 * back into the JVM via stored JNIEnv* and callback object references.
 *
 * Sound:      gs = gnubgsound enum value identifying which sound to play
 * Setting:    pv = pointer to the changed field in ms (e.g. &ms.nCube)
 * Navigation: ev = GnubgNavEvent value
 * Board:      called whenever board state changes and should be redrawn
 * Filename:   called when the current match file name changes
 */
__attribute__((weak)) void gnubg_on_sound_event(gnubgsound gs)
    { (void)gs; }

__attribute__((weak)) void gnubg_on_setting_changed(void *pv)
    { (void)pv; }

__attribute__((weak)) void gnubg_on_board_changed(void)
    {}

__attribute__((weak)) void gnubg_on_navigation_event(int ev)
    { (void)ev; }

__attribute__((weak)) void gnubg_on_filename_changed(const char *sz)
    { (void)sz; }

/* -- Rollout progress callbacks (replacing progress.c) -----------------------
 * progress.c is deeply GTK -- it implements a rollout progress dialog with
 * GtkWidget, GtkListStore etc. These functions replace it on Android.
 * The Android layer receives progress via gnubg_on_rollout_progress().
 */

/* Android rollout progress callback -- weak default, overridden in native-lib.c */
__attribute__((weak)) void gnubg_on_rollout_progress(int iGame, int nTrials,
        float rJsd, int fStopped) {
    (void)iGame; (void)nTrials; (void)rJsd; (void)fStopped;
}

void RolloutProgressStart(const cubeinfo *pci, int n,
                           rolloutstat aars[2][2], rolloutcontext *prc,
                           char asz[][FORMATEDMOVESIZE], gboolean multiple,
                           void **pp) {
    (void)pci; (void)n; (void)aars; (void)prc;
    (void)asz; (void)multiple;
    *pp = NULL; /* no state needed */
}

void RolloutProgress(float aarOutput[][NUM_ROLLOUT_OUTPUTS],
                     float aarStdDev[][NUM_ROLLOUT_OUTPUTS],
                     const rolloutcontext *prc,
                     const cubeinfo aci[],
                     unsigned int initial_game_count,
                     int iGame, int iAlternative, int nRank,
                     float rJsd, int fStopped, int fShowRanks,
                     int fCubeRollout, void *pUserData) {
    (void)aarOutput; (void)aarStdDev; (void)prc; (void)aci;
    (void)initial_game_count; (void)iAlternative; (void)nRank;
    (void)fShowRanks; (void)fCubeRollout; (void)pUserData;
    gnubg_on_rollout_progress(iGame, (int)prc->nTrials, rJsd, fStopped);
}

int RolloutProgressEnd(void **pp, gboolean destroy) {
    (void)pp; (void)destroy;
    return 0;
}

/* -- Additional globals from gnubg.c -----------------------------------------
 * Declared extern in backgammon.h / export.h, defined in gnubg.c on desktop.
 */
#include "export.h"
#include "analysis.h"
#include "movefilters.inc"

matchinfo mi;

float rRatingOffset    = 2050.0f;
int   fAnalyseCube     = TRUE;
int   fAutoGame        = TRUE;
int   fAutoRoll        = FALSE;
int   fConfirmSave     = TRUE;
int   fDisplay         = TRUE;
/* fOutputDigits: provided by format.c */
/* rErrorRateFactor: provided by format.c */

float arLuckLevel[] = {
    0.6f,   /* LUCK_VERYBAD  */
    0.3f,   /* LUCK_BAD      */
    0.0f,   /* LUCK_NONE     */
    0.3f,   /* LUCK_GOOD     */
    0.6f    /* LUCK_VERYGOOD */
};

/* arSkillLevel: equity-loss thresholds for move classification. Values are
 * gnubg.c canonical (0.16/0.08/0.04). A prior port revision used 0.75x-scaled
 * values (0.12/0.06/0.03) with no recorded rationale; realigned to gnubg as the
 * sole authority. Consumed by Skill() (analysis.c:287). See PROVENANCE.md. */
float arSkillLevel[] = {
    0.16f,  /* SKILL_VERYBAD  */
    0.08f,  /* SKILL_BAD      */
    0.04f,  /* SKILL_DOUBTFUL */
    0.0f    /* SKILL_NONE     */
};

exportsetup exsExport = {
    TRUE, TRUE, TRUE, TRUE,
    1, -1, 5, TRUE,
    {FALSE, TRUE},
    {TRUE, TRUE, TRUE, TRUE},
    TRUE,
    {FALSE, TRUE},
    {TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE},
    NULL, HTML_EXPORT_TYPE_GNU, NULL, HTML_EXPORT_CSS_HEAD,
    4, 4
};

/* -- GetMatchStateCubeInfo ---------------------------------------------------
 * Source: gnubg.c:1274 -- copied verbatim.
 */
void GetMatchStateCubeInfo(cubeinfo *pci, const matchstate *pms) {
    SetCubeInfo(pci, pms->nCube, pms->fCubeOwner, pms->fMove,
                pms->nMatchTo, pms->anScore, pms->fCrawford,
                pms->fJacoby, nBeavers, pms->bgv);
}

/* -- ShowBoard ---------------------------------------------------------------
 * Source: gnubg.c:1364 -- adapted. GTK rendering replaced by Android callback.
 */
void ShowBoard(void) {
    gnubg_on_board_changed();
}

/* -- Skill helper functions --------------------------------------------------
 * Source: gnubg.c:2023-2148 -- copied verbatim.
 * Evaluate skill level of cube and move decisions.
 */
static skilltype no_double_skill(moverecord *pmr, cubeinfo *pci) {
    float arDouble[4];
    float eq = 0.0f;
    cubedecision cd;
    if (pmr->CubeDecPtr->esDouble.et == EVAL_NONE)
        return SKILL_NONE;
    cd = FindCubeDecision(arDouble, pmr->CubeDecPtr->aarOutput, pci);
    switch (cd) {
    case DOUBLE_TAKE: case DOUBLE_BEAVER: case REDOUBLE_TAKE:
        eq = arDouble[OUTPUT_NODOUBLE] - arDouble[OUTPUT_TAKE]; break;
    case DOUBLE_PASS: case REDOUBLE_PASS:
        eq = arDouble[OUTPUT_NODOUBLE] - arDouble[OUTPUT_DROP]; break;
    default: break;
    }
    return Skill(eq);
}

static skilltype double_skill(moverecord *pmr, cubeinfo *pci) {
    float arDouble[4];
    float eq = 0.0f;
    cubedecision cd;
    if (pmr->CubeDecPtr->esDouble.et == EVAL_NONE)
        return SKILL_NONE;
    cd = FindCubeDecision(arDouble, pmr->CubeDecPtr->aarOutput, pci);
    switch (cd) {
    case NODOUBLE_TAKE: case NODOUBLE_BEAVER: case NO_REDOUBLE_TAKE:
    case NO_REDOUBLE_BEAVER: case TOOGOOD_TAKE: case TOOGOODRE_TAKE:
        eq = arDouble[OUTPUT_TAKE] - arDouble[OUTPUT_NODOUBLE]; break;
    case TOOGOOD_PASS: case TOOGOODRE_PASS:
        eq = arDouble[OUTPUT_DROP] - arDouble[OUTPUT_NODOUBLE]; break;
    default: break;
    }
    return Skill(eq);
}

static skilltype drop_skill(moverecord *pmr, cubeinfo *pci) {
    float arDouble[4];
    float eq = 0.0f;
    cubedecision cd;
    if (pmr->CubeDecPtr->esDouble.et == EVAL_NONE)
        return SKILL_NONE;
    cd = FindCubeDecision(arDouble, pmr->CubeDecPtr->aarOutput, pci);
    switch (cd) {
    case DOUBLE_TAKE: case DOUBLE_BEAVER: case REDOUBLE_TAKE:
    case NODOUBLE_TAKE: case NODOUBLE_BEAVER: case NO_REDOUBLE_TAKE:
    case NO_REDOUBLE_BEAVER:
        eq = arDouble[OUTPUT_DROP] - arDouble[OUTPUT_TAKE]; break;
    default: break;
    }
    return Skill(eq);
}

static skilltype take_skill(moverecord *pmr, cubeinfo *pci) {
    float arDouble[4];
    float eq = 0.0f;
    cubedecision cd;
    if (pmr->CubeDecPtr->esDouble.et == EVAL_NONE)
        return SKILL_NONE;
    cd = FindCubeDecision(arDouble, pmr->CubeDecPtr->aarOutput, pci);
    switch (cd) {
    case DOUBLE_PASS: case REDOUBLE_PASS: case TOOGOOD_PASS:
    case TOOGOODRE_PASS: case OPTIONAL_DOUBLE_PASS: case OPTIONAL_REDOUBLE_PASS:
        eq = arDouble[OUTPUT_TAKE] - arDouble[OUTPUT_DROP]; break;
    default: break;
    }
    return Skill(eq);
}

static skilltype move_skill(moverecord *pmr) {
    move *move_i, *move_0;
    if (pmr->n.iMove >= pmr->ml.cMoves || !pmr->ml.amMoves)
        return SKILL_NONE;
    move_i = &pmr->ml.amMoves[pmr->n.iMove];
    move_0 = &pmr->ml.amMoves[0];
    if (move_i->esMove.et == EVAL_NONE || move_0->esMove.et == EVAL_NONE)
        return SKILL_NONE;
    return Skill(move_i->rScore - move_0->rScore);
}

/* -- find_skills -------------------------------------------------------------
 * Source: gnubg.c:2150 -- copied verbatim.
 */
void find_skills(moverecord *pmr, const matchstate *pms,
                 int did_double, int did_take) {
    cubeinfo ci;
    doubletype dt = DoubleType(pms->fDoubled, pms->fMove, pms->fTurn);
    taketype tt = (taketype) dt;
    GetMatchStateCubeInfo(&ci, pms);

    if (pmr->mt != MOVE_NORMAL && pmr->mt != MOVE_DOUBLE &&
        pmr->mt != MOVE_TAKE  && pmr->mt != MOVE_DROP) {
        pmr->n.stMove = SKILL_NONE;
        pmr->stCube   = SKILL_NONE;
        return;
    }
    if (pmr->mt == MOVE_DOUBLE && dt != DT_NORMAL) {
        pmr->stCube = SKILL_NONE;
        return;
    }
    if (pmr->mt == MOVE_TAKE && tt > TT_NORMAL) {
        pmr->stCube = SKILL_NONE;
        return;
    }
    if (did_double == FALSE)
        pmr->stCube = no_double_skill(pmr, &ci);
    else if (did_double == TRUE)
        pmr->stCube = double_skill(pmr, &ci);
    else if (did_take == FALSE)
        pmr->stCube = drop_skill(pmr, &ci);
    else if (did_take == TRUE)
        pmr->stCube = take_skill(pmr, &ci);

    if (pmr->mt == MOVE_NORMAL && pmr->ml.cMoves > 0 &&
        pmr->n.iMove < pmr->ml.cMoves)
        pmr->n.stMove = move_skill(pmr);
}

/* -- confirmOverwrite --------------------------------------------------------
 * Source: gnubg.c:5135 -- adapted.
 * Always returns TRUE on Android; UI layer handles confirmation dialogs.
 */
int confirmOverwrite(const char *sz, const int f) {
    (void)sz; (void)f;
    return TRUE;
}

/* -- CommandShowScore --------------------------------------------------------
 * Posts board-changed notification (score is part of board state).
 */
void CommandShowScore(char *UNUSED(sz)) {
    gnubg_on_board_changed();
}

/* -- RunAsyncProcess ---------------------------------------------------------
 * Source: backgammon.h:274 defines AsyncFun as void (*)(void *).
 * On desktop runs function in a thread pool. On Android runs synchronously.
 */
int RunAsyncProcess(AsyncFun pf, void *p, const char *UNUSED(sz)) {
    pf(p);
    return 0;
}

/* -- EVALSETUP_2PLY -- copied from gnubg.c:348 -------------------------------
 * Defined locally in gnubg.c, not in any header.
 *
 * NOT verbatim: the rolloutcontext tail was transcribed positionally and lost
 * eight bit-fields (see the note on the macro below). It is now written by
 * field name, which is both faithful and drift-proof.
 */
/* The rolloutcontext tail below is initialised BY NAME. It used to be
 * positional, and it supplied only three of the ELEVEN bit-fields that lead
 * that part of the struct (fCubeful, fVarRedn, fInitial, fRotate,
 * fTruncBearoff2, fTruncBearoffOS, fLateEvals, fDoTruncate, fStopOnSTD,
 * fStopOnJsd, fStopMoveOnJsd -- eval.h). Every value after fInitial therefore
 * slid eight positions: 1296 landed in fTruncBearoff2:1, RNG_MERSENNE in
 * fLateEvals:1, 2.33f in nTruncate, and nTrials came out as ZERO. That was
 * latent rather than live -- these evalsetups carry et = EVAL_EVAL, and
 * analysis.c consults .rc only when et == EVAL_ROLLOUT -- but it was wrong, and
 * it produced 24 of the build's warnings.
 *
 * The values are exactly the ones the macro always meant to set; the eight
 * unnamed bit-fields zero-initialise precisely as they did before. Naming them
 * makes the order impossible to drift against eval.h again.
 */
#define EVALSETUP_2PLY { \
  EVAL_EVAL, \
  { .fCubeful = TRUE, .nPlies = 2, .fUsePrune = TRUE, .fDeterministic = TRUE, .rNoise = 0.0f }, \
  { \
    { \
      { .fCubeful = FALSE, .nPlies = 2, .fUsePrune = TRUE, .fDeterministic = TRUE, .rNoise = 0.0f }, \
      { .fCubeful = FALSE, .nPlies = 2, .fUsePrune = TRUE, .fDeterministic = TRUE, .rNoise = 0.0f } \
    }, \
    { \
      { .fCubeful = FALSE, .nPlies = 0, .fUsePrune = TRUE, .fDeterministic = TRUE, .rNoise = 0.0f }, \
      { .fCubeful = FALSE, .nPlies = 0, .fUsePrune = TRUE, .fDeterministic = TRUE, .rNoise = 0.0f } \
    }, \
    { \
      { .fCubeful = FALSE, .nPlies = 2, .fUsePrune = TRUE, .fDeterministic = TRUE, .rNoise = 0.0f }, \
      { .fCubeful = FALSE, .nPlies = 2, .fUsePrune = TRUE, .fDeterministic = TRUE, .rNoise = 0.0f } \
    }, \
    { \
      { .fCubeful = FALSE, .nPlies = 0, .fUsePrune = TRUE, .fDeterministic = TRUE, .rNoise = 0.0f }, \
      { .fCubeful = FALSE, .nPlies = 0, .fUsePrune = TRUE, .fDeterministic = TRUE, .rNoise = 0.0f } \
    }, \
    { .fCubeful = FALSE, .nPlies = 2, .fUsePrune = TRUE, .fDeterministic = TRUE, .rNoise = 0.0f }, \
    { .fCubeful = FALSE, .nPlies = 2, .fUsePrune = TRUE, .fDeterministic = TRUE, .rNoise = 0.0f }, \
    { MOVEFILTER_NORMAL, MOVEFILTER_NORMAL }, \
    { MOVEFILTER_NORMAL, MOVEFILTER_NORMAL }, \
    .fCubeful = FALSE, .fVarRedn = TRUE, .fInitial = FALSE, \
    .nTruncate = 0, .nTrials = 1296, .nLate = 0, .rngRollout = RNG_MERSENNE, \
    .nSeed = 0, .nMinimumGames = 144, .rStdLimit = 0.01f, \
    .nMinimumJsdGames = 144, .rJsdLimit = 2.33f, .nGamesDone = 0, \
    .rStoppedOnJSD = 0.0f, .nSkip = 0 \
  } \
}

/* -- Analysis globals from gnubg.c ------------------------------------------
 * Source: gnubg.c lines 185-404
 */
int          fAnalyseDice       = TRUE;
int          fAnalyseMove       = TRUE;
int          fRecord            = TRUE;
int          fTutorCube         = TRUE;
int          fTutor             = FALSE;
unsigned int cAutoDoubles       = 0;
unsigned int nBeavers           = 3;
int          fBackgroundAnalysis = FALSE;
evalsetup    esAnalysisChequer  = EVALSETUP_2PLY;
evalsetup    esAnalysisCube     = EVALSETUP_2PLY;
movefilter   aamfAnalysis[MAX_FILTER_PLIES][MAX_FILTER_PLIES] = MOVEFILTER_NORMAL;

skilltype TutorSkill = SKILL_DOUBTFUL;

/* -- asyncEvalRoll -----------------------------------------------------------
 * Source: gnubg.c:5496 -- copied verbatim.
 */
void asyncEvalRoll(decisionData *pdd) {
    EvaluateRoll(pdd->aarOutput[0], ms.anDice[0], ms.anDice[1],
                 pdd->pboard, pdd->pci, pdd->pec);
    /* EvaluateRoll has no return value -- no error check needed */
}

/* -- asyncMoveDecisionE ------------------------------------------------------
 * Source: gnubg.c:5516 -- copied verbatim.
 */
void asyncMoveDecisionE(decisionData *pdd) {
    int asyncRet = 0;
    (void)asyncRet;
    if (GeneralEvaluationE(pdd->aarOutput[0], pdd->pboard,
                            pdd->pci, pdd->pec) < 0)
        asyncRet = -1;
}

/* -- asyncAnalyzeMove --------------------------------------------------------
 * Source: gnubg.c:5502 -- copied verbatim.
 */
void asyncAnalyzeMove(moveData *pmd) {
    int asyncRet = 0;
    (void)asyncRet;
    if (AnalyzeMove(pmd->pmr, pmd->pms, plGame, NULL,
                    pmd->pesChequer, pmd->pesCube,
                    pmd->aamf, NULL, NULL) < 0)
        asyncRet = -1;
}

/* -- CheckGameExists ---------------------------------------------------------
 * Source: gnubg.c:5586 -- copied verbatim.
 */
int CheckGameExists(void) {
    if (plGame) {
        return TRUE;
    } else {
        outputl(_("No game in progress."));
        return FALSE;
    }
}

/* -- delete_autosave ---------------------------------------------------------
 * Source: gnubg.c:5638 -- adapted (autosave is static in gnubg.c, stubbed here).
 * On Android autosave is managed by the Android filesystem layer.
 */
void delete_autosave(void) {
    /* Android: no autosave file to delete -- managed by Android layer */
}

/* -- GiveAdvice --------------------------------------------------------------
 * Source: gnubg.c:5225 -- adapted.
 * On desktop: shows a GTK dialog asking user to confirm a bad move.
 * On Android: posts an advice event to the Kotlin layer; always returns TRUE
 * (do not block engine) -- Android UI handles the confirmation asynchronously.
 */
int GiveAdvice(skilltype Skill) {
    if (!fTutor)
        return TRUE;
    if (Skill > TutorSkill)
        return TRUE;
    gnubg_on_navigation_event(GNUBG_NAV_FIRST_GAME); /* reuse as advice signal */
    return TRUE;
}

/* -- ProgressStartValue ------------------------------------------------------
 * Source: gnubg.c:4002 -- adapted.
 * On desktop: starts a GTK progress bar.
 * On Android: no-op (progress handled by Android layer via rollout callback).
 */
void ProgressStartValue(char *sz, int iMax) {
    (void)sz; (void)iMax;
}

/* -- Additional globals from gnubg.c -----------------------------------------
 * Source: gnubg.c lines 187-227
 */
int          fAutoBearoff  = FALSE;
int          fAutoMove     = FALSE;
int          fCheat        = FALSE;
int          fTutorChequer = TRUE;
unsigned int afCheatRoll[2] = { 0, 0 };

/* -- asyncFindMove -----------------------------------------------------------
 * Source: gnubg.c:5473 -- copied verbatim (MT_SetResultFailed -> local asyncRet)
 */
void asyncFindMove(findData *pfd) {
    int asyncRet = 0; (void)asyncRet;
    if (FindnSaveBestMoves(pfd->pml, ms.anDice[0], ms.anDice[1], pfd->pboard,
                           pfd->keyMove, pfd->fAnalyse, pfd->rThr,
                           pfd->pci, pfd->pec, pfd->aamf) < 0)
        asyncRet = -1;
}

/* -- asyncCubeDecision -------------------------------------------------------
 * Source: gnubg.c:5530 -- copied verbatim.
 */
void asyncCubeDecision(decisionData *pdd) {
    int asyncRet = 0; (void)asyncRet;
    if (GeneralCubeDecision(pdd->aarOutput, pdd->aarStdDev,
                             pdd->aarsStatistics, pdd->pboard,
                             pdd->pci, pdd->pes, NULL, NULL) < 0)
        asyncRet = -1;
}

/* -- GetInputYN --------------------------------------------------------------
 * Source: gnubg.c:3887 -- adapted.
 * On desktop: prompts user in terminal or GTK dialog.
 * On Android: always returns TRUE (UI layer handles confirmation dialogs).
 */
int GetInputYN(char *szPrompt) {
    (void)szPrompt;
    return TRUE;
}

/* -- hint_double, hint_take, hint_move ---------------------------------------
 * These are large functions in gnubg.c that implement the tutor system.
 * They evaluate the position, compare with the player's move, and offer
 * advice. For now, stub them -- full implementation is part of the tutor
 * feature which requires the full analysis pipeline to be connected.
 */
void hint_double(int show, int did_double) {
    (void)show; (void)did_double;
}

void hint_take(int show, int did_take) {
    (void)show; (void)did_take;
}

void hint_move(char *sz, gboolean show, procrecorddata *procdatarec) {
    (void)sz; (void)show; (void)procdatarec;
}

/* -- HandleCommand -----------------------------------------------------------
 * Source: gnubg.c:1166 -- the command dispatcher.
 * On Android the command system will be driven by the Kotlin layer.
 * For now stub it -- full implementation requires commands.inc.
 */
void HandleCommand(char *sz, command *ac) {
    (void)sz; (void)ac;
    outputl(_("Commands not yet implemented on Android."));
}

/* -- ParseMov ----------------------------------------------------------------
 * Move parser -- find where it lives
 */

/* -- Additional globals from gnubg.c -----------------------------------------
 * Source: gnubg.c lines 194-528
 */
int          fConfirmNew        = TRUE;
int          fCubeUse           = TRUE;
int          fMarkedSamePlayer  = FALSE;
unsigned int nDefaultLength     = 7;
evalsetup    esEvalChequer      = EVALSETUP_2PLY;
evalsetup    esEvalCube         = EVALSETUP_2PLY;
char        *default_sgf_folder = NULL;
int          fEvalSameAsAnalysis = FALSE;
movefilter   aamfEval[MAX_FILTER_PLIES][MAX_FILTER_PLIES] = MOVEFILTER_NORMAL;

/* -- GetEvalChequer / GetEvalCube / GetEvalMoveFilter ------------------------
 * Source: gnubg.c:410-426 -- copied verbatim.
 */
evalsetup *GetEvalChequer(void) {
    return fEvalSameAsAnalysis ? &esAnalysisChequer : &esEvalChequer;
}

evalsetup *GetEvalCube(void) {
    return fEvalSameAsAnalysis ? &esAnalysisCube : &esEvalCube;
}

TmoveFilter *GetEvalMoveFilter(void) {
    /* TmoveFilter is movefilter[4][4], so TmoveFilter* is movefilter(*)[4][4].
     * A bare aamfAnalysis decays to movefilter(*)[4] -- one level short. The
     * address happens to be identical, which is why the sole caller
     * (external.c:522, *GetEvalMoveFilter()) has always worked; the types were
     * still wrong. Take the address of the array. */
    return fEvalSameAsAnalysis ? &aamfAnalysis : &aamfEval;
}

/* Apply a gnubg preset (aecSettings[idx]) to the full set of evalcontexts
 * that gnubg's own machinery consults during play.
 *
 * Per CLAUDE.md PORT CHECKPOINT (audit V4): the previous implementation
 * wrote only esEvalChequer. The ComputerTurn cube branch at
 * engine-core/play.c:1316 reads ap[ms.fTurn].esCube.ec, which our writing
 * never touched. As a result the engine's proactive cube decision ran at
 * the EVALSETUP_2PLY default the slot was initialised with and never
 * proposed cube. Likewise GetEvalCube() (used by gnubg_mobile_cube_decision
 * after audit B.1) returned esEvalCube at its 2-ply default rather than
 * the user's chosen strength.
 *
 * Desktop gnubg's save/restore pattern at engine-core/play.c:3542-3614
 * touches BOTH ap[i].esChequer.ec AND ap[i].esCube.ec for both players,
 * which is the canonical pattern for "the player's strength changed".
 * Mirror it: write all six evalsetups in scope -- the two globals
 * (esEvalChequer, esEvalCube) consulted via GetEval*() under
 * fEvalSameAsAnalysis=FALSE, and the four per-player slots
 * (ap[0/1].esChequer/esCube) consulted by ComputerTurn and other
 * play.c paths.
 *
 * idx maps to eval.h SETTINGS_*: 0=Beginner, 1=Casual play (NOVICE),
 * 2=Intermediate, 3=Advanced. Out-of-range idx is ignored.
 *
 * Analysis contexts (esAnalysisChequer, esAnalysisCube) are intentionally
 * left alone -- analysis is a separate gnubg concept, sourced from
 * different desktop commands, and the strength preset is a play-time
 * setting. fEvalSameAsAnalysis is forced FALSE so GetEval*() returns the
 * strength-preset esEval* contexts, not the analysis ones.
 *
 * Move filter: the four named presets (idx 0..3) have aiSettingsMoveFilter
 * == -1, so aamfEval is left as-is. The strength selector only exposes
 * named presets today; when ply presets become user-selectable, the
 * move filter wiring lands here too.
 */
void gnubg_mobile_set_engine_strength(int idx) {
    if (idx < 0 || idx >= NUM_SETTINGS) return;
    fEvalSameAsAnalysis = FALSE;

    /* Globals consulted via GetEvalChequer() / GetEvalCube(). */
    esEvalChequer.et = EVAL_EVAL;
    esEvalChequer.ec = aecSettings[idx];
    esEvalCube.et    = EVAL_EVAL;
    esEvalCube.ec    = aecSettings[idx];

    /* Per-player slots consulted by ComputerTurn (play.c:1316 cube branch,
     * play.c:1441 chequer branch). Both players written so the strength
     * applies regardless of which side is on roll. */
    ap[0].esChequer.et = EVAL_EVAL;
    ap[0].esChequer.ec = aecSettings[idx];
    ap[0].esCube.et    = EVAL_EVAL;
    ap[0].esCube.ec    = aecSettings[idx];
    ap[1].esChequer.et = EVAL_EVAL;
    ap[1].esChequer.ec = aecSettings[idx];
    ap[1].esCube.et    = EVAL_EVAL;
    ap[1].esCube.ec    = aecSettings[idx];
}

/* -- PortableSignal / PortableSignalRestore ----------------------------------
 * Source: gnubg.c:1094,1128 -- adapted for Android/POSIX.
 * Sets up signal handlers with SA_RESTART flag on POSIX systems.
 */
void PortableSignal(int nSignal, void (*p)(int),
                    psighandler *pOld, int fRestart) {
    struct sigaction sa;
    sa.sa_handler = p;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = fRestart ? SA_RESTART : 0;
    sigaction(nSignal, &sa, pOld);
}

void PortableSignalRestore(int nSignal, psighandler *p) {
    sigaction(nSignal, p, NULL);
}

/* -- ParseNumber -------------------------------------------------------------
 * Source: gnubg.c:828 -- copied verbatim.
 * Parses an integer token from a command string.
 */
int ParseNumber(char **ppch) {
    char *pch, *pchOrig;
    if (!ppch || !(pchOrig = NextToken(ppch)))
        return INT_MIN;
    for (pch = pchOrig; *pch; pch++)
        if (!isdigit(*pch) && *pch != '-')
            return INT_MIN;
    return atoi(pchOrig);
}

/* -- acAnnotateMove ----------------------------------------------------------
 * Command table for move annotation -- referenced by play.c.
 * On desktop defined via commands.inc. On Android provide a minimal stub.
 */
command acAnnotateMove[] = {
    { NULL, NULL, NULL, NULL, NULL }
};

/* -- HandleCommand -----------------------------------------------------------
 * Already stubbed above -- duplicate declaration guard.
 */

/* -- Additional globals from gnubg.c -----------------------------------------
 * Source: gnubg.c lines 189-274
 */
int   fAutoDB           = FALSE;
int   fFullScreen       = FALSE;
int   fStyledGamelist   = TRUE;
int   nConfirmDefault   = -1;
float rEvalsPerSec      = -1.0f;
int   fEvalSameAsAnalysis_dup = 0; /* already defined above */

/* fQuiet: declared in sound.h, defined here */
/* already defined in sound section above */

/* -- Command tables -- minimal stubs ------------------------------------------
 * acTop and acSetEvaluation are command dispatch tables defined via
 * commands.inc in gnubg.c. On Android the command system is driven by
 * the Kotlin layer; provide empty tables as placeholders.
 */
command acSetEvaluation[] = {
    { NULL, NULL, NULL, NULL, NULL }
};

command acTop[] = {
    { NULL, NULL, NULL, NULL, NULL }
};

/* -- ParseReal ---------------------------------------------------------------
 * Source: gnubg.c:808 -- copied verbatim.
 */
float ParseReal(char **ppch) {
    char *pch, *pchOrig;
    float r;
    if (!ppch || !(pchOrig = NextToken(ppch)))
        return ERR_VAL;
    r = (float) g_ascii_strtod(pchOrig, &pch);
    return *pch ? ERR_VAL : r;
}

/* -- ParsePosition -----------------------------------------------------------
 * Source: gnubg.c:885 -- copied verbatim.
 */
int ParsePosition(TanBoard an, char **ppch, char *pchDesc) {
    int i;
    char *pch;
    if (!ppch || !(pch = NextToken(ppch))) {
        memcpy(an, msBoard(), sizeof(TanBoard));
        if (pchDesc)
            strcpy(pchDesc, _("Current position"));
        return 0;
    }
    if (!strcmp(pch, "simple")) {
        for (i = 0; i < 26; i++) {
            int n;
            if ((n = ParseNumber(ppch)) == INT_MIN) {
                outputf(_("`simple' must be followed by 26 integers; "
                          "found only %d\n"), i);
                return -1;
            }
            if (i == 0) {
                an[1][24] = n;
            } else if (i == 25) {
                an[0][24] = n;
            } else if (n > 0) {
                an[1][i - 1] = n;
                an[0][24 - i] = 0;
            } else {
                an[0][24 - i] = -n;
                an[1][i - 1] = 0;
            }
        }
        return 0;
    }
    return PositionFromID(an, pch);
}

/* -- ParseKeyValue -----------------------------------------------------------
 * Source: gnubg.c:982 -- copied verbatim.
 */
int ParseKeyValue(char **ppch, char *apch[2]) {
    if (!ppch || !(apch[0] = NextToken(ppch)))
        return 0;
    if (!(apch[1] = strchr(apch[0], '=')))
        return 1;
    *apch[1] = 0;
    apch[1]++;
    return 2;
}

/* -- SetToggle ---------------------------------------------------------------
 * Source: gnubg.c:1052 -- copied verbatim.
 */
int SetToggle(const char *szName, int *pf, char *sz,
              const char *szOn, const char *szOff) {
    char *pch = NextToken(&sz);
    size_t cch;
    if (!pch) {
        outputf(_("You must specify whether to set '%s' on or off.\n"), szName);
        return -1;
    }
    cch = strlen(pch);
    if (!StrCaseCmp("on", pch) || !StrNCaseCmp("yes", pch, cch) ||
        !StrNCaseCmp("true", pch, cch)) {
        outputl(szOn);
        if (*pf != TRUE) { *pf = TRUE; UpdateSetting(pf); }
        return TRUE;
    }
    if (!StrCaseCmp("off", pch) || !StrNCaseCmp("no", pch, cch) ||
        !StrNCaseCmp("false", pch, cch)) {
        outputl(szOff);
        if (*pf != FALSE) { *pf = FALSE; UpdateSetting(pf); }
        return FALSE;
    }
    outputf(_("Illegal keyword `%s'.\n"), pch);
    return -1;
}

/* -- set_web_browser ---------------------------------------------------------
 * On desktop opens a URL in the system browser.
 * On Android, post a URL event to the Kotlin layer.
 */
void set_web_browser(const char *sz) {
    (void)sz;
    /* Android: URL opening handled by Kotlin layer via Intent */
}

/* -- ExtInitParse / ExtStartParse / ExtDestroyParse --------------------------
 * External player protocol parser -- in external.c on desktop.
 * Check if they're there or need stubs.
 */

/* -- External parser stubs ---------------------------------------------------
 * ExtInitParse/ExtStartParse/ExtDestroyParse are generated from external_l.l
 * and external_y.y (lex/yacc). Not in build -- stub for now.
 * The external player protocol is a future feature.
 */
int  ExtInitParse(void **scancontext) { (void)scancontext; return 0; }
void ExtStartParse(void *scanner, const char *szCommand) {
    (void)scanner; (void)szCommand;
}
void ExtDestroyParse(void *scancontext) { (void)scancontext; }

/* -- Final batch of globals and functions from gnubg.c -----------------------
 * Source: gnubg.c lines 150-257
 */

/* Prompt strings */
const char szDefaultPrompt[] = "(\\p) ";
const char *szPrompt = szDefaultPrompt;

/* Match/rollout option globals */
int fCubeEqualChequer  = FALSE;
int fPlayersAreSame    = TRUE;
int fTruncEqualPlayer0 = TRUE;

/* Analysis file setting */
analyzeFileSetting AnalyzeFileSettingDef = AnalyzeFileBatch;
const char *aszAnalyzeFileSettingCommands[NUM_AnalyzeFileSettings] = {
    "batch", "single", "smart"
};

int fOutputRawboard = FALSE;

/* -- ParsePlayer -------------------------------------------------------------
 * Source: gnubg.c:849 -- copied verbatim.
 * Parses a player name/number token, returns 0, 1, 2 (both), or -1 (error).
 */
int ParsePlayer(char *sz) {
    int i;
    if (!sz)
        return -1;
    if ((*sz == '0' || *sz == '1') && !sz[1])
        return *sz - '0';
    for (i = 0; i < 2; i++)
        if (!CompareNames(sz, ap[i].szName))
            return i;
    if (!StrNCaseCmp(sz, "both", strlen(sz)))
        return 2;
    return -1;
}

/* -- CompareNames ------------------------------------------------------------
 * Source: gnubg.c:999 -- copied verbatim.
 * Case-insensitive comparison treating whitespace and underscores as equal.
 */
int CompareNames(char *sz0, char *sz1) {
    static char ach[] = " \t\r\n\f\v_";
    for (; *sz0 || *sz1; sz0++, sz1++)
        if (toupper(*sz0) != toupper(*sz1) &&
            (!strchr(ach, *sz0) || !strchr(ach, *sz1)))
            return toupper(*sz0) - toupper(*sz1);
    return 0;
}

/* -- Command tables -- all defined via commands.inc in gnubg.c ----------------
 * On Android the command system is driven by the Kotlin layer.
 * Provide empty sentinel-terminated tables as placeholders.
 * These will be populated when the command layer is implemented.
 */
command acSetEvalParam[]        = { { NULL, NULL, NULL, NULL, NULL } };
command acSetPlayer[]           = { { NULL, NULL, NULL, NULL, NULL } };
command acSetRNG[]              = { { NULL, NULL, NULL, NULL, NULL } };
command acSetRollout[]          = { { NULL, NULL, NULL, NULL, NULL } };
command acSetRolloutJsd[]       = { { NULL, NULL, NULL, NULL, NULL } };
command acSetRolloutLate[]      = { { NULL, NULL, NULL, NULL, NULL } };
command acSetRolloutLatePlayer[]= { { NULL, NULL, NULL, NULL, NULL } };
command acSetRolloutLimit[]     = { { NULL, NULL, NULL, NULL, NULL } };
command acSetRolloutPlayer[]    = { { NULL, NULL, NULL, NULL, NULL } };
command acSetTruncation[]       = { { NULL, NULL, NULL, NULL, NULL } };

/* -- Final batch of globals from gnubg.c -------------------------------------
 * Source: gnubg.c lines 148-527
 */
char *szLang               = NULL;
int   fInvertMET           = FALSE;
int   nThreadPriority      = 0;
int   nTutorSkillCurrent   = 0;
char *default_import_folder = NULL;
char *default_export_folder = NULL;

/* HTML export type/CSS strings -- defined in export.c or gnubg.c */
const char *aszHTMLExportType[] = { "gnu", "bbs", "fibs2html", NULL };
const char *aszHTMLExportCSS[]  = { "head", "inline", "external", NULL };

/* -- Command tables -- final batch --------------------------------------------
 * All defined via commands.inc in gnubg.c.
 */
command acSetAnalysisPlayer[]   = { { NULL, NULL, NULL, NULL, NULL } };
command acSetCheatPlayer[]      = { { NULL, NULL, NULL, NULL, NULL } };
command acSetExportParameters[] = { { NULL, NULL, NULL, NULL, NULL } };

/* -- CommandClearHint --------------------------------------------------------
 * Clears the hint display. On Android: board-changed notification.
 */
void CommandClearHint(char *UNUSED(sz)) {
    gnubg_on_board_changed();
}

/* -- CommandNotImplemented ---------------------------------------------------
 * Placeholder for commands not yet implemented on Android.
 */
void CommandNotImplemented(char *UNUSED(sz)) {
    outputl(_("This command is not yet implemented on Android."));
}

/* -- CommandShowVariation ----------------------------------------------------
 * Shows variation info. On Android: board-changed notification.
 */
void CommandShowVariation(char *UNUSED(sz)) {
    gnubg_on_board_changed();
}

/* -- SetupLanguage -----------------------------------------------------------
 * On desktop sets up gettext locale. On Android locale is managed by the OS.
 */
char *SetupLanguage(const char *newLangCode) {
    (void)newLangCode;
    /* Android: locale managed by Android OS via app resources */
    return NULL;
}

/* -- Sound globals -----------------------------------------------------------
 * Declared extern in sound.h. Defined here on Android.
 */
int fSound = TRUE;
int fQuiet = FALSE;

static char *aszSoundFiles[NUM_SOUNDS] = { NULL };

void SoundWait(void) {}

char *GetDefaultSoundFile(gnubgsound sound) { (void)sound; return NULL; }

char *GetSoundFile(gnubgsound sound) {
    if ((int)sound >= 0 && (int)sound < NUM_SOUNDS)
        return aszSoundFiles[sound];
    return NULL;
}

void SetSoundFile(gnubgsound sound, const char *file) {
    if ((int)sound >= 0 && (int)sound < NUM_SOUNDS) {
        g_free(aszSoundFiles[sound]);
        aszSoundFiles[sound] = file ? g_strdup(file) : NULL;
    }
}

void playSoundFile(char *file, gboolean sync) { (void)file; (void)sync; }

const char *sound_get_command(void) { return NULL; }

char *sound_set_command(const char *sz) { (void)sz; return NULL; }

void SetExitSoundOff(void) { fSound = FALSE; }

const char *sound_description[NUM_SOUNDS] = {
    "Starting GNU Backgammon", "Exiting GNU Backgammon",
    "Agree", "Double", "Drop", "Chequer movement", "Move",
    "Redouble", "Resign", "Roll", "Take",
    "Human dance", "Human wins game", "Human wins match",
    "Computer dance", "Computer wins game", "Computer wins match",
    "Analysis finished",
};

const char *sound_command[NUM_SOUNDS] = { NULL };
