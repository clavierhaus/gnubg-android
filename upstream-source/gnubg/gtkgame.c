/*
 * Copyright (C) 2000-2003 Gary Wong <gtw@gnu.org>
 * Copyright (C) 2001-2023 the AUTHORS
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"
#include "gnubgmodule.h"
#include "gtkuidefs.h"

#include <glib.h>
#include <ctype.h>
#include <glib/gstdio.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <locale.h>

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif
#if defined(HAVE_LIB_READLINE)
#include <readline/history.h>
#include <readline/readline.h>
#endif

#if defined(WIN32)
#include <io.h>
#endif

#include "analysis.h"
#include "backgammon.h"
#include "sgf.h"
#include "dice.h"
#include "drawboard.h"
#include "gtkboard.h"
#include "gtkchequer.h"
#include "gtkcube.h"
#include "gtkgame.h"
#include "gtkmet.h"
#include "gtkmovefilter.h"
#include "gtkprefs.h"
#include "gtksplash.h"
#include "gtkrelational.h"
#include "gtkfile.h"
#include "matchequity.h"
#include "openurl.h"
#include "positionid.h"
#include "sound.h"
#include "gtkoptions.h"
#include "gtktoolbar.h"
#include "format.h"
#include "formatgs.h"
#include "renderprefs.h"
#include "credits.h"
#include "matchid.h"
#include "gtkwindows.h"
#include "export.h"
#include "gtkmovelistctrl.h"
#include "rollout.h"
#include "util.h"
#include "gtkscoremap.h"
#if defined(USE_BOARD3D)
#include "inc3d.h"
#endif
#include "gnubgstock.h"

#define KEY_ESCAPE -229

#if defined(USE_BOARD3D)
	gboolean widget3dValid;

	#if defined(USE_GTKITEMFACTORY)
		/* Offset action to avoid predefined values */
		#define MENU_OFFSET 50
	#endif
#endif

#if !defined(USE_GTKITEMFACTORY)
static void TogglePanel(guint iType, guint iActionID, GtkToggleAction * action,
                        GtkToggleAction * alt, gpointer user_data);

static void ToolbarStyle(guint iType, guint iActionID, GtkRadioAction * action,
                         GtkRadioAction * alt, gpointer user_data);
#endif

static char *newLang;

/* Hack this for now to stop re-entering - should be fixed when menu switched to actions */
int inCallback = FALSE;

/* Enumeration to be used as index to the table of command strings below
 * (since GTK will only let us put integers into a GtkItemFactoryEntry,
 * and that might not be big enough to hold a pointer).  Must be kept in
 * sync with the string array aszCommands! */
typedef enum {
    CMD_ACCEPT,
    CMD_ANALYSE_CLEAR_MOVE,
    CMD_ANALYSE_CLEAR_GAME,
    CMD_ANALYSE_CLEAR_MATCH,
    CMD_ANALYSE_MOVE,
    CMD_ANALYSE_GAME,
    CMD_ANALYSE_MATCH,
    CMD_ANALYSE_ROLLOUT_CUBE,
    CMD_ANALYSE_ROLLOUT_MOVE,
    CMD_ANALYSE_ROLLOUT_GAME,
    CMD_ANALYSE_ROLLOUT_MATCH,
    CMD_CLEAR_TURN,
    CMD_CMARK_CUBE_CLEAR,
    CMD_CMARK_CUBE_SHOW,
    CMD_CMARK_MOVE_CLEAR,
    CMD_CMARK_MOVE_SHOW,
    CMD_CMARK_GAME_CLEAR,
    CMD_CMARK_GAME_SHOW,
    CMD_CMARK_MATCH_CLEAR,
    CMD_CMARK_MATCH_SHOW,
    CMD_END_GAME,
    CMD_DECLINE,
    CMD_DOUBLE,
    CMD_EVAL,
    CMD_HELP,
    CMD_HINT,
    CMD_LIST_GAME,
    CMD_NEXT,
    CMD_NEXT_GAME,
    CMD_NEXT_MARKED,
    CMD_NEXT_CMARKED,
    CMD_NEXT_ROLL,
    CMD_NEXT_ROLLED,
    CMD_PLAY,
    CMD_PREV,
    CMD_PREV_GAME,
    CMD_PREV_MARKED,
    CMD_PREV_CMARKED,
    CMD_PREV_ROLL,
    CMD_PREV_ROLLED,
    CMD_QUIT,
    CMD_REJECT,
    CMD_RELATIONAL_ADD_MATCH,
    CMD_ROLL,
    CMD_ROLLOUT,
    CMD_SET_ANNOTATION_ON,
    CMD_SET_APPEARANCE,
    CMD_SET_MESSAGE_ON,
    CMD_SET_TURN_0,
    CMD_SET_TURN_1,
    CMD_SHOW_CALIBRATION,
    CMD_SHOW_COPYING,
    CMD_SHOW_ENGINE,
    CMD_SHOW_EXPORT,
    CMD_SHOW_HISTORY,
    CMD_SHOW_MARKETWINDOW,
    CMD_SHOW_MATCHEQUITYTABLE,
    CMD_SHOW_KLEINMAN,
    CMD_SHOW_MANUAL_ABOUT,
    CMD_SHOW_MANUAL_WEB,
    CMD_SHOW_ROLLS,
    CMD_SHOW_STATISTICS_MATCH,
    CMD_SHOW_TEMPERATURE_MAP,
    CMD_SHOW_TEMPERATURE_MAP_CUBE,
    CMD_SHOW_SCORE_MAP_CUBE,
    CMD_SHOW_SCORE_MAP_MOVE,
    CMD_SHOW_VERSION,
    CMD_SHOW_WARRANTY,
    CMD_SWAP_PLAYERS,
    NUM_CMDS,
    TOGGLE_GAMELIST,
    TOGGLE_ANALYSIS,
    TOGGLE_COMMENTARY,
    TOGGLE_MESSAGE,
    TOGGLE_THEORY,
    TOGGLE_COMMAND,
    VIEW_TOOLBAR_ICONSONLY,
    VIEW_TOOLBAR_TEXTONLY,
    VIEW_TOOLBAR_BOTH
} gnubgcommand;

/* TRUE if GNUbg is automatically setting the state of a menu item. */
static int fAutoCommand;

#if !defined(USE_GTKITEMFACTORY)

/*
 * Some of the callback functions below, either explicit or
 * constructed with macros, are unused (were they added "just in case" ?)
 * Keep them for reference, but between #if 0 / #endif to avoid
 * compiler warnings.
 */

#if 0
static void
ExecToggleActionCommand_internal(guint UNUSED(iWidgetType), guint UNUSED(iCommand), gchar *szCommand,
                                 gpointer *widget, gpointer *UNUSED(widgetalt), gpointer UNUSED(user_data))
{

    char sz[80];

    if (fAutoCommand)
        return;

    sprintf(sz, "%s %s", szCommand, gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(widget)) ? "on" : "off");
    UserCommand(sz);

    return;
}
#endif

static void
ExecRadioActionCommand_internal(guint UNUSED(iWidgetType), guint iCommand, gchar * UNUSED(szCommand), gpointer * widget,
                                gpointer * UNUSED(widgetalt), gpointer UNUSED(user_data))
{

    char sz[80];
    gint actionID;

    if (fAutoCommand)
        return;

    actionID = gtk_radio_action_get_current_value(GTK_RADIO_ACTION(widget));
    switch (actionID) {

    case CMD_SET_TURN_0:
    case CMD_SET_TURN_1:
        sprintf(sz, "set turn %s", ap[actionID - iCommand].szName);
        UserCommand(sz);
        break;
    }

    return;
}

static void
ExecActionCommand_internal(guint UNUSED(iWidgetType), guint iCommand, gchar * szCommand, gpointer * UNUSED(widget),
                           gpointer * UNUSED(widgetalt), gpointer UNUSED(user_data))
{

    char sz[80];

    if (fAutoCommand)
        return;

    switch (iCommand) {
    case CMD_SET_APPEARANCE:
        BoardPreferences(pwBoard);
        return;

    case CMD_SET_TURN_0:
    case CMD_SET_TURN_1:
        sprintf(sz, "set turn %s", ap[iCommand - CMD_SET_TURN_0].szName);
        UserCommand(sz);
        return;

    case CMD_ANALYSE_MATCH:
        UserCommand("analyse match");
        if (fAutoDB) {
            /* add match to db */
            CommandRelationalAddMatch(NULL);
        }
        UserCommand("show statistics match");
        return;

    default:
        UserCommand(szCommand);
    }
}

#define CREATE_CMD_ACTION_CALLBACK(CMDID, szCommand) \
	static void CMDID##_action_cb ( GtkAction *action, gpointer user_data ) \
	{ \
		ExecActionCommand_internal ( 0, CMDID, szCommand, (gpointer)action, NULL, user_data ); \
		return; \
	} ;

#if 0
#define CREATE_CMD_TOGGLE_CALLBACK(CMDID, szCommand) \
	static void CMDID##_toggle_cb ( GtkToggleAction *action, gpointer user_data ) \
	{ \
		ExecToggleActionCommand_internal ( 1, CMDID, szCommand, (gpointer)action, NULL, user_data ); \
		return; \
	} ;
#endif

#define CREATE_CMD_RADIO_CALLBACK(CMDID, szCommand) \
	static void CMDID##_radio_cb ( GtkRadioAction *action, GtkRadioAction *current, gpointer user_data ) \
	{ \
		ExecRadioActionCommand_internal ( 2, CMDID, szCommand, (gpointer)action, (gpointer)current, user_data ); \
		return; \
	} ;

#if 0
#define CREATE_GENERIC_ACTION_CALLBACK(ACTIONID, callbackfunc) \
	static void ACTIONID##_action_g_cb ( GtkAction *action, gpointer user_data ) \
	{ \
		callbackfunc ( 0, ACTIONID, action, NULL, user_data ); \
		return; \
	} ;
#endif

#define CREATE_GENERIC_TOGGLE_CALLBACK(ACTIONID, callbackfunc) \
	static void ACTIONID##_toggle_g_cb ( GtkToggleAction *action, gpointer user_data ) \
	{ \
		callbackfunc ( 1, ACTIONID, action, NULL, user_data ); \
		return; \
	} ;

#define CREATE_GENERIC_RADIO_CALLBACK(ACTIONID, callbackfunc) \
	static void ACTIONID##_radio_g_cb ( GtkRadioAction *action, GtkRadioAction *current, gpointer user_data ) \
	{ \
		callbackfunc ( 2, ACTIONID, action, current, user_data ); \
		return; \
	} ;


#define CMD_ACTION_CALLBACK_FROMID(CMDID) (GCallback)CMDID##_action_cb

#if 0
#define CMD_TOGGLE_CALLBACK_FROMID(CMDID) (GCallback)CMDID##_toggle_cb
#endif

#define CMD_RADIO_CALLBACK_FROMID(CMDID)  (GCallback)CMDID##_radio_cb

#if 0
#define GENERIC_ACTION_CALLBACK_FROMID(ACTIONID) (GCallback)ACTIONID##_action_g_cb
#endif

#define GENERIC_TOGGLE_CALLBACK_FROMID(ACTIONID) (GCallback)ACTIONID##_toggle_g_cb
#define GENERIC_RADIO_CALLBACK_FROMID(ACTIONID)  (GCallback)ACTIONID##_radio_g_cb

/* Create callback functions for all the commands. These do no need to be in any particular order */
CREATE_CMD_ACTION_CALLBACK(CMD_ACCEPT, "accept");
CREATE_CMD_ACTION_CALLBACK(CMD_ANALYSE_CLEAR_MOVE, "analyse clear move");
CREATE_CMD_ACTION_CALLBACK(CMD_ANALYSE_CLEAR_GAME, "analyse clear game");
CREATE_CMD_ACTION_CALLBACK(CMD_ANALYSE_CLEAR_MATCH, "analyse clear match");
CREATE_CMD_ACTION_CALLBACK(CMD_ANALYSE_MOVE, "analyse move");
CREATE_CMD_ACTION_CALLBACK(CMD_ANALYSE_GAME, "analyse game");
CREATE_CMD_ACTION_CALLBACK(CMD_ANALYSE_MATCH, "analyse match");
CREATE_CMD_ACTION_CALLBACK(CMD_ANALYSE_ROLLOUT_CUBE, "analyse rollout cube");
CREATE_CMD_ACTION_CALLBACK(CMD_ANALYSE_ROLLOUT_MOVE, "analyse rollout move");
CREATE_CMD_ACTION_CALLBACK(CMD_ANALYSE_ROLLOUT_GAME, "analyse rollout game");
CREATE_CMD_ACTION_CALLBACK(CMD_ANALYSE_ROLLOUT_MATCH, "analyse rollout match");
CREATE_CMD_ACTION_CALLBACK(CMD_CLEAR_TURN, "clear turn");
CREATE_CMD_ACTION_CALLBACK(CMD_CMARK_CUBE_CLEAR, "cmark cube clear");
CREATE_CMD_ACTION_CALLBACK(CMD_CMARK_CUBE_SHOW, "cmark cube show");
CREATE_CMD_ACTION_CALLBACK(CMD_CMARK_MOVE_CLEAR, "cmark move clear");
CREATE_CMD_ACTION_CALLBACK(CMD_CMARK_MOVE_SHOW, "cmark move show");
CREATE_CMD_ACTION_CALLBACK(CMD_CMARK_GAME_CLEAR, "cmark game clear");
CREATE_CMD_ACTION_CALLBACK(CMD_CMARK_GAME_SHOW, "cmark game show");
CREATE_CMD_ACTION_CALLBACK(CMD_CMARK_MATCH_CLEAR, "cmark match clear");
CREATE_CMD_ACTION_CALLBACK(CMD_CMARK_MATCH_SHOW, "cmark match show");
CREATE_CMD_ACTION_CALLBACK(CMD_END_GAME, "end game");
#if 0
CREATE_CMD_ACTION_CALLBACK(CMD_DECLINE, "decline");
#endif
CREATE_CMD_ACTION_CALLBACK(CMD_DOUBLE, "double");
CREATE_CMD_ACTION_CALLBACK(CMD_EVAL, "eval");
CREATE_CMD_ACTION_CALLBACK(CMD_HELP, "help");
CREATE_CMD_ACTION_CALLBACK(CMD_HINT, "hint");
#if 0
CREATE_CMD_ACTION_CALLBACK(CMD_LIST_GAME, "list game");
CREATE_CMD_ACTION_CALLBACK(CMD_NEXT, "next");
#endif
CREATE_CMD_ACTION_CALLBACK(CMD_NEXT_GAME, "next game");
CREATE_CMD_ACTION_CALLBACK(CMD_NEXT_MARKED, "next marked");
CREATE_CMD_ACTION_CALLBACK(CMD_NEXT_CMARKED, "next cmarked");
CREATE_CMD_ACTION_CALLBACK(CMD_NEXT_ROLL, "next roll");
#if 0
CREATE_CMD_ACTION_CALLBACK(CMD_NEXT_ROLLED, "next rolled");
#endif
CREATE_CMD_ACTION_CALLBACK(CMD_PLAY, "play");
#if 0
CREATE_CMD_ACTION_CALLBACK(CMD_PREV, "previous");
#endif
CREATE_CMD_ACTION_CALLBACK(CMD_PREV_GAME, "previous game");
CREATE_CMD_ACTION_CALLBACK(CMD_PREV_MARKED, "previous marked");
CREATE_CMD_ACTION_CALLBACK(CMD_PREV_CMARKED, "previous cmarked");
CREATE_CMD_ACTION_CALLBACK(CMD_PREV_ROLL, "previous roll");
#if 0
CREATE_CMD_ACTION_CALLBACK(CMD_PREV_ROLLED, "previous rolled");
#endif
CREATE_CMD_ACTION_CALLBACK(CMD_QUIT, "quit");
CREATE_CMD_ACTION_CALLBACK(CMD_REJECT, "reject");
#if 0
CREATE_CMD_ACTION_CALLBACK(CMD_RELATIONAL_ADD_MATCH, "relational add match");
#endif
CREATE_CMD_ACTION_CALLBACK(CMD_ROLL, "roll");
CREATE_CMD_ACTION_CALLBACK(CMD_ROLLOUT, "rollout");
#if 0
CREATE_CMD_ACTION_CALLBACK(CMD_SET_ANNOTATION_ON, "set annotation on");
#endif
CREATE_CMD_ACTION_CALLBACK(CMD_SET_APPEARANCE, NULL);   /* set appearance */
#if 0
CREATE_CMD_ACTION_CALLBACK(CMD_SET_MESSAGE_ON, "set message on");
CREATE_CMD_ACTION_CALLBACK(CMD_SET_TURN_0, NULL);       /* set turn 0 */
CREATE_CMD_ACTION_CALLBACK(CMD_SET_TURN_1, NULL);       /* set turn 1 */
#endif
CREATE_CMD_ACTION_CALLBACK(CMD_SHOW_CALIBRATION, "show calibration");
#if 0
CREATE_CMD_ACTION_CALLBACK(CMD_SHOW_COPYING, "show copying");
CREATE_CMD_ACTION_CALLBACK(CMD_SHOW_ENGINE, "show engine");
#endif
CREATE_CMD_ACTION_CALLBACK(CMD_SHOW_EXPORT, "show export");
CREATE_CMD_ACTION_CALLBACK(CMD_SHOW_HISTORY, "show history");
CREATE_CMD_ACTION_CALLBACK(CMD_SHOW_MARKETWINDOW, "show marketwindow");
CREATE_CMD_ACTION_CALLBACK(CMD_SHOW_MATCHEQUITYTABLE, "show matchequitytable");
CREATE_CMD_ACTION_CALLBACK(CMD_SHOW_KLEINMAN, "show kleinman"); /* opens race theory window */
CREATE_CMD_ACTION_CALLBACK(CMD_SHOW_MANUAL_ABOUT, "show manual about");
CREATE_CMD_ACTION_CALLBACK(CMD_SHOW_MANUAL_WEB, "show manual web");
CREATE_CMD_ACTION_CALLBACK(CMD_SHOW_ROLLS, "show rolls");
CREATE_CMD_ACTION_CALLBACK(CMD_SHOW_STATISTICS_MATCH, "show statistics match");
CREATE_CMD_ACTION_CALLBACK(CMD_SHOW_TEMPERATURE_MAP, "show temperaturemap");
CREATE_CMD_ACTION_CALLBACK(CMD_SHOW_TEMPERATURE_MAP_CUBE, "show temperaturemap =cube");
CREATE_CMD_ACTION_CALLBACK(CMD_SHOW_SCORE_MAP_CUBE, "show scoremap");
CREATE_CMD_ACTION_CALLBACK(CMD_SHOW_SCORE_MAP_MOVE, "show scoremap =move");
CREATE_CMD_ACTION_CALLBACK(CMD_SHOW_VERSION, "show version");
#if 0
CREATE_CMD_ACTION_CALLBACK(CMD_SHOW_WARRANTY, "show warranty");
#endif
CREATE_CMD_ACTION_CALLBACK(CMD_SWAP_PLAYERS, "swap players");
CREATE_GENERIC_TOGGLE_CALLBACK(TOGGLE_GAMELIST, TogglePanel);
CREATE_GENERIC_TOGGLE_CALLBACK(TOGGLE_COMMENTARY, TogglePanel);
CREATE_GENERIC_TOGGLE_CALLBACK(TOGGLE_MESSAGE, TogglePanel);
CREATE_GENERIC_TOGGLE_CALLBACK(TOGGLE_ANALYSIS, TogglePanel);
CREATE_GENERIC_TOGGLE_CALLBACK(TOGGLE_THEORY, TogglePanel);
CREATE_GENERIC_TOGGLE_CALLBACK(TOGGLE_COMMAND, TogglePanel);
CREATE_GENERIC_RADIO_CALLBACK(VIEW_TOOLBAR_ICONSONLY, ToolbarStyle);
CREATE_CMD_RADIO_CALLBACK(CMD_SET_TURN_0, NULL);
#else

static const char *aszCommands[NUM_CMDS] = {
    "accept",
    "analyse clear move",
    "analyse clear game",
    "analyse clear match",
    "analyse move",
    "analyse game",
    "analyse match",
    "analyse rollout cube",
    "analyse rollout move",
    "analyse rollout game",
    "analyse rollout match",
    "clear turn",
    "cmark cube clear",
    "cmark cube show",
    "cmark move clear",
    "cmark move show",
    "cmark game clear",
    "cmark game show",
    "cmark match clear",
    "cmark match show",
    "end game",
    "decline",
    "double",
    "eval",
    "help",
    "hint",
    "list game",
    "next",
    "next game",
    "next marked",
    "next cmarked",
    "next roll",
    "next rolled",
    "play",
    "previous",
    "previous game",
    "previous marked",
    "previous cmarked",
    "previous roll",
    "previous rolled",
    "quit",
    "reject",
    "relational add match",
    "roll",
    "rollout",
    "set annotation on",
    NULL,                       /* set appearance */
    "set message on",
    NULL,                       /* set turn 0 */
    NULL,                       /* set turn 1 */
    "show calibration",
    "show copying",
    "show engine",
    "show export",
    "show history",
    "show marketwindow",
    "show matchequitytable",
    "show kleinman",            /* opens race theory window */
    "show manual about",
    "show manual web",
    "show rolls",
    "show statistics match",
    "show temperaturemap",
    "show temperaturemap =cube",
    "show scoremap",
    "show scoremap =move",
    "show version",
    "show warranty",
    "swap players",
};

static void
Command(gpointer UNUSED(p), guint iCommand, GtkWidget * widget)
{

    char sz[80];

    if (fAutoCommand)
        return;

    /* FIXME this isn't very good -- if UserCommand fails, the setting
     * won't have been changed, but the widget will automatically have
     * updated itself. */

    if (GTK_IS_RADIO_MENU_ITEM(widget)) {
        if (!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget)))
            return;
    } else if (GTK_IS_CHECK_MENU_ITEM(widget)) {
        sprintf(sz, "%s %s", aszCommands[iCommand],
                gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget)) ? "on" : "off");
        UserCommand(sz);

        return;
    }

    switch (iCommand) {
    case CMD_SET_APPEARANCE:
        BoardPreferences(pwBoard);
        return;

    case CMD_SET_TURN_0:
    case CMD_SET_TURN_1:
        sprintf(sz, "set turn %s", ap[iCommand - CMD_SET_TURN_0].szName);
        UserCommand(sz);
        return;

    case CMD_ANALYSE_MATCH:
        UserCommand(aszCommands[CMD_ANALYSE_MATCH]);
        if (fAutoDB) {
            /* add match to db */
            CommandRelationalAddMatch(NULL);
        }
        UserCommand(aszCommands[CMD_SHOW_STATISTICS_MATCH]);
        return;

    default:
        UserCommand(aszCommands[iCommand]);
    }
}

#endif

typedef struct {
    const char *title;
    evalcontext *esChequer;
    movefilter *mfChequer;
    evalcontext *esCube;
    movefilter *mfCube;
    GtkWidget *pwCube, *pwChequer, *pwOptionMenu, *pwSettingWidgets;
    int cubeDisabled;
    int fWeakLevels;
} AnalysisDetails;


typedef struct {

    evalsetup esChequer;
    evalsetup esCube;
    movefilter aamf[MAX_FILTER_PLIES][MAX_FILTER_PLIES];

    evalsetup esEvalChequer;
    evalsetup esEvalCube;
    movefilter aaEvalmf[MAX_FILTER_PLIES][MAX_FILTER_PLIES];

    GtkWidget *pwNoteBook;
    GtkAdjustment *apadjSkill[3], *apadjLuck[4];
    GtkWidget *pwMoves, *pwCube, *pwLuck, *pwHintSame, *pwCubeSummary;
    GtkWidget *apwAnalysePlayers[2];
    GtkWidget *pwAutoDB;
    GtkWidget *pwBackgroundAnalysis;
    GtkWidget *apwAnalyzeFileSetting[NUM_AnalyzeFileSettings];

    GtkWidget *pwScoreMap;
    GtkWidget *apwScoreMapPly[NUM_PLY];
    GtkWidget *apwScoreMapMatchLength[NUM_MATCH_LENGTH];
    GtkWidget *apwScoreMapLabel[NUM_LABEL];
    GtkWidget *apwScoreMapJacoby[NUM_JACOBY];
    GtkWidget *apwScoreMapCubeEquityDisplay[NUM_CUBEDISP];
    GtkWidget *apwScoreMapMoveEquityDisplay[NUM_MOVEDISP];
    GtkWidget *apwScoreMapColour[NUM_COLOUR];
    GtkWidget *apwScoreMapLayout[NUM_LAYOUT];

    /* defining these just to be able to g_free them */
    AnalysisDetails *pAnalDetailSettings1;
    AnalysisDetails *pAnalDetailSettings2;

} analysiswidget;

/* A dummy widget that can grab events when others shouldn't see them. */
GtkWidget *pwGrab;
GtkWidget *pwOldGrab;

GtkWidget *pwBoard;
GtkWidget *pwMain = NULL;
static GtkWidget *pwMenuBar;
GtkWidget *pwToolbar;

static GtkWidget *pwStatus;
static GtkWidget *pwIDBox;
GtkWidget *pwGnubgID;
static GtkWidget *pwProgress;
GtkWidget *pwMessageText;
GtkWidget *pwPanelVbox;
GtkWidget *pwAnalysis;
GtkWidget *pwCommentary;
static moverecord *pmrAnnotation;
GtkAccelGroup *pagMain;
#if defined(USE_GTKITEMFACTORY)
GtkItemFactory *pif;
#else
GtkUIManager *puim = NULL;
#endif
guint nNextTurn = 0;            /* GTK idle function */
static guint idOutput, idProgress;
int fTTY = TRUE;
int fGUISetWindowPos = TRUE;
int frozen = FALSE;
static GString *output_str = NULL;
static int fullScreenOnStartup = FALSE;

static guint nStdin, nDisabledCount = 1;
static GIOChannel *pStdin;

/* Save state of windows for full screen */
static int showingPanels, showingIDs, maximised;


static gulong grabIdSignal;
static int suspendCount = 0;
static GtkWidget *grabbedWidget;

/* Language selection code */
static GtkWidget *curSel;
static GtkWidget *pwLangDialog, *pwLangRadio1, *pwLangRadio2, *pwLangTable;

static GtkWidget *pwHelpTree, *pwHelpLabel;

GtkWidget *hpaned;
static GtkWidget *pwGameBox;
static GtkWidget *pwPanelGameBox;
static GtkWidget *pwEventBox;
static int panelSize = 325;
static GtkWidget *pwStop;


extern void
GTKSuspendInput(void)
{
    if (!fX)
        return;

    /* when the fBackgroundAnalysis global variable is set, we allow the user to
     * (1) continue browsing and (2) stop the computation;
     * else we grab the focus and kill any user input
     */
    if (!fBackgroundAnalysis) {
        if (suspendCount == 0 && pwGrab && GDK_IS_WINDOW(gtk_widget_get_window(pwGrab))) {
            /* Grab events so that the board window knows this is a re-entrant */
            /*  call, and won't allow commands like roll, move or double. */
            grabbedWidget = pwGrab;
            if (pwGrab == pwStop) {
                gtk_widget_grab_focus(pwStop);
                gtk_widget_set_sensitive(pwStop, TRUE);
            }
            gtk_grab_add(pwGrab);
            grabIdSignal = g_signal_connect_after(G_OBJECT(pwGrab), "key-press-event", G_CALLBACK(gtk_true), NULL);
        }

        /* Don't check stdin here; readline isn't ready yet. */
        GTKDisallowStdin();
    } else {
        if (pwGrab == pwStop) {
            gtk_widget_grab_focus(pwStop);
            gtk_widget_set_sensitive(pwStop, TRUE);
        }
    }
    suspendCount++;
}

extern void
GTKResumeInput(void)
{
    if (!fX)
        return;
    g_assert(suspendCount > 0);
    suspendCount--;
    if (suspendCount == 0) {
        if (GTK_IS_WIDGET(grabbedWidget) && gtk_widget_has_grab(grabbedWidget)) {
            if (g_signal_handler_is_connected(G_OBJECT(grabbedWidget), grabIdSignal))
                g_signal_handler_disconnect(G_OBJECT(grabbedWidget), grabIdSignal);
            gtk_grab_remove(grabbedWidget);
        }
        if (pwGrab == pwStop)
            gtk_widget_set_sensitive(pwStop, FALSE);
    }

    GTKAllowStdin();
}

static gboolean
StdinReadNotify(GIOChannel * UNUSED(source), GIOCondition UNUSED(cond), gpointer UNUSED(p))
{
#if defined(HAVE_LIB_READLINE)
    /* Handle "next turn" processing before more input (otherwise we might
     * not even have a readline handler installed!) */
    while (nNextTurn)
        NextTurnNotify(NULL);

    rl_callback_read_char();

    return TRUE;
#else
    char sz[2048], *pch;

    while (nNextTurn)
        NextTurnNotify(NULL);

    if (fgets(sz, sizeof(sz), stdin) == NULL) {
        if (!isatty(STDIN_FILENO))
            exit(EXIT_SUCCESS);

        PromptForExit();
        return TRUE;
    }

    if ((pch = strchr(sz, '\n')))
        *pch = 0;

    fInterrupt = FALSE;

    HandleCommand(sz, acTop);

    ResetInterrupt();

    if (nNextTurn)
        fNeedPrompt = TRUE;
    else
        Prompt();

    return TRUE;
#endif
}


extern void
GTKAllowStdin(void)
{

    if (!fTTY || !nDisabledCount)
        return;

    if (!--nDisabledCount) {
        pStdin = g_io_channel_unix_new(STDIN_FILENO);
        nStdin = g_io_add_watch_full(pStdin, G_PRIORITY_HIGH, G_IO_IN | G_IO_PRI, StdinReadNotify, NULL, NULL);
    }
}

extern void
GTKDisallowStdin(void)
{

    if (!fTTY)
        return;

    nDisabledCount++;

    if (nStdin) {
        g_source_remove(nStdin);
        g_io_channel_unref(pStdin);
        nStdin = 0;
    }
}

int fEndDelay;

extern void
GTKDelay(void)
{

    GTKSuspendInput();

    while (!fInterrupt && !fEndDelay)
        gtk_main_iteration();

    fEndDelay = FALSE;

    GTKResumeInput();
}


static void
gui_clear_turn(GtkWidget * UNUSED(pw), GtkWidget * dialog)
{
    if (dialog)
        gtk_widget_destroy(dialog);
    CommandClearTurn(NULL);
}

extern int
GTKGetManualDice(unsigned int an[2])
{
    GtkWidget *dialog;
    GtkWidget *dice;
    GtkWidget *buttons;
    GtkWidget *clear;
    BoardData *bd = BOARD(pwBoard)->board_data;

    manualDiceType mdt;
    if (ToolbarIsEditing(pwToolbar))
        mdt = MT_EDIT;
    else if (plLastMove && ((moverecord *) plLastMove->p)->mt == MOVE_GAMEINFO)
        mdt = MT_FIRSTMOVE;
    else
        mdt = MT_STANDARD;

    dialog = GTKCreateDialog(_("GNU Backgammon - Dice"), DT_INFO, NULL,
                             DIALOG_FLAG_MODAL | DIALOG_FLAG_CLOSEBUTTON, NULL, NULL);
    dice = board_dice_widget(BOARD(pwBoard), mdt);

    gtk_container_add(GTK_CONTAINER(DialogArea(dialog, DA_MAIN)), dice);

    buttons = DialogArea(dialog, DA_BUTTONS);
    clear = gtk_button_new_with_label(_("Clear Dice"));
    gtk_container_add(GTK_CONTAINER(buttons), clear);
    g_signal_connect(G_OBJECT(clear), "clicked", G_CALLBACK(gui_clear_turn), dialog);
    gtk_widget_set_sensitive(GTK_WIDGET(clear), bd->diceShown == DICE_ON_BOARD);

    g_object_set_data(G_OBJECT(dice), "user_data", an);
    an[0] = 0;

    GTKRunDialog(dialog);

    if (mdt == MT_EDIT && an[0]) {
        if (an[0] > an[1] && bd->turn != -1)
            UserCommand("set turn 0");
        else if (an[0] < an[1] && bd->turn != 1)
            UserCommand("set turn 1");
    }

    return an[0] ? 0 : -1;
}

extern void
GTKSetDice(gpointer UNUSED(p), guint UNUSED(n), GtkWidget * UNUSED(pw))
{
    unsigned int an[2];

    if (!GTKGetManualDice(an)) {
        char sz[13];            /* "set dice x y" */

        sprintf(sz, "set dice %u %u", an[0], an[1]);
        UserCommand(sz);
    }
}

extern void
GTKSetCube(gpointer UNUSED(p), guint UNUSED(n), GtkWidget * UNUSED(pw))
{
    gchar *sz;
    GtkWidget *pwDialog, *pwCube;
    int an[2];
    int valChanged;

    if (ms.gs != GAME_PLAYING || ms.fCrawford || !ms.fCubeUse)
        return;

    pwDialog = GTKCreateDialog(_("GNU Backgammon - Cube"), DT_INFO,
                               NULL, DIALOG_FLAG_MODAL | DIALOG_FLAG_CLOSEBUTTON, NULL, NULL);
    pwCube = board_cube_widget(BOARD(pwBoard));

    an[0] = -1;

    gtk_container_add(GTK_CONTAINER(DialogArea(pwDialog, DA_MAIN)), pwCube);
    g_object_set_data(G_OBJECT(pwCube), "user_data", an);

    g_signal_connect(G_OBJECT(pwCube), "destroy", G_CALLBACK(DestroySetCube), pwDialog);

    GTKRunDialog(pwDialog);

    if (an[0] < 0)
        return;

    valChanged = (1 << an[0] != ms.nCube);
    if (valChanged) {
        sz = g_strdup_printf("set cube value %d", 1 << an[0]);
        UserCommand(sz);
        g_free(sz);
    }

    if (an[1] != ms.fCubeOwner) {
        if (an[1] >= 0) {
            sz = g_strdup_printf("set cube owner %d", an[1]);
            UserCommand(sz);
            g_free(sz);
        } else
            UserCommand("set cube centre");
    }
}

static int fAutoCommentaryChange;

extern void
CommentaryChanged(GtkWidget * UNUSED(pw), GtkTextBuffer * buffer)
{

    char *pch;
    GtkTextIter begin, end;

    if (fAutoCommentaryChange)
        return;

    g_assert(pmrAnnotation);

    /* FIXME Copying the entire text every time it's changed is horribly
     * inefficient, but the only alternatives seem to be lazy copying
     * (which is much harder to get right) or requiring a specific command
     * to update the text (which is probably inconvenient for the user). */

    if (pmrAnnotation->sz)
        g_free(pmrAnnotation->sz);

    gtk_text_buffer_get_bounds(buffer, &begin, &end);
    pch = gtk_text_buffer_get_text(buffer, &begin, &end, FALSE);
    /* This copy is absolutely disgusting, but is necessary because GTK
     * insists on giving us something allocated with g_malloc() instead
     * of malloc(). */
    pmrAnnotation->sz = g_strdup(pch);
    g_free(pch);
}

extern void
GTKFreeze(void)
{

    frozen = TRUE;
}

extern void
GTKThaw(void)
{

    frozen = FALSE;
    /* Make sure analysis window is correct */
    if (plLastMove)
        GTKSetMoveRecord(plLastMove->p);
}

static GtkWidget *
ResignAnalysis(float arResign[NUM_ROLLOUT_OUTPUTS], int nResigned, evalsetup * pesResign)
{

    cubeinfo ci;
#if GTK_CHECK_VERSION(3,0,0)
    GtkWidget *pwGrid = gtk_grid_new();
#else
    GtkWidget *pwTable = gtk_table_new(3, 2, FALSE);
#endif
    GtkWidget *pwLabel;

    float rAfter, rBefore;

    char sz[64];

    if (pesResign->et == EVAL_NONE)
        return NULL;

    GetMatchStateCubeInfo(&ci, &ms);


    /* First column with text */

    pwLabel = gtk_label_new(_("Equity before resignation: "));
#if GTK_CHECK_VERSION(3,0,0)
    gtk_widget_set_halign(pwLabel, GTK_ALIGN_START);
    gtk_widget_set_valign(pwLabel, GTK_ALIGN_CENTER);
    gtk_grid_attach(GTK_GRID(pwGrid), pwLabel, 0, 0, 1, 1);
    gtk_widget_set_hexpand(pwLabel, TRUE);
    gtk_widget_set_vexpand(pwLabel, TRUE);
#if GTK_CHECK_VERSION(3,12,0)
    gtk_widget_set_margin_start(pwLabel, 8);
    gtk_widget_set_margin_end(pwLabel, 8);
#else
    gtk_widget_set_margin_left(pwLabel, 8);
    gtk_widget_set_margin_right(pwLabel, 8);
#endif
    gtk_widget_set_margin_top(pwLabel, 2);
    gtk_widget_set_margin_bottom(pwLabel, 2);
#else
    gtk_misc_set_alignment(GTK_MISC(pwLabel), 0, 0.5);
    gtk_table_attach(GTK_TABLE(pwTable), pwLabel, 0, 1, 0, 1, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 8, 2);
#endif

    pwLabel = gtk_label_new(_("Equity after resignation: "));
#if GTK_CHECK_VERSION(3,0,0)
    gtk_widget_set_halign(pwLabel, GTK_ALIGN_START);
    gtk_widget_set_valign(pwLabel, GTK_ALIGN_CENTER);
    gtk_grid_attach(GTK_GRID(pwGrid), pwLabel, 0, 1, 1, 1);
    gtk_widget_set_hexpand(pwLabel, TRUE);
    gtk_widget_set_vexpand(pwLabel, TRUE);
#if GTK_CHECK_VERSION(3,12,0)
    gtk_widget_set_margin_start(pwLabel, 8);
    gtk_widget_set_margin_end(pwLabel, 8);
#else
    gtk_widget_set_margin_left(pwLabel, 8);
    gtk_widget_set_margin_right(pwLabel, 8);
#endif
    gtk_widget_set_margin_top(pwLabel, 2);
    gtk_widget_set_margin_bottom(pwLabel, 2);
#else
    gtk_misc_set_alignment(GTK_MISC(pwLabel), 0, 0.5);
    gtk_table_attach(GTK_TABLE(pwTable), pwLabel, 0, 1, 1, 2, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 8, 2);
#endif

    pwLabel = gtk_label_new(_("Difference: "));
#if GTK_CHECK_VERSION(3,0,0)
    gtk_widget_set_halign(pwLabel, GTK_ALIGN_START);
    gtk_widget_set_valign(pwLabel, GTK_ALIGN_CENTER);
    gtk_grid_attach(GTK_GRID(pwGrid), pwLabel, 0, 2, 1, 1);
    gtk_widget_set_hexpand(pwLabel, TRUE);
    gtk_widget_set_vexpand(pwLabel, TRUE);
#if GTK_CHECK_VERSION(3,12,0)
    gtk_widget_set_margin_start(pwLabel, 8);
    gtk_widget_set_margin_end(pwLabel, 8);
#else
    gtk_widget_set_margin_left(pwLabel, 8);
    gtk_widget_set_margin_right(pwLabel, 8);
#endif
    gtk_widget_set_margin_top(pwLabel, 2);
    gtk_widget_set_margin_bottom(pwLabel, 2);
#else
    gtk_misc_set_alignment(GTK_MISC(pwLabel), 0, 0.5);
    gtk_table_attach(GTK_TABLE(pwTable), pwLabel, 0, 1, 2, 3, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 8, 2);
#endif

    /* Second column: equities/mwc */

    getResignEquities(arResign, &ci, nResigned, &rBefore, &rAfter);

    if (fOutputMWC && ms.nMatchTo)
        sprintf(sz, "%6.2f%%", eq2mwc(rBefore, &ci) * 100.0f);
    else
        sprintf(sz, "%+6.3f", rBefore);

    pwLabel = gtk_label_new(sz);
#if GTK_CHECK_VERSION(3,0,0)
    gtk_widget_set_halign(pwLabel, GTK_ALIGN_END);
    gtk_widget_set_valign(pwLabel, GTK_ALIGN_CENTER);
    gtk_grid_attach(GTK_GRID(pwGrid), pwLabel, 1, 0, 1, 1);
    gtk_widget_set_hexpand(pwLabel, TRUE);
    gtk_widget_set_vexpand(pwLabel, TRUE);
#if GTK_CHECK_VERSION(3,12,0)
    gtk_widget_set_margin_start(pwLabel, 8);
    gtk_widget_set_margin_end(pwLabel, 8);
#else
    gtk_widget_set_margin_left(pwLabel, 8);
    gtk_widget_set_margin_right(pwLabel, 8);
#endif
    gtk_widget_set_margin_top(pwLabel, 2);
    gtk_widget_set_margin_bottom(pwLabel, 2);
#else
    gtk_misc_set_alignment(GTK_MISC(pwLabel), 1, 0.5);
    gtk_table_attach(GTK_TABLE(pwTable), pwLabel, 1, 2, 0, 1, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 8, 2);
#endif


    if (fOutputMWC && ms.nMatchTo)
        sprintf(sz, "%6.2f%%", eq2mwc(rAfter, &ci) * 100.0f);
    else
        sprintf(sz, "%+6.3f", rAfter);

    pwLabel = gtk_label_new(sz);
#if GTK_CHECK_VERSION(3,0,0)
    gtk_widget_set_halign(pwLabel, GTK_ALIGN_END);
    gtk_widget_set_valign(pwLabel, GTK_ALIGN_CENTER);
    gtk_grid_attach(GTK_GRID(pwGrid), pwLabel, 1, 1, 1, 1);
    gtk_widget_set_hexpand(pwLabel, TRUE);
    gtk_widget_set_vexpand(pwLabel, TRUE);
#if GTK_CHECK_VERSION(3,12,0)
    gtk_widget_set_margin_start(pwLabel, 8);
    gtk_widget_set_margin_end(pwLabel, 8);
#else
    gtk_widget_set_margin_left(pwLabel, 8);
    gtk_widget_set_margin_right(pwLabel, 8);
#endif
    gtk_widget_set_margin_top(pwLabel, 2);
    gtk_widget_set_margin_bottom(pwLabel, 2);
#else
    gtk_misc_set_alignment(GTK_MISC(pwLabel), 1, 0.5);
    gtk_table_attach(GTK_TABLE(pwTable), pwLabel, 1, 2, 1, 2, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 8, 2);
#endif


    if (fOutputMWC && ms.nMatchTo)
        sprintf(sz, "%+6.2f%%", (eq2mwc(rAfter, &ci) - eq2mwc(rBefore, &ci)) * 100.0f);
    else
        sprintf(sz, "%+6.3f", rAfter - rBefore);

    pwLabel = gtk_label_new(sz);
#if GTK_CHECK_VERSION(3,0,0)
    gtk_widget_set_halign(pwLabel, GTK_ALIGN_END);
    gtk_widget_set_valign(pwLabel, GTK_ALIGN_CENTER);
    gtk_grid_attach(GTK_GRID(pwGrid), pwLabel, 1, 2, 1, 1);
    gtk_widget_set_hexpand(pwLabel, TRUE);
    gtk_widget_set_vexpand(pwLabel, TRUE);
#if GTK_CHECK_VERSION(3,12,0)
    gtk_widget_set_margin_start(pwLabel, 8);
    gtk_widget_set_margin_end(pwLabel, 8);
#else
    gtk_widget_set_margin_left(pwLabel, 8);
    gtk_widget_set_margin_right(pwLabel, 8);
#endif
    gtk_widget_set_margin_top(pwLabel, 2);
    gtk_widget_set_margin_bottom(pwLabel, 2);
#else
    gtk_misc_set_alignment(GTK_MISC(pwLabel), 1, 0.5);
    gtk_table_attach(GTK_TABLE(pwTable), pwLabel, 1, 2, 2, 3, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 8, 2);
#endif

#if GTK_CHECK_VERSION(3,0,0)
    return pwGrid;
#else
    return pwTable;
#endif
}



GtkWidget *pwMoveAnalysis = NULL;

static GtkWidget *
luck_label(lucktype lt)
{
    GtkWidget *label;
    gchar *markup;
    const gchar *skill;
    const gchar *color[N_LUCKS] = { "red", "orange", "black", "green", "white" };

    label = gtk_label_new(NULL);
    skill = aszLuckType[lt] ? gettext(aszLuckType[lt]) : "";
    markup =
        g_strdup_printf("<span foreground=\"%s\" background=\"black\" weight=\"bold\">%s</span>", color[lt], skill);
    gtk_label_set_markup(GTK_LABEL(label), markup);
    g_free(markup);
    return label;
}

static GtkWidget *
skill_label(skilltype st)
{
    GtkWidget *label;
    gchar *markup;
    const gchar *skill;
    const gchar *color[N_SKILLS] = { "red", "orange", "yellow", "black" };

    label = gtk_label_new(NULL);
    skill = aszSkillType[st] ? gettext(aszSkillType[st]) : "";
    markup =
        g_strdup_printf("<span foreground=\"%s\" background=\"black\" weight=\"bold\">%s</span>", color[st], skill);
    gtk_label_set_markup(GTK_LABEL(label), markup);
    g_free(markup);
    return label;
}

extern void
SetAnnotation(moverecord * pmr)
{
    GtkWidget *pwParent = gtk_widget_get_parent(pwAnalysis), *pw = NULL;
    listOLD *pl;
    GtkWidget *pwCubeAnalysis = NULL;
    doubletype dt;
    taketype tt;
    cubeinfo ci;
    GtkTextBuffer *buffer;
    pwMoveAnalysis = NULL;

    /* Select the moverecord _after_ pmr.  FIXME this is very ugly! */
    pmrCurAnn = pmr;

    for (pl = plGame->plNext; pl != plGame; pl = pl->plNext)
        if (pl->p == pmr) {
            pmr = pl->plNext->p;
            break;
        }

    if (pl == plGame)
        pmr = NULL;

    pmrAnnotation = pmr;

    /* FIXME optimise by ignoring set if pmr is unchanged */

    if (pwAnalysis) {
        gtk_widget_destroy(pwAnalysis);
        pwAnalysis = NULL;
    }

    gtk_widget_set_sensitive(pwCommentary, pmr != NULL);
    fAutoCommentaryChange = TRUE;
    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(pwCommentary));
    gtk_text_buffer_set_text(buffer, "", -1);
    fAutoCommentaryChange = FALSE;

    if (pmr) {
        GtkWidget *pwBox;
#if !GTK_CHECK_VERSION(3,0,0)
        GtkWidget *pwAlign;
#endif
        GList *pl;

        char sz[64], *pch;
        int fMoveOld, fTurnOld;

        if (pmr->sz) {
            fAutoCommentaryChange = TRUE;
            gtk_text_buffer_set_text(buffer, pmr->sz, -1);
            fAutoCommentaryChange = FALSE;
        }

        switch (pmr->mt) {
        case MOVE_NORMAL:
            fMoveOld = ms.fMove;
            fTurnOld = ms.fTurn;

#if GTK_CHECK_VERSION(3,0,0)
            pwAnalysis = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
            pwBox = gtk_grid_new();
            gtk_grid_set_column_homogeneous(GTK_GRID(pwBox), TRUE);
            gtk_grid_set_row_homogeneous(GTK_GRID(pwBox), TRUE);
#else
            pwAnalysis = gtk_vbox_new(FALSE, 0);
            pwBox = gtk_table_new(2, 3, TRUE);
#endif
            gtk_box_pack_start(GTK_BOX(pwAnalysis), pwBox, FALSE, FALSE, 4);

            ms.fMove = ms.fTurn = pmr->fPlayer;

            /*
             * Skill and luck
             */

            /* Skill for cube */

            GetMatchStateCubeInfo(&ci, &ms);
            if (GetDPEq(NULL, NULL, &ci)) {
#if GTK_CHECK_VERSION(3,0,0)
                gtk_grid_attach(GTK_GRID(pwBox),
                                gtk_label_new(pmr->stCube == SKILL_NONE ? "" : _("Didn't double")), 0, 0, 1, 1);
                gtk_grid_attach(GTK_GRID(pwBox), skill_label(pmr->stCube), 0, 1, 1, 1);
            } else {
                /* Neeeded for proper layout of grid but not for table */
                gtk_grid_attach(GTK_GRID(pwBox), gtk_label_new(""), 0, 0, 1, 1);
#else
                gtk_table_attach_defaults(GTK_TABLE(pwBox),
                                          gtk_label_new(pmr->stCube == SKILL_NONE ? "" : _("Didn't double")), 0, 1, 0,
                                          1);
                gtk_table_attach_defaults(GTK_TABLE(pwBox), skill_label(pmr->stCube), 0, 1, 1, 2);
#endif
            }

            /* luck */

            pch = sz + sprintf(sz, _("Rolled %u%u"), pmr->anDice[0], pmr->anDice[1]);

            if (pmr->rLuck != ERR_VAL) {
                if (fOutputMWC && ms.nMatchTo)
                    sprintf(pch, " (%+0.3f%%)", 100.0f * (eq2mwc(pmr->rLuck, &ci) - eq2mwc(0.0f, &ci)));
                else
                    sprintf(pch, " (%+0.3f)", pmr->rLuck);
            }
#if GTK_CHECK_VERSION(3,0,0)
            gtk_grid_attach(GTK_GRID(pwBox), gtk_label_new(sz), 1, 0, 1, 1);
            gtk_grid_attach(GTK_GRID(pwBox), luck_label(pmr->lt), 1, 1, 1, 1);
#else
            gtk_table_attach_defaults(GTK_TABLE(pwBox), gtk_label_new(sz), 1, 2, 0, 1);
            gtk_table_attach_defaults(GTK_TABLE(pwBox), luck_label(pmr->lt), 1, 2, 1, 2);
#endif
            /* chequer play skill */

            strcpy(sz, _("Moved "));
            FormatMove(sz + strlen(_("Moved ")), msBoard(), pmr->n.anMove);

#if GTK_CHECK_VERSION(3,0,0)
            gtk_grid_attach(GTK_GRID(pwBox), gtk_label_new(sz), 2, 0, 1, 1);

            gtk_grid_attach(GTK_GRID(pwBox), skill_label(pmr->n.stMove), 2, 1, 1, 1);
#else
            gtk_table_attach_defaults(GTK_TABLE(pwBox), gtk_label_new(sz), 2, 3, 0, 1);

            gtk_table_attach_defaults(GTK_TABLE(pwBox), skill_label(pmr->n.stMove), 2, 3, 1, 2);
#endif

            /* cube */

            pwCubeAnalysis = CreateCubeAnalysis(pmr, &ms, FALSE, -1, TRUE);


            /* move */

            if (pmr->ml.cMoves)
                pwMoveAnalysis = CreateMoveList(pmr, TRUE, FALSE, !IsPanelDocked(WINDOW_ANALYSIS), TRUE);

            if (pwMoveAnalysis && pwCubeAnalysis) {
                /* notebook with analysis */
                pw = gtk_notebook_new();

                gtk_box_pack_start(GTK_BOX(pwAnalysis), pw, TRUE, TRUE, 0);

                gtk_notebook_append_page(GTK_NOTEBOOK(pw), pwCubeAnalysis, gtk_label_new(_("Cube decision")));

                gtk_notebook_append_page(GTK_NOTEBOOK(pw), pwMoveAnalysis, gtk_label_new(_("Chequer play")));


            } else if (pwMoveAnalysis) {
                if (IsPanelDocked(WINDOW_ANALYSIS))
                    gtk_widget_set_size_request(GTK_WIDGET(pwMoveAnalysis), 0, 200);

                gtk_box_pack_start(GTK_BOX(pwAnalysis), pwMoveAnalysis, TRUE, TRUE, 0);
            } else if (pwCubeAnalysis)
                gtk_box_pack_start(GTK_BOX(pwAnalysis), pwCubeAnalysis, TRUE, TRUE, 0);

            if (!g_list_first(pl = gtk_container_get_children(GTK_CONTAINER(GTK_BOX(pwAnalysis))))) {
                gtk_widget_destroy(pwAnalysis);
                pwAnalysis = NULL;
                g_list_free(pl);
            }

            ms.fMove = fMoveOld;
            ms.fTurn = fTurnOld;
            break;

        case MOVE_DOUBLE:

            dt = DoubleType(ms.fDoubled, ms.fMove, ms.fTurn);

#if GTK_CHECK_VERSION(3,0,0)
            pwAnalysis = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
            pwBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
            pwAnalysis = gtk_vbox_new(FALSE, 0);
            pwBox = gtk_vbox_new(FALSE, 0);
#endif

            gtk_box_pack_start(GTK_BOX(pwBox), gtk_label_new(Q_(aszDoubleTypes[dt])), FALSE, FALSE, 2);
            gtk_box_pack_start(GTK_BOX(pwBox), skill_label(pmr->stCube), FALSE, FALSE, 2);
            gtk_box_pack_start(GTK_BOX(pwAnalysis), pwBox, FALSE, FALSE, 0);

            if (dt == DT_NORMAL || dt == DT_BEAVER) {

                if ((pw = CreateCubeAnalysis(pmr, &ms, TRUE, -1, TRUE)))
                    gtk_box_pack_start(GTK_BOX(pwAnalysis), pw, FALSE, FALSE, 0);

            } else
                gtk_box_pack_start(GTK_BOX(pwAnalysis),
                                   gtk_label_new(_("GNU Backgammon cannot "
                                                   "analyse neither beavers " "nor raccoons yet")), FALSE, FALSE, 0);
            break;

        case MOVE_TAKE:
        case MOVE_DROP:

            tt = (taketype) DoubleType(ms.fDoubled, ms.fMove, ms.fTurn);

#if GTK_CHECK_VERSION(3,0,0)
            pwAnalysis = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
            pwBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
            pwAnalysis = gtk_vbox_new(FALSE, 0);
            pwBox = gtk_vbox_new(FALSE, 0);
#endif

            gtk_box_pack_start(GTK_BOX(pwBox),
                               gtk_label_new(pmr->mt == MOVE_TAKE ? _("Take") : _("Drop")), FALSE, FALSE, 2);
            gtk_box_pack_start(GTK_BOX(pwBox), skill_label(pmr->stCube), FALSE, FALSE, 2);
            gtk_box_pack_start(GTK_BOX(pwAnalysis), pwBox, FALSE, FALSE, 0);

            if (tt <= TT_NORMAL) {
                if ((pw = CreateCubeAnalysis(pmr, &ms, -1, pmr->mt == MOVE_TAKE, TRUE)))
                    gtk_box_pack_start(GTK_BOX(pwAnalysis), pw, FALSE, FALSE, 0);
            } else
                gtk_box_pack_start(GTK_BOX(pwAnalysis),
                                   gtk_label_new(_("GNU Backgammon cannot "
                                                   "analyse neither beavers " "nor raccoons yet")), FALSE, FALSE, 0);

            break;

        case MOVE_RESIGN:
#if GTK_CHECK_VERSION(3,0,0)
            pwAnalysis = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
            pwAnalysis = gtk_vbox_new(FALSE, 0);
#endif

            /* equities for resign */

            if ((pw = ResignAnalysis(pmr->r.arResign, pmr->r.nResigned, &pmr->r.esResign)))
                gtk_box_pack_start(GTK_BOX(pwAnalysis), pw, FALSE, FALSE, 0);

            /* skill for resignation */

#if GTK_CHECK_VERSION(3,0,0)
            pwBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
            gtk_widget_set_halign(pwBox, GTK_ALIGN_CENTER);
            gtk_widget_set_valign(pwBox, GTK_ALIGN_CENTER);
            gtk_box_pack_start(GTK_BOX(pwBox), gtk_label_new(_("Resign")), FALSE, FALSE, 2);
            gtk_box_pack_start(GTK_BOX(pwAnalysis), pwBox, FALSE, FALSE, 0);
#else
            pwBox = gtk_hbox_new(FALSE, 0);
            pwAlign = gtk_alignment_new(0.5f, 0.5f, 0.0f, 0.0f);
            gtk_container_add(GTK_CONTAINER(pwAlign), pwBox);
            gtk_box_pack_start(GTK_BOX(pwBox), gtk_label_new(_("Resign")), FALSE, FALSE, 2);
            gtk_box_pack_start(GTK_BOX(pwAnalysis), pwAlign, FALSE, FALSE, 0);
#endif

            /* skill for accept */

#if GTK_CHECK_VERSION(3,0,0)
            pwBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
            gtk_widget_set_halign(pwBox, GTK_ALIGN_CENTER);
            gtk_widget_set_valign(pwBox, GTK_ALIGN_CENTER);
            gtk_box_pack_start(GTK_BOX(pwBox), gtk_label_new(_("Accept")), FALSE, FALSE, 2);
            gtk_box_pack_start(GTK_BOX(pwAnalysis), pwBox, FALSE, FALSE, 0);
#else
            pwBox = gtk_hbox_new(FALSE, 0);
            pwAlign = gtk_alignment_new(0.5f, 0.5f, 0.0f, 0.0f);
            gtk_container_add(GTK_CONTAINER(pwAlign), pwBox);
            gtk_box_pack_start(GTK_BOX(pwBox), gtk_label_new(_("Accept")), FALSE, FALSE, 2);
            gtk_box_pack_start(GTK_BOX(pwAnalysis), pwAlign, FALSE, FALSE, 0);
#endif

            break;

        default:
            break;
        }
    }

    if (!pwAnalysis)
        pwAnalysis = gtk_label_new(_("No analysis available."));

    if (!IsPanelDocked(WINDOW_ANALYSIS))
        gtk_paned_pack1(GTK_PANED(pwParent), pwAnalysis, TRUE, FALSE);
    else
        gtk_box_pack_start(GTK_BOX(pwParent), pwAnalysis, TRUE, TRUE, 0);

    gtk_widget_show_all(pwAnalysis);


    if (pmr && pmr->mt == MOVE_NORMAL && pwMoveAnalysis && pwCubeAnalysis) {

        if (badSkill(pmr->stCube))
            gtk_notebook_set_current_page(GTK_NOTEBOOK(pw), 0);
        else
            gtk_notebook_set_current_page(GTK_NOTEBOOK(pw), 1);

    }
}

extern void
GTKSaveSettings(void)
{

    char *sz = g_build_filename(szHomeDirectory, "gnubgmenurc", NULL);
    gtk_accel_map_save(sz);
    g_free(sz);
}

static gboolean
main_delete(GtkWidget * UNUSED(pw))
{

    getWindowGeometry(WINDOW_MAIN);

    PromptForExit();

    return TRUE;
}

/* The brain-damaged gtk_statusbar_pop interface doesn't return a value,
 * so we have to use a signal to see if anything was actually popped. */
static int fFinishedPopping;

static void
TextPopped(GtkWidget * UNUSED(pw), guint UNUSED(id), gchar * text, void *UNUSED(p))
{

    if (!text)
        fFinishedPopping = TRUE;
}

extern int
GetPanelSize(void)
{
    if (!fFullScreen && fX && gtk_widget_get_realized(pwMain)) {
        int pos = gtk_paned_get_position(GTK_PANED(hpaned));
        GtkAllocation allocation;
        gtk_widget_get_allocation(pwMain, &allocation);
        return allocation.width - pos;
    } else
        return panelSize;
}

extern void
SetPanelWidth(int size)
{
    panelSize = size;
    if (gtk_widget_get_realized(pwMain)) {
        GtkAllocation allocation;
        gtk_widget_get_allocation(pwMain, &allocation);
        if (panelSize > allocation.width * .8)
            panelSize = (int) (allocation.width * .8);
    }
}

extern void
SwapBoardToPanel(int ToPanel, int updateEvents)
{                               /* Show/Hide panel on right of screen */
    GtkAllocation allocation;
    gtk_widget_get_allocation(pwMain, &allocation);
    if (ToPanel) {
#if GTK_CHECK_VERSION(3,0,0)
        g_object_ref(pwEventBox);
        gtk_container_remove(GTK_CONTAINER(gtk_widget_get_parent(GTK_WIDGET(pwEventBox))), GTK_WIDGET(pwEventBox));
        gtk_container_add(GTK_CONTAINER(pwPanelGameBox), GTK_WIDGET(pwEventBox));
        g_object_unref(pwEventBox);
#else
        gtk_widget_reparent(pwEventBox, pwPanelGameBox);
#endif
        gtk_widget_show(hpaned);
        if (updateEvents)
            ProcessEvents();
        gtk_widget_hide(pwGameBox);
        gtk_paned_set_position(GTK_PANED(hpaned), allocation.width - panelSize);

#if ! GTK_CHECK_VERSION(3,0,0)
        {                       /* Hack to sort out widget positions - may be removed if works in later version of gtk */
            GtkAllocation temp = allocation;
            temp.height++;
            gtk_widget_size_allocate(pwMain, &temp);
            temp.height--;
            gtk_widget_size_allocate(pwMain, &temp);
        }
#endif
    } else {
        /* Need to hide these, as handle box seems to be buggy and gets confused */
        gtk_widget_hide(gtk_widget_get_parent(pwMenuBar));
        if (fToolbarShowing)
            gtk_widget_hide(gtk_widget_get_parent(pwToolbar));

#if GTK_CHECK_VERSION(3,0,0)
        g_object_ref(pwEventBox);
        gtk_container_remove(GTK_CONTAINER(gtk_widget_get_parent(GTK_WIDGET(pwEventBox))), GTK_WIDGET(pwEventBox));
        gtk_container_add(GTK_CONTAINER(pwGameBox), GTK_WIDGET(pwEventBox));
        g_object_unref(pwEventBox);
#else
        gtk_widget_reparent(pwEventBox, pwGameBox);
#endif

        gtk_widget_show(pwGameBox);
        if (updateEvents)
            ProcessEvents();
        if (gtk_widget_get_visible(hpaned)) {
            panelSize = GetPanelSize();
            gtk_widget_hide(hpaned);
        }
        gtk_widget_show(gtk_widget_get_parent(pwMenuBar));
        if (fToolbarShowing)
            gtk_widget_show(gtk_widget_get_parent(pwToolbar));
    }
}

#if ! GTK_CHECK_VERSION(3,0,0)
static void
MainSize(GtkWidget * pw, GtkRequisition * preq, gpointer p)
{

    /* Give the main window a size big enough that the board widget gets
     * board_size=4, if it will fit on the screen. */

    int width;

    if (gtk_widget_get_realized(pw))
        g_signal_handlers_disconnect_by_func(G_OBJECT(pw), (gpointer) G_CALLBACK(MainSize), p);

    else if (!SetMainWindowSize())
        gtk_window_set_default_size(GTK_WINDOW(pw),
                                    MAX(480, preq->width), MIN(preq->height + 79 * 3, gdk_screen_height() - 20));
    width = GetPanelWidth(WINDOW_MAIN);
    if (width)
        gtk_paned_set_position(GTK_PANED(hpaned), width - panelSize);
    else
        gtk_paned_set_position(GTK_PANED(hpaned), preq->width - panelSize);
}
#endif

#if defined(USE_GTKITEMFACTORY)
static gchar *
GTKTranslate(const gchar * path, gpointer UNUSED(func_data))
{
    return (gchar *) gettext((const char *) path);
}
#endif

#if !defined(USE_GTKITEMFACTORY)
static void
ToolbarStyle(guint UNUSED(iType), guint UNUSED(iActionID), GtkRadioAction * action, GtkRadioAction * UNUSED(alt),
             gpointer UNUSED(user_data))
{
    guint actionID = gtk_radio_action_get_current_value(GTK_RADIO_ACTION(action));
    /* If radio button has been selected set style */
    SetToolbarStyle(actionID - VIEW_TOOLBAR_ICONSONLY);
}
#else
static void
ToolbarStyle(gpointer UNUSED(callback_data), guint callback_action, GtkWidget * widget)
{
    if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget))) {  /* If radio button has been selected set style */
        SetToolbarStyle(callback_action - TOOLBAR_ACTION_OFFSET);
    }
}
#endif

static GtkClipboard *clipboard = NULL;

static void
PasteIDs(void)
{
    char *text = gtk_clipboard_wait_for_text(clipboard);
    int editing = ToolbarIsEditing(pwToolbar);
    char *sz;

    if (!text)
        return;

    if (editing)
        click_edit();

    sz = g_strdup_printf("set gnubgid %s", text);
    g_free(text);

    UserCommand(sz);
    g_free(sz);

    strcpy(ap[0].szName, default_names[0]);
    strcpy(ap[1].szName, default_names[1]);

#if USE_GTK
    if (fX)
        GTKSet(ap);
#endif                          /* USE_GTK */

    if (editing)
        click_edit();
}

extern void
GTKTextToClipboard(const char *text)
{
    gtk_clipboard_set_text(clipboard, text, -1);
}

static gboolean
configure_event(GtkWidget * UNUSED(widget), GdkEventConfigure * eCon, void *UNUSED(null))
{                               /* Maintain panel size */
    if (DockedPanelsShowing())
        gtk_paned_set_position(GTK_PANED(hpaned), eCon->width - GetPanelSize());

    return FALSE;
}

static void
NewClicked(gpointer UNUSED(p), guint UNUSED(n), GtkWidget * UNUSED(pw))
{
    GTKNew();
}

static void
CopyAsGOL(gpointer UNUSED(p), guint UNUSED(n), GtkWidget * UNUSED(pw))
{

    UserCommand("export position bgo2clipboard");

}

static void
CopyIDs(gpointer UNUSED(p), guint UNUSED(n), GtkWidget * UNUSED(pw))
{                               /* Copy the position and match ids to the clipboard */
    char buffer[L_MATCHID + 1 + L_POSITIONID + 1];

    if (ms.gs == GAME_NONE) {
        output(_("No game in progress."));
        outputx();
        return;
    }

    sprintf(buffer, "%s:%s", PositionID(msBoard()), MatchIDFromMatchState(&ms));

    GTKTextToClipboard(buffer);

    gtk_statusbar_push(GTK_STATUSBAR(pwStatus), idOutput, _("Position and Match IDs copied to the clipboard"));
}

static void
CopyMatchID(gpointer UNUSED(p), guint UNUSED(n), GtkWidget * UNUSED(pw))
{                               /* Copy the position and match ids to the clipboard */
    gchar *sz;

    if (ms.gs == GAME_NONE) {
        output(_("No game in progress."));
        outputx();
        return;
    }

    sz = g_strdup_printf("%s %s", _("Match ID:"), MatchIDFromMatchState(&ms));

    GTKTextToClipboard(sz);

    g_free(sz);

    gtk_statusbar_push(GTK_STATUSBAR(pwStatus), idOutput, _("Match ID copied to the clipboard"));
}

static void
CopyPositionID(gpointer UNUSED(p), guint UNUSED(n), GtkWidget * UNUSED(pw))
{                               /* Copy the position and match ids to the clipboard */
    gchar *sz;

    if (ms.gs == GAME_NONE) {
        output(_("No game in progress."));
        outputx();
        return;
    }

    sz = g_strdup_printf("%s %s", _("Position ID:"), PositionID(msBoard()));

    GTKTextToClipboard(sz);

    g_free(sz);

    gtk_statusbar_push(GTK_STATUSBAR(pwStatus), idOutput, _("Position ID copied to the clipboard"));
}

#if !defined(USE_GTKITEMFACTORY)
static void
TogglePanel(guint UNUSED(iType), guint iActionID, GtkToggleAction * action, GtkToggleAction * UNUSED(alt),
            gpointer UNUSED(user_data))
{
    int f;
    gnubgwindow panel = WINDOW_MAIN;

    f = gtk_toggle_action_get_active(action);
    switch (iActionID) {
    case TOGGLE_ANALYSIS:
        panel = WINDOW_ANALYSIS;
        break;
    case TOGGLE_COMMENTARY:
        panel = WINDOW_ANNOTATION;
        break;
    case TOGGLE_GAMELIST:
        panel = WINDOW_GAME;
        break;
    case TOGGLE_MESSAGE:
        panel = WINDOW_MESSAGE;
        break;
    case TOGGLE_THEORY:
        panel = WINDOW_THEORY;
        break;
    case TOGGLE_COMMAND:
        panel = WINDOW_COMMAND;
        break;
    default:
        g_assert_not_reached();
    }
    if (f)
        PanelShow(panel);
    else
        PanelHide(panel);

    /* Resize screen */
    SetMainWindowSize();
}

#else
static void
TogglePanel(gpointer UNUSED(p), guint n, GtkWidget * pw)
{
    int f;
    gnubgwindow panel = WINDOW_MAIN;

    g_assert(GTK_IS_CHECK_MENU_ITEM(pw));

    f = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(pw));
    switch (n) {
    case TOGGLE_ANALYSIS:
        panel = WINDOW_ANALYSIS;
        break;
    case TOGGLE_COMMENTARY:
        panel = WINDOW_ANNOTATION;
        break;
    case TOGGLE_GAMELIST:
        panel = WINDOW_GAME;
        break;
    case TOGGLE_MESSAGE:
        panel = WINDOW_MESSAGE;
        break;
    case TOGGLE_THEORY:
        panel = WINDOW_THEORY;
        break;
    case TOGGLE_COMMAND:
        panel = WINDOW_COMMAND;
        break;
    default:
        g_assert_not_reached();
    }
    if (f)
        PanelShow(panel);
    else
        PanelHide(panel);

    /* Resize screen */
    SetMainWindowSize();
}
#endif

extern void
GTKUndo(void)
{
    BoardData *bd = BOARD(pwBoard)->board_data;

    if (bd->drag_point >= 0) {  /* Drop piece */
        GdkEventButton dummyEvent;
        dummyEvent.x = dummyEvent.y = 0;
        board_button_release(pwBoard, &dummyEvent, bd);
    }

    ShowBoard();

    UpdateTheoryData(bd, TT_RETURNHITS, msBoard());
}

#if defined(USE_BOARD3D)
extern void
SetSwitchModeMenuText(void)
{                               /* Update menu text */
    BoardData *bd = BOARD(pwBoard)->board_data;
#if !defined(USE_GTKITEMFACTORY)
    GtkWidget *pMenuItem = gtk_ui_manager_get_widget(puim,
                                                     "/MainMenu/ViewMenu/SwitchMode");
#else
    GtkWidget *pMenuItem = gtk_item_factory_get_widget_by_action(pif, TOOLBAR_ACTION_OFFSET + MENU_OFFSET);
#endif
    char *text;
    if (display_is_2d(bd->rd))
        text = _("Switch to 3D view");
    else
        text = _("Switch to 2D view");
    gtk_label_set_text(GTK_LABEL(gtk_bin_get_child(GTK_BIN(pMenuItem))), text);
    gtk_widget_set_sensitive(pMenuItem, widget3dValid);
}

static void
SwitchDisplayMode(gpointer UNUSED(p), guint UNUSED(n), GtkWidget * UNUSED(pw))
{
    BoardData *bd = BOARD(pwBoard)->board_data;
    BoardData3d *bd3d = bd->bd3d;
    renderdata *prd = bd->rd;

    if (display_is_2d(prd)) {
        prd->fDisplayType = DT_3D;
        /* Reset 3d settings */
        MakeCurrent3d(bd3d);
        preDraw3d(bd, bd3d, prd);
        UpdateShadows(bd->bd3d);
        if (bd->diceShown == DICE_ON_BOARD)
            setDicePos(bd, bd3d);       /* Make sure dice appear ok */

        /* Needed for 2d dice+chequer widgets */
        board_free_pixmaps(bd);
        board_create_pixmaps(pwBoard, bd);
    } else {
        prd->fDisplayType = DT_2D;
        /* Make sure 2d pixmaps are correct */
        board_free_pixmaps(bd);
        board_create_pixmaps(pwBoard, bd);
        /* Make sure dice are visible if rolled */
        if (bd->diceShown == DICE_ON_BOARD && bd->x_dice[0] <= 0)
            RollDice2d(bd);
    }

    DisplayCorrectBoardType(bd, bd3d, prd);
    SetSwitchModeMenuText();
    /* Make sure chequers correct below board */
    gtk_widget_queue_draw(bd->table);
}

#endif

#if !defined(USE_GTKITEMFACTORY)
static void
ToggleShowingIDs(GtkToggleAction * action, gpointer UNUSED(user_data))
{
    int newValue = gtk_toggle_action_get_active(action);
    char *sz = g_strdup_printf("set gui showids %s", newValue ? "on" : "off");
    UserCommand(sz);
    g_free(sz);
    UserCommand("save settings");
}
#else
static void
ToggleShowingIDs(gpointer UNUSED(p), guint UNUSED(n), GtkWidget * pw)
{
    int newValue = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(pw));
    char *sz = g_strdup_printf("set gui showids %s", newValue ? "on" : "off");
    UserCommand(sz);
    g_free(sz);
    UserCommand("save settings");
}
#endif

int fToolbarShowing = TRUE;

extern void
ShowToolbar(void)
{
    GtkWidget *pwHandle = gtk_widget_get_parent(pwToolbar);
    gtk_widget_show(pwToolbar);
    gtk_widget_show(pwHandle);

#if !defined(USE_GTKITEMFACTORY)
    gtk_widget_show((gtk_ui_manager_get_widget(puim, "/MainMenu/ViewMenu/ToolBarMenu/HideToolBar")));
    gtk_widget_hide((gtk_ui_manager_get_widget(puim, "/MainMenu/ViewMenu/ToolBarMenu/ShowToolBar")));
    gtk_widget_set_sensitive((gtk_ui_manager_get_widget(puim, "/MainMenu/ViewMenu/ToolBarMenu/TextOnly")), TRUE);
    gtk_widget_set_sensitive((gtk_ui_manager_get_widget(puim, "/MainMenu/ViewMenu/ToolBarMenu/IconsOnly")), TRUE);
    gtk_widget_set_sensitive((gtk_ui_manager_get_widget(puim, "/MainMenu/ViewMenu/ToolBarMenu/Both")), TRUE);
#else
    gtk_widget_show(gtk_item_factory_get_widget(pif, "/View/Toolbar/Hide Toolbar"));
    gtk_widget_hide(gtk_item_factory_get_widget(pif, "/View/Toolbar/Show Toolbar"));
    gtk_widget_set_sensitive(gtk_item_factory_get_widget(pif, "/View/Toolbar/Text only"), TRUE);
    gtk_widget_set_sensitive(gtk_item_factory_get_widget(pif, "/View/Toolbar/Icons only"), TRUE);
    gtk_widget_set_sensitive(gtk_item_factory_get_widget(pif, "/View/Toolbar/Both"), TRUE);
#endif
    fToolbarShowing = TRUE;
    UserCommand("save settings");
}

extern void
HideToolbar(void)
{
    GtkWidget *pwHandle = gtk_widget_get_parent(pwToolbar);
    gtk_widget_hide(pwToolbar);
    gtk_widget_hide(pwHandle);

#if !defined(USE_GTKITEMFACTORY)
    gtk_widget_hide((gtk_ui_manager_get_widget(puim, "/MainMenu/ViewMenu/ToolBarMenu/HideToolBar")));
    gtk_widget_show((gtk_ui_manager_get_widget(puim, "/MainMenu/ViewMenu/ToolBarMenu/ShowToolBar")));
    gtk_widget_set_sensitive((gtk_ui_manager_get_widget(puim, "/MainMenu/ViewMenu/ToolBarMenu/TextOnly")), FALSE);
    gtk_widget_set_sensitive((gtk_ui_manager_get_widget(puim, "/MainMenu/ViewMenu/ToolBarMenu/IconsOnly")), FALSE);
    gtk_widget_set_sensitive((gtk_ui_manager_get_widget(puim, "/MainMenu/ViewMenu/ToolBarMenu/Both")), FALSE);
#else
    gtk_widget_hide(gtk_item_factory_get_widget(pif, "/View/Toolbar/Hide Toolbar"));
    gtk_widget_show(gtk_item_factory_get_widget(pif, "/View/Toolbar/Show Toolbar"));
    gtk_widget_set_sensitive(gtk_item_factory_get_widget(pif, "/View/Toolbar/Text only"), FALSE);
    gtk_widget_set_sensitive(gtk_item_factory_get_widget(pif, "/View/Toolbar/Icons only"), FALSE);
    gtk_widget_set_sensitive(gtk_item_factory_get_widget(pif, "/View/Toolbar/Both"), FALSE);
#endif
    fToolbarShowing = FALSE;
    UserCommand("save settings");
}

static gboolean
EndFullScreen(GtkWidget * UNUSED(widget), GdkEventKey * event, gpointer UNUSED(user_data))
{
    short k = (short) event->keyval;

    if (k == KEY_ESCAPE)
        FullScreenMode(FALSE);

    return FALSE;
}

static void
SetFullscreenWindowSettings(int panels, int ids, int maxed)
{
    showingPanels = panels;
    showingIDs = ids;
    maximised = maxed;
}

static void
DoFullScreenMode(gpointer UNUSED(p), guint UNUSED(n), GtkWidget * UNUSED(pw))
{
    BoardData *bd = BOARD(pwBoard)->board_data;
    GtkWindow *ptl = GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(bd->table)));
    GtkWidget *pwHandle = gtk_widget_get_parent(pwToolbar);
    static gulong id;
    static int changedRP, changedDP;

#if !defined(USE_GTKITEMFACTORY)
    GtkWidget *pmiRP = gtk_ui_manager_get_widget(puim, "/MainMenu/ViewMenu/RestorePanels");
    GtkWidget *pmiDP = gtk_ui_manager_get_widget(puim, "/MainMenu/ViewMenu/DockPanels");
#else
    GtkWidget *pmiRP = gtk_item_factory_get_widget(pif, "/View/Restore panels");
    GtkWidget *pmiDP = gtk_item_factory_get_widget(pif, "/View/Dock panels");
#endif

#if defined(USE_BOARD3D)
    if (display_is_3d(bd->rd)) {        /* Stop any 3d animations */
        StopIdle3d(bd, bd->bd3d);
    }
#endif

#if !defined(USE_GTKITEMFACTORY)
    fFullScreen = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(gtk_ui_manager_get_widget(puim,
                                                                                               "/MainMenu/ViewMenu/FullScreen")));
#else
    fFullScreen =
        gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(pif, "/View/Full screen")));
#endif
    if (fFullScreen) {
        if (!fullScreenOnStartup)
            GTKShowWarning(WARN_FULLSCREEN_EXIT, NULL);
        else
            fullScreenOnStartup = FALSE;

        bd->rd->fShowGameInfo = FALSE;

        if (pmiRP && gtk_widget_get_visible(pmiRP)
            && gtk_widget_is_sensitive(pmiRP))
            changedRP = TRUE;
        if (pmiDP && gtk_widget_get_visible(pmiDP)
            && gtk_widget_is_sensitive(pmiDP))
            changedDP = TRUE;
        /* Check if window is maximized */
        {
            GdkWindowState state = gdk_window_get_state(gtk_widget_get_window(GTK_WIDGET(ptl)));
            int maxed = ((state & GDK_WINDOW_STATE_MAXIMIZED) == GDK_WINDOW_STATE_MAXIMIZED);
            SetFullscreenWindowSettings(ArePanelsShowing(), fShowIDs, maxed);
        }

        id = g_signal_connect(G_OBJECT(ptl), "key-press-event", G_CALLBACK(EndFullScreen), 0);

        gtk_widget_hide(GTK_WIDGET(bd->table));
        gtk_widget_hide(GTK_WIDGET(bd->dice_area));
        gtk_widget_hide(pwStatus);
        gtk_widget_hide(pwProgress);
        gtk_widget_hide(pwStop);

        fFullScreen = FALSE;

        DoHideAllPanels(FALSE);

        fFullScreen = TRUE;
        fShowIDs = FALSE;

        gtk_widget_hide(pwToolbar);
        gtk_widget_hide(pwHandle);

        gtk_window_fullscreen(ptl);
        gtk_window_set_decorated(ptl, FALSE);
        if (pmiRP)
            gtk_widget_set_sensitive(pmiRP, FALSE);
        if (pmiDP)
            gtk_widget_set_sensitive(pmiDP, FALSE);
    } else {
        bd->rd->fShowGameInfo = TRUE;
        gtk_widget_show(pwMenuBar);
        if (fToolbarShowing) {
            gtk_widget_show(pwToolbar);
            gtk_widget_show(pwHandle);
        }
        gtk_widget_show(GTK_WIDGET(bd->table));
#if defined(USE_BOARD3D)
        /* Only show 2d dice below board if in 2d */
        if (display_is_2d(bd->rd))
#endif
            gtk_widget_show(GTK_WIDGET(bd->dice_area));
        gtk_widget_show(pwStatus);
        gtk_widget_show(pwProgress);
        gtk_widget_show(pwStop);

        GetFullscreenWindowSettings(&showingPanels, &fShowIDs, &maximised);

        if (g_signal_handler_is_connected(G_OBJECT(ptl), id))
            g_signal_handler_disconnect(G_OBJECT(ptl), id);

        gtk_window_unfullscreen(ptl);
        gtk_window_set_decorated(ptl, TRUE);

        if (showingPanels) {
            fFullScreen = TRUE; /* Avoid panel sizing code */
            ShowAllPanels(NULL, 0, NULL);
            fFullScreen = FALSE;
        }

        if (changedRP) {
            gtk_widget_set_sensitive(pmiRP, TRUE);
            changedRP = FALSE;
        }
        if (changedDP) {
            gtk_widget_set_sensitive(pmiDP, TRUE);
            changedDP = FALSE;
        }
    }
    UpdateSetting(&fShowIDs);
}

extern void
FullScreenMode(int state)
{
    BoardData *bd = BOARD(pwBoard)->board_data;
#if !defined(USE_GTKITEMFACTORY)
    GtkWidget *pw = gtk_ui_manager_get_widget(puim,
                                              "/MainMenu/ViewMenu/FullScreen");
#else
    GtkWidget *pw = gtk_item_factory_get_widget(pif, "/View/Full screen");
#endif
    if (gtk_widget_get_realized(bd->table))
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(pw), state);
}

void
GetFullscreenWindowSettings(int *panels, int *ids, int *maxed)
{
    *panels = showingPanels;
    *ids = showingIDs;
    *maxed = maximised;
}

static void
FinishMove(gpointer UNUSED(p), guint UNUSED(n), GtkWidget * UNUSED(pw))
{

    int anMove[8];
    char sz[FORMATEDMOVESIZE];

    if (!GTKGetMove(anMove))
        /* no valid move */
        return;

    UserCommand(FormatMove(sz, msBoard(), anMove));

}


typedef struct {
    evalcontext *pec;
    movefilter *pmf;
    GtkWidget *pwCubeful, *pwUsePrune, *pwDeterministic;
    GtkAdjustment *padjPlies, *padjNoise;
    int *pfOK;
    GtkWidget *pwOptionMenu;
    int fMoveFilter;
    GtkWidget *pwMoveFilter;
} evalwidget;

static void
EvalGetValues(evalcontext * pec, evalwidget * pew)
{

    pec->nPlies = (unsigned int) gtk_adjustment_get_value(pew->padjPlies);
    pec->fCubeful = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pew->pwCubeful));

    pec->fUsePrune = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pew->pwUsePrune));

    pec->rNoise = (float) gtk_adjustment_get_value(pew->padjNoise);
    pec->fDeterministic = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pew->pwDeterministic));
}


static void
EvalChanged(GtkWidget * UNUSED(pw), evalwidget * pew)
{
    evalcontext ecCurrent;
    int fFound = FALSE;

    EvalGetValues(&ecCurrent, pew);

    /* update predefined settings menu */

    for (int i = 0; i < NUM_SETTINGS; i++) {

        int fEval = !cmp_evalcontext(&aecSettings[i], &ecCurrent);
        int fMoveFilter = !aecSettings[i].nPlies ||
            (!pew->fMoveFilter || equal_movefilters((movefilter(*)[MAX_FILTER_PLIES]) pew->pmf,
                                                    aaamfMoveFilterSettings[aiSettingsMoveFilter[i]]));

        if (fEval && fMoveFilter) {

            /* current settings equal to a predefined setting */

            gtk_combo_box_set_active(GTK_COMBO_BOX(pew->pwOptionMenu), i);
            fFound = TRUE;
            break;

        }

    }


    /* user defined setting */

    if (!fFound)
        gtk_combo_box_set_active(GTK_COMBO_BOX(pew->pwOptionMenu), NUM_SETTINGS);


    if (pew->fMoveFilter)
        gtk_widget_set_sensitive(GTK_WIDGET(pew->pwMoveFilter), ecCurrent.nPlies);

}


static void
EvalNoiseValueChanged(GtkAdjustment * padj, evalwidget * pew)
{

    gtk_widget_set_sensitive(pew->pwDeterministic, gtk_adjustment_get_value(padj) != 0.0);

    EvalChanged(NULL, pew);

}

static void
EvalPliesValueChanged(GtkAdjustment * padj, evalwidget * pew)
{
    gtk_widget_set_sensitive(pew->pwUsePrune, gtk_adjustment_get_value(padj) > 0);
    EvalChanged(NULL, pew);

}

static void
SettingsMenuActivate(GtkComboBox * box, evalwidget * pew)
{

    evalcontext *pec;
    int iSelected;


    iSelected = gtk_combo_box_get_active(box);
    if (iSelected == NUM_SETTINGS)
        return;                 /* user defined */

    /* set all widgets to predefined values */

    pec = &aecSettings[iSelected];

    gtk_adjustment_set_value(pew->padjPlies, pec->nPlies);
    gtk_adjustment_set_value(pew->padjNoise, pec->rNoise);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pew->pwUsePrune), pec->fUsePrune);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pew->pwCubeful), pec->fCubeful);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pew->pwDeterministic), pec->fDeterministic);

    if (pew->fMoveFilter)
        MoveFilterSetPredefined(pew->pwMoveFilter, aiSettingsMoveFilter[iSelected]);

}

/*
 * Create widget for displaying evaluation settings
 *
 */

static GtkWidget *
EvalWidget(evalcontext * pec, movefilter * pmf, int *pfOK, const int fMoveFilter)
{

    evalwidget *pew;
    GtkWidget *pwEval, *pw;

    GtkWidget *pwFrame, *pwFrame2;
    GtkWidget *pw2, *pw3;


    GtkWidget *pwev;
    int i;

    if (pfOK)
        *pfOK = FALSE;

#if GTK_CHECK_VERSION(3,0,0)
    pwEval = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
    pwEval = gtk_vbox_new(FALSE, 0);
#endif
    gtk_container_set_border_width(GTK_CONTAINER(pwEval), 8);

    pew = g_malloc(sizeof *pew);

    /*
     * Frame with prefined settings
     */

    pwev = gtk_event_box_new();
    gtk_event_box_set_visible_window(GTK_EVENT_BOX(pwev), FALSE);
    gtk_container_add(GTK_CONTAINER(pwEval), pwev);

    pwFrame = gtk_frame_new(_("Predefined settings"));
    gtk_container_add(GTK_CONTAINER(pwev), pwFrame);

#if GTK_CHECK_VERSION(3,0,0)
    pw2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
#else
    pw2 = gtk_vbox_new(FALSE, 8);
#endif
    gtk_container_add(GTK_CONTAINER(pwFrame), pw2);
    gtk_container_set_border_width(GTK_CONTAINER(pw2), 8);

    /* option menu with selection of predefined settings */

    gtk_container_add(GTK_CONTAINER(pw2), gtk_label_new(_("Select a predefined setting:")));

    gtk_widget_set_tooltip_text(pwev,
                                _("Select a predefined setting, ranging from " "beginner's play to the 4ply setting."));

    pew->pwOptionMenu = gtk_combo_box_text_new();

    for (i = 0; i < NUM_SETTINGS; i++)
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(pew->pwOptionMenu), Q_(aszSettings[i]));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(pew->pwOptionMenu), _("user defined"));
    g_signal_connect(G_OBJECT(pew->pwOptionMenu), "changed", G_CALLBACK(SettingsMenuActivate), pew);
    gtk_container_add(GTK_CONTAINER(pw2), pew->pwOptionMenu);



    /*
     * Frame with user settings
     */

    pwFrame = gtk_frame_new(_("User defined settings"));
    gtk_container_add(GTK_CONTAINER(pwEval), pwFrame);

#if GTK_CHECK_VERSION(3,0,0)
    pw2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
#else
    pw2 = gtk_vbox_new(FALSE, 8);
#endif
    gtk_container_set_border_width(GTK_CONTAINER(pw2), 8);
    gtk_container_add(GTK_CONTAINER(pwFrame), pw2);

    /* lookahead */

    pwev = gtk_event_box_new();
    gtk_event_box_set_visible_window(GTK_EVENT_BOX(pwev), FALSE);
    gtk_container_add(GTK_CONTAINER(pw2), pwev);

    gtk_widget_set_tooltip_text(pwev,
                                _("Specify how many rolls GNU Backgammon should "
                                  "lookahead. Each ply costs approximately a factor "
                                  "of 21 in computational time. Also note that "
                                  "2-ply is equivalent to Snowie and XG's 3-ply setting."));

    pwFrame2 = gtk_frame_new(_("Lookahead"));
    gtk_container_add(GTK_CONTAINER(pwev), pwFrame2);

#if GTK_CHECK_VERSION(3,0,0)
    pw = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
#else
    pw = gtk_hbox_new(FALSE, 0);
#endif
    gtk_container_add(GTK_CONTAINER(pwFrame2), pw);

    pew->padjPlies = GTK_ADJUSTMENT(gtk_adjustment_new(pec->nPlies, 0, 7, 1, 1, 0));
    gtk_container_add(GTK_CONTAINER(pw), gtk_label_new(_("Plies:")));
    gtk_container_add(GTK_CONTAINER(pw), gtk_spin_button_new(pew->padjPlies, 1, 0));

    /* Use pruning neural nets */

    pwFrame2 = gtk_frame_new(_("Pruning neural nets"));
    gtk_container_add(GTK_CONTAINER(pw2), pwFrame2);

    gtk_container_add(GTK_CONTAINER(pwFrame2),
                      pew->pwUsePrune = gtk_check_button_new_with_label(_("Use neural net pruning")));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pew->pwUsePrune), pec->fUsePrune);

    gtk_widget_set_tooltip_text(pew->pwUsePrune,
                                _("Instruct GNU Backgammon to use a set of neural networks "
                                  "just to prune away move candidates within a deeper ply search. "
                                  "This increases the speed considerably at a negligible cost in playing strength. "
                                  "It is recommended to enable this option."));

    /* cubeful */

    pwFrame2 = gtk_frame_new(_("Cubeful evaluations"));
    gtk_container_add(GTK_CONTAINER(pw2), pwFrame2);

    gtk_container_add(GTK_CONTAINER(pwFrame2),
                      pew->pwCubeful = gtk_check_button_new_with_label(_("Cubeful chequer evaluation")));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pew->pwCubeful), pec->fCubeful);

    if (fMoveFilter)
        /* checker play */
        gtk_widget_set_tooltip_text(pew->pwCubeful,
                                    _("Instruct GNU Backgammon to use cubeful "
                                      "evaluations, i.e., include the value of "
                                      "cube ownership in the evaluations. It is "
                                      "recommended to enable this option."));
    else
        /* checker play */
        gtk_widget_set_tooltip_text(pew->pwCubeful,
                                    _("GNU Backgammon will always perform "
                                      "cubeful evaluations for cube decisions. "
                                      "Disabling this option will make GNU Backgammon "
                                      "use cubeless evaluations in the interval nodes "
                                      "of higher ply evaluations. It is recommended " "to enable this option."));

    /* noise */

    pwev = gtk_event_box_new();
    gtk_event_box_set_visible_window(GTK_EVENT_BOX(pwev), FALSE);
    gtk_container_add(GTK_CONTAINER(pw2), pwev);

    gtk_widget_set_tooltip_text(pwev,
                                _("You can use this option to introduce noise "
                                  "or errors in the evaluations. This is useful for "
                                  "introducing levels below 0-ply. The weaker levels "
                                  "(beginner to advanced) use this technique. "
                                  "The introduced noise can be "
                                  "deterministic, i.e., always the same noise for "
                                  "the same position, or it can be random."));

    pwFrame2 = gtk_frame_new(_("Noise"));
    gtk_container_add(GTK_CONTAINER(pwev), pwFrame2);

#if GTK_CHECK_VERSION(3,0,0)
    pw3 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
    pw3 = gtk_vbox_new(FALSE, 0);
#endif
    gtk_container_add(GTK_CONTAINER(pwFrame2), pw3);


    pew->padjNoise = GTK_ADJUSTMENT(gtk_adjustment_new(pec->rNoise, 0, 1, 0.001, 0.001, 0.0));
#if GTK_CHECK_VERSION(3,0,0)
    pw = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
#else
    pw = gtk_hbox_new(FALSE, 0);
#endif
    gtk_container_add(GTK_CONTAINER(pw3), pw);
    gtk_container_add(GTK_CONTAINER(pw), gtk_label_new(_("Noise:")));
    gtk_container_add(GTK_CONTAINER(pw), gtk_spin_button_new(pew->padjNoise, 0.001, 3));

    gtk_container_add(GTK_CONTAINER(pw3),
                      pew->pwDeterministic = gtk_check_button_new_with_label(_("Deterministic noise")));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pew->pwDeterministic), pec->fDeterministic);

    /* misc data */

    pew->fMoveFilter = fMoveFilter;
    pew->pec = pec;
    pew->pmf = pmf;
    pew->pfOK = pfOK;

    /* move filter */

    if (fMoveFilter) {

        pew->pwMoveFilter = MoveFilterWidget(pmf, pfOK, G_CALLBACK(EvalChanged), pew);

        pwev = gtk_event_box_new();
        gtk_event_box_set_visible_window(GTK_EVENT_BOX(pwev), FALSE);
        gtk_container_add(GTK_CONTAINER(pwEval), pwev);
        gtk_container_add(GTK_CONTAINER(pwev), pew->pwMoveFilter);

        gtk_widget_set_tooltip_text(pwev,
                                    _("GNU Backgammon will evaluate all moves at "
                                      "0-ply. The move filter controls how many "
                                      "moves will be evaluated at higher plies. "
                                      "A \"smaller\" filter will be faster, but "
                                      "GNU Backgammon may not find the best move. "
                                      "Power users may set up their own filters "
                                      "by clicking on the [Modify] button."));

    } else
        pew->pwMoveFilter = NULL;

    /* setup signals */

    g_signal_connect(G_OBJECT(pew->padjPlies), "value-changed", G_CALLBACK(EvalPliesValueChanged), pew);
    EvalPliesValueChanged(pew->padjPlies, pew);

    g_signal_connect(G_OBJECT(pew->padjNoise), "value-changed", G_CALLBACK(EvalNoiseValueChanged), pew);
    EvalNoiseValueChanged(pew->padjNoise, pew);

    g_signal_connect(G_OBJECT(pew->pwDeterministic), "toggled", G_CALLBACK(EvalChanged), pew);

    g_signal_connect(G_OBJECT(pew->pwCubeful), "toggled", G_CALLBACK(EvalChanged), pew);

    g_signal_connect(G_OBJECT(pew->pwUsePrune), "toggled", G_CALLBACK(EvalChanged), pew);

    g_object_set_data_full(G_OBJECT(pwEval), "user_data", pew, g_free);

    return pwEval;
}

static void
EvalOK(GtkWidget * pw, void *p)
{

    GtkWidget *pwEval = GTK_WIDGET(p);
    evalwidget *pew = g_object_get_data(G_OBJECT(pwEval), "user_data");

    if (pew->pfOK)
        *pew->pfOK = TRUE;

    EvalGetValues(pew->pec, pew);

    if (pew->pfOK)
        gtk_widget_destroy(gtk_widget_get_toplevel(pw));
}

static void
SetEvalCommands(const char *szPrefix, evalcontext * pec, evalcontext * pecOrig)
{

    char sz[256];

    outputpostpone();

    if (pec->nPlies != pecOrig->nPlies) {
        sprintf(sz, "%s plies %u", szPrefix, pec->nPlies);
        UserCommand(sz);
    }

    if (pec->fUsePrune != pecOrig->fUsePrune) {
        sprintf(sz, "%s prune %s", szPrefix, pec->fUsePrune ? "on" : "off");
        UserCommand(sz);
    }

    if (pec->fCubeful != pecOrig->fCubeful) {
        sprintf(sz, "%s cubeful %s", szPrefix, pec->fCubeful ? "on" : "off");
        UserCommand(sz);
    }

    if (pec->rNoise != pecOrig->rNoise) {
        gchar buf[G_ASCII_DTOSTR_BUF_SIZE];
        sprintf(sz, "%s noise %s", szPrefix, g_ascii_formatd(buf, G_ASCII_DTOSTR_BUF_SIZE, "%.3f", pec->rNoise));
        UserCommand(sz);
    }

    if (pec->fDeterministic != pecOrig->fDeterministic) {
        sprintf(sz, "%s deterministic %s", szPrefix, pec->fDeterministic ? "on" : "off");
        UserCommand(sz);
    }

    outputresume();
}


static void
DetailedAnalysisOK(GtkWidget * pw, AnalysisDetails * pDetails)
{
    EvalOK(pDetails->pwChequer, pDetails->pwChequer);
    EvalOK(pDetails->pwCube, pDetails->pwCube);
    gtk_widget_destroy(gtk_widget_get_toplevel(pw));
}

static int
EvalDefaultSetting(evalcontext * pec, movefilter * pmf)
{
    /* Look for predefined settings */
    for (int i = 0; i < NUM_SETTINGS; i++) {
        int fEval = !cmp_evalcontext(&aecSettings[i], pec);
        int fMoveFilter = !aecSettings[i].nPlies || (!pmf || equal_movefilters((movefilter(*)[MAX_FILTER_PLIES]) pmf,
                                                                               aaamfMoveFilterSettings
                                                                               [aiSettingsMoveFilter[i]]));

        if (fEval && fMoveFilter)
            return i;
    }

    return NUM_SETTINGS;
}

static void
UpdateSummaryEvalMenuSetting(AnalysisDetails * pAnalDetails)
{
    int chequerDefault = EvalDefaultSetting(pAnalDetails->esChequer, pAnalDetails->mfChequer);
    int cubeDefault = EvalDefaultSetting(pAnalDetails->esCube, pAnalDetails->mfCube);
    int setting = NUM_SETTINGS;

    if (chequerDefault == cubeDefault
        /* Special case as cube_supremo==cube_worldclass */
        || (chequerDefault == SETTINGS_SUPREMO && cubeDefault == SETTINGS_WORLDCLASS))
        setting = chequerDefault;

    setting -= (pAnalDetails->fWeakLevels ? 0 : SETTINGS_EXPERT);
    if (setting < 0)
        /* This combo box doesn't accept weak levels
         * but one of them was selected through user defined */
        setting = NUM_SETTINGS - (pAnalDetails->fWeakLevels ? 0 : SETTINGS_EXPERT);

    gtk_combo_box_set_active(GTK_COMBO_BOX(pAnalDetails->pwOptionMenu), setting);
}

static void
ShowDetailedAnalysis(GtkWidget * button, AnalysisDetails * pDetails)
{
    GtkWidget *pwvbox, *pwFrame, *pwDialog, *hbox;
    pwDialog = GTKCreateDialog(pDetails->title,
                               DT_INFO, button, DIALOG_FLAG_MODAL | DIALOG_FLAG_CLOSEBUTTON,
                               G_CALLBACK(DetailedAnalysisOK), pDetails);

#if GTK_CHECK_VERSION(3,0,0)
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
#else
    hbox = gtk_hbox_new(FALSE, 0);
#endif

    gtk_container_add(GTK_CONTAINER(DialogArea(pwDialog, DA_MAIN)), hbox);

    pwFrame = gtk_frame_new(_("Chequer play"));
    gtk_box_pack_start(GTK_BOX(hbox), pwFrame, TRUE, TRUE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(pwFrame), 4);

    gtk_container_add(GTK_CONTAINER(pwFrame),
                      pDetails->pwChequer = EvalWidget(pDetails->esChequer, pDetails->mfChequer,
                                                       NULL, pDetails->mfChequer != NULL));

    pwFrame = gtk_frame_new(_("Cube decisions"));
    gtk_box_pack_start(GTK_BOX(hbox), pwFrame, TRUE, TRUE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(pwFrame), 4);

#if GTK_CHECK_VERSION(3,0,0)
    pwvbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
    pwvbox = gtk_vbox_new(FALSE, 0);
#endif
    gtk_container_add(GTK_CONTAINER(pwFrame), pwvbox);

    gtk_box_pack_start(GTK_BOX(pwvbox),
                       pDetails->pwCube = EvalWidget(pDetails->esCube, (movefilter *) & pDetails->mfCube[9],
                                                     NULL, pDetails->mfCube != NULL), FALSE, FALSE, 0);

    if (pDetails->cubeDisabled)
        gtk_widget_set_sensitive(pDetails->pwCube, FALSE);

    GTKRunDialog(pwDialog);
    UpdateSummaryEvalMenuSetting(pDetails);
}

static void
SummaryMenuActivate(GtkComboBox * box, AnalysisDetails * pAnalDetails)
{
    int selected = gtk_combo_box_get_active(box) + (pAnalDetails->fWeakLevels ? 0 : SETTINGS_EXPERT);

    if (selected == NUM_SETTINGS)
        return;                 /* user defined */

    /* set eval settings to predefined values */
    *pAnalDetails->esChequer = aecSettings[selected];
    *pAnalDetails->esCube = aecSettings[selected];

    if (pAnalDetails->mfChequer && aiSettingsMoveFilter[selected] >= 0)
        memcpy(pAnalDetails->mfChequer, aaamfMoveFilterSettings[aiSettingsMoveFilter[selected]],
               sizeof(aaamfMoveFilterSettings[aiSettingsMoveFilter[selected]]));
    if (pAnalDetails->mfCube && aiSettingsMoveFilter[selected] >= 0)
        memcpy(pAnalDetails->mfCube, aaamfMoveFilterSettings[aiSettingsMoveFilter[selected]],
               sizeof(aaamfMoveFilterSettings[aiSettingsMoveFilter[selected]]));
}

static GtkWidget *
AddLevelSettings(GtkWidget * pwFrame, AnalysisDetails * pAnalDetails)
{
    GtkWidget *vbox, *hbox, *pw2, *pwAdvanced, *vboxSpacer;
    int i;

#if GTK_CHECK_VERSION(3,0,0)
    vboxSpacer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
    vboxSpacer = gtk_vbox_new(FALSE, 0);
#endif
    gtk_container_set_border_width(GTK_CONTAINER(vboxSpacer), 8);
    gtk_container_add(GTK_CONTAINER(pwFrame), vboxSpacer);

#if GTK_CHECK_VERSION(3,0,0)
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
    vbox = gtk_vbox_new(FALSE, 0);
#endif
    gtk_container_add(GTK_CONTAINER(vboxSpacer), vbox);

    /*
     * Frame with prefined settings
     */

#if GTK_CHECK_VERSION(3,0,0)
    pw2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
#else
    pw2 = gtk_vbox_new(FALSE, 4);
#endif
    gtk_box_pack_start(GTK_BOX(vbox), pw2, FALSE, FALSE, 0);

    /* option menu with selection of predefined settings */

    pAnalDetails->pwOptionMenu = gtk_combo_box_text_new();

    for (i = (pAnalDetails->fWeakLevels ? 0 : SETTINGS_EXPERT); i < NUM_SETTINGS; i++)
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(pAnalDetails->pwOptionMenu), Q_(aszSettings[i]));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(pAnalDetails->pwOptionMenu), _("user defined"));
    g_signal_connect(G_OBJECT(pAnalDetails->pwOptionMenu), "changed", G_CALLBACK(SummaryMenuActivate), pAnalDetails);
    gtk_container_add(GTK_CONTAINER(pw2), pAnalDetails->pwOptionMenu);

    pwAdvanced = gtk_button_new_with_label(_("Advanced Settings..."));
    g_signal_connect(G_OBJECT(pwAdvanced), "clicked", G_CALLBACK(ShowDetailedAnalysis), (void *) pAnalDetails);
#if GTK_CHECK_VERSION(3,0,0)
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
#else
    hbox = gtk_hbox_new(FALSE, 0);
#endif
    gtk_box_pack_start(GTK_BOX(hbox), pwAdvanced, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);
    UpdateSummaryEvalMenuSetting(pAnalDetails);
    return vboxSpacer;          /* Container */
}

#define CHECKUPDATE(button,flag,string) \
   n = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( (button) ) ); \
   if ( n != (flag)){ \
           sprintf(sz, (string), n ? "on" : "off"); \
           UserCommand(sz); \
   }

#define ADJUSTSKILLUPDATE(button,flag,string) \
  if( gtk_adjustment_get_value ( paw->apadjSkill[(button)] ) != (gdouble)arSkillLevel[(flag)] ) \
  { \
     gchar buf[G_ASCII_DTOSTR_BUF_SIZE]; \
     sprintf(sz,  (string), g_ascii_formatd( buf, G_ASCII_DTOSTR_BUF_SIZE, \
			           "%0.3f", (gdouble) gtk_adjustment_get_value( paw->apadjSkill[(button)] ) ) ); \
     UserCommand(sz); \
  }

#define ADJUSTLUCKUPDATE(button,flag,string) \
  if( gtk_adjustment_get_value( paw->apadjLuck[(button)] ) != (gdouble)arLuckLevel[(flag)] ) \
  { \
     gchar buf[G_ASCII_DTOSTR_BUF_SIZE]; \
     sprintf(sz,  (string), g_ascii_formatd( buf, G_ASCII_DTOSTR_BUF_SIZE, \
			           "%0.3f", (gdouble) gtk_adjustment_get_value( paw->apadjLuck[(button)] ) ) ); \
     UserCommand(sz); \
  }

static void
AnalysisOK(GtkWidget * pw, analysiswidget * paw)
{

    char sz[128];
    int n, i;

    gtk_widget_hide(gtk_widget_get_toplevel(pw));

    CHECKUPDATE(paw->pwMoves, fAnalyseMove, "set analysis moves %s")
    CHECKUPDATE(paw->pwCube, fAnalyseCube, "set analysis cube %s")
    CHECKUPDATE(paw->pwLuck, fAnalyseDice, "set analysis luck %s")
    CHECKUPDATE(paw->apwAnalysePlayers[0], afAnalysePlayers[0], "set analysis player 0 analyse %s")
    CHECKUPDATE(paw->apwAnalysePlayers[1], afAnalysePlayers[1], "set analysis player 1 analyse %s")
    CHECKUPDATE(paw->pwAutoDB, fAutoDB, "set automatic db %s")
    CHECKUPDATE(paw->pwBackgroundAnalysis, fBackgroundAnalysis, "set analysis background %s")

    ADJUSTSKILLUPDATE(0, SKILL_DOUBTFUL, "set analysis threshold doubtful %s")
    ADJUSTSKILLUPDATE(1, SKILL_BAD, "set analysis threshold bad %s")
    ADJUSTSKILLUPDATE(2, SKILL_VERYBAD, "set analysis threshold verybad %s")

    ADJUSTLUCKUPDATE(0, LUCK_VERYGOOD, "set analysis threshold verylucky %s")
    ADJUSTLUCKUPDATE(1, LUCK_GOOD, "set analysis threshold lucky %s")
    ADJUSTLUCKUPDATE(2, LUCK_BAD, "set analysis threshold unlucky %s")
    ADJUSTLUCKUPDATE(3, LUCK_VERYBAD, "set analysis threshold veryunlucky %s")

    for (i = 0; i < NUM_AnalyzeFileSettings; ++i)
        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(paw->apwAnalyzeFileSetting[i])) && AnalyzeFileSettingDef != (analyzeFileSetting) i) {
                // g_message("new def: %d->%d\n", AnalyzeFileSettingDef,i);
            sprintf(sz, "set analysis filesetting %s", aszAnalyzeFileSettingCommands[i]);
            UserCommand(sz);
            break;
        } 

    /* Score Map */

    for (i = 0; i < NUM_PLY; ++i)
        {if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(paw->apwScoreMapPly[i])) && scoreMapPlyDefault != (scoreMapPly) i) {
            sprintf(sz, "set scoremapply %s", aszScoreMapPlyCommands[i]);
            UserCommand(sz);
            break;
        } }


    for (i = 0; i < NUM_MATCH_LENGTH; ++i){
        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(paw->apwScoreMapMatchLength[i])) && scoreMapMatchLengthDefIdx != (scoreMapMatchLength) i) {
            sprintf(sz, "set scoremapmatchlength %s", aszScoreMapMatchLengthCommands[i]);
            UserCommand(sz);
            break;
        } 
    }


    for (i = 0; i < NUM_LABEL; ++i)
        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(paw->apwScoreMapLabel[i])) && scoreMapLabelDef != (scoreMapLabel) i) {
            sprintf(sz, "set scoremaplabel %s", aszScoreMapLabelCommands[i]);
            UserCommand(sz);
            break;
        } 
    for (i = 0; i < NUM_JACOBY; ++i)
       { if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(paw->apwScoreMapJacoby[i])) && scoreMapJacobyDef != (scoreMapJacoby) i) {
            sprintf(sz, "set scoremapjacoby %s", aszScoreMapJacobyCommands[i]);
            UserCommand(sz);
            break;
        }} 
    for (i = 0; i < NUM_CUBEDISP; ++i)
        {if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(paw->apwScoreMapCubeEquityDisplay[i])) && scoreMapCubeEquityDisplayDef != (scoreMapCubeEquityDisplay) i) {
            sprintf(sz, "set scoremapcubeequitydisplay %s", aszScoreMapCubeEquityDisplayCommands[i]);
            UserCommand(sz);
            break;
        } }
    for (i = 0; i < NUM_MOVEDISP; ++i)
        {if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(paw->apwScoreMapMoveEquityDisplay[i])) && scoreMapMoveEquityDisplayDef != (scoreMapMoveEquityDisplay) i) {
            sprintf(sz, "set scoremapmoveequitydisplay %s", aszScoreMapMoveEquityDisplayCommands[i]);
            UserCommand(sz);
            break;
        }} 
    for (i = 0; i < NUM_COLOUR; ++i)
        {if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(paw->apwScoreMapColour[i])) && scoreMapColourDef != (scoreMapColour) i) {
            sprintf(sz, "set scoremapcolour %s", aszScoreMapColourCommands[i]);
            UserCommand(sz);
            break;
        }} 
    for (i = 0; i < NUM_LAYOUT; ++i)
       { if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(paw->apwScoreMapLayout[i])) && scoreMapLayoutDef != (scoreMapLayout) i) {
            sprintf(sz, "set scoremaplayout %s", aszScoreMapLayoutCommands[i]);
            UserCommand(sz);
            break;
        }} 


    /* Group output in one batch */
    outputpostpone();

    SetEvalCommands("set analysis chequerplay eval", &paw->esChequer.ec, &esAnalysisChequer.ec);
    SetMovefilterCommands("set analysis movefilter", paw->aamf, aamfAnalysis);
    SetEvalCommands("set analysis cubedecision eval", &paw->esCube.ec, &esAnalysisCube.ec);

    n = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(paw->pwHintSame));
    if (n != fEvalSameAsAnalysis) {
        sprintf(sz, "set eval sameasanalysis %s", n ? "yes" : "no");
        UserCommand(sz);
    }

    if (!fEvalSameAsAnalysis) {
        SetEvalCommands("set evaluation chequer eval", &paw->esEvalChequer.ec, &GetEvalChequer()->ec);
        SetMovefilterCommands("set evaluation movefilter", paw->aaEvalmf, *GetEvalMoveFilter());
        SetEvalCommands("set evaluation cubedecision eval", &paw->esEvalCube.ec, &GetEvalCube()->ec);
    }
    UserCommand("save settings");
    outputresume();

    gtk_widget_destroy(gtk_widget_get_toplevel(pw));

}

#undef CHECKUPDATE
#undef ADJUSTSKILLUPDATE
#undef ADJUSTLUCKUPDATE

static void
AnalysisSet(analysiswidget * paw)
{

    int i;
    //char sz[128];


    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(paw->pwMoves), fAnalyseMove);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(paw->pwCube), fAnalyseCube);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(paw->pwLuck), fAnalyseDice);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(paw->pwAutoDB), fAutoDB);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(paw->pwBackgroundAnalysis), fBackgroundAnalysis);

    for (i = 0; i < 2; ++i)
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(paw->apwAnalysePlayers[i]), afAnalysePlayers[i]);

    gtk_adjustment_set_value(GTK_ADJUSTMENT(paw->apadjSkill[0]), arSkillLevel[SKILL_DOUBTFUL]);
    gtk_adjustment_set_value(GTK_ADJUSTMENT(paw->apadjSkill[1]), arSkillLevel[SKILL_BAD]);
    gtk_adjustment_set_value(GTK_ADJUSTMENT(paw->apadjSkill[2]), arSkillLevel[SKILL_VERYBAD]);
    gtk_adjustment_set_value(GTK_ADJUSTMENT(paw->apadjLuck[0]), arLuckLevel[LUCK_VERYGOOD]);
    gtk_adjustment_set_value(GTK_ADJUSTMENT(paw->apadjLuck[1]), arLuckLevel[LUCK_GOOD]);
    gtk_adjustment_set_value(GTK_ADJUSTMENT(paw->apadjLuck[2]), arLuckLevel[LUCK_BAD]);
    gtk_adjustment_set_value(GTK_ADJUSTMENT(paw->apadjLuck[3]), arLuckLevel[LUCK_VERYBAD]);

    // g_message("initial def: %d\n", AnalyzeFileSettingDef);
    for (i = 0; i < NUM_AnalyzeFileSettings; ++i)
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(paw->apwAnalyzeFileSetting[i]), AnalyzeFileSettingDef == (analyzeFileSetting) i); 

    /*Score Map*/ 

    for (i = 0; i < NUM_PLY; ++i){
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(paw->apwScoreMapPly[i]), scoreMapPlyDefault == (scoreMapPly)i);
    }

    for (i = 0; i < NUM_MATCH_LENGTH; ++i)
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(paw->apwScoreMapMatchLength[i]), scoreMapMatchLengthDefIdx == (scoreMapMatchLength)i); 

    for (i = 0; i < NUM_LABEL; ++i)
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(paw->apwScoreMapLabel[i]), scoreMapLabelDef == (scoreMapLabel) i); 

    for (i = 0; i < NUM_JACOBY; ++i)
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(paw->apwScoreMapJacoby[i]), scoreMapJacobyDef == (scoreMapJacoby) i); 

    for (i = 0; i < NUM_CUBEDISP; ++i)
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(paw->apwScoreMapCubeEquityDisplay[i]), scoreMapCubeEquityDisplayDef == (scoreMapCubeEquityDisplay) i); 

    for (i = 0; i < NUM_MOVEDISP; ++i)
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(paw->apwScoreMapMoveEquityDisplay[i]), scoreMapMoveEquityDisplayDef == (scoreMapMoveEquityDisplay) i); 

    for (i = 0; i < NUM_COLOUR; ++i)
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(paw->apwScoreMapColour[i]), scoreMapColourDef == (scoreMapColour) i); 

    for (i = 0; i < NUM_LAYOUT; ++i)
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(paw->apwScoreMapLayout[i]), scoreMapLayoutDef == (scoreMapLayout) i); 

}

static void
HintSameToggled(GtkWidget * UNUSED(notused), analysiswidget * paw)
{

    int active = !(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(paw->pwHintSame)));

    gtk_widget_set_sensitive(paw->pwCubeSummary, active);

}

static AnalysisDetails *
CreateEvalSettings(GtkWidget * pwParent, const char *title, evalcontext * pechequer, movefilter * pmfchequer,
                   evalcontext * pecube, movefilter * pmfcube, int fWeakLevels)
{
    AnalysisDetails *pAnalDetail = g_malloc(sizeof(AnalysisDetails));
    pAnalDetail->title = title;
    pAnalDetail->esChequer = pechequer;
    pAnalDetail->mfChequer = pmfchequer;
    pAnalDetail->esCube = pecube;
    pAnalDetail->mfCube = pmfcube;
    pAnalDetail->cubeDisabled = FALSE;
    pAnalDetail->fWeakLevels = fWeakLevels;

    pAnalDetail->pwSettingWidgets = AddLevelSettings(pwParent, pAnalDetail);
    return pAnalDetail;
}

//Module to add text
extern void
AddText(GtkWidget* pwBox, char* Text)
{
    GtkRcStyle * ps = gtk_rc_style_new();
    GtkWidget * pwText = gtk_label_new(Text);
    GtkWidget * pwHBox;

#if GTK_CHECK_VERSION(3,0,0)
    pwHBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
#else
    pwHBox = gtk_hbox_new(FALSE, 0);
#endif
    gtk_box_pack_start(GTK_BOX(pwBox), pwHBox, FALSE, FALSE, 4);

    ps->font_desc = pango_font_description_new();
    //pango_font_description_set_family_static(ps->font_desc, "serif");
    //pango_font_description_set_size(ps->font_desc, 8 * PANGO_SCALE);
    gtk_widget_modify_style(pwText, ps);
    g_object_unref(ps);

    gtk_box_pack_start(GTK_BOX(pwHBox), pwText, FALSE, FALSE, 0);
}


static void
BuildRadioButtons(GtkWidget* pwvbox, GtkWidget* apwScoreMapFrame[], const char* frameTitle, const char* frameToolTip, const char* labelStrings[],
    int labelStringsLen, int toggleDefault) { 
    /* Sub-function to build a new box with a new set of labels, with a whole bunch of needed parameters

    - pwvbox ----------
    | --title----------|
    | -- pwh2 -------- |
    | |text | options| |
    | ---------------- |
    --------------------
    */
    int* pi;
    int i;
    // GtkWidget* pwFrame;
    GtkWidget* pwh2;

    AddText(pwvbox, _(frameTitle));

//     pwFrame = gtk_frame_new(_(frameTitle));
//     gtk_box_pack_start(GTK_BOX(pwScoreMapBox), pwFrame, vAlignExpand, FALSE, 0);
//     gtk_widget_set_tooltip_text(pwFrame, _(frameToolTip)); 
//     gtk_widget_set_sensitive(pwFrame, sensitive);

#if GTK_CHECK_VERSION(3,0,0)
    pwh2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
#else
    pwh2 = gtk_hbox_new(FALSE, 8);
#endif
//     gtk_container_add(GTK_CONTAINER(pwFrame), pwh2);

    gtk_box_pack_start(GTK_BOX(pwvbox), pwh2, FALSE, FALSE, 0);

    AddText(pwh2, ("   "));

    for (i = 0; i < labelStringsLen; i++) {
        if (i == 0)
            apwScoreMapFrame[0] = gtk_radio_button_new_with_label(NULL, _(labelStrings[0])); // First radio button
        else
            apwScoreMapFrame[i] = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(apwScoreMapFrame[0]), _(labelStrings[i])); // Associate this to the other radio buttons
        gtk_box_pack_start(GTK_BOX(pwh2), apwScoreMapFrame[i], FALSE, FALSE, 0);
        gtk_widget_set_tooltip_text(apwScoreMapFrame[i], _(frameToolTip));    
        pi = (int*)g_malloc(sizeof(int));
        *pi = (int)i; // here use "=(int)labelEnum[i];" and put it in the input of the function if needed, while
             //  defining sth like " int labelEnum[] = { NUMBERS, ENGLISH, BOTH };" before calling the function
        g_object_set_data_full(G_OBJECT(apwScoreMapFrame[i]), "user_data", pi, g_free);
        if (toggleDefault == i) // again use "if (DefaultLabel==labelEnum[i])" if needed
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(apwScoreMapFrame[i]), 1); //we set this to toggle it on in case it's the default option
        //g_signal_connect(G_OBJECT(pw), "toggled", G_CALLBACK((*functionWhenToggled)), psm);
    }}



static void
append_scoremap_options(analysiswidget* paw) 
{
    GtkWidget* pwvbox;
    GtkWidget* pwFrame;
    GtkWidget* pwv;

#if !GTK_CHECK_VERSION(3,0,0)
    GtkWidget* pwp;
#endif

    //BoardData* bd = BOARD(pwBoard)->board_data;

    int vAlignExpand = FALSE; // set to true to expand vertically the group of frames rather than packing them to the top
    //int evalPlies = 3;

    /* Display options */

#if GTK_CHECK_VERSION(3,0,0)
    pwvbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_halign(pwvbox, GTK_ALIGN_START);
    gtk_widget_set_valign(pwvbox, GTK_ALIGN_START);
    gtk_container_set_border_width(GTK_CONTAINER(pwvbox), 4);
    gtk_notebook_append_page(GTK_NOTEBOOK(paw->pwNoteBook), pwvbox, gtk_label_new(_("ScoreMap")));
#else
    pwvbox = gtk_vbox_new(FALSE, 0);
    pwp = gtk_alignment_new(0, 0, 0, 0);
    gtk_container_set_border_width(GTK_CONTAINER(pwp), 4);
    gtk_notebook_append_page(GTK_NOTEBOOK(paw->pwNoteBook), pwp, gtk_label_new(_("ScoreMap")));
    gtk_container_add(GTK_CONTAINER(pwp), pwvbox);
#endif

    // #if GTK_CHECK_VERSION(3,0,0)
//     pwScoreMapBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
// #else
//     pwScoreMapBox = gtk_hbox_new(FALSE, 0);
// #endif
//     gtk_box_pack_start(GTK_BOX(pwvbox), pwScoreMapBox, FALSE, FALSE, 0);
    // AddText(pwvbox, _(frameTitle));

    pwFrame = gtk_frame_new(_("Default ScoreMap settings"));
    gtk_box_pack_start(GTK_BOX(pwvbox), pwFrame, vAlignExpand, FALSE, 0);
    gtk_widget_set_tooltip_text(pwFrame, _("Select the settings with which to initialize each new ScoreMap window")); 
    gtk_widget_set_sensitive(pwFrame, TRUE);

#if GTK_CHECK_VERSION(3,0,0)
        pwv = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
#else
        pwv = gtk_vbox_new(FALSE, 0);
#endif
    gtk_container_add(GTK_CONTAINER(pwFrame), pwv);

    BuildRadioButtons(pwv, paw->apwScoreMapPly,_("Evaluation strength:"), _("Select the ply at which to evaluate the equity at each score"), aszScoreMapPly, NUM_PLY, scoreMapPlyDefault);
    BuildRadioButtons(pwv, paw->apwScoreMapMatchLength,_("Simulated match length:"), _("Select the default match length for which to draw the ScoreMap; a variable length picks a length of 3 for current real short matches, 7 for long, and 5 otherwise."), aszScoreMapMatchLength, NUM_MATCH_LENGTH, scoreMapMatchLengthDefIdx);
    BuildRadioButtons(pwv, paw->apwScoreMapJacoby,_("Money-play analysis:"), _("Select the default Jacoby option in the money play analysis of the top-left ScoreMap square"), aszScoreMapJacoby, NUM_JACOBY, scoreMapJacobyDef);
    BuildRadioButtons(pwv, paw->apwScoreMapCubeEquityDisplay,_("Cube equity display:"), _("Select the default equity text to display in the squares of the cube ScoreMap"), aszScoreMapCubeEquityDisplay, NUM_CUBEDISP, scoreMapCubeEquityDisplayDef);
    BuildRadioButtons(pwv, paw->apwScoreMapMoveEquityDisplay,_("Move equity display:"), _("Select the default equity text to display in the squares of the move ScoreMap"), aszScoreMapMoveEquityDisplay, NUM_MOVEDISP, scoreMapMoveEquityDisplayDef);
    BuildRadioButtons(pwv, paw->apwScoreMapColour,_("In cube ScoreMaps, colour by:"), _("Select what equity to use when deciding to colour the cube ScoreMap"), aszScoreMapColour, NUM_COLOUR, scoreMapColourDef);
    BuildRadioButtons(pwv, paw->apwScoreMapLabel,_("Axis orientation:"), _("Select how to orient the ScoreMap axes by default"), aszScoreMapLabel, NUM_LABEL, scoreMapLabelDef);
    BuildRadioButtons(pwv, paw->apwScoreMapLayout,_("Option pane location:"), _("Decide where to place the options with respect to the ScoreMap table"), aszScoreMapLayout, NUM_LAYOUT, scoreMapLayoutDef);

}

static void
append_analysis_options(analysiswidget * paw)
{
    const char *aszSkillLabel[3] = { N_("Doubtful:"), N_("Bad:"), N_("Very bad:") };
    const char *aszLuckLabel[4] = { N_("Very lucky:"), N_("Lucky:"),
        N_("Unlucky:"), N_("Very unlucky:")
    };
    int i;
    // AnalysisDetails *pAnalDetailSettings1,*pAnalDetailSettings2;
    GtkWidget *pwPage, *pwFrame, *pwLabel, *pwSpin;
#if GTK_CHECK_VERSION(3,0,0)
    GtkWidget *pwGrid;
#else
    GtkWidget *pwTable;
    GtkWidget* pwp;
#endif
    GtkWidget *hboxTop, *hboxMid, *hboxBottom, *vbox1, *vbox2, *vbox3, *hbox, *pwvbox;

    memcpy(&paw->esCube, &esAnalysisCube, sizeof(paw->esCube));
    memcpy(&paw->esChequer, &esAnalysisChequer, sizeof(paw->esChequer));
    memcpy(&paw->aamf, aamfAnalysis, sizeof(paw->aamf));

    memcpy(&paw->esEvalChequer, &esEvalChequer, sizeof(paw->esEvalChequer));
    memcpy(&paw->esEvalCube, &esEvalCube, sizeof(paw->esEvalCube));
    memcpy(&paw->aaEvalmf, aamfEval, sizeof(paw->aaEvalmf));

/*  //Older version:  
    memcpy(&aw.esCube, &esAnalysisCube, sizeof(aw.esCube));
    memcpy(&aw.esChequer, &esAnalysisChequer, sizeof(aw.esChequer));
    memcpy(&aw.aamf, aamfAnalysis, sizeof(aw.aamf));

    memcpy(&aw.esEvalChequer, &esEvalChequer, sizeof esEvalChequer);
    memcpy(&aw.esEvalCube, &esEvalCube, sizeof esEvalCube);
    memcpy(aw.aaEvalmf, aamfEval, sizeof(aamfEval));
*/

#if GTK_CHECK_VERSION(3,0,0)
    pwvbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_halign(pwvbox, GTK_ALIGN_START);
    gtk_widget_set_valign(pwvbox, GTK_ALIGN_START);
    gtk_container_set_border_width(GTK_CONTAINER(pwvbox), 4);
    gtk_notebook_append_page(GTK_NOTEBOOK(paw->pwNoteBook), pwvbox, gtk_label_new(_("Analysis")));
#else
    pwvbox = gtk_vbox_new(FALSE, 0);
    pwp = gtk_alignment_new(0, 0, 0, 0);
    gtk_container_set_border_width(GTK_CONTAINER(pwp), 4);
    gtk_notebook_append_page(GTK_NOTEBOOK(paw->pwNoteBook), pwp, gtk_label_new(_("Analysis")));
    gtk_container_add(GTK_CONTAINER(pwp), pwvbox);
#endif

#if GTK_CHECK_VERSION(3,0,0)
    pwPage = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
#else
    pwPage = gtk_hbox_new(FALSE, 6);
#endif
    gtk_container_set_border_width(GTK_CONTAINER(pwPage), 8);
    gtk_box_pack_start(GTK_BOX(pwvbox), pwPage, FALSE, FALSE, 0);

#if GTK_CHECK_VERSION(3,0,0)
    vbox1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
    vbox1 = gtk_vbox_new(FALSE, 0);
#endif
    gtk_box_pack_start(GTK_BOX(pwPage), vbox1, TRUE, TRUE, 0);

#if GTK_CHECK_VERSION(3,0,0)
    hboxTop = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
#else
    hboxTop = gtk_hbox_new(FALSE, 0);
#endif
    gtk_box_pack_start(GTK_BOX(vbox1), hboxTop, TRUE, TRUE, 0);
#if GTK_CHECK_VERSION(3,0,0)
    hboxMid = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    hboxBottom = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
#else
    hboxMid = gtk_hbox_new(FALSE, 0);
    hboxBottom = gtk_hbox_new(FALSE, 0);
#endif
    gtk_box_pack_start(GTK_BOX(vbox1), hboxMid, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(pwvbox), hboxBottom, TRUE, TRUE, 0);

    pwFrame = gtk_frame_new(_("Analysis"));
    gtk_box_pack_start(GTK_BOX(hboxTop), pwFrame, TRUE, TRUE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(pwFrame), 4);

#if GTK_CHECK_VERSION(3,0,0)
    vbox2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
    vbox2 = gtk_vbox_new(FALSE, 0);
#endif
    gtk_container_add(GTK_CONTAINER(pwFrame), vbox2);

    paw->pwMoves = gtk_check_button_new_with_label(_("Chequer play"));
    gtk_box_pack_start(GTK_BOX(vbox2), paw->pwMoves, FALSE, FALSE, 0);

    paw->pwCube = gtk_check_button_new_with_label(_("Cube decisions"));
    gtk_box_pack_start(GTK_BOX(vbox2), paw->pwCube, FALSE, FALSE, 0);

    paw->pwLuck = gtk_check_button_new_with_label(_("Luck"));
    gtk_box_pack_start(GTK_BOX(vbox2), paw->pwLuck, FALSE, FALSE, 0);

    for (i = 0; i < 2; ++i) {

        gchar *sz = g_strdup_printf(_("Analyse player %s"), ap[i].szName);

        paw->apwAnalysePlayers[i] = gtk_check_button_new_with_label(sz);
        gtk_box_pack_start(GTK_BOX(vbox2), paw->apwAnalysePlayers[i], FALSE, FALSE, 0);
	g_free(sz);

    }

    pwFrame = gtk_frame_new(_("Skill thresholds"));
    gtk_box_pack_start(GTK_BOX(hboxMid), pwFrame, TRUE, TRUE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(pwFrame), 4);

#if GTK_CHECK_VERSION(3,0,0)
    pwGrid = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(pwFrame), pwGrid);
#else
    pwTable = gtk_table_new(5, 2, FALSE);
    gtk_container_add(GTK_CONTAINER(pwFrame), pwTable);
#endif

    for (i = 0; i < 3; i++) {
        pwLabel = gtk_label_new(gettext(aszSkillLabel[i]));
        gtk_label_set_justify(GTK_LABEL(pwLabel), GTK_JUSTIFY_RIGHT);
#if GTK_CHECK_VERSION(3,0,0)
        gtk_widget_set_halign(pwLabel, GTK_ALIGN_START);
        gtk_widget_set_valign(pwLabel, GTK_ALIGN_CENTER);
        gtk_grid_attach(GTK_GRID(pwGrid), pwLabel, 0, i, 1, 1);
#else
        gtk_misc_set_alignment(GTK_MISC(pwLabel), 0, 0.5);
        gtk_table_attach(GTK_TABLE(pwTable), pwLabel, 0, 1, i, i + 1,
                         (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);
#endif
    }

    for (i = 0; i < 3; i++) {
        paw->apadjSkill[i] = GTK_ADJUSTMENT(gtk_adjustment_new(1, 0, 1, 0.01, 0.05, 0));

        pwSpin = gtk_spin_button_new(GTK_ADJUSTMENT(paw->apadjSkill[i]), 1, 3);
#if GTK_CHECK_VERSION(3,0,0)
        gtk_grid_attach(GTK_GRID(pwGrid), pwSpin, 1, i, 1, 1);
        gtk_widget_set_hexpand(pwSpin, TRUE);
#else
        gtk_table_attach(GTK_TABLE(pwTable), pwSpin, 1, 2, i, i + 1,
                         (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), (GtkAttachOptions) (0), 0, 0);
#endif
        gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(pwSpin), TRUE);
    }

    pwFrame = gtk_frame_new(_("Luck thresholds"));
    gtk_box_pack_start(GTK_BOX(hboxMid), pwFrame, TRUE, TRUE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(pwFrame), 4);

#if GTK_CHECK_VERSION(3,0,0)
    pwGrid = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(pwFrame), pwGrid);
#else
    pwTable = gtk_table_new(4, 2, FALSE);
    gtk_container_add(GTK_CONTAINER(pwFrame), pwTable);
#endif

    for (i = 0; i < 4; i++) {
        pwLabel = gtk_label_new(gettext(aszLuckLabel[i]));
        gtk_label_set_justify(GTK_LABEL(pwLabel), GTK_JUSTIFY_RIGHT);
#if GTK_CHECK_VERSION(3,0,0)
        gtk_widget_set_halign(pwLabel, GTK_ALIGN_START);
        gtk_widget_set_valign(pwLabel, GTK_ALIGN_CENTER);
        gtk_grid_attach(GTK_GRID(pwGrid), pwLabel, 0, i, 1, 1);
#else
        gtk_misc_set_alignment(GTK_MISC(pwLabel), 0, 0.5);
        gtk_table_attach(GTK_TABLE(pwTable), pwLabel, 0, 1, i, i + 1,
                         (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);
#endif
    }

    for (i = 0; i < 4; i++) {
        paw->apadjLuck[i] = GTK_ADJUSTMENT(gtk_adjustment_new(1, 0, 1, 0.01, 0.05, 0));

        pwSpin = gtk_spin_button_new(GTK_ADJUSTMENT(paw->apadjLuck[i]), 1, 2);
#if GTK_CHECK_VERSION(3,0,0)
        gtk_grid_attach(GTK_GRID(pwGrid), pwSpin, 1, i, 1, 1);
        gtk_widget_set_hexpand(pwSpin, TRUE);
#else
        gtk_table_attach(GTK_TABLE(pwTable), pwSpin, 1, 2, i, i + 1,
                         (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), (GtkAttachOptions) (0), 0, 0);
#endif
        gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(pwSpin), TRUE);
    }

    pwFrame = gtk_frame_new(_("Analysis Level"));
    gtk_container_set_border_width(GTK_CONTAINER(pwFrame), 4);

#if GTK_CHECK_VERSION(3,0,0)
    vbox1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
    vbox1 = gtk_vbox_new(FALSE, 0);
#endif
    gtk_box_pack_start(GTK_BOX(hboxTop), vbox1, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox1), pwFrame, FALSE, FALSE, 0);

    /*giving a name to be able to g_free it later*/
    paw->pAnalDetailSettings1 = CreateEvalSettings(pwFrame, _("Analysis settings"),
            &paw->esChequer.ec, (movefilter *) &paw->aamf, &paw->esCube.ec, NULL, FALSE);
//    pAnalDetailSettings1 = CreateEvalSettings(pwFrame, _("Analysis settings"),
//                                              &aw.esChequer.ec, (movefilter *) & aw.aamf, &aw.esCube.ec, NULL, FALSE);

#if GTK_CHECK_VERSION(3,0,0)
    gtk_box_pack_start(GTK_BOX(pwPage), gtk_separator_new(GTK_ORIENTATION_VERTICAL), TRUE, TRUE, 0);
#else
    gtk_box_pack_start(GTK_BOX(pwPage), gtk_vseparator_new(), TRUE, TRUE, 0);
#endif

    pwFrame = gtk_frame_new(_("Eval Hint/Tutor Level"));
    gtk_container_set_border_width(GTK_CONTAINER(pwFrame), 4);
#if GTK_CHECK_VERSION(3,0,0)
    vbox2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
    vbox2 = gtk_vbox_new(FALSE, 0);
#endif
    gtk_box_pack_start(GTK_BOX(vbox2), pwFrame, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(pwPage), vbox2, FALSE, FALSE, 0);

#if GTK_CHECK_VERSION(3,0,0)
    vbox1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
    vbox1 = gtk_vbox_new(FALSE, 0);
#endif
    gtk_container_add(GTK_CONTAINER(pwFrame), vbox1);

    paw->pwHintSame = gtk_check_button_new_with_label(_("Same as analysis"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(paw->pwHintSame), fEvalSameAsAnalysis);
    g_signal_connect(G_OBJECT(paw->pwHintSame), "toggled", G_CALLBACK(HintSameToggled), paw);
#if GTK_CHECK_VERSION(3,0,0)
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
#else
    hbox = gtk_hbox_new(FALSE, 0);
#endif
    gtk_box_pack_start(GTK_BOX(hbox), paw->pwHintSame, FALSE, FALSE, 8);
    gtk_box_pack_start(GTK_BOX(vbox1), hbox, FALSE, FALSE, 0);

    paw->pAnalDetailSettings2 = CreateEvalSettings(vbox1, _("Hint/Tutor settings"),
                                              &paw->esEvalChequer.ec, (movefilter *) &paw->aaEvalmf, &paw->esEvalCube.ec,
                                              NULL, FALSE);
    paw->pwCubeSummary = paw->pAnalDetailSettings2->pwSettingWidgets;

    //gtk_container_add(GTK_CONTAINER(DialogArea(pwDialog, DA_MAIN)), pwPage);
    //AnalysisSet(paw);

    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(paw->pwHintSame));

    HintSameToggled(NULL, paw);

#if GTK_CHECK_VERSION(3,0,0)
    vbox3 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
#else
    vbox3 = gtk_vbox_new(FALSE, 0); //gtk_vbox_new (gboolean homogeneous, gint spacing);
#endif

    gtk_box_pack_start(GTK_BOX(hboxBottom), vbox3, TRUE, TRUE, 0);

    paw->pwAutoDB= gtk_check_button_new_with_label(_("Automatically add analysis to database"));
    gtk_box_pack_start(GTK_BOX(vbox3), paw->pwAutoDB, FALSE, FALSE, 0);
    gtk_widget_set_tooltip_text(paw->pwAutoDB,
                                _("Whenever the analysis of a game or match is complete, automatically "
                                  "add it to the database. The database needs to have been defined in "
                                  "Settings -> Options -> Database"));


    paw->pwBackgroundAnalysis= gtk_check_button_new_with_label(_("Allow background analysis (experimental) "));
    gtk_box_pack_start(GTK_BOX(vbox3), paw->pwBackgroundAnalysis, FALSE, FALSE, 0);
    gtk_widget_set_tooltip_text(paw->pwBackgroundAnalysis,
                                _("Allow browsing a match and its early analysis results while "
                                "analysis is still running in the background. Some features may be "
                                "disabled until the analysis is over."));

    BuildRadioButtons(vbox3, paw->apwAnalyzeFileSetting,
        _("Select the default file analysis settings (hover for details):"), 
        _("- Batch analysis can analyze several files, but does not allow browsing the results at the same time\n "
        "- Single-file analysis can only analyze one file, but can work in the background. \n"
        "- Smart analysis will open the latest file in the most recently opened folder and/or the default import "
        "folder, then run the single-file analysis."), 
        aszAnalyzeFileSetting, NUM_AnalyzeFileSettings, AnalyzeFileSettingDef);
    // BuildRadioButtons(vbox3, paw->apwAnalyzeFileSetting,  _("Select:"),   _("- Baanalysis."), aszAnalyzeFileSetting, NUM_AnalyzeFileSettings, AnalyzeFileSettingDef);

    // g_free(pAnalDetailSettings2); //<- not sure where to put it
    // g_free(pAnalDetailSettings1);

}

static GtkWidget *
AnalysisPages(analysiswidget * paw)
{

    paw->pwNoteBook = gtk_notebook_new();
    gtk_container_set_border_width(GTK_CONTAINER(paw->pwNoteBook), 8);

    append_analysis_options(paw);
    append_scoremap_options(paw);

    AnalysisSet(paw);

    return paw->pwNoteBook;
}

static void
AnalysisPageChange(GtkNotebook * UNUSED(notebook), gpointer * UNUSED(page), gint UNUSED(tabNumber), gpointer UNUSED(data))
{
    // if (tabNumber == relPage && !relPageActivated) {
    //     RelationalOptionsShown();
    //     relPageActivated = TRUE;
    // }
}

extern void
SetAnalysis(gpointer UNUSED(p), guint UNUSED(n), GtkWidget * UNUSED(pw))
{
     GtkWidget *pwDialog, *pwAnalysisSettings;
     analysiswidget aw;
    // AnalysisDetails *pAnalDetailSettings1 = NULL;
    // AnalysisDetails *pAnalDetailSettings2 = NULL;

    pwDialog = GTKCreateDialog(_("GNU Backgammon - Analysis Settings"),
                               DT_QUESTION, NULL, DIALOG_FLAG_MODAL, G_CALLBACK(AnalysisOK), &aw);

    gtk_container_add(GTK_CONTAINER(DialogArea(pwDialog, DA_MAIN)), pwAnalysisSettings = AnalysisPages(&aw));

    g_signal_connect(G_OBJECT(pwAnalysisSettings), "switch-page", G_CALLBACK(AnalysisPageChange), NULL);

    AnalysisSet(&aw);

    GTKRunDialog(pwDialog);
    g_free(aw.pAnalDetailSettings1);
    g_free(aw.pAnalDetailSettings2);
}

typedef struct {
    int *pfOK;
    player *ap;
    GtkWidget *apwName[2], *apwRadio[2][3], *apwSocket[2], *apwExternal[2];
    char aszSocket[2][128];
    evalsetup esChequer[2];
    evalsetup esCube[2];
    AnalysisDetails *pLevelSettings[2];
} playerswidget;

static void
PlayerTypeToggled(GtkWidget * UNUSED(pw), playerswidget * ppw)
{
    int i;

    for (i = 0; i < 2; i++) {
        gtk_widget_set_sensitive(ppw->pLevelSettings[i]->pwSettingWidgets,
                                 gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ppw->apwRadio[i][1])));
        gtk_widget_set_sensitive(ppw->apwExternal[i],
                                 gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ppw->apwRadio[i][2])));
    }
}

static GtkWidget *
PlayersPage(playerswidget * ppw, int i, const char *title)
{
    GtkWidget *pw, *pwFrame, *pwVBox;

    pwFrame = gtk_frame_new(title);
    gtk_container_set_border_width(GTK_CONTAINER(pwFrame), 4);
#if GTK_CHECK_VERSION(3,0,0)
    pwVBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
    pwVBox = gtk_vbox_new(FALSE, 0);
#endif
    gtk_container_add(GTK_CONTAINER(pwFrame), pwVBox);

#if GTK_CHECK_VERSION(3,0,0)
    pw = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
#else
    pw = gtk_hbox_new(FALSE, 0);
#endif
    gtk_container_set_border_width(GTK_CONTAINER(pw), 4);
    gtk_container_add(GTK_CONTAINER(pwVBox), pw);
    gtk_container_add(GTK_CONTAINER(pw), gtk_label_new(_("Default Name:")));
    gtk_container_add(GTK_CONTAINER(pw), ppw->apwName[i] = gtk_entry_new());
    gtk_entry_set_text(GTK_ENTRY(ppw->apwName[i]), (ppw->ap[i].szName));

    gtk_container_add(GTK_CONTAINER(pwVBox), ppw->apwRadio[i][0] = gtk_radio_button_new_with_label(NULL, _("Human")));

    gtk_container_add(GTK_CONTAINER(pwVBox),
                      ppw->apwRadio[i][1] =
                      gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(ppw->apwRadio[i][0]),
                                                                  _("GNU Backgammon")));

    memcpy(&ppw->esCube[i], &ppw->ap[i].esChequer.ec, sizeof(ppw->esCube[i]));
    memcpy(&ppw->esChequer[i], &ppw->ap[i].esCube.ec, sizeof(ppw->esChequer[i]));

    ppw->pLevelSettings[i] = CreateEvalSettings(pwVBox, _("GNU Backgammon settings"),
                                                &ppw->ap[i].esChequer.ec, (movefilter *) ppw->ap[i].aamf,
                                                &ppw->ap[i].esCube.ec, NULL, TRUE);

    gtk_container_add(GTK_CONTAINER(pwVBox),
                      ppw->apwRadio[i][2] =
                      gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(ppw->apwRadio[i][0]),
                                                                  _("External")));
#if GTK_CHECK_VERSION(3,0,0)
    ppw->apwExternal[i] = pw = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
#else
    ppw->apwExternal[i] = pw = gtk_hbox_new(FALSE, 0);
#endif
    gtk_container_set_border_width(GTK_CONTAINER(pw), 4);
    gtk_widget_set_sensitive(pw, ap[i].pt == PLAYER_EXTERNAL);
    gtk_container_add(GTK_CONTAINER(pwVBox), pw);
    gtk_container_add(GTK_CONTAINER(pw), gtk_label_new(_("Socket:")));
    gtk_container_add(GTK_CONTAINER(pw), ppw->apwSocket[i] = gtk_entry_new());
    if (ap[i].szSocket)
        gtk_entry_set_text(GTK_ENTRY(ppw->apwSocket[i]), ap[i].szSocket);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ppw->apwRadio[i][ap[i].pt]), TRUE);

    g_signal_connect(G_OBJECT(ppw->apwRadio[i][1]), "toggled", G_CALLBACK(PlayerTypeToggled), ppw);
    g_signal_connect(G_OBJECT(ppw->apwRadio[i][2]), "toggled", G_CALLBACK(PlayerTypeToggled), ppw);

    return pwFrame;
}

static void
PlayersOK(GtkWidget * pw, playerswidget * pplw)
{

    int i;
    playertype j;
    *pplw->pfOK = TRUE;

    for (i = 0; i < 2; i++) {
        g_strlcpy(pplw->ap[i].szName, gtk_entry_get_text(GTK_ENTRY(pplw->apwName[i])), MAX_NAME_LEN);

        for (j = (playertype) 0; j < (playertype) 3; j++)
            if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pplw->apwRadio[i][j]))) {
                pplw->ap[i].pt = j;
                break;
            }

        g_strlcpy(pplw->aszSocket[i], gtk_entry_get_text(GTK_ENTRY(pplw->apwSocket[i])), 128);
    }

    gtk_widget_destroy(gtk_widget_get_toplevel(pw));
}

static void
SetPlayers(gpointer UNUSED(p), guint UNUSED(n), GtkWidget * UNUSED(pw))
{
    GtkWidget *pwDialog, *pwHBox;
    int fOK = FALSE;
    player apTemp[2];
    playerswidget plw;

    memcpy(apTemp, ap, sizeof ap);
    plw.ap = apTemp;
    plw.pfOK = &fOK;

    pwDialog =
        GTKCreateDialog(_("GNU Backgammon - Players"), DT_QUESTION,
                        NULL, DIALOG_FLAG_MODAL, G_CALLBACK(PlayersOK), &plw);

#if GTK_CHECK_VERSION(3,0,0)
    pwHBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
#else
    pwHBox = gtk_hbox_new(FALSE, 0);
#endif

    gtk_container_add(GTK_CONTAINER(DialogArea(pwDialog, DA_MAIN)), pwHBox);
    gtk_box_pack_start(GTK_BOX(pwHBox), PlayersPage(&plw, 0, _("Player 0")), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(pwHBox), PlayersPage(&plw, 1, _("Player 1")), FALSE, FALSE, 0);
    PlayerTypeToggled(NULL, &plw);

    GTKRunDialog(pwDialog);

    free(plw.pLevelSettings[0]);
    free(plw.pLevelSettings[1]);

    if (fOK) {
        char sz[256];

        outputpostpone();

        sprintf(sz, "set defaultnames \"%s\" \"%s\"", apTemp[0].szName, apTemp[1].szName);
        UserCommand(sz);

        for (int i = 0; i < 2; i++) {
            /* NB: this comparison is case-sensitive, and does not use
             * CompareNames(), so that the user can modify the case of
             * names. */
            switch (apTemp[i].pt) {
            case PLAYER_HUMAN:
                if (ap[i].pt != PLAYER_HUMAN) {
                    sprintf(sz, "set player %d human", i);
                    UserCommand(sz);
                }
                break;

            case PLAYER_GNU:
                if (ap[i].pt != PLAYER_GNU) {
                    sprintf(sz, "set player %d gnubg", i);
                    UserCommand(sz);
                }

                /* FIXME another temporary hack (should be some way to set
                 * chequer and cube parameters independently) */
                sprintf(sz, "set player %d chequer evaluation", i);
                SetEvalCommands(sz, &apTemp[i].esChequer.ec, &ap[i].esChequer.ec);
                sprintf(sz, "set player %d movefilter", i);
                SetMovefilterCommands(sz, apTemp[i].aamf, ap[i].aamf);
                sprintf(sz, "set player %d cube evaluation", i);
                SetEvalCommands(sz, &apTemp[i].esCube.ec, &ap[i].esCube.ec);
                break;

            case PLAYER_EXTERNAL:
                if (ap[i].pt != PLAYER_EXTERNAL || strcmp(ap[i].szSocket, plw.aszSocket[i])) {
                    sprintf(sz, "set player %d external %s", i, plw.aszSocket[i]);
                    UserCommand(sz);
                }
                break;
            }
        }

        UserCommand("save settings");
        outputresume();
    }
}

static void
SetOptions(gpointer UNUSED(p), guint UNUSED(n), GtkWidget * UNUSED(pw))
{

    GTKSetOptions();

}

/* Language screen name, code and flag name */
static char *aaszLang[][3] = {
    {N_("System default"), "system", NULL},
    {N_("Czech"), "cs_CZ.UTF-8", "flags/czech.png"},
    {N_("Danish"), "da_DK.UTF-8", "flags/denmark.png"},
    {N_("English (GB)"), "en_GB.UTF-8", "flags/england.png"},
    {N_("English (US)"), "en_US.UTF-8", "flags/usa.png"},
    {N_("Finnish"), "fi_FI.UTF-8", "flags/finland.png"},
    {N_("French"), "fr_FR.UTF-8", "flags/france.png"},
    {N_("German"), "de_DE.UTF-8", "flags/germany.png"},
    {N_("Greek"), "el_GR.UTF-8", "flags/greece.png"},
    {N_("Icelandic"), "is_IS.UTF-8", "flags/iceland.png"},
    {N_("Italian"), "it_IT.UTF-8", "flags/italy.png"},
    {N_("Japanese"), "ja_JP.UTF-8", "flags/japan.png"},
    {N_("Romanian"), "ro_RO.UTF-8", "flags/romania.png"},
    {N_("Russian"), "ru_RU.UTF-8", "flags/russia.png"},
    {N_("Spanish"), "es_ES.UTF-8", "flags/spain.png"},
    {N_("Turkish"), "tr_TR.UTF-8", "flags/turkey.png"},
    {NULL, NULL, NULL}
};

static void
TranslateWidgets(GtkWidget * p)
{
    if (GTK_IS_CONTAINER(p)) {
        GList *pl = gtk_container_get_children(GTK_CONTAINER(p));
        while (pl) {
            TranslateWidgets(pl->data);
            pl = pl->next;
        }
        g_list_free(pl);
    }
    if (GTK_IS_LABEL(p)) {
        char *name = (char *) g_object_get_data(G_OBJECT(p), "lang");
        gtk_label_set_text(GTK_LABEL(p), _(name));
    }
}

static void
SetLangDialogText(void)
{
    gtk_window_set_title(GTK_WINDOW(pwLangDialog), _("Select language"));
    TranslateWidgets(pwLangDialog);
}

static void
SetLangOk(void)
{
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pwLangRadio1)))
        newLang = aaszLang[0][1];
    else
        newLang = (char *) g_object_get_data(G_OBJECT(curSel), "lang");

    gtk_widget_destroy(pwLangDialog);
}

static gboolean
FlagClicked(GtkWidget * pw, GdkEventButton * event, void *UNUSED(dummy))
{
    /* Manually highlight clicked flag */
    GtkWidget *frame, *eb;
#if GTK_CHECK_VERSION(3,0,0)
    GdkRGBA color;
#endif

    if (event && event->type == GDK_2BUTTON_PRESS && curSel == pw)
        SetLangOk();

    if (curSel == pw)
        return FALSE;

    if (curSel) {               /* Reset old item */
        frame = gtk_bin_get_child(GTK_BIN(curSel));
        eb = gtk_bin_get_child(GTK_BIN(frame));
        gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
#if GTK_CHECK_VERSION(3,0,0)
        gtk_widget_override_background_color(eb, GTK_STATE_FLAG_NORMAL, NULL);
#else
        gtk_widget_modify_bg(eb, GTK_STATE_NORMAL, NULL);
#endif
    }

    curSel = pw;
    frame = gtk_bin_get_child(GTK_BIN(pw));
    eb = gtk_bin_get_child(GTK_BIN(frame));

    gtk_frame_set_shadow_type(GTK_FRAME(gtk_bin_get_child(GTK_BIN(pw))), GTK_SHADOW_ETCHED_OUT);
#if GTK_CHECK_VERSION(3,0,0)
    gtk_style_context_get_background_color(gtk_widget_get_style_context(eb), GTK_STATE_FLAG_SELECTED, &color);
    gtk_widget_override_background_color(eb, GTK_STATE_FLAG_NORMAL, &color);
#else
    gtk_widget_modify_bg(eb, GTK_STATE_NORMAL, &gtk_widget_get_style(pwMain)->bg[GTK_STATE_SELECTED]);
#endif

    if (SetupLanguage((char *) g_object_get_data(G_OBJECT(curSel), "lang")))
        /* Immediately translate this dialog */
        SetLangDialogText();
    else
        outputerrf(_("Locale '%s' not supported by C library."), (char *) g_object_get_data(G_OBJECT(curSel), "lang"));

    gtk_widget_set_sensitive(DialogArea(pwLangDialog, DA_OK), TRUE);

    return FALSE;
}

static GtkWidget *
GetFlagWidget(char *language, char *langCode, const char *flagfilename)
{                               /* Create a flag */
    GtkWidget *eb, *eb2, *vbox, *lab1;
    GtkWidget *frame;
#if GTK_CHECK_VERSION(3,0,0)
    GdkRGBA color;
#endif
    GError *pix_error = NULL;

    eb = gtk_event_box_new();
#if GTK_CHECK_VERSION(3,0,0)
    gtk_style_context_get_background_color(gtk_widget_get_style_context(eb), GTK_STATE_FLAG_NORMAL, &color);
    gtk_widget_override_background_color(eb, GTK_STATE_FLAG_INSENSITIVE, &color);
#else
    gtk_widget_modify_bg(eb, GTK_STATE_INSENSITIVE, &gtk_widget_get_style(pwMain)->bg[GTK_STATE_NORMAL]);
#endif
    frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
    gtk_container_add(GTK_CONTAINER(eb), frame);

    eb2 = gtk_event_box_new();
#if GTK_CHECK_VERSION(3,0,0)
    gtk_style_context_get_background_color(gtk_widget_get_style_context(eb2), GTK_STATE_FLAG_NORMAL, &color);
    gtk_widget_override_background_color(eb2, GTK_STATE_FLAG_INSENSITIVE, &color);
#else
    gtk_widget_modify_bg(eb2, GTK_STATE_INSENSITIVE, &gtk_widget_get_style(pwMain)->bg[GTK_STATE_NORMAL]);
#endif
    gtk_container_add(GTK_CONTAINER(frame), eb2);

#if GTK_CHECK_VERSION(3,0,0)
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
#else
    vbox = gtk_vbox_new(FALSE, 5);
#endif
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);
    gtk_container_add(GTK_CONTAINER(eb2), vbox);

    if (flagfilename) {
        char *file = BuildFilename(flagfilename);
        GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(file, &pix_error);

        if (pix_error)
            outputerrf(_("Failed to open flag image: %s\n"), file);
        else {
            GtkWidget *image = gtk_image_new_from_pixbuf(pixbuf);
            gtk_box_pack_start(GTK_BOX(vbox), image, FALSE, FALSE, 0);
        }
        g_free(file);
    }
    lab1 = gtk_label_new(NULL);
    gtk_widget_set_size_request(lab1, 80, -1);
    gtk_box_pack_start(GTK_BOX(vbox), lab1, FALSE, FALSE, 0);
    g_object_set_data(G_OBJECT(lab1), "lang", language);

    g_signal_connect(G_OBJECT(eb), "button_press_event", G_CALLBACK(FlagClicked), NULL);

    g_object_set_data(G_OBJECT(eb), "lang", langCode);

    return eb;
}

static int
defclick(GtkWidget * UNUSED(pw), void *UNUSED(dummy), GtkWidget * table)
{
    gtk_widget_set_sensitive(table, FALSE);
    SetupLanguage("");
    SetLangDialogText();
    gtk_widget_set_sensitive(DialogArea(pwLangDialog, DA_OK), TRUE);
    return FALSE;
}

static int
selclick(GtkWidget * UNUSED(pw), void *UNUSED(dummy), GtkWidget * table)
{
    gtk_widget_set_sensitive(table, TRUE);
    if (curSel) {
        SetupLanguage((char *) g_object_get_data(G_OBJECT(curSel), "lang"));
        SetLangDialogText();
    } else
        gtk_widget_set_sensitive(DialogArea(pwLangDialog, DA_OK), FALSE);

    return FALSE;
}

static void
SetWidgetLabelLang(GtkWidget * pw, char *text)
{
    if (GTK_IS_CONTAINER(pw)) {
        GList *pl = gtk_container_get_children(GTK_CONTAINER(pw));
        while (pl) {
            SetWidgetLabelLang(pl->data, text);
            pl = pl->next;
        }
        g_list_free(pl);
    }
    if (GTK_IS_LABEL(pw))
        g_object_set_data(G_OBJECT(pw), "lang", text);
}

static void
AddLangWidgets(GtkWidget * cont)
{
    int i, numLangs;
    GtkWidget *pwVbox, *pwHbox, *selLang = NULL;
#if GTK_CHECK_VERSION(3,0,0)
    pwVbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
    pwVbox = gtk_vbox_new(FALSE, 0);
#endif
    gtk_container_add(GTK_CONTAINER(cont), pwVbox);

    pwLangRadio1 = gtk_radio_button_new_with_label(NULL, "");
    SetWidgetLabelLang(pwLangRadio1, N_("System default"));
    pwLangRadio2 = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(pwLangRadio1), "");
    SetWidgetLabelLang(pwLangRadio2, N_("Select language"));
    gtk_box_pack_start(GTK_BOX(pwVbox), pwLangRadio1, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(pwVbox), pwLangRadio2, FALSE, FALSE, 0);

    numLangs = 0;
    while (*aaszLang[numLangs])
        numLangs++;
    numLangs--;                 /* Don't count system default */

#define NUM_COLS 4              /* Display in 4 columns */
#if GTK_CHECK_VERSION(3,0,0)
    pwLangTable = gtk_grid_new();
    gtk_grid_set_column_homogeneous(GTK_GRID(pwLangTable), TRUE);
    gtk_grid_set_row_homogeneous(GTK_GRID(pwLangTable), TRUE);
#else
    pwLangTable = gtk_table_new(numLangs / NUM_COLS + 1, NUM_COLS, TRUE);
#endif

    for (i = 0; i < numLangs; i++) {
        GtkWidget *pwFlag = GetFlagWidget(aaszLang[i + 1][0], aaszLang[i + 1][1], aaszLang[i + 1][2]);
        int row = i / NUM_COLS;
        int col = i - row * NUM_COLS;
#if GTK_CHECK_VERSION(3,0,0)
        gtk_grid_attach(GTK_GRID(pwLangTable), pwFlag, col, row, 1, 1);
#else
        gtk_table_attach(GTK_TABLE(pwLangTable), pwFlag, col, col + 1, row, row + 1, (GtkAttachOptions) 0,
                         (GtkAttachOptions) 0, 0, 0);
#endif
        if (!StrCaseCmp(szLang, aaszLang[i + 1][1]))
            selLang = pwFlag;
    }
#if GTK_CHECK_VERSION(3,0,0)
    pwHbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
#else
    pwHbox = gtk_hbox_new(FALSE, 0);
#endif
    gtk_box_pack_start(GTK_BOX(pwVbox), pwHbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(pwHbox), pwLangTable, FALSE, FALSE, 20);

    if (selLang == NULL)
        defclick(0, 0, pwLangTable);
    else {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pwLangRadio2), TRUE);
        FlagClicked(selLang, 0, 0);
    }
    g_signal_connect(G_OBJECT(pwLangRadio1), "button_press_event", G_CALLBACK(defclick), pwLangTable);
    g_signal_connect(G_OBJECT(pwLangRadio2), "button_press_event", G_CALLBACK(selclick), pwLangTable);
}


static void
SetLanguage(gpointer UNUSED(p), guint UNUSED(n), GtkWidget * UNUSED(w))
{
    GList *pl;

    pwLangDialog = GTKCreateDialog(NULL, DT_QUESTION, NULL, DIALOG_FLAG_MODAL, SetLangOk, NULL);

    pl = gtk_container_get_children(GTK_CONTAINER(DialogArea(pwLangDialog, DA_BUTTONS)));
    SetWidgetLabelLang(GTK_WIDGET(pl->data), N_("Cancel"));
    SetWidgetLabelLang(GTK_WIDGET(pl->next->data), N_("OK"));
    g_list_free(pl);

    curSel = NULL;
    newLang = NULL;

    AddLangWidgets(DialogArea(pwLangDialog, DA_MAIN));

    GTKRunDialog(pwLangDialog);

    if (newLang)
        CommandSetLang(newLang);        /* Set new language (after dialog has closed) */
    else
        SetupLanguage(szLang);  /* If cancelled make sure language stays the same */
}


static void
ReportBug(gpointer UNUSED(p), guint UNUSED(n), GtkWidget * UNUSED(pwEvent))
{
    OpenURL("https://savannah.gnu.org/bugs/?func=additem&group=gnubg");
}

#if !defined(USE_GTKITEMFACTORY)

static GtkActionEntry actionEntries[] = {
    { "FileMenuAction", NULL, N_("_File"), NULL, NULL, G_CALLBACK(NULL) },
    { "FileNewAction", GTK_STOCK_NEW, N_("_New..."), "<control>N", N_("Start new game, match, session or position"),
     G_CALLBACK(NewClicked) },
    { "FileOpenAction", GTK_STOCK_OPEN, N_("_Open"), "<control>O", N_("Open game, match, session or position"),
     G_CALLBACK(GTKOpen) },
    { "FileSaveAction", GTK_STOCK_SAVE, N_("_Save"), "<control>S", N_("Save match, session, game or position"),
     G_CALLBACK(GTKSave) },
    { "FileCommandsOpenAction", NULL, N_("Open _Commands..."), NULL, NULL, G_CALLBACK(GTKCommandsOpen) },
    { "FileMatchInfoAction", NULL, N_("Match information..."), NULL, NULL, G_CALLBACK(GTKMatchInfo) },
#if defined(WIN32)
    { "FileExitAction", NULL, N_("E_xit"), "<control>Q", NULL, CMD_ACTION_CALLBACK_FROMID(CMD_QUIT) },
#else
    { "FileExitAction", NULL, N_("_Quit"), "<control>Q", NULL, CMD_ACTION_CALLBACK_FROMID(CMD_QUIT) },
#endif

    { "EditMenuAction", NULL, N_("_Edit"), NULL, NULL, G_CALLBACK(NULL) },
    { "UndoAction", GTK_STOCK_UNDO, N_("_Undo"), "<control>Z", N_("Undo moves"), G_CALLBACK(GTKUndo) },
    { "CopyIDMenuAction", NULL, N_("_Copy ID to Clipboard"), NULL, NULL, G_CALLBACK(NULL) },
    { "CopyGNUBGIDAction", NULL, N_("GNUbg ID"), "<control>C", NULL, G_CALLBACK(CopyIDs) },
    { "CopyMatchIDAction", NULL, N_("Match ID"), "<control>M", NULL, G_CALLBACK(CopyMatchID) },
    { "CopyPositionIDAction", NULL, N_("Position ID"), "<control>P", NULL, G_CALLBACK(CopyPositionID) },

    { "CopyAsMenuAction", NULL, N_("Copy as"), NULL, NULL, G_CALLBACK(NULL) },
    { "CopyPosAsAsciiAction", NULL, N_("Position as ASCII"), NULL, NULL, G_CALLBACK(CommandCopy) },
    { "CopyAsGammOnLineAction", NULL, N_("BGonline (HTML)"), NULL, NULL, G_CALLBACK(CopyAsGOL) },

    { "PasteIDAction", GTK_STOCK_PASTE, N_("_Paste ID"), "<control>V", NULL, G_CALLBACK(PasteIDs) },

    { "ViewMenuAction", NULL, N_("_View"), NULL, NULL, G_CALLBACK(NULL) },
    { "PanelsMenuAction", NULL, N_("_Panels"), NULL, NULL, G_CALLBACK(NULL) },

    { "RestorePanelsAction", NULL, N_("Restore panels"), NULL, NULL, G_CALLBACK(ShowAllPanels) },
    { "HidePanelsAction", NULL, N_("Hide panels"), NULL, NULL, G_CALLBACK(HideAllPanels) },

    { "ToolBarMenuAction", NULL, N_("_Toolbar"), NULL, NULL, G_CALLBACK(NULL) },
    { "HideToolBarAction", NULL, N_("_Hide Toolbar"), NULL, NULL, G_CALLBACK(HideToolbar) },
    { "ShowToolBarAction", NULL, N_("_Show Toolbar"), NULL, NULL, G_CALLBACK(ShowToolbar) },

#if defined(USE_BOARD3D)
    { "SwitchModeAction", NULL, N_("Switch to xD view"), NULL, NULL, G_CALLBACK(SwitchDisplayMode) },
#endif

    { "GameMenuAction", NULL, N_("_Game"), NULL, NULL, G_CALLBACK(NULL) },
    { "RollAction", NULL, N_("_Roll"), "<control>R", NULL, CMD_ACTION_CALLBACK_FROMID(CMD_ROLL) },
    { "FinishMoveAction", NULL, N_("_Finish move"), "<control>F", NULL, G_CALLBACK(FinishMove) },
    { "DoubleAction", GNUBG_STOCK_DOUBLE, N_("_Double"), "<control>D", N_("Double or redouble(beaver)"),
     CMD_ACTION_CALLBACK_FROMID(CMD_DOUBLE) },
    { "ResignAction", GNUBG_STOCK_RESIGN, N_("Re_sign"), NULL, N_("Resign the current game"), G_CALLBACK(GTKResign) },
    { "AcceptAction", GNUBG_STOCK_ACCEPT, N_("_Accept"), NULL,
     N_("Take the offered cube or accept the offered resignation"), CMD_ACTION_CALLBACK_FROMID(CMD_ACCEPT) },
    { "RejectAction", GNUBG_STOCK_REJECT, N_("Re_ject"), NULL,
     N_("Drop the offered cube or decline the offered resignation"), CMD_ACTION_CALLBACK_FROMID(CMD_REJECT) },
    { "PlayComputerTurnAction", NULL, N_("Play computer turn"), NULL, NULL, CMD_ACTION_CALLBACK_FROMID(CMD_PLAY) },
    { "EndGameAction", GNUBG_STOCK_END_GAME, N_("_End Game"), "<control>G", N_("Let the computer end the game"),
     CMD_ACTION_CALLBACK_FROMID(CMD_END_GAME) },
    { "SwapPlayersAction", NULL, N_("Swap players"), NULL, NULL, CMD_ACTION_CALLBACK_FROMID(CMD_SWAP_PLAYERS) },
    { "SetCubeAction", NULL, N_("Set cube..."), NULL, NULL, G_CALLBACK(GTKSetCube) },
    { "SetDiceAction", NULL, N_("Set _dice..."), NULL, NULL, G_CALLBACK(GTKSetDice) },

    { "SetTurnMenuAction", NULL, N_("Set _turn"), NULL, NULL, G_CALLBACK(NULL) },

    { "ClearTurnAction", NULL, N_("Clear turn"), NULL, NULL, CMD_ACTION_CALLBACK_FROMID(CMD_CLEAR_TURN) },

    { "AnalyseMenuAction", NULL, N_("_Analyse"), NULL, NULL, G_CALLBACK(NULL) },
    { "EvaluateAction", NULL, N_("_Evaluate"), "<control>E", NULL, CMD_ACTION_CALLBACK_FROMID(CMD_EVAL) },
    { "HintAction", GNUBG_STOCK_HINT, N_("_Hint"), "<control>H", N_("Show the best moves or cube action"),
     CMD_ACTION_CALLBACK_FROMID(CMD_HINT) },
    { "RolloutAction", NULL, N_("_Rollout position"), NULL, NULL, CMD_ACTION_CALLBACK_FROMID(CMD_ROLLOUT) },
    { "AnalyseMoveAction", NULL, N_("Analyse move"), NULL, NULL, CMD_ACTION_CALLBACK_FROMID(CMD_ANALYSE_MOVE) },
    { "AnalyseGameAction", NULL, N_("Analyse game"), NULL, NULL, CMD_ACTION_CALLBACK_FROMID(CMD_ANALYSE_GAME) },
    { "AnalyseMatchAction", NULL, N_("Analyse match or session"), NULL, NULL,
     CMD_ACTION_CALLBACK_FROMID(CMD_ANALYSE_MATCH) },

    { "ClearAnalysisMenuAction", NULL, N_("Clear analysis"), NULL, NULL, G_CALLBACK(NULL) },
    { "ClearAnalysisMoveAction", GTK_STOCK_CLEAR, N_("Move"), NULL, NULL,
     CMD_ACTION_CALLBACK_FROMID(CMD_ANALYSE_CLEAR_MOVE) },
    { "ClearAnalysisGameAction", GTK_STOCK_CLEAR, N_("_Game"), NULL, NULL,
     CMD_ACTION_CALLBACK_FROMID(CMD_ANALYSE_CLEAR_GAME) },
    { "ClearAnalysisMatchOrSessionAction", GTK_STOCK_CLEAR, N_("_Match or session"), NULL, NULL,
     CMD_ACTION_CALLBACK_FROMID(CMD_ANALYSE_CLEAR_MATCH) },

    { "CMarkMenuAction", NULL, N_("CMark"), NULL, NULL, G_CALLBACK(NULL) },
    { "CMarkCubeMenuAction", NULL, N_("Cube"), NULL, NULL, G_CALLBACK(NULL) },
    { "CMarkCubeClearAction", NULL, N_("Clear"), NULL, NULL, CMD_ACTION_CALLBACK_FROMID(CMD_CMARK_CUBE_CLEAR) },
    { "CMarkCubeShowAction", NULL, N_("Show"), NULL, NULL, CMD_ACTION_CALLBACK_FROMID(CMD_CMARK_CUBE_SHOW) },
    { "CMarkMoveMenuAction", NULL, N_("Move"), NULL, NULL, G_CALLBACK(NULL) },
    { "CMarkMoveClearAction", NULL, N_("Clear"), NULL, NULL, CMD_ACTION_CALLBACK_FROMID(CMD_CMARK_MOVE_CLEAR) },
    { "CMarkMoveShowAction", NULL, N_("Show"), NULL, NULL, CMD_ACTION_CALLBACK_FROMID(CMD_CMARK_MOVE_SHOW) },
    { "CMarkGameMenuAction", NULL, N_("Game"), NULL, NULL, G_CALLBACK(NULL) },
    { "CMarkGameClearAction", NULL, N_("Clear"), NULL, NULL, CMD_ACTION_CALLBACK_FROMID(CMD_CMARK_GAME_CLEAR) },
    { "CMarkGameShowAction", NULL, N_("Show"), NULL, NULL, CMD_ACTION_CALLBACK_FROMID(CMD_CMARK_GAME_SHOW) },
    { "CMarkMatchMenuAction", NULL, N_("Match"), NULL, NULL, G_CALLBACK(NULL) },
    { "CMarkMatchClearAction", NULL, N_("Clear"), NULL, NULL, CMD_ACTION_CALLBACK_FROMID(CMD_CMARK_MATCH_CLEAR) },
    { "CMarkMatchShowAction", NULL, N_("Show"), NULL, NULL, CMD_ACTION_CALLBACK_FROMID(CMD_CMARK_MATCH_SHOW) },

    { "RolloutMenuAction", NULL, N_("Rollout"), NULL, NULL, G_CALLBACK(NULL) },
    { "RolloutCubeAction", NULL, N_("Cube"), NULL, NULL, CMD_ACTION_CALLBACK_FROMID(CMD_ANALYSE_ROLLOUT_CUBE) },
    { "RolloutMoveAction", NULL, N_("CMarked from Move"), NULL, NULL,
     CMD_ACTION_CALLBACK_FROMID(CMD_ANALYSE_ROLLOUT_MOVE) },
    { "RolloutGameAction", NULL, N_("CMarked from Game"), NULL, NULL,
     CMD_ACTION_CALLBACK_FROMID(CMD_ANALYSE_ROLLOUT_GAME) },
    { "RolloutMatchAction", NULL, N_("CMarked from Match"), NULL, NULL,
     CMD_ACTION_CALLBACK_FROMID(CMD_ANALYSE_ROLLOUT_MATCH) },
    { "AnalyseCurrentAction", GTK_STOCK_EXECUTE, N_("Analyse"), NULL,
     N_("Analyse current match (set default behavior in Settings -> Analysis)"), G_CALLBACK(GTKAnalyzeCurrent) },
    { "AnalyseFileAction", GTK_STOCK_DIRECTORY, N_("Analyse File"), NULL,
     N_("Analyze match from file (set default behaviour in Settings -> Analysis -> Analysis Buttons)"),
     G_CALLBACK(GTKAnalyzeFile) },
    { "BatchAnalyseAction", NULL, N_("Batch analyse..."), NULL, NULL, G_CALLBACK(GTKBatchAnalyse) },
    { "MatchOrSessionStatsAction", NULL, N_("Match or session statistics"), NULL, NULL,
     CMD_ACTION_CALLBACK_FROMID(CMD_SHOW_STATISTICS_MATCH) },
    { "AddMatchOrSessionStatsToDBAction", GTK_STOCK_ADD, N_("Add match or session to database"), NULL, NULL,
     G_CALLBACK(GtkRelationalAddMatch) },
    { "ShowRecordsAction", NULL, N_("Show Records"), NULL, NULL, G_CALLBACK(GtkShowRelational) },
    { "PlotHistoryAction", NULL, N_("Plot History"), NULL, NULL,
     CMD_ACTION_CALLBACK_FROMID(CMD_SHOW_HISTORY) },
    { "DistributionOfRollsAction", NULL, N_("Distribution of rolls"), NULL, NULL,
     CMD_ACTION_CALLBACK_FROMID(CMD_SHOW_ROLLS) },
    { "TemperatureMapAction", NULL, N_("Temperature Map"), NULL, NULL,
     CMD_ACTION_CALLBACK_FROMID(CMD_SHOW_TEMPERATURE_MAP) },
    { "TemperatureMapCubeAction", NULL, N_("Temperature Map (cube decision)"), NULL, NULL,
     CMD_ACTION_CALLBACK_FROMID(CMD_SHOW_TEMPERATURE_MAP_CUBE) },
    { "ScoreMapCubeAction", NULL, N_("ScoreMap (cube decision)"), NULL, NULL,
     CMD_ACTION_CALLBACK_FROMID(CMD_SHOW_SCORE_MAP_CUBE) },
    { "ScoreMapMoveAction", NULL, N_("ScoreMap (move decision)"), NULL, NULL,
     CMD_ACTION_CALLBACK_FROMID(CMD_SHOW_SCORE_MAP_MOVE) },
    { "RaceTheoryAction", NULL, N_("_Race Theory"), NULL, NULL, CMD_ACTION_CALLBACK_FROMID(CMD_SHOW_KLEINMAN) },
    { "MarketWindowAction", NULL, N_("_Market window"), NULL, NULL, CMD_ACTION_CALLBACK_FROMID(CMD_SHOW_MARKETWINDOW) },
    { "MatchEquityTableAction", NULL, N_("M_atch equity table"), NULL, NULL,
     CMD_ACTION_CALLBACK_FROMID(CMD_SHOW_MATCHEQUITYTABLE) },
    { "EvaluationSpeedAction", NULL, N_("Evaluation speed"), NULL, NULL,
     CMD_ACTION_CALLBACK_FROMID(CMD_SHOW_CALIBRATION) },

    { "SettingsMenuAction", NULL, N_("_Settings"), NULL, NULL, G_CALLBACK(NULL) },
    { "SettingsAnalysisAction", NULL, N_("_Analysis..."), NULL, NULL, G_CALLBACK(SetAnalysis) },
    { "SettingsBoardAppearanceAction", NULL, N_("_Board Appearance..."), NULL, NULL,
     CMD_ACTION_CALLBACK_FROMID(CMD_SET_APPEARANCE) },
    { "SettingsExportAction", NULL, N_("E_xport..."), NULL, NULL, CMD_ACTION_CALLBACK_FROMID(CMD_SHOW_EXPORT) },
    { "SettingsPlayersAction", NULL, N_("_Players..."), NULL, NULL, G_CALLBACK(SetPlayers) },
    { "SettingsRolloutsAction", NULL, N_("_Rollouts..."), NULL, NULL, G_CALLBACK(SetRollouts) },
    { "SettingsOptionsAction", NULL, N_("_Options..."), NULL, NULL, G_CALLBACK(SetOptions) },
    { "SettingsLanguageAction", NULL, N_("_Language..."), NULL, NULL, G_CALLBACK(SetLanguage) },

    { "GoMenuAction", NULL, N_("G_o"), NULL, NULL, G_CALLBACK(NULL) },
    { "GoPreviousMarkedMoveAction", GNUBG_STOCK_GO_PREV_MARKED, N_("Previous marked move"), "<shift><control>Page_Up",
     N_("Go to Previous Marked"), CMD_ACTION_CALLBACK_FROMID(CMD_PREV_MARKED) },
    { "GoPreviousCMarkedMoveAction", GNUBG_STOCK_GO_PREV_CMARKED, N_("Previous cmarked move"), "<shift>Page_Up",
     N_("Go to Previous CMarked"),
     CMD_ACTION_CALLBACK_FROMID(CMD_PREV_CMARKED) },
    { "GoPreviousRollAction", GNUBG_STOCK_GO_PREV, N_("Previous rol_l"), "Page_Up", N_("Go to Previous Roll"),
     CMD_ACTION_CALLBACK_FROMID(CMD_PREV_ROLL) },
    { "GoPreviousGameAction", GNUBG_STOCK_GO_PREV_GAME, N_("Pre_vious game"), "<control>Page_Up",
     N_("Go to Previous Game"),
     CMD_ACTION_CALLBACK_FROMID(CMD_PREV_GAME) },
    { "GoNextGameAction", GNUBG_STOCK_GO_NEXT_GAME, N_("Next _game"), "<control>Page_Down", N_("Go to Next Game"),
     CMD_ACTION_CALLBACK_FROMID(CMD_NEXT_GAME) },
    { "GoNextRollAction", GNUBG_STOCK_GO_NEXT, N_("Next _roll"), "Page_Down", N_("Go to Next Roll"),
     CMD_ACTION_CALLBACK_FROMID(CMD_NEXT_ROLL) },
    { "GoNextCMarkedMoveAction", GNUBG_STOCK_GO_NEXT_CMARKED, N_("Next cmarked move"), "<shift>Page_Down",
     N_("Go to Next CMarked"),
     CMD_ACTION_CALLBACK_FROMID(CMD_NEXT_CMARKED) },
    { "GoNextMarkedMoveAction", GNUBG_STOCK_GO_NEXT_MARKED, N_("Next marked move"), "<shift><control>Page_Down",
     N_("Go to Next Marked"),
     CMD_ACTION_CALLBACK_FROMID(CMD_NEXT_MARKED) },

    { "HelpMenuAction", NULL, N_("_Help"), NULL, NULL, G_CALLBACK(NULL) },
    { "HelpCommandsAction", NULL, N_("_Commands"), NULL, NULL, CMD_ACTION_CALLBACK_FROMID(CMD_HELP) },
    { "HelpManualAllAboutAction", NULL, N_("_Manual (all about)"), NULL, NULL,
     CMD_ACTION_CALLBACK_FROMID(CMD_SHOW_MANUAL_ABOUT) },
    { "HelpManualWebAction", NULL, N_("Manual (_web)"), NULL, NULL, CMD_ACTION_CALLBACK_FROMID(CMD_SHOW_MANUAL_WEB) },
    { "HelpAboutGNUBGAction", GTK_STOCK_ABOUT, N_("_About GNU Backgammon"), NULL, NULL,
     CMD_ACTION_CALLBACK_FROMID(CMD_SHOW_VERSION) }
};

static GtkToggleActionEntry toggleActionEntries[] = {
    { "EditPositionAction", GTK_STOCK_EDIT, N_("_Edit Position"), NULL, N_("Toggle Edit Mode"), G_CALLBACK(ToggleEdit),
     FALSE },

    { "PanelGameRecordAction", NULL, N_("_Game record"), NULL, NULL, GENERIC_TOGGLE_CALLBACK_FROMID(TOGGLE_GAMELIST), FALSE },  /* TOGGLE */
    { "PanelAnalysisAction", NULL, N_("_Analysis"), NULL, NULL, GENERIC_TOGGLE_CALLBACK_FROMID(TOGGLE_ANALYSIS), FALSE },       /* TOGGLE */
    { "PanelCommentaryAction", NULL, N_("_Commentary"), NULL, NULL, GENERIC_TOGGLE_CALLBACK_FROMID(TOGGLE_COMMENTARY), FALSE }, /* TOGGLE */
    { "PanelMessageAction", NULL, N_("_Message"), NULL, NULL, GENERIC_TOGGLE_CALLBACK_FROMID(TOGGLE_MESSAGE), FALSE },  /* TOGGLE */
    { "PanelTheoryAction", NULL, N_("_Theory"), NULL, NULL, GENERIC_TOGGLE_CALLBACK_FROMID(TOGGLE_THEORY), FALSE },     /* TOGGLE */
    { "PanelCommandAction", NULL, N_("Command"), NULL, NULL, GENERIC_TOGGLE_CALLBACK_FROMID(TOGGLE_COMMAND), FALSE },   /* TOGGLE */

    { "DockPanelsAction", NULL, N_("_Dock panels"), NULL, NULL, G_CALLBACK(ToggleDockPanels), FALSE },
    { "ShowIDStatusBarAction", NULL, N_("Show _ID in status bar"), NULL, NULL, G_CALLBACK(ToggleShowingIDs), FALSE },   /* TOGGLE */
    { "FullScreenAction", NULL, N_("Full screen"), "F11", NULL, G_CALLBACK(DoFullScreenMode), FALSE },  /* TOGGLE */
    { "PlayClockwiseAction", NULL, N_("Play _Clockwise"), NULL, N_("Reverse direction of play"), G_CALLBACK(ToggleClockwise), FALSE }   /* TOGGLE */
};

static GtkRadioActionEntry toolbarRadioActionEntries[] = {
    { "TextOnlyToolBarAction", NULL, N_("_Text only"), NULL, NULL, VIEW_TOOLBAR_TEXTONLY },
    { "IconsOnlyToolBarAction", NULL, N_("_Icons only"), NULL, NULL, VIEW_TOOLBAR_ICONSONLY },
    { "BothToolBarAction", NULL, N_("_Both"), NULL, NULL, VIEW_TOOLBAR_BOTH }
};

static GtkRadioActionEntry setTurnRadioActionEntries[] = {
    { "SetTurnPlayer0Action", NULL, "0", NULL, NULL, CMD_SET_TURN_0 },
    { "SetTurnPlayer1Action", NULL, "1", NULL, NULL, CMD_SET_TURN_1 },
};

#else
static GtkItemFactoryEntry aife[] = {
    { N_("/_File"), NULL, NULL, 0, "<Branch>", NULL },
    { N_("/_File/_New..."), "<control>N", G_CALLBACK(NewClicked), 0,
     "<StockItem>", GTK_STOCK_NEW },
    { N_("/_File/_Open..."), "<control>O", G_CALLBACK(GTKOpen), 0,
     "<StockItem>", GTK_STOCK_OPEN },
    { N_("/_File/_Save..."), "<control>S", G_CALLBACK(GTKSave), 0,
     "<StockItem>", GTK_STOCK_SAVE },
    { N_("/_File/-"), NULL, NULL, 0, "<Separator>", NULL },
    { N_("/_File/Open _Commands..."), NULL, G_CALLBACK(GTKCommandsOpen), 0,
     NULL, NULL },
    { N_("/_File/-"), NULL, NULL, 0, "<Separator>", NULL },
    { N_("/_File/Match information..."), NULL, GTKMatchInfo, 0, NULL,
     NULL },
    { N_("/_File/-"), NULL, NULL, 0, "<Separator>", NULL },
    {
#if defined(WIN32)
     N_("/_File/E_xit"),
#else
     N_("/_File/_Quit"),
#endif
     "<control>Q", G_CALLBACK(Command), CMD_QUIT, "<StockItem>", GTK_STOCK_QUIT },
    { N_("/_Edit"), NULL, NULL, 0, "<Branch>", NULL },
    { N_("/_Edit/_Undo"), "<control>Z", GTKUndo, 0,
     "<StockItem>", GTK_STOCK_UNDO },
    { N_("/_Edit/-"), NULL, NULL, 0, "<Separator>", NULL },

    { N_("/_Edit/_Copy ID to Clipboard"), NULL, NULL, 0, "<Branch>", NULL },
    { N_("/_Edit/_Copy ID to Clipboard/GNUbg ID"), "<control>C", G_CALLBACK(CopyIDs), 0, NULL, NULL },
    { N_("/_Edit/_Copy ID to Clipboard/Match ID"), "<control>M", G_CALLBACK(CopyMatchID), 0, NULL, NULL },
    { N_("/_Edit/_Copy ID to Clipboard/Position ID"), "<control>P", G_CALLBACK(CopyPositionID), 0, NULL, NULL },

    { N_("/_Edit/Copy as"), NULL, NULL, 0, "<Branch>", NULL },
    { N_("/_Edit/Copy as/Position as ASCII"), NULL,
     G_CALLBACK(CommandCopy), 0, NULL, NULL },
    { N_("/_Edit/Copy as/BGonline (HTML)"), NULL,
     G_CALLBACK(CopyAsGOL), 0, NULL, NULL },

    { N_("/_Edit/_Paste ID"), "<control>V", PasteIDs, 0,
     "<StockItem>", GTK_STOCK_PASTE },

    { N_("/_Edit/-"), NULL, NULL, 0, "<Separator>", NULL },
    { N_("/_Edit/_Edit Position"), NULL, click_edit, 0,
     "<CheckItem>", NULL },

    { N_("/_View"), NULL, NULL, 0, "<Branch>", NULL },
    { N_("/_View/_Panels"), NULL, NULL, 0, "<Branch>", NULL },
    { N_("/_View/_Panels/_Game record"), NULL, G_CALLBACK(TogglePanel), TOGGLE_GAMELIST,
     "<CheckItem>", NULL },
    { N_("/_View/_Panels/_Analysis"), NULL, G_CALLBACK(TogglePanel), TOGGLE_ANALYSIS,
     "<CheckItem>", NULL },
    { N_("/_View/_Panels/_Commentary"), NULL, G_CALLBACK(TogglePanel), TOGGLE_COMMENTARY,
     "<CheckItem>", NULL },
    { N_("/_View/_Panels/_Message"), NULL, G_CALLBACK(TogglePanel), TOGGLE_MESSAGE,
     "<CheckItem>", NULL },
    { N_("/_View/_Panels/_Theory"), NULL, G_CALLBACK(TogglePanel), TOGGLE_THEORY,
     "<CheckItem>", NULL },
    { N_("/_View/_Panels/_Command"), NULL, G_CALLBACK(TogglePanel), TOGGLE_COMMAND,
     "<CheckItem>", NULL },
    { N_("/_View/_Dock panels"), NULL, G_CALLBACK(ToggleDockPanels), 0, "<CheckItem>", NULL },
    { N_("/_View/Restore panels"), NULL, G_CALLBACK(ShowAllPanels), 0, NULL, NULL },
    { N_("/_View/Hide panels"), NULL, G_CALLBACK(HideAllPanels), 0, NULL, NULL },
    { N_("/_View/-"), NULL, NULL, 0, "<Separator>", NULL },
    { N_("/_View/Show _ID in status bar"), NULL, G_CALLBACK(ToggleShowingIDs), 0, "<CheckItem>", NULL },
    { N_("/_View/_Toolbar"), NULL, NULL, 0, "<Branch>", NULL },
    { N_("/_View/_Toolbar/_Hide Toolbar"), NULL, HideToolbar, 0, NULL, NULL },
    { N_("/_View/_Toolbar/_Show Toolbar"), NULL, ShowToolbar, 0, NULL, NULL },
    { N_("/_View/_Toolbar/-"), NULL, NULL, 0, "<Separator>", NULL },
    { N_("/_View/_Toolbar/_Text only"), NULL, G_CALLBACK(ToolbarStyle), TOOLBAR_ACTION_OFFSET + GTK_TOOLBAR_TEXT,
     "<RadioItem>", NULL },
    { N_("/_View/_Toolbar/_Icons only"), NULL, G_CALLBACK(ToolbarStyle), TOOLBAR_ACTION_OFFSET + GTK_TOOLBAR_ICONS,
     "/View/Toolbar/Text only", NULL },
    { N_("/_View/_Toolbar/_Both"), NULL, G_CALLBACK(ToolbarStyle), TOOLBAR_ACTION_OFFSET + GTK_TOOLBAR_BOTH,
     "/View/Toolbar/Text only", NULL },
    { N_("/_View/Full screen"), "F11", G_CALLBACK(DoFullScreenMode), 0, "<CheckItem>", NULL },
    { N_("/_View/-"), NULL, NULL, 0, "<Separator>", NULL },
    { N_("/_View/Play _Clockwise"), NULL, click_swapdirection, 0, "<CheckItem>", NULL },
#if defined(USE_BOARD3D)
    { N_("/_View/-"), NULL, NULL, 0, "<Separator>", NULL },
    { N_("/_View/Switch to xD view"), NULL, G_CALLBACK(SwitchDisplayMode), TOOLBAR_ACTION_OFFSET + MENU_OFFSET, NULL,
     NULL },
#endif
    { N_("/_Game"), NULL, NULL, 0, "<Branch>", NULL },
    { N_("/_Game/_Roll"), "<control>R", G_CALLBACK(Command), CMD_ROLL, NULL, NULL },
    { N_("/_Game/_Finish move"), "<control>F", G_CALLBACK(FinishMove), 0, NULL, NULL },
    { N_("/_Game/-"), NULL, NULL, 0, "<Separator>", NULL },
    { N_("/_Game/_Double"), "<control>D", G_CALLBACK(Command), CMD_DOUBLE, "<StockItem>", GNUBG_STOCK_DOUBLE },
    { N_("/_Game/Re_sign"), NULL, G_CALLBACK(GTKResign), 0, "<StockItem>", GNUBG_STOCK_RESIGN },
    { N_("/_Game/-"), NULL, NULL, 0, "<Separator>", NULL },
    { N_("/_Game/_Accept"), NULL, G_CALLBACK(Command), CMD_ACCEPT, "<StockItem>", GNUBG_STOCK_ACCEPT },
    { N_("/_Game/Re_ject"), NULL, G_CALLBACK(Command), CMD_REJECT, "<StockItem>", GNUBG_STOCK_REJECT },
    { N_("/_Game/-"), NULL, NULL, 0, "<Separator>", NULL },
    { N_("/_Game/Play computer turn"), NULL, G_CALLBACK(Command), CMD_PLAY, NULL,
     NULL },
    { N_("/_Game/_End Game"), "<control>G", G_CALLBACK(Command), CMD_END_GAME, "<StockItem>", GNUBG_STOCK_END_GAME },
    { N_("/_Game/-"), NULL, NULL, 0, "<Separator>", NULL },
    { N_("/_Game/Swap players"), NULL, G_CALLBACK(Command), CMD_SWAP_PLAYERS, NULL,
     NULL },
    { N_("/_Game/-"), NULL, NULL, 0, "<Separator>", NULL },
    { N_("/_Game/Set cube..."), NULL, G_CALLBACK(GTKSetCube), 0, NULL, NULL },
    { N_("/_Game/Set _dice..."), NULL, G_CALLBACK(GTKSetDice), 0, NULL, NULL },
    { N_("/_Game/Set _turn"), NULL, NULL, 0, "<Branch>", NULL },
    { N_("/_Game/Set turn/0"),
     NULL, G_CALLBACK(Command), CMD_SET_TURN_0, "<RadioItem>", NULL },
    { N_("/_Game/Set turn/1"), NULL, G_CALLBACK(Command), CMD_SET_TURN_1,
     "/Game/Set turn/0", NULL },
    { N_("/_Game/Clear turn"), NULL, G_CALLBACK(Command), CMD_CLEAR_TURN, NULL, NULL },
    { N_("/_Analyse"), NULL, NULL, 0, "<Branch>", NULL },
    { N_("/_Analyse/_Evaluate"), "<control>E", G_CALLBACK(Command), CMD_EVAL, NULL, NULL },
    { N_("/_Analyse/_Hint"), "<control>H", G_CALLBACK(Command), CMD_HINT,
     "<StockItem>", GNUBG_STOCK_HINT },
    { N_("/_Analyse/_Rollout position"), NULL, G_CALLBACK(Command), CMD_ROLLOUT, NULL, NULL },
    { N_("/_Analyse/-"), NULL, NULL, 0, "<Separator>", NULL },
    { N_("/_Analyse/Analyse move"),
     NULL, G_CALLBACK(Command), CMD_ANALYSE_MOVE, NULL, NULL },
    { N_("/_Analyse/Analyse game"),
     NULL, G_CALLBACK(Command), CMD_ANALYSE_GAME, NULL, NULL },
    { N_("/_Analyse/Analyse match or session"),
     NULL, G_CALLBACK(Command), CMD_ANALYSE_MATCH, NULL, NULL },
    { N_("/_Analyse/-"), NULL, NULL, 0, "<Separator>", NULL },
    { N_("/_Analyse/Clear analysis"), NULL, NULL, 0, "<Branch>", NULL },
    { N_("/_Analyse/Clear analysis/Move"),
     NULL, G_CALLBACK(Command), CMD_ANALYSE_CLEAR_MOVE,
     "<StockItem>", GTK_STOCK_CLEAR },
    { N_("/_Analyse/Clear analysis/_Game"),
     NULL, G_CALLBACK(Command), CMD_ANALYSE_CLEAR_GAME,
     "<StockItem>", GTK_STOCK_CLEAR },
    { N_("/_Analyse/Clear analysis/_Match or session"),
     NULL, G_CALLBACK(Command), CMD_ANALYSE_CLEAR_MATCH,
     "<StockItem>", GTK_STOCK_CLEAR },
    { N_("/_Analyse/-"), NULL, NULL, 0, "<Separator>", NULL },
    { N_("/_Analyse/CMark"), NULL, NULL, 0, "<Branch>", NULL },
    { N_("/_Analyse/CMark/Cube"), NULL, NULL, 0, "<Branch>", NULL },
    { N_("/_Analyse/CMark/Cube/Clear"), NULL, G_CALLBACK(Command), CMD_CMARK_CUBE_CLEAR, NULL, NULL },
    { N_("/_Analyse/CMark/Cube/Show"), NULL, G_CALLBACK(Command), CMD_CMARK_CUBE_SHOW, NULL, NULL },
    { N_("/_Analyse/CMark/Move"), NULL, NULL, 0, "<Branch>", NULL },
    { N_("/_Analyse/CMark/Move/Clear"), NULL, G_CALLBACK(Command), CMD_CMARK_MOVE_CLEAR, NULL, NULL },
    { N_("/_Analyse/CMark/Move/Show"), NULL, G_CALLBACK(Command), CMD_CMARK_MOVE_SHOW, NULL, NULL },
    { N_("/_Analyse/CMark/Game"), NULL, NULL, 0, "<Branch>", NULL },
    { N_("/_Analyse/CMark/Game/Clear"), NULL, G_CALLBACK(Command), CMD_CMARK_GAME_CLEAR, NULL, NULL },
    { N_("/_Analyse/CMark/Game/Show"), NULL, G_CALLBACK(Command), CMD_CMARK_GAME_SHOW, NULL, NULL },
    { N_("/_Analyse/CMark/Match"), NULL, NULL, 0, "<Branch>", NULL },
    { N_("/_Analyse/CMark/Match/Clear"), NULL, G_CALLBACK(Command), CMD_CMARK_MATCH_CLEAR, NULL, NULL },
    { N_("/_Analyse/CMark/Match/Show"), NULL, G_CALLBACK(Command), CMD_CMARK_MATCH_SHOW, NULL, NULL },
    { N_("/_Analyse/-"), NULL, NULL, 0, "<Separator>", NULL },
    { N_("/_Analyse/Rollout"), NULL, NULL, 0, "<Branch>", NULL },
    { N_("/_Analyse/Rollout/Cube"), NULL, G_CALLBACK(Command), CMD_ANALYSE_ROLLOUT_CUBE, NULL, NULL },
    { N_("/_Analyse/Rollout/CMarked from Move"), NULL, G_CALLBACK(Command), CMD_ANALYSE_ROLLOUT_MOVE, NULL, NULL },
    { N_("/_Analyse/Rollout/CMarked from Game"), NULL, G_CALLBACK(Command), CMD_ANALYSE_ROLLOUT_GAME, NULL, NULL },
    { N_("/_Analyse/Rollout/CMarked from Match"), NULL, G_CALLBACK(Command), CMD_ANALYSE_ROLLOUT_MATCH, NULL, NULL },
    { N_("/_Analyse/-"), NULL, NULL, 0, "<Separator>", NULL },
    { N_("/_Analyse/Analyse File"), NULL, GTKAnalyzeFile, 0, NULL, NULL },
    { N_("/_Analyse/Batch analyse..."), NULL, G_CALLBACK(GTKBatchAnalyse), 0, NULL,
     NULL },
    { N_("/_Analyse/-"), NULL, NULL, 0, "<Separator>", NULL },
    { N_("/_Analyse/Match or session statistics"), NULL, G_CALLBACK(Command),
     CMD_SHOW_STATISTICS_MATCH, NULL, NULL },
    { N_("/_Analyse/-"), NULL, NULL, 0, "<Separator>", NULL },
    { N_("/_Analyse/Add match or session to database"), NULL,
     G_CALLBACK(GtkRelationalAddMatch), 0,
     "<StockItem>", GTK_STOCK_ADD },
    { N_("/_Analyse/Show Records"), NULL,
     G_CALLBACK(GtkShowRelational), 0, NULL, NULL },
    { N_("/_Analyse/Plot History"), NULL, G_CALLBACK(Command),
     CMD_SHOW_HISTORY, NULL, NULL },
    { N_("/_Analyse/-"), NULL, NULL, 0, "<Separator>", NULL },
    { N_("/_Analyse/Distribution of rolls"), NULL, G_CALLBACK(Command),
     CMD_SHOW_ROLLS, NULL, NULL },
    { N_("/_Analyse/Temperature Map"), NULL, G_CALLBACK(Command),
     CMD_SHOW_TEMPERATURE_MAP, NULL, NULL },
    { N_("/_Analyse/Temperature Map (cube decision)"), NULL, G_CALLBACK(Command),
     CMD_SHOW_TEMPERATURE_MAP_CUBE, NULL, NULL },
    { N_("/_Analyse/ScoreMap (move decision)"), NULL, G_CALLBACK(Command),
     CMD_SHOW_SCORE_MAP_MOVE, NULL, NULL },
    { N_("/_Analyse/ScoreMap (cube decision)"), NULL, G_CALLBACK(Command),
     CMD_SHOW_SCORE_MAP_CUBE, NULL, NULL },
    { N_("/_Analyse/-"), NULL, NULL, 0, "<Separator>", NULL },
    { N_("/_Analyse/_Race Theory"),
     NULL, G_CALLBACK(Command), CMD_SHOW_KLEINMAN, NULL, NULL },
    { N_("/_Analyse/-"), NULL, NULL, 0, "<Separator>", NULL },
    { N_("/_Analyse/_Market window"), NULL, G_CALLBACK(Command), CMD_SHOW_MARKETWINDOW,
     NULL, NULL },
    { N_("/_Analyse/M_atch equity table"), NULL, G_CALLBACK(Command),
     CMD_SHOW_MATCHEQUITYTABLE, NULL, NULL },
    { N_("/_Analyse/-"), NULL, NULL, 0, "<Separator>", NULL },
    { N_("/_Analyse/Evaluation speed"), NULL, G_CALLBACK(Command),
     CMD_SHOW_CALIBRATION, NULL, NULL },
    { N_("/_Settings"), NULL, NULL, 0, "<Branch>", NULL },
    { N_("/_Settings/_Analysis..."), NULL, G_CALLBACK(SetAnalysis), 0, NULL, NULL },
    { N_("/_Settings/_Board Appearance..."), NULL, G_CALLBACK(Command), CMD_SET_APPEARANCE,
     NULL, NULL },
    { N_("/_Settings/E_xport..."), NULL, G_CALLBACK(Command), CMD_SHOW_EXPORT,
     NULL, NULL },
    { N_("/_Settings/_Players..."), NULL, G_CALLBACK(SetPlayers), 0, NULL, NULL },
    { N_("/_Settings/_Rollouts..."), NULL, G_CALLBACK(SetRollouts), 0, NULL, NULL },
    { N_("/_Settings/-"), NULL, NULL, 0, "<Separator>", NULL },
    { N_("/_Settings/_Options..."), NULL, G_CALLBACK(SetOptions), 0, NULL, NULL },
    { N_("/_Settings/_Language..."), NULL, G_CALLBACK(SetLanguage), 0, NULL, NULL },
    { N_("/G_o"), NULL, NULL, 0, "<Branch>", NULL },
    { N_("/G_o/Previous marked move"), "<shift><control>Page_Up", G_CALLBACK(Command), CMD_PREV_MARKED, "<StockItem>",
     GNUBG_STOCK_GO_PREV_MARKED },
    { N_("/G_o/Previous cmarked move"), "<shift>Page_Up", G_CALLBACK(Command), CMD_PREV_CMARKED, "<StockItem>",
     GNUBG_STOCK_GO_PREV_CMARKED },
    { N_("/G_o/Previous rol_l"), "Page_Up", G_CALLBACK(Command), CMD_PREV_ROLL, "<StockItem>", GNUBG_STOCK_GO_PREV },
    { N_("/G_o/Pre_vious game"), "<control>Page_Up", G_CALLBACK(Command), CMD_PREV_GAME, "<StockItem>",
     GNUBG_STOCK_GO_PREV_GAME },
    { N_("/G_o/-"), NULL, NULL, 0, "<Separator>", NULL },
    { N_("/G_o/Next _game"), "<control>Page_Down", G_CALLBACK(Command), CMD_NEXT_GAME, "<StockItem>",
     GNUBG_STOCK_GO_NEXT_GAME },
    { N_("/G_o/Next _roll"), "Page_Down", G_CALLBACK(Command), CMD_NEXT_ROLL, "<StockItem>", GNUBG_STOCK_GO_NEXT },
    { N_("/G_o/Next cmarked move"), "<shift>Page_Down", G_CALLBACK(Command), CMD_NEXT_CMARKED, "<StockItem>",
     GNUBG_STOCK_GO_NEXT_CMARKED },
    { N_("/G_o/Next marked move"), "<shift><control>Page_Down", G_CALLBACK(Command), CMD_NEXT_MARKED, "<StockItem>",
     GNUBG_STOCK_GO_NEXT_MARKED },
    { N_("/_Help"), NULL, NULL, 0, "<Branch>", NULL },
    { N_("/_Help/_Commands"), NULL, G_CALLBACK(Command), CMD_HELP, NULL, NULL },
    { N_("/_Help/-"), NULL, NULL, 0, "<Separator>", NULL },
    { N_("/_Help/_Manual (all about)"), NULL, G_CALLBACK(Command),
     CMD_SHOW_MANUAL_ABOUT, NULL, NULL },
    { N_("/_Help/Manual (_web)"), NULL, G_CALLBACK(Command),
     CMD_SHOW_MANUAL_WEB, NULL, NULL },
    { N_("/_Help/-"), NULL, NULL, 0, "<Separator>", NULL },
    { N_("/_Help/_About GNU Backgammon"), NULL, G_CALLBACK(Command), CMD_SHOW_VERSION,
     "<StockItem>", GTK_STOCK_ABOUT }
};
#endif

static void
Stop(GtkWidget *pw, gpointer UNUSED(unused))
{
    if (automaticTask)
        StopAutomaticPlay();
    else if (!GTKShowWarning(WARN_STOP, pw))
        return;

    fInterrupt = TRUE;
#if defined(USE_BOARD3D)
    {
        BoardData *bd = BOARD(pwBoard)->board_data;
        if (display_is_3d(bd->rd)) {
            StopIdle3d(bd, bd->bd3d);
        }
    }
#endif
    gtk_statusbar_push(GTK_STATUSBAR(pwStatus), idOutput, _("Process interrupted"));
}

static gboolean
StopAnyAnimations(void)
{
#if defined(USE_BOARD3D)
    BoardData *bd = BOARD(pwBoard)->board_data;

    if (display_is_3d(bd->rd)) {
        if (Animating3d(bd->bd3d)) {
            StopIdle3d(bd, bd->bd3d);
            return TRUE;
        }
    } else
#endif
    if (!animation_finished) {
        fInterrupt = TRUE;
        return TRUE;
    }
    return FALSE;
}

static void
StopNotButton(GtkWidget * UNUSED(pw), gpointer UNUSED(unused))
{                               /* Interrupt any animations or show message in status bar */
    if (!StopAnyAnimations())
        gtk_statusbar_push(GTK_STATUSBAR(pwStatus), idOutput,
                           _("Press the stop button to interrupt the current process"));
}

static void
FileDragDropped(GtkWidget * UNUSED(widget), GdkDragContext * UNUSED(drag_context),
                gint UNUSED(x), gint UNUSED(y), GtkSelectionData * data, guint UNUSED(info), guint UNUSED(time))
{
    gchar **list;
    list = g_uri_list_extract_uris((const gchar *) gtk_selection_data_get_data(data));

    if (list[0]) {
        char *next, *file, *quoted;
        char *uri = (char *) list[0];
        if (StrNCaseCmp("file:", uri, 5) != 0) {
            outputerrf(_("Only local files supported in dnd"));
            g_strfreev(list);
            return;
        }

        file = g_filename_from_uri(uri, NULL, NULL);

        if (!file) {
            outputerrf(_("Failed to parse uri"));
            g_strfreev(list);
            return;
        }

        next = strchr(file, '\r');
        if (next)
            *next = 0;
        quoted = g_strdup_printf("\"%s\"", file);
        CommandImportAuto(quoted);
        g_free(quoted);
        g_free(file);
    }

    if (list)
        g_strfreev(list);
}


static gboolean
ContextMenu(GtkWidget * UNUSED(widget), GdkEventButton * event, GtkWidget * menu)
{
    if (event->type != GDK_BUTTON_PRESS || event->button != 3)
        return FALSE;

#if GTK_CHECK_VERSION(3,22,0)
    gtk_menu_popup_at_pointer(GTK_MENU(menu), (GdkEvent *)event);
#else
    gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, event->button, event->time);
#endif

    return TRUE;
}

static void
CreateMainWindow(void)
{
    GtkWidget *pwVbox, *pwHbox, *pwHbox2, *pwHandle, *pwPanelHbox, *pwStopButton, *idMenu, *menu_item, *pwFrame;
    GtkTargetEntry fileDrop = { "text/uri-list", GTK_TARGET_OTHER_APP, 1 };
#if !defined(USE_GTKITEMFACTORY)
    GError *error = NULL;
    GtkActionGroup *action_group;
#endif

    pwMain = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_maximize(GTK_WINDOW(pwMain));
    SetPanelWidget(WINDOW_MAIN, pwMain);
    gtk_window_set_role(GTK_WINDOW(pwMain), "main");
    gtk_window_set_type_hint(GTK_WINDOW(pwMain), GDK_WINDOW_TYPE_HINT_NORMAL);
    gtk_window_set_title(GTK_WINDOW(pwMain), _("GNU Backgammon"));
    /* Enable dropping of files on to main window */
    gtk_drag_dest_set(pwMain, GTK_DEST_DEFAULT_ALL, &fileDrop, 1, GDK_ACTION_DEFAULT);
    g_signal_connect(G_OBJECT(pwMain), "drag_data_received", G_CALLBACK(FileDragDropped), NULL);

#if GTK_CHECK_VERSION(3,0,0)
    pwVbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
    pwVbox = gtk_vbox_new(FALSE, 0);
#endif

    gtk_container_add(GTK_CONTAINER(pwMain), pwVbox);

#if !defined(USE_GTKITEMFACTORY)
    puim = gtk_ui_manager_new();

    action_group = gtk_action_group_new("Actions");
    gtk_action_group_set_translation_domain(action_group, GETTEXT_PACKAGE);
    gtk_action_group_add_actions(action_group, actionEntries, G_N_ELEMENTS(actionEntries), NULL);
    gtk_action_group_add_toggle_actions(action_group, toggleActionEntries, G_N_ELEMENTS(toggleActionEntries), NULL);
    gtk_action_group_add_radio_actions(action_group, toolbarRadioActionEntries, G_N_ELEMENTS(toolbarRadioActionEntries),
                                       0, GENERIC_RADIO_CALLBACK_FROMID(VIEW_TOOLBAR_ICONSONLY), NULL);
    gtk_action_group_add_radio_actions(action_group, setTurnRadioActionEntries, G_N_ELEMENTS(setTurnRadioActionEntries),
                                       0, CMD_RADIO_CALLBACK_FROMID(CMD_SET_TURN_0), NULL);
    gtk_ui_manager_insert_action_group(puim, action_group, 0);

    gtk_ui_manager_add_ui_from_string(puim, GNUBG_MAIN_UI, -1, &error);
    if (error) {
        g_warning(_("Cannot load UI: %s"), error->message);
        g_error_free(error);
        error = NULL;
    }
#if defined(USE_BOARD3D)
    gtk_ui_manager_add_ui_from_string(puim, UIADDITIONS3D, -1, &error);
    if (error) {
        g_warning(_("Cannot load UI: %s"), error->message);
        g_error_free(error);
        error = NULL;
    }
#endif
    /* Bind the accelerators */
    gtk_window_add_accel_group(GTK_WINDOW(pwMain), pagMain = gtk_ui_manager_get_accel_group(puim));


#else
    pagMain = gtk_accel_group_new();
    pif = gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<main>", pagMain);

    gtk_item_factory_set_translate_func(pif, GTKTranslate, NULL, NULL);

    gtk_item_factory_create_items(pif, sizeof(aife) / sizeof(aife[0]), aife, NULL);

    /* Tick default toolbar style */
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM
                                   (gtk_item_factory_get_widget_by_action(pif, nToolbarStyle + TOOLBAR_ACTION_OFFSET)),
                                   TRUE);
    gtk_window_add_accel_group(GTK_WINDOW(pwMain), pagMain);
#endif
#if GTK_CHECK_VERSION(3,0,0)
    pwHandle = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
    pwHandle = gtk_vbox_new(FALSE, 0);
#endif
    gtk_box_pack_start(GTK_BOX(pwVbox), pwHandle, FALSE, FALSE, 0);
#if !defined(USE_GTKITEMFACTORY)
    pwMenuBar = gtk_ui_manager_get_widget(puim, "/MainMenu");
    gtk_container_add(GTK_CONTAINER(pwHandle), pwMenuBar);
#else
    gtk_container_add(GTK_CONTAINER(pwHandle), pwMenuBar = gtk_item_factory_get_widget(pif, "<main>"));
#endif

#if GTK_CHECK_VERSION(3,0,0)
    pwHandle = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
    pwHandle = gtk_vbox_new(FALSE, 0);
#endif
    gtk_box_pack_start(GTK_BOX(pwVbox), pwHandle, FALSE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(pwHandle), pwToolbar = ToolbarNew());

#if GTK_CHECK_VERSION(3,0,0)
    pwGameBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
#else
    pwGameBox = gtk_hbox_new(FALSE, 0);
#endif
    gtk_box_pack_start(GTK_BOX(pwVbox), pwGameBox, TRUE, TRUE, 0);
#if GTK_CHECK_VERSION(3,0,0)
    hpaned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
#else
    hpaned = gtk_hpaned_new();
#endif
    gtk_box_pack_start(GTK_BOX(pwVbox), hpaned, TRUE, TRUE, 0);

#if GTK_CHECK_VERSION(3,0,0)
    pwPanelGameBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
#else
    pwPanelGameBox = gtk_hbox_new(FALSE, 0);
#endif
    gtk_paned_pack1(GTK_PANED(hpaned), pwPanelGameBox, TRUE, TRUE);
    gtk_container_add(GTK_CONTAINER(pwPanelGameBox), pwEventBox = gtk_event_box_new());
    gtk_event_box_set_visible_window(GTK_EVENT_BOX(pwEventBox), FALSE);

    gtk_container_add(GTK_CONTAINER(pwEventBox), pwBoard = board_new(GetMainAppearance(), FALSE));
    g_signal_connect(G_OBJECT(pwEventBox), "button-press-event", G_CALLBACK(board_button_press),
                     BOARD(pwBoard)->board_data);

#if GTK_CHECK_VERSION(3,0,0)
    pwPanelHbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
#else
    pwPanelHbox = gtk_hbox_new(FALSE, 0);
#endif
    gtk_paned_pack2(GTK_PANED(hpaned), pwPanelHbox, TRUE, FALSE);
#if GTK_CHECK_VERSION(3,0,0)
    pwPanelVbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 1);
#else
    pwPanelVbox = gtk_vbox_new(FALSE, 1);
#endif
    gtk_box_pack_start(GTK_BOX(pwPanelHbox), pwPanelVbox, TRUE, TRUE, 0);

    /* Do this so that the menu is packed now instead of in the idle loop */
#if !defined(USE_GTKITEMFACTORY)
    gtk_ui_manager_ensure_update(puim);
#endif

    DockPanels();

    /* Status bar */

#if GTK_CHECK_VERSION(3,0,0)
    pwHbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
#else
    pwHbox = gtk_hbox_new(FALSE, 0);
#endif
    gtk_box_pack_end(GTK_BOX(pwVbox), pwHbox, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(pwHbox), pwStatus = gtk_statusbar_new(), TRUE, TRUE, 0);

    gtk_statusbar_set_has_resize_grip(GTK_STATUSBAR(pwStatus), FALSE);
    /* It's a bit naughty to access pwStatus->label, but its default alignment
     * is ugly, and GTK gives us no other way to change it. */
#if GTK_CHECK_VERSION(3,0,0)
    gtk_widget_set_halign(get_statusbar_label(GTK_STATUSBAR(pwStatus)), GTK_ALIGN_START);
    gtk_widget_set_valign(get_statusbar_label(GTK_STATUSBAR(pwStatus)), GTK_ALIGN_CENTER);
#else
    gtk_misc_set_alignment(GTK_MISC(get_statusbar_label(GTK_STATUSBAR(pwStatus))), 0, 0.5);
#endif

    idOutput = gtk_statusbar_get_context_id(GTK_STATUSBAR(pwStatus), "gnubg output");
    idProgress = gtk_statusbar_get_context_id(GTK_STATUSBAR(pwStatus), "progress");
    g_signal_connect(G_OBJECT(pwStatus), "text-popped", G_CALLBACK(TextPopped), NULL);

    idMenu = gtk_menu_new();

    menu_item = gtk_menu_item_new_with_label(_("Copy GNUbg ID"));
    gtk_menu_shell_append(GTK_MENU_SHELL(idMenu), menu_item);
    gtk_widget_show(menu_item);
    g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(CopyIDs), NULL);

    menu_item = gtk_menu_item_new_with_label(_("Paste GNUbg ID"));
    gtk_menu_shell_append(GTK_MENU_SHELL(idMenu), menu_item);
    gtk_widget_show(menu_item);
    g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(PasteIDs), NULL);

    pwIDBox = gtk_event_box_new();
    gtk_box_pack_start(GTK_BOX(pwHbox), pwIDBox, FALSE, FALSE, 0);

#if GTK_CHECK_VERSION(3,0,0)
    pwHbox2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
#else
    pwHbox2 = gtk_hbox_new(FALSE, 0);
#endif
    gtk_container_add(GTK_CONTAINER(pwIDBox), pwHbox2);

    gtk_box_pack_start(GTK_BOX(pwHbox2), gtk_label_new("GNUbg ID:"), FALSE, FALSE, 0);
    pwFrame = gtk_frame_new(NULL);
    gtk_box_pack_start(GTK_BOX(pwHbox2), pwFrame, FALSE, FALSE, 0);

    pwGnubgID = gtk_label_new("");
    gtk_container_add(GTK_CONTAINER(pwFrame), pwGnubgID);
    gtk_container_set_border_width(GTK_CONTAINER(pwFrame), 2);

    gtk_widget_set_tooltip_text(pwIDBox, _("This is a unique id for this position."
                                           " Ctrl+C copies the current ID and Ctrl+V pastes an ID from the clipboard."));
    g_signal_connect(G_OBJECT(pwIDBox), "button-press-event", G_CALLBACK(ContextMenu), idMenu);

    pwStop = gtk_event_box_new();
    pwStopButton = gtk_event_box_new();
    gtk_container_add(GTK_CONTAINER(pwStop), pwStopButton);
    gtk_container_add(GTK_CONTAINER(pwStopButton),
                      gtk_image_new_from_icon_name("process-stop", GTK_ICON_SIZE_SMALL_TOOLBAR));
    gtk_box_pack_start(GTK_BOX(pwHbox), pwStop, FALSE, FALSE, 2);
    g_signal_connect(G_OBJECT(pwStop), "button-press-event", G_CALLBACK(StopNotButton), NULL);
    g_signal_connect(G_OBJECT(pwStopButton), "button-press-event", G_CALLBACK(Stop), NULL);
    gtk_widget_set_sensitive(pwStop, FALSE);
    pwGrab = pwStop;

    gtk_box_pack_start(GTK_BOX(pwHbox), pwProgress = gtk_progress_bar_new(), FALSE, FALSE, 0);
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(pwProgress), 0.0);
#if GTK_CHECK_VERSION(3,0,0)
    gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(pwProgress), TRUE);
#endif
    /* This is a kludge to work around an ugly bug in GTK: we don't want to
     * show text in the progress bar yet, but we might later.  So we have to
     * pretend we want text in order to be sized correctly, and then set the
     * format string to something so we don't get the default text. */
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(pwProgress), " ");

    g_signal_connect(G_OBJECT(pwMain), "configure_event", G_CALLBACK(configure_event), NULL);
#if ! GTK_CHECK_VERSION(3,0,0)
    g_signal_connect(G_OBJECT(pwMain), "size-request", G_CALLBACK(MainSize), NULL);
#endif
    g_signal_connect(G_OBJECT(pwMain), "delete_event", G_CALLBACK(main_delete), NULL);
    g_signal_connect(G_OBJECT(pwMain), "destroy", G_CALLBACK(gtk_main_quit), NULL);
}

#if !defined(WIN32)
/* g_object_unref() with suitable signature to pass it to g_list_foreach() */
static inline void
my_g_object_unref(gpointer data, gpointer UNUSED(user_data))
{
    g_object_unref(data);
}
#endif

static void
gnubg_set_default_icon(void)
{
    /* win32 uses the ico file for this */
    /* adapted from pidgin */
#if !defined(WIN32)
    GList *icons = NULL;
    GdkPixbuf *icon = NULL;
    struct {
        const char *dir;
        const char *fn;
    } is[] = {
        {
        "16x16", "gnubg.png"}, {
        "24x24", "gnubg.png"}, {
        "32x32", "gnubg.png"}, {
        "48x48", "gnubg.png"}
    };

    for (guint i = 0; i < G_N_ELEMENTS(is); i++) {
        char *ip = g_build_filename(getDataDir(), "icons", "hicolor", is[i].dir, "apps", is[i].fn, NULL);
        icon = gdk_pixbuf_new_from_file(ip, NULL);
        g_free(ip);
        if (icon)
            icons = g_list_append(icons, icon);
        /* fail silently */

    }
    gtk_window_set_default_icon_list(icons);

    g_list_foreach(icons, my_g_object_unref, NULL);
    g_list_free(icons);
#endif
}

#if GTK_CHECK_VERSION(3,0,0)
static void
ApplyDefaultCss(void)
{
    GtkCssProvider *cssProvider;
    char *cssPath;

    cssProvider = gtk_css_provider_new();
    if (cssProvider == NULL)
        return;

    cssPath = BuildFilename("gnubg.css");
    gtk_css_provider_load_from_path(cssProvider, cssPath, NULL);
    gtk_style_context_add_provider_for_screen(gtk_window_get_screen(GTK_WINDOW(pwMain)),
                                              GTK_STYLE_PROVIDER(cssProvider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    g_free(cssPath);
    g_object_unref(G_OBJECT(cssProvider));
}
#endif

extern void
InitGTK(int *argc, char ***argv)
{
    char *sz;
    GtkIconFactory *picf;
    GdkAtom cb;

#if ! GTK_CHECK_VERSION(3,0,0)
    sz = BuildFilename("gnubg.gtkrc");
    gtk_rc_add_default_file(sz);
    g_free(sz);

    sz = g_build_filename(szHomeDirectory, "gnubg.gtkrc", NULL);
    gtk_rc_add_default_file(sz);
    g_free(sz);
#endif

    sz = g_build_filename(szHomeDirectory, "gnubgmenurc", NULL);
    gtk_accel_map_load(sz);
    g_free(sz);

    fX = gtk_init_check(argc, argv);
    if (!fX)
        return;

    gnubg_stock_init();

#if defined(USE_BOARD3D)
    widget3dValid = InitGTK3d(argc, argv);
#endif

    /*add two xpm based icons */
    picf = gtk_icon_factory_new();
    gtk_icon_factory_add_default(picf);

    gnubg_set_default_icon();

    CreateMainWindow();
#if GTK_CHECK_VERSION(3,0,0)
    ApplyDefaultCss();
#endif

    /*Create string for handling messages from output* functions */
    output_str = g_string_new(NULL);

    cb = gdk_atom_intern("CLIPBOARD", TRUE);
    clipboard = gtk_clipboard_get(cb);
}

enum { RE_NONE, RE_LANGUAGE_CHANGE };

static int reasonExited;

extern void
RunGTK(GtkWidget * pwSplash, char *commands, char *python_script, char *match)
{
#if defined(USE_BOARD3D)
    /* Use 1st predefined board settings if none have been loaded */
    Default3dSettings(BOARD(pwBoard)->board_data);
#endif

    do {
        reasonExited = RE_NONE;

        GTKSet(&ms.fCubeOwner);
        GTKSet(&ms.nCube);
        GTKSet(ap);
        GTKSet(&ms.fTurn);
        GTKSet(&ms.gs);
        GTKSet(&ms.fJacoby);

        PushSplash(pwSplash, _("Rendering"), _("Board"));

        GTKAllowStdin();

        if (fTTY) {
#if defined(HAVE_LIB_READLINE)
            fReadingCommand = TRUE;
            rl_callback_handler_install(FormatPrompt(), ProcessInput);
            atexit(rl_callback_handler_remove);
#else
            Prompt();
#endif
        }

        /* Show everything */
        gtk_widget_show_all(pwMain);

        GTKSet(&fShowIDs);

        /* Set the default arrow cursor in the stop window so obvious it can be clicked */
        gdk_window_set_cursor(gtk_widget_get_window(pwStop),
                              gdk_cursor_new_for_display(gtk_widget_get_display(pwStop), GDK_LEFT_PTR));

        /* Make sure toolbar looks correct */
        {
            int style = nToolbarStyle;
            nToolbarStyle = 2;  /* Default style is fine */
            SetToolbarStyle(style);
#if !defined(USE_GTKITEMFACTORY)
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_ui_manager_get_widget(puim,
                                                                                         "/MainMenu/ViewMenu/ToolBarMenu/Both")),
                                           nToolbarStyle);
#endif

        }

#if defined(USE_BOARD3D)
        {
            BoardData *bd = BOARD(pwBoard)->board_data;
            BoardData3d *bd3d = bd->bd3d;
            renderdata *prd = bd->rd;

            SetSwitchModeMenuText();
            DisplayCorrectBoardType(bd, bd3d, prd);
        }
#endif

        DestroySplash(pwSplash);
        pwSplash = NULL;

        /* Display any other windows now */
        DisplayWindows();

        /* Make sure some things stay hidden */
        if (!ArePanelsDocked()) {
            gtk_widget_hide(hpaned);
#if !defined(USE_GTKITEMFACTORY)
            gtk_widget_hide((gtk_ui_manager_get_widget(puim, "/MainMenu/ViewMenu/PanelsMenu/Commentary")));
            gtk_widget_hide((gtk_ui_manager_get_widget(puim, "/MainMenu/ViewMenu/HidePanels")));
            gtk_widget_hide((gtk_ui_manager_get_widget(puim, "/MainMenu/ViewMenu/RestorePanels")));
#else
            gtk_widget_hide(gtk_item_factory_get_widget(pif, "/View/Panels/Commentary"));
            gtk_widget_hide(gtk_item_factory_get_widget(pif, "/View/Hide panels"));
            gtk_widget_hide(gtk_item_factory_get_widget(pif, "/View/Restore panels"));
#endif
        } else {
            if (ArePanelsShowing()) {
#if !defined(USE_GTKITEMFACTORY)
                gtk_widget_hide((gtk_ui_manager_get_widget(puim, "/MainMenu/ViewMenu/RestorePanels")));
#else
                gtk_widget_hide(gtk_item_factory_get_widget(pif, "/View/Restore panels"));
#endif
                gtk_widget_hide(pwGameBox);
            } else
#if !defined(USE_GTKITEMFACTORY)
                gtk_widget_hide((gtk_ui_manager_get_widget(puim, "/MainMenu/ViewMenu/HidePanels")));
#else
                gtk_widget_hide(gtk_item_factory_get_widget(pif, "/View/Hide panels"));
#endif
        }

        /* Make sure main window is on top */
        gdk_window_raise(gtk_widget_get_window(pwMain));

        /* force update of board; needed to display board correctly if user
         * has special settings, e.g., clockwise or nackgammon */
        ShowBoard();

        if (fToolbarShowing)
#if !defined(USE_GTKITEMFACTORY)
            gtk_widget_hide((gtk_ui_manager_get_widget(puim, "/MainMenu/ViewMenu/ToolBarMenu/ShowToolBar")));
#else
            gtk_widget_hide(gtk_item_factory_get_widget(pif, "/View/Toolbar/Show Toolbar"));
#endif

        if (fFullScreen) {      /* Change to full screen (but hide warning) */
            fullScreenOnStartup = TRUE;
            FullScreenMode(TRUE);
        } else if (!fToolbarShowing)
            HideToolbar();

        if (match) {
            CommandImportAuto(match);
            g_free(match);
            match = NULL;
        }

        if (commands) {
            CommandLoadCommands(commands);
            g_free(commands);
            commands = NULL;
        }

        if (python_script) {
#if defined(WIN32)
            outputerrf(_
                       ("The Windows GTK interface does not support the '-p' option. Use the command-line interface instead."));
#else
#if defined(USE_PYTHON)
            g_idle_add(python_run_file, g_strdup(python_script));
#endif
#endif
            g_free(python_script);
            python_script = NULL;
        }

        /* initialize by saying there is no analysis running now in the background*/
        if(fBackgroundAnalysis)
             fAnalysisRunning = FALSE;

        gtk_main();

        if (reasonExited == RE_LANGUAGE_CHANGE) {       /* Recreate main window with new language */
            CreateMainWindow();
            setWindowGeometry(WINDOW_MAIN);
        }
    } while (reasonExited != RE_NONE);
}

extern void
GtkChangeLanguage(void)
{
    setlocale(LC_ALL, "");
    if (pwMain && gtk_widget_get_realized(pwMain)) {
        reasonExited = RE_LANGUAGE_CHANGE;
        custom_cell_renderer_invalidate_size(); /* Recalculate widget sizes */
        ClosePanels();
        getWindowGeometry(WINDOW_MAIN);
        DestroyPanel(WINDOW_MAIN);
        GTKGameSelectDestroy();
        pwProgress = NULL;

    }
}

extern void
ShowList(char *psz[], const char *szTitle, GtkWidget * parent)
{

    GString *gst = g_string_new(NULL);
    while (*psz)
        g_string_append_printf(gst, "%s\n", *psz++);
    GTKTextWindow(gst->str, szTitle, DT_INFO, parent);
    g_string_free(gst, TRUE);
}

extern void
OK(GtkWidget * pw, int *pf)
{

    if (pf)
        *pf = TRUE;

    gtk_widget_destroy(gtk_widget_get_toplevel(pw));
}

/*
 * put up a tutor window with a message about how bad the intended
 * play is. Allow selections:
 * Continue (play it anyway)
 * Cancel   (rethink)
 * returns TRUE if play it anyway
 */

static void
TutorEnd(GtkWidget * pw, int *pf)
{
    if (pf)
        *pf = TRUE;

    fTutor = FALSE;
    gtk_widget_destroy(gtk_widget_get_toplevel(pw));
}

static void
TutorHint(GtkWidget * pw, void *UNUSED(unused))
{
    gtk_widget_destroy(gtk_widget_get_toplevel(pw));
    UserCommand("hint");
}

static void
TutorRethink(GtkWidget * pw, void *UNUSED(unused))
{
    gtk_widget_destroy(gtk_widget_get_toplevel(pw));
    ShowBoard();
}

extern int
GtkTutor(char *sz)
{
    int f = FALSE;
    GtkWidget *pwTutorDialog, *pwOK, *pwCancel, *pwEndTutor, *pwButtons, *pwPrompt, *pwHint;

    pwTutorDialog = GTKCreateDialog(_("GNU Backgammon - Tutor"),
                                    DT_CUSTOM, NULL, DIALOG_FLAG_MODAL, G_CALLBACK(OK), (void *) &f);

    pwOK = DialogArea(pwTutorDialog, DA_OK);
    gtk_button_set_label(GTK_BUTTON(pwOK), _("Play Anyway"));

    pwCancel = gtk_button_new_with_label(_("Rethink"));
    pwEndTutor = gtk_button_new_with_label(_("End Tutor Mode"));
    pwHint = gtk_button_new_with_label(_("Hint"));
    pwButtons = DialogArea(pwTutorDialog, DA_BUTTONS);

    gtk_container_add(GTK_CONTAINER(pwButtons), pwCancel);
    g_signal_connect(G_OBJECT(pwCancel), "clicked", G_CALLBACK(TutorRethink), (void *) &f);

    gtk_container_add(GTK_CONTAINER(pwButtons), pwEndTutor);
    g_signal_connect(G_OBJECT(pwEndTutor), "clicked", G_CALLBACK(TutorEnd), (void *) &f);

    gtk_container_add(GTK_CONTAINER(pwButtons), pwHint);
    g_signal_connect(G_OBJECT(pwHint), "clicked", G_CALLBACK(TutorHint), (void *) &f);

    pwPrompt = gtk_label_new(sz);

#if GTK_CHECK_VERSION(3,0,0)
    g_object_set(pwPrompt, "margin", 8, NULL);
#else
    gtk_misc_set_padding(GTK_MISC(pwPrompt), 8, 8);
#endif
    gtk_label_set_justify(GTK_LABEL(pwPrompt), GTK_JUSTIFY_LEFT);
    gtk_label_set_line_wrap(GTK_LABEL(pwPrompt), TRUE);
    gtk_container_add(GTK_CONTAINER(DialogArea(pwTutorDialog, DA_MAIN)), pwPrompt);

    gtk_window_set_resizable(GTK_WINDOW(pwTutorDialog), FALSE);

    /* This dialog should be REALLY modal -- disable "next turn" idle
     * processing and stdin handler, to avoid reentrancy problems. */
    if (nNextTurn)
        g_source_remove(nNextTurn);

    GTKRunDialog(pwTutorDialog);

    if (nNextTurn)
        nNextTurn = g_idle_add(NextTurnNotify, NULL);

    /* if tutor mode was disabled, update the checklist */
    if (!fTutor) {
        GTKSet((void *) &fTutor);
    }

    return f;
}

extern void
GTKOutput(const char *sz)
{
    if (!sz || !*sz)
        return;
    g_string_append(output_str, sz);
}

extern void
GTKOutputX(void)
{
    gchar *str;
    if (output_str->len == 0)
        return;
    str = g_strchomp(output_str->str);
    if (PanelShowing(WINDOW_MESSAGE)) {
        GtkTextBuffer *buffer;
        GtkTextIter iter;
        buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(pwMessageText));
        gtk_text_buffer_get_end_iter(buffer, &iter);
        gtk_text_buffer_insert(buffer, &iter, "\n", -1);
        gtk_text_buffer_insert(buffer, &iter, g_strchomp(str), -1);
        gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(pwMessageText),
                                     gtk_text_buffer_create_mark(buffer, "last", &iter, FALSE), 0.0, TRUE, 0.0, 1.0);
    } else if (output_str->len > 80 || strchr(str, '\n')) {
        GTKMessage(str, DT_INFO);
    } else {
        gtk_statusbar_push(GTK_STATUSBAR(pwStatus), idOutput, str);
    }
    g_string_set_size(output_str, 0);
}

extern void
GTKOutputErr(const char *sz)
{
    GtkTextIter iter;
    GTKMessage(sz, DT_ERROR);

    if (PanelShowing(WINDOW_MESSAGE)) {
        GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(pwMessageText));
        gtk_text_buffer_get_end_iter(buffer, &iter);
        gtk_text_buffer_insert(buffer, &iter, sz, -1);
        gtk_text_buffer_insert(buffer, &iter, "\n", -1);
        gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(pwMessageText), &iter, 0.0, FALSE, 0.0, 1.0);
    }
}

extern void
GTKOutputNew(void)
{
    /* This is horribly ugly, but fFinishedPopping will never be set if
     * the progress bar leaves a message in the status stack.  There should
     * be at most one message, so we get rid of it here. */
    gtk_statusbar_pop(GTK_STATUSBAR(pwStatus), idProgress);

    fFinishedPopping = FALSE;

    do
        gtk_statusbar_pop(GTK_STATUSBAR(pwStatus), idOutput);
    while (!fFinishedPopping);
}

typedef struct {
    GtkWidget *pwCPS, *pwML, *pwGNUvsHuman, *pwHumanHuman, *pwManualDice, *pwTutorMode;
} newwidget;

static GtkWidget *
button_from_image(GtkWidget * pwImage)
{

    GtkWidget *pw = gtk_button_new();

    gtk_container_add(GTK_CONTAINER(pw), pwImage);

    return pw;

}

static void
UpdatePlayerSettings(newwidget * pnw)
{

    int fManDice = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pnw->pwManualDice));
    int fTM = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pnw->pwTutorMode));
    int fCPS = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pnw->pwCPS));
    int fGH = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pnw->pwGNUvsHuman));
    int fHH = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pnw->pwHumanHuman));

    if (!fCPS) {
        if (fGH) {
            UserCommand("set player 0 gnubg");
            UserCommand("set player 1 human");
        }
        if (fHH) {
            UserCommand("set player 0 human");
            UserCommand("set player 1 human");
        }
    }

    /* apply the default names chosen by the user, not the names used in the random previous match */
    strcpy(ap[0].szName, default_names[0]);
    strcpy(ap[1].szName, default_names[1]);

    if ((fManDice) && (rngCurrent != RNG_MANUAL))
        UserCommand("set rng manual");

    if ((!fManDice) && (rngCurrent == RNG_MANUAL))
        UserCommand("set rng mersenne");

    if ((fTM) && (!fTutor))
        UserCommand("set tutor mode on");

    if ((!fTM) && (fTutor))
        UserCommand("set tutor mode off");

    UserCommand("save settings");
}

static void
SettingsPressed(GtkWidget * pw, gpointer UNUSED(data))
{
    GTKSetCurrentParent(pw);
    SetPlayers(NULL, 0, NULL);
}

static void
ToolButtonPressedMS(GtkWidget * pw, newwidget * pnw)
{
    UpdatePlayerSettings(pnw);
    gtk_widget_destroy(gtk_widget_get_toplevel(pw));
    UserCommand("new session");
}

static void
ToolButtonPressed(GtkWidget * pw, newwidget * pnw)
{
    char sz[40];
    int *pi;

    pi = (int *) g_object_get_data(G_OBJECT(pw), "user_data");
    sprintf(sz, "new match %d", *pi);
    UpdatePlayerSettings(pnw);
    gtk_widget_destroy(gtk_widget_get_toplevel(pw));
    UserCommand(sz);
}

extern int
edit_new(unsigned int length)
{
    matchstate lms;

    lms.anDice[0] = ms.anDice[0];
    lms.anDice[1] = ms.anDice[1];

    lms.fTurn = lms.fMove = 1;
    lms.fResigned = 0;
    lms.fDoubled = 0;
    if (ms.fCubeOwner == -1)
        lms.fCubeOwner = ms.fCubeOwner;
    else
        lms.fCubeOwner = (ms.fTurn == 1 ? ms.fCubeOwner : !ms.fCubeOwner);
    lms.fCrawford = FALSE;
    lms.fJacoby = fJacoby;
    lms.anScore[0] = lms.anScore[1] = 0;
    lms.nCube = ms.nCube;
    lms.gs = GAME_PLAYING;

    lms.nMatchTo = length;

    CommandSetMatchID(MatchIDFromMatchState(&lms));

    return 0;
}

static void
edit_new_clicked(GtkWidget * pw, newwidget * pnw)
{
    unsigned int length = (unsigned int) gtk_spin_button_get_value(GTK_SPIN_BUTTON(pnw->pwML));
    gtk_widget_destroy(gtk_widget_get_toplevel(pw));
    edit_new(length);
    if (!ToolbarIsEditing(NULL))
        click_edit();
}

static GtkWidget *
NewWidget(newwidget * pnw)
{
    int i, j = 1;
    GtkWidget *pwVbox, *pwHbox, *pwLabel, *pwToolbar2;
    GtkWidget *pwButtons, *pwFrame, *pwVbox2;
    GtkToolItem *pwToolButton;
#if GTK_CHECK_VERSION(3,0,0)
    pwVbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
    pwVbox = gtk_vbox_new(FALSE, 0);
#endif
    pwToolbar2 = gtk_toolbar_new();

    gtk_orientable_set_orientation(GTK_ORIENTABLE(pwToolbar2), GTK_ORIENTATION_HORIZONTAL);
    gtk_toolbar_set_style(GTK_TOOLBAR(pwToolbar2), GTK_TOOLBAR_ICONS);
    gtk_toolbar_set_show_arrow(GTK_TOOLBAR(pwToolbar2), FALSE);
    pwFrame = gtk_frame_new(_("Shortcut buttons"));
    gtk_box_pack_start(GTK_BOX(pwVbox), pwFrame, TRUE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(pwFrame), pwToolbar2);
    gtk_container_set_border_width(GTK_CONTAINER(pwToolbar2), 4);

    /* Edit button */

    pwToolButton = gtk_tool_button_new_from_stock(GTK_STOCK_EDIT);
    gtk_widget_set_tooltip_text(GTK_WIDGET(pwToolButton), _("Edit position"));
    gtk_toolbar_insert(GTK_TOOLBAR(pwToolbar2), pwToolButton, -1);
    g_signal_connect(pwToolButton, "clicked", G_CALLBACK(edit_new_clicked), pnw);

    gtk_toolbar_insert(GTK_TOOLBAR(pwToolbar2), gtk_separator_tool_item_new(), -1);

    pwToolButton = gtk_tool_button_new_from_stock(GNUBG_STOCK_NEW0);
    gtk_widget_set_tooltip_text(GTK_WIDGET(pwToolButton), _("Start a new money game session"));
    gtk_toolbar_insert(GTK_TOOLBAR(pwToolbar2), pwToolButton, -1);
    g_signal_connect(G_OBJECT(pwToolButton), "clicked", G_CALLBACK(ToolButtonPressedMS), pnw);

    gtk_toolbar_insert(GTK_TOOLBAR(pwToolbar2), gtk_separator_tool_item_new(), -1);

    for (i = 1; i < 19; i = i + 2, j++) {
        gchar *sz;
        gchar stock[50];
        int *pi;

        sz = g_strdup_printf(ngettext("Start a new %d point match", "Start a new %d points match", i), i);
        sprintf(stock, "gnubg-stock-new%d", i);
        pwToolButton = gtk_tool_button_new_from_stock(stock);
        gtk_widget_set_tooltip_text(GTK_WIDGET(pwToolButton), sz);
        gtk_toolbar_insert(GTK_TOOLBAR(pwToolbar2), pwToolButton, -1);
        gtk_tool_item_set_homogeneous(pwToolButton, FALSE);

        g_free(sz);

        pi = g_malloc(sizeof(int));
        *pi = i;
        g_object_set_data_full(G_OBJECT(pwToolButton), "user_data", pi, g_free);

        g_signal_connect(G_OBJECT(pwToolButton), "clicked", G_CALLBACK(ToolButtonPressed), pnw);
    }

    pwFrame = gtk_frame_new(_("Match settings"));

#if GTK_CHECK_VERSION(3,0,0)
    pwHbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
#else
    pwHbox = gtk_hbox_new(FALSE, 0);
#endif

    pwLabel = gtk_label_new(_("Length:"));
    gtk_label_set_justify(GTK_LABEL(pwLabel), GTK_JUSTIFY_RIGHT);
    pnw->pwML = gtk_spin_button_new_with_range(0, MAXSCORE, 1);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(pnw->pwML), TRUE);

    gtk_box_pack_start(GTK_BOX(pwHbox), pwLabel, FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(pwHbox), pnw->pwML, FALSE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(pwFrame), pwHbox);
    gtk_container_add(GTK_CONTAINER(pwVbox), pwFrame);

    /* Here the simplified player settings starts */

    pwFrame = gtk_frame_new(_("Player settings"));

#if GTK_CHECK_VERSION(3,0,0)
    pwHbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    pwVbox2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
    pwHbox = gtk_hbox_new(FALSE, 0);
    pwVbox2 = gtk_vbox_new(FALSE, 0);
#endif

    gtk_container_add(GTK_CONTAINER(pwHbox), pwVbox2);

    pnw->pwCPS = gtk_radio_button_new_with_label(NULL, _("Current player settings"));
    gtk_box_pack_start(GTK_BOX(pwVbox2), pnw->pwCPS, FALSE, FALSE, 0);

    pnw->pwGNUvsHuman =
        gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(pnw->pwCPS), _("GNU Backgammon vs. Human"));
    pnw->pwHumanHuman = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(pnw->pwCPS), _("Human vs. Human"));
    gtk_box_pack_start(GTK_BOX(pwVbox2), pnw->pwGNUvsHuman, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(pwVbox2), pnw->pwHumanHuman, FALSE, FALSE, 0);

    pwButtons = gtk_button_new_with_label(_("Modify player settings..."));
    gtk_container_set_border_width(GTK_CONTAINER(pwButtons), 10);

    gtk_container_add(GTK_CONTAINER(pwVbox2), pwButtons);

    g_signal_connect(G_OBJECT(pwButtons), "clicked", G_CALLBACK(SettingsPressed), NULL);

#if GTK_CHECK_VERSION(3,0,0)
    pwVbox2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
    pwVbox2 = gtk_vbox_new(FALSE, 0);
#endif

    gtk_container_add(GTK_CONTAINER(pwHbox), pwVbox2);

    pnw->pwManualDice = gtk_check_button_new_with_label(_("Manual dice"));
    pnw->pwTutorMode = gtk_check_button_new_with_label(_("Tutor mode"));

    gtk_box_pack_start(GTK_BOX(pwVbox2), pnw->pwManualDice, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(pwVbox2), pnw->pwTutorMode, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(pwFrame), pwHbox);
    gtk_container_add(GTK_CONTAINER(pwVbox), pwFrame);

    return pwVbox;
}

static void
NewOK(GtkWidget * pw, newwidget * pnw)
{
    char sz[40];
    unsigned int Mlength = (unsigned int) gtk_spin_button_get_value(GTK_SPIN_BUTTON(pnw->pwML));

    UpdatePlayerSettings(pnw);

    gtk_widget_destroy(gtk_widget_get_toplevel(pw));

    if (ToolbarIsEditing(NULL))
        click_edit();           /* Come out of editing mode */

    sprintf(sz, "new match %u", Mlength);
    UserCommand(sz);

    GL_SetNames();
}

static void
NewSet(newwidget * pnw)
{
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pnw->pwTutorMode), fTutor);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pnw->pwManualDice), rngCurrent == RNG_MANUAL);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(pnw->pwML), nDefaultLength);
}

extern void
GTKNew(void)
{
    GtkWidget *pwDialog;
    newwidget nw;
    GtkAllocation allocation;

    pwDialog = GTKCreateDialog(_("GNU Backgammon - New"), DT_QUESTION, NULL, DIALOG_FLAG_MODAL, G_CALLBACK(NewOK), &nw);
    gtk_container_add(GTK_CONTAINER(DialogArea(pwDialog, DA_MAIN)), NewWidget(&nw));
    gtk_widget_get_allocation(pwToolbar, &allocation);
    gtk_widget_set_size_request(pwToolbar, allocation.width, allocation.height);

    gtk_widget_grab_focus(DialogArea(pwDialog, DA_OK));

    NewSet(&nw);

    GTKRunDialog(pwDialog);

}

extern void
SetMET(GtkWidget * UNUSED(pw), gpointer p)
{
    gchar *met_dir = BuildFilename("met");
    gchar *filename = GTKFileSelect(_("Set match equity table"), "*.xml", met_dir, NULL, GTK_FILE_CHOOSER_ACTION_OPEN);
    g_free(met_dir);

    if (filename) {
        gchar *command = g_strconcat("set matchequitytable \"", filename, "\"", NULL);
        UserCommand(command);
        g_free(command);
        g_free(filename);
        /* update filename on option page */
        if (p && gtk_widget_get_visible(p))
            gtk_label_set_text(GTK_LABEL(p), (char *) miCurrent.szFileName);
    }
    UserCommand("save settings");
}

typedef struct {
    int *pfOK;
    evalcontext *precCube, *precCheq;
    movefilter *pmf;
} rolloutpagewidget;

typedef struct {
    int *pfOK;
    GtkWidget *pwCubeful, *pwVarRedn, *pwInitial, *pwRotate, *pwDoLate;
    GtkWidget *pwDoTrunc, *pwCubeEqualChequer, *pwPlayersAreSame;
    GtkWidget *pwTruncEqualPlayer0;
    GtkWidget *pwTruncBearoff2, *pwTruncBearoffOS, *pwTruncBearoffOpts;
    GtkWidget *pwAdjLatePlies, *pwAdjTruncPlies, *pwAdjMinGames;
    GtkWidget *pwDoSTDStop, *pwAdjMaxError;
    GtkWidget *pwJsdDoStop;
    GtkWidget *pwJsdMinGames, *pwJsdAdjMinGames, *pwAdjJsdLimit;
    GtkAdjustment *padjTrials, *padjTruncPlies, *padjLatePlies;
    GtkAdjustment *padjSeed, *padjMinGames, *padjMaxError;
    GtkAdjustment *padjJsdMinGames, *padjJsdLimit;
    GtkWidget *pwMinGames, *pwMaxError, *pwJsdLimit;
} rolloutpagegeneral;

typedef struct {
    rolloutcontext rcRollout;
    rolloutpagegeneral *prwGeneral;
    rolloutpagewidget *prpwPages[4], *prpwTrunc;
    GtkWidget *RolloutNotebook, *frame[2];
    AnalysisDetails *analysisDetails[5];
    int fCubeEqualChequer, fPlayersAreSame, fTruncEqualPlayer0;
    int *pfOK;
    int *psaveAs;
    int *ploadRS;
} rolloutwidget;

/***************************************************************************
 *****
 *****  Change SGF_ROLLOUT_VER in eval.h if rollout settings change
 *****  such that previous .sgf files won't be able to extend rollouts
 *****
 ***************************************************************************/
static void
GetRolloutSettings(GtkWidget * pw, rolloutwidget * prw)
{
    int fCubeEqChequer, fSamePlayers;

    prw->rcRollout.nTrials = (int) gtk_adjustment_get_value(prw->prwGeneral->padjTrials);

    prw->rcRollout.nTruncate = (unsigned short) gtk_adjustment_get_value(prw->prwGeneral->padjTruncPlies);

    prw->rcRollout.fDoTruncate = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prw->prwGeneral->pwDoTrunc));

    prw->rcRollout.nSeed = (int) gtk_adjustment_get_value(prw->prwGeneral->padjSeed);

    prw->rcRollout.fCubeful = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prw->prwGeneral->pwCubeful));

    prw->rcRollout.fVarRedn = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prw->prwGeneral->pwVarRedn));

    prw->rcRollout.fRotate = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prw->prwGeneral->pwRotate));

    prw->rcRollout.fInitial = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prw->prwGeneral->pwInitial));

    prw->rcRollout.fTruncBearoff2 = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prw->prwGeneral->pwTruncBearoff2));

    prw->rcRollout.fTruncBearoffOS = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prw->prwGeneral->pwTruncBearoffOS));

    prw->rcRollout.nLate = (unsigned short) gtk_adjustment_get_value(prw->prwGeneral->padjLatePlies);

    prw->rcRollout.fLateEvals = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prw->prwGeneral->pwDoLate));

    fCubeEqChequer = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prw->prwGeneral->pwCubeEqualChequer));

    fSamePlayers = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prw->prwGeneral->pwPlayersAreSame));

    prw->rcRollout.fStopOnSTD = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prw->prwGeneral->pwDoSTDStop));
    prw->rcRollout.nMinimumGames =
        (unsigned int) gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(prw->prwGeneral->pwMinGames));
    prw->rcRollout.rStdLimit = (float) gtk_spin_button_get_value(GTK_SPIN_BUTTON(prw->prwGeneral->pwMaxError));

    prw->rcRollout.fStopOnJsd = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prw->prwGeneral->pwJsdDoStop));
    prw->rcRollout.nMinimumJsdGames =
        (unsigned int) gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(prw->prwGeneral->pwJsdMinGames));
    prw->rcRollout.rJsdLimit = (float) gtk_spin_button_get_value(GTK_SPIN_BUTTON(prw->prwGeneral->pwJsdLimit));

    /* if the players are the same, copy player 0 settings to player 1 */
    if (fSamePlayers) {
        int p0, p1;

        for (p0 = 0, p1 = 1; p0 < 4; p0 += 2, p1 += 2) {
            memcpy(prw->prpwPages[p1]->precCheq, prw->prpwPages[p0]->precCheq, sizeof(evalcontext));
            memcpy(prw->prpwPages[p1]->precCube, prw->prpwPages[p0]->precCube, sizeof(evalcontext));

            memcpy(prw->prpwPages[p1]->pmf, prw->prpwPages[p0]->pmf,
                   MAX_FILTER_PLIES * MAX_FILTER_PLIES * sizeof(movefilter));
        }
    }

    /* if cube is same as chequer, copy the chequer settings to the
     * cube settings */

    if (fCubeEqChequer) {
        for (int i = 0; i < 4; ++i) {
            memcpy(prw->prpwPages[i]->precCube, prw->prpwPages[i]->precCheq, sizeof(evalcontext));
        }

        memcpy(prw->prpwTrunc->precCube, prw->prpwTrunc->precCheq, sizeof(evalcontext));
    }

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prw->prwGeneral->pwTruncEqualPlayer0))) {
        memcpy(prw->prpwTrunc->precCube, prw->prpwPages[0]->precCheq, sizeof(evalcontext));
        memcpy(prw->prpwTrunc->precCheq, prw->prpwPages[0]->precCube, sizeof(evalcontext));
    }

    gtk_widget_destroy(gtk_widget_get_toplevel(pw));

}

static void
SetRolloutsOK(GtkWidget * pw, rolloutwidget * prw)
{
    *prw->pfOK = TRUE;
    GetRolloutSettings(pw, prw);
}

static void
save_rollout_as_clicked(GtkWidget * pw, rolloutwidget * prw)
{
    *prw->psaveAs = TRUE;
    GetRolloutSettings(pw, prw);
}

static void
load_rs_clicked(GtkWidget * pw, rolloutwidget * prw)
{
    *prw->ploadRS = TRUE;
    gtk_widget_destroy(gtk_widget_get_toplevel(pw));
}

/* create one page for rollout settings for play & truncation */

static AnalysisDetails *
RolloutPage(rolloutpagewidget * prpw, const char *title, const int UNUSED(fMoveFilter), GtkWidget ** frameRet)
{
    GtkWidget *pwFrame;
    pwFrame = gtk_frame_new(title);
    if (frameRet)
        *frameRet = pwFrame;

    return CreateEvalSettings(pwFrame, title, prpw->precCheq, prpw->pmf, prpw->precCube, NULL, FALSE);
}

static void
LateEvalToggled(GtkWidget * UNUSED(pw), rolloutwidget * prw)
{

    int do_late = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prw->prwGeneral->pwDoLate));
    int are_same = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prw->prwGeneral->pwPlayersAreSame));

    /* turn on/off the late pages */
    gtk_widget_set_sensitive(prw->analysisDetails[2]->pwSettingWidgets, do_late);
    gtk_widget_set_sensitive(prw->analysisDetails[3]->pwSettingWidgets, do_late && !are_same);

    /* turn on/off the ply setting in the general page */
    gtk_widget_set_sensitive(GTK_WIDGET(prw->prwGeneral->pwAdjLatePlies), do_late);
}

static void
STDStopToggled(GtkWidget * UNUSED(pw), rolloutwidget * prw)
{
    int do_std_stop = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prw->prwGeneral->pwDoSTDStop));

    gtk_widget_set_sensitive(GTK_WIDGET(prw->prwGeneral->pwAdjMinGames), do_std_stop);
    gtk_widget_set_sensitive(GTK_WIDGET(prw->prwGeneral->pwAdjMaxError), do_std_stop);
}

static void
JsdStopToggled(GtkWidget * UNUSED(pw), rolloutwidget * prw)
{
    int do_jsd_stop = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prw->prwGeneral->pwJsdDoStop));

    gtk_widget_set_sensitive(GTK_WIDGET(prw->prwGeneral->pwJsdAdjMinGames), do_jsd_stop);
    gtk_widget_set_sensitive(GTK_WIDGET(prw->prwGeneral->pwAdjJsdLimit), do_jsd_stop);
}

static void
TruncEnableToggled(GtkWidget * UNUSED(pw), rolloutwidget * prw)
{
    int do_trunc = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prw->prwGeneral->pwDoTrunc));

    int sameas_p0 = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prw->prwGeneral->pwTruncEqualPlayer0));

    /* turn on/off the truncation page */
    gtk_widget_set_sensitive(prw->analysisDetails[4]->pwSettingWidgets, do_trunc && !sameas_p0);

    /* turn on/off the truncation ply setting */
    gtk_widget_set_sensitive(GTK_WIDGET(prw->prwGeneral->pwAdjTruncPlies), do_trunc);

}

static void
TruncEqualPlayer0Toggled(GtkWidget * UNUSED(pw), rolloutwidget * prw)
{
    int do_trunc = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prw->prwGeneral->pwDoTrunc));
    int sameas_p0 = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prw->prwGeneral->pwTruncEqualPlayer0));

    prw->fTruncEqualPlayer0 = sameas_p0;
    /* turn on/off the truncation page */
    gtk_widget_set_sensitive(prw->analysisDetails[4]->pwSettingWidgets, do_trunc && !sameas_p0);
}

static void
CubeEqCheqToggled(GtkWidget * UNUSED(pw), rolloutwidget * prw)
{
    int i, are_same = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prw->prwGeneral->pwCubeEqualChequer));

    prw->fCubeEqualChequer = are_same;

    for (i = 0; i < 5; ++i)
        prw->analysisDetails[i]->cubeDisabled = are_same;
}

static void
CubefulToggled(GtkWidget * UNUSED(pw), rolloutwidget * prw)
{

    int f = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prw->prwGeneral->pwCubeful));

    gtk_widget_set_sensitive(GTK_WIDGET(prw->prwGeneral->pwTruncBearoffOS), !f);

}

static void
PlayersSameToggled(GtkWidget * UNUSED(pw), rolloutwidget * prw)
{
    int are_same = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prw->prwGeneral->pwPlayersAreSame));
    int do_late = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prw->prwGeneral->pwDoLate));

    prw->fPlayersAreSame = are_same;

    gtk_widget_set_sensitive(prw->analysisDetails[1]->pwSettingWidgets, !are_same);
    gtk_widget_set_sensitive(prw->analysisDetails[3]->pwSettingWidgets, !are_same && do_late);
    prw->analysisDetails[0]->title = are_same ? _("First Play Both") : _("First Play (0) ");
    gtk_frame_set_label(GTK_FRAME(prw->frame[0]), prw->analysisDetails[0]->title);
    prw->analysisDetails[2]->title = are_same ? _("Later Play Both") : _("Later Play (0) ");
    gtk_frame_set_label(GTK_FRAME(prw->frame[1]), prw->analysisDetails[2]->title);
}

/* create the General page for rollouts */

static GtkWidget *
RolloutPageGeneral(rolloutpagegeneral * prpw, rolloutwidget * prw)
{
    GtkWidget *pwPage;
    GtkWidget *pwh, *pwv, *pwHBox;
#if GTK_CHECK_VERSION(3,0,0)
    GtkWidget *pwGrid;
#else
    GtkWidget *pwTable;
#endif
    GtkWidget *pwFrame;
    GtkWidget *pwLabel;

#if GTK_CHECK_VERSION(3,0,0)
    pwPage = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
    pwPage = gtk_vbox_new(FALSE, 0);
#endif
    gtk_container_set_border_width(GTK_CONTAINER(pwPage), 8);

    prpw->padjSeed = GTK_ADJUSTMENT(gtk_adjustment_new((gdouble) prw->rcRollout.nSeed, 0, INT_MAX, 1, 1, 0));
    prpw->padjTrials = GTK_ADJUSTMENT(gtk_adjustment_new(prw->rcRollout.nTrials, 1, 36 * 1296 * 1296, 36, 36, 0));

#if GTK_CHECK_VERSION(3,0,0)
    pwh = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
#else
    pwh = gtk_hbox_new(FALSE, 8);
#endif
    gtk_box_pack_start(GTK_BOX(pwPage), pwh, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(pwh), pwLabel = gtk_label_new(_("Seed:")), FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(pwh), gtk_spin_button_new(prpw->padjSeed, 1, 0), FALSE, FALSE, 4);

    gtk_widget_set_tooltip_text(pwLabel,
                                _("The seed is a number used to initialise the dice rolls generator. Reusing the same seed allows to reproduce the rollout results. 0 is a special value that leaves GNU Backgammon use a random value."));

    gtk_box_pack_end(GTK_BOX(pwh), gtk_spin_button_new(prpw->padjTrials, 36, 0), FALSE, FALSE, 4);
    gtk_box_pack_end(GTK_BOX(pwh), pwLabel = gtk_label_new(_("Trials:")), FALSE, FALSE, 4);

    gtk_widget_set_tooltip_text(pwLabel,
                                _("The number of games to play out; the standard deviation of the results decreases like the square root of the trials number. It is recommended to use a multiple of 36 or 1296."));

    pwFrame = gtk_frame_new(_("Truncation"));
    gtk_box_pack_start(GTK_BOX(pwPage), pwFrame, FALSE, FALSE, 0);

    gtk_widget_set_tooltip_text(pwFrame,
                                _("Truncated rollouts are rollouts played to a certain number of moves as opposed to the end of the game or a double/pass cube action. " "They are faster and their results have a lower variance than full rollouts, but their accuracy depends on that of the static evaluation at the trucation point."));

#if GTK_CHECK_VERSION(3,0,0)
    pwh = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
#else
    pwh = gtk_hbox_new(FALSE, 8);
#endif
    gtk_container_set_border_width(GTK_CONTAINER(pwh), 8);
    gtk_container_add(GTK_CONTAINER(pwFrame), pwh);

    prpw->pwDoTrunc = gtk_check_button_new_with_label(_("Truncate Rollouts"));
    gtk_box_pack_start(GTK_BOX(pwh), prpw->pwDoTrunc, FALSE, FALSE, 4);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(prpw->pwDoTrunc), prw->rcRollout.fDoTruncate);
    g_signal_connect(G_OBJECT(prpw->pwDoTrunc), "toggled", G_CALLBACK(TruncEnableToggled), prw);

#if GTK_CHECK_VERSION(3,0,0)
    prpw->pwAdjTruncPlies = pwHBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
#else
    prpw->pwAdjTruncPlies = pwHBox = gtk_hbox_new(FALSE, 0);
#endif
    gtk_box_pack_end(GTK_BOX(pwh), pwHBox, FALSE, FALSE, 4);

    prpw->padjTruncPlies = GTK_ADJUSTMENT(gtk_adjustment_new(prw->rcRollout.nTruncate, 1, 1000, 1, 1, 0));
    gtk_box_pack_end(GTK_BOX(pwHBox), gtk_spin_button_new(prpw->padjTruncPlies, 1, 0), FALSE, FALSE, 4);
    gtk_box_pack_end(GTK_BOX(pwHBox), gtk_label_new(_("Truncate at ply:")), FALSE, FALSE, 4);


    pwFrame = gtk_frame_new(_("Evaluation for later plies"));
    gtk_box_pack_start(GTK_BOX(pwPage), pwFrame, FALSE, FALSE, 0);

    gtk_widget_set_tooltip_text(pwFrame,
                                _("It can be useful to use a different (higher) level of evaluation for the first few plies if one thinks that the current position is complex but will quickly evolve into simpler ones that will be adequately handled by a lower level."));

#if GTK_CHECK_VERSION(3,0,0)
    pwh = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
#else
    pwh = gtk_hbox_new(FALSE, 8);
#endif
    gtk_container_set_border_width(GTK_CONTAINER(pwh), 8);
    gtk_container_add(GTK_CONTAINER(pwFrame), pwh);

    prpw->pwDoLate = gtk_check_button_new_with_label(_("Enable separate evaluations "));
    gtk_box_pack_start(GTK_BOX(pwh), prpw->pwDoLate, FALSE, FALSE, 4);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(prw->prwGeneral->pwDoLate), prw->rcRollout.fLateEvals);
    g_signal_connect(G_OBJECT(prw->prwGeneral->pwDoLate), "toggled", G_CALLBACK(LateEvalToggled), prw);

#if GTK_CHECK_VERSION(3,0,0)
    prpw->pwAdjLatePlies = pwHBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
#else
    prpw->pwAdjLatePlies = pwHBox = gtk_hbox_new(FALSE, 0);
#endif
    gtk_box_pack_end(GTK_BOX(pwh), pwHBox, FALSE, FALSE, 4);

    prpw->padjLatePlies = GTK_ADJUSTMENT(gtk_adjustment_new(prw->rcRollout.nLate, 1, 1000, 1, 1, 0));
    gtk_box_pack_end(GTK_BOX(pwHBox), gtk_spin_button_new(prpw->padjLatePlies, 1, 0), FALSE, FALSE, 4);
    gtk_box_pack_end(GTK_BOX(pwHBox), gtk_label_new(_("Change eval after ply:")), FALSE, FALSE, 4);


    pwFrame = gtk_frame_new(_("Stop when result is accurate"));
    gtk_box_pack_start(GTK_BOX(pwPage), pwFrame, FALSE, FALSE, 0);

    /* an hbox for the frame */
#if GTK_CHECK_VERSION(3,0,0)
    pwh = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
#else
    pwh = gtk_hbox_new(FALSE, 8);
#endif
    gtk_container_set_border_width(GTK_CONTAINER(pwh), 8);
    gtk_container_add(GTK_CONTAINER(pwFrame), pwh);

    prpw->pwDoSTDStop = gtk_check_button_new_with_label(_("Stop when SE is small enough "));
    gtk_box_pack_start(GTK_BOX(pwh), prpw->pwDoSTDStop, FALSE, FALSE, 4);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(prw->prwGeneral->pwDoSTDStop), prw->rcRollout.fStopOnSTD);
    g_signal_connect(G_OBJECT(prw->prwGeneral->pwDoSTDStop), "toggled", G_CALLBACK(STDStopToggled), prw);
    gtk_widget_set_tooltip_text(prpw->pwDoSTDStop,
                                _("Standard Error (SE) of the candidate play equity is a measure of "
                                  "the accuracy of the current rollout result. "
                                  "If the rollout were to be continued indefinitely, its result would "
                                  "have about 95 chances out of 100 to be in an interval of +/-2 SE "
                                  "around the current result."));

    /* a vbox for the adjusters */
#if GTK_CHECK_VERSION(3,0,0)
    pwv = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
    pwv = gtk_vbox_new(FALSE, 0);
#endif
    gtk_box_pack_end(GTK_BOX(pwh), pwv, FALSE, FALSE, 4);

#if GTK_CHECK_VERSION(3,0,0)
    prpw->pwAdjMinGames = pwHBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
#else
    prpw->pwAdjMinGames = pwHBox = gtk_hbox_new(FALSE, 0);
#endif
    gtk_box_pack_start(GTK_BOX(pwv), pwHBox, TRUE, TRUE, 0);

    prpw->padjMinGames = GTK_ADJUSTMENT(gtk_adjustment_new(prw->rcRollout.nMinimumGames, 1, 36 * 1296 * 1296, 36, 36, 0));
    prpw->pwMinGames = gtk_spin_button_new(prpw->padjMinGames, 1, 0);
    gtk_box_pack_end(GTK_BOX(pwHBox), prpw->pwMinGames, FALSE, FALSE, 4);
    gtk_box_pack_end(GTK_BOX(pwHBox), gtk_label_new(_("Minimum Trials:")), FALSE, FALSE, 4);

#if GTK_CHECK_VERSION(3,0,0)
    prpw->pwAdjMaxError = pwHBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
#else
    prpw->pwAdjMaxError = pwHBox = gtk_hbox_new(FALSE, 0);
#endif
    gtk_box_pack_end(GTK_BOX(pwv), pwHBox, TRUE, TRUE, 0);

    prpw->padjMaxError = GTK_ADJUSTMENT(gtk_adjustment_new(prw->rcRollout.rStdLimit, 0, 1, .0001, .0001, 0));
    prpw->pwMaxError = gtk_spin_button_new(prpw->padjMaxError, .0001, 4);
    gtk_box_pack_end(GTK_BOX(pwHBox), prpw->pwMaxError, FALSE, FALSE, 4);
    gtk_box_pack_end(GTK_BOX(pwHBox), gtk_label_new(_("Equity SE threshold:")), FALSE, FALSE, 4);


    pwFrame = gtk_frame_new(_("Stop when result is clear"));
    gtk_box_pack_start(GTK_BOX(pwPage), pwFrame, FALSE, FALSE, 0);

    /* an hbox for the frame */
#if GTK_CHECK_VERSION(3,0,0)
    pwh = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
#else
    pwh = gtk_hbox_new(FALSE, 8);
#endif
    gtk_container_set_border_width(GTK_CONTAINER(pwh), 8);
    gtk_container_add(GTK_CONTAINER(pwFrame), pwh);

    prpw->pwJsdDoStop = gtk_check_button_new_with_label(_("Stop when JSD margin is high enough"));
    gtk_box_pack_start(GTK_BOX(pwh), prpw->pwJsdDoStop, FALSE, FALSE, 4);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(prw->prwGeneral->pwJsdDoStop), prw->rcRollout.fStopOnJsd);
    g_signal_connect(G_OBJECT(prw->prwGeneral->pwJsdDoStop), "toggled", G_CALLBACK(JsdStopToggled), prw);
    gtk_widget_set_tooltip_text(prpw->pwJsdDoStop,
                                _("The number of Joint Standard Deviations (JSD) between "
                                  "the current favourite and another candidate decision is a "
                                  "measure of how unlikely continuing the rollout is to "
                                  "change the current result. "
                                  "A candidate trailing by 2.33 JSD has about 1 chance out of 100 to "
                                  "end as winner if the rollout were to be continued indefinitely."));

    /* a vbox for the adjusters */
#if GTK_CHECK_VERSION(3,0,0)
    pwv = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
    pwv = gtk_vbox_new(FALSE, 0);
#endif
    gtk_box_pack_end(GTK_BOX(pwh), pwv, FALSE, FALSE, 4);

#if GTK_CHECK_VERSION(3,0,0)
    prpw->pwJsdAdjMinGames = pwHBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
#else
    prpw->pwJsdAdjMinGames = pwHBox = gtk_hbox_new(FALSE, 0);
#endif
    gtk_box_pack_start(GTK_BOX(pwv), pwHBox, TRUE, TRUE, 0);

    prpw->padjJsdMinGames =
        GTK_ADJUSTMENT(gtk_adjustment_new(prw->rcRollout.nMinimumJsdGames, 1, 36 * 1296 * 1296, 36, 36, 0));
    prpw->pwJsdMinGames = gtk_spin_button_new(prpw->padjJsdMinGames, 1, 0);
    gtk_box_pack_end(GTK_BOX(pwHBox), prpw->pwJsdMinGames, FALSE, FALSE, 4);
    gtk_box_pack_end(GTK_BOX(pwHBox), gtk_label_new(_("Minimum Trials:")), FALSE, FALSE, 4);

#if GTK_CHECK_VERSION(3,0,0)
    prpw->pwAdjJsdLimit = pwHBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
#else
    prpw->pwAdjJsdLimit = pwHBox = gtk_hbox_new(FALSE, 0);
#endif
    gtk_box_pack_end(GTK_BOX(pwv), pwHBox, TRUE, TRUE, 0);

    prpw->padjJsdLimit = GTK_ADJUSTMENT(gtk_adjustment_new(prw->rcRollout.rJsdLimit, 0, 8, .0001, .0001, 0));
    prpw->pwJsdLimit = gtk_spin_button_new(prpw->padjJsdLimit, .0001, 4);
    gtk_box_pack_end(GTK_BOX(pwHBox), prpw->pwJsdLimit, FALSE, FALSE, 4);
    gtk_box_pack_end(GTK_BOX(pwHBox), gtk_label_new(_("JSD threshold:")), FALSE, FALSE, 4);


    pwFrame = gtk_frame_new(_("Bearoff Truncation"));
    gtk_box_pack_start(GTK_BOX(pwPage), pwFrame, FALSE, FALSE, 0);

    gtk_widget_set_tooltip_text(pwFrame,
        _("Truncating the rollouts when reaching the bearoff databases slighly improves the speed and decrease their variance. " "The first option is totally accurate and should be selected for practical use; the second one can be slightly inaccurate but is useful on average and its use is recommended."));

#if GTK_CHECK_VERSION(3,0,0)
    prpw->pwTruncBearoffOpts = pwv = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
#else
    prpw->pwTruncBearoffOpts = pwv = gtk_vbox_new(FALSE, 8);
#endif
    gtk_container_set_border_width(GTK_CONTAINER(pwv), 8);
    gtk_container_add(GTK_CONTAINER(pwFrame), pwv);

    prpw->pwTruncBearoff2 =
        gtk_check_button_new_with_label(_("Truncate cubeless and cubeful money at exact bearoff database"));

    gtk_box_pack_start(GTK_BOX(pwv), prpw->pwTruncBearoff2, FALSE, FALSE, 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(prpw->pwTruncBearoff2), prw->rcRollout.fTruncBearoff2);

    prpw->pwTruncBearoffOS = gtk_check_button_new_with_label(_("Truncate cubeless at one-sided bearoff database"));

    gtk_box_pack_start(GTK_BOX(pwv), prpw->pwTruncBearoffOS, FALSE, FALSE, 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(prpw->pwTruncBearoffOS), prw->rcRollout.fTruncBearoffOS);

#if GTK_CHECK_VERSION(3,0,0)
    pwGrid = gtk_grid_new();
    gtk_grid_set_column_homogeneous(GTK_GRID(pwGrid), TRUE);
    gtk_grid_set_row_homogeneous(GTK_GRID(pwGrid), TRUE);
    gtk_container_add(GTK_CONTAINER(pwPage), pwGrid);
#else
    pwTable = gtk_table_new(2, 2, TRUE);
    gtk_container_add(GTK_CONTAINER(pwPage), pwTable);
#endif

    prpw->pwCubeful = gtk_check_button_new_with_label(_("Cubeful rollout"));
#if GTK_CHECK_VERSION(3,0,0)
    gtk_grid_attach(GTK_GRID(pwGrid), prpw->pwCubeful, 0, 0, 1, 1);
#if GTK_CHECK_VERSION(3,12,0)
    gtk_widget_set_margin_start(prpw->pwCubeful, 2);
    gtk_widget_set_margin_end(prpw->pwCubeful, 2);
#else
    gtk_widget_set_margin_left(prpw->pwCubeful, 2);
    gtk_widget_set_margin_right(prpw->pwCubeful, 2);
#endif
    gtk_widget_set_margin_top(prpw->pwCubeful, 2);
    gtk_widget_set_margin_bottom(prpw->pwCubeful, 2);
#else
    gtk_table_attach(GTK_TABLE(pwTable), prpw->pwCubeful, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 2, 2);
#endif

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(prpw->pwCubeful), prw->rcRollout.fCubeful);

    g_signal_connect(G_OBJECT(prpw->pwCubeful), "toggled", G_CALLBACK(CubefulToggled), prw);

    gtk_widget_set_tooltip_text(prpw->pwCubeful,
                                _("A cubeful rollout means that it is using the cube in the rollout. " "It is recommended to enable this option when analyzing cubeful play."));

    prpw->pwVarRedn = gtk_check_button_new_with_label(_("Use variance reduction"));
#if GTK_CHECK_VERSION(3,0,0)
    gtk_grid_attach(GTK_GRID(pwGrid), prpw->pwVarRedn, 1, 0, 1, 1);
#if GTK_CHECK_VERSION(3,12,0)
    gtk_widget_set_margin_start(prpw->pwVarRedn, 2);
    gtk_widget_set_margin_end(prpw->pwVarRedn, 2);
#else
    gtk_widget_set_margin_left(prpw->pwVarRedn, 2);
    gtk_widget_set_margin_right(prpw->pwVarRedn, 2);
#endif
    gtk_widget_set_margin_top(prpw->pwVarRedn, 2);
    gtk_widget_set_margin_bottom(prpw->pwVarRedn, 2);
#else
    gtk_table_attach(GTK_TABLE(pwTable), prpw->pwVarRedn, 1, 2, 0, 1, GTK_FILL, GTK_FILL, 2, 2);
#endif

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(prpw->pwVarRedn), prw->rcRollout.fVarRedn);
    gtk_widget_set_tooltip_text(prpw->pwVarRedn,
                                _("Variance Reduction is a procedure used to considerably increase "
                                  "the accuracy of rollout results. "
                                  "It has a significant cost for 0-ply rollouts "
                                  "(but is still valuable on an accuracy per elapsed-time basis) "
                                  "and is almost free for higher plies. " "It is recommended to enable this option."));

    prpw->pwRotate = gtk_check_button_new_with_label(_("Use quasi-random dice"));
#if GTK_CHECK_VERSION(3,0,0)
    gtk_grid_attach(GTK_GRID(pwGrid), prpw->pwRotate, 0, 1, 1, 1);
#if GTK_CHECK_VERSION(3,12,0)
    gtk_widget_set_margin_start(prpw->pwRotate, 2);
    gtk_widget_set_margin_end(prpw->pwRotate, 2);
#else
    gtk_widget_set_margin_left(prpw->pwRotate, 2);
    gtk_widget_set_margin_right(prpw->pwRotate, 2);
#endif
    gtk_widget_set_margin_top(prpw->pwRotate, 2);
    gtk_widget_set_margin_bottom(prpw->pwRotate, 2);
#else
    gtk_table_attach(GTK_TABLE(pwTable), prpw->pwRotate, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 2, 2);
#endif
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(prpw->pwRotate), prw->rcRollout.fRotate);
    gtk_widget_set_tooltip_text(prpw->pwRotate,
                                _("This option ensures that every possible roll appears once in each set of 36 trials. This is only moderately useful if variance reduction is used but important if it isn't. It is recommended to enable this option."));

    prpw->pwInitial = gtk_check_button_new_with_label(_("Rollout as initial position"));
#if GTK_CHECK_VERSION(3,0,0)
    gtk_grid_attach(GTK_GRID(pwGrid), prpw->pwInitial, 1, 1, 1, 1);
#if GTK_CHECK_VERSION(3,12,0)
    gtk_widget_set_margin_start(prpw->pwInitial, 2);
    gtk_widget_set_margin_end(prpw->pwInitial, 2);
#else
    gtk_widget_set_margin_left(prpw->pwInitial, 2);
    gtk_widget_set_margin_right(prpw->pwInitial, 2);
#endif
    gtk_widget_set_margin_top(prpw->pwInitial, 2);
    gtk_widget_set_margin_bottom(prpw->pwInitial, 2);
#else
    gtk_table_attach(GTK_TABLE(pwTable), prpw->pwInitial, 1, 2, 1, 2, GTK_FILL, GTK_FILL, 2, 2);
#endif
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(prpw->pwInitial), prw->rcRollout.fInitial);
    gtk_widget_set_tooltip_text(prpw->pwInitial,
                                _("This option ensures that the first ply roll is not a doublet."));

    return pwPage;
}

static void
gtk_save_rollout_settings(void)
{
    char *filename;
    char *folder;
    FILE *pf;
    folder = g_build_filename(szHomeDirectory, "rol", NULL);
    if (!g_file_test(folder, G_FILE_TEST_IS_DIR)) {
        g_mkdir(folder, 0700);
        if (!g_file_test(folder, G_FILE_TEST_IS_DIR)) {
            outputerrf(_("Failed to create %s"), folder);
            return;
        }
    }
    filename =
        GTKFileSelect(_("Save Rollout Settings As (*.rol)"), "*.rol", folder, NULL, GTK_FILE_CHOOSER_ACTION_SAVE);
    if (!filename)
        return;

    if (!g_str_has_suffix(filename, ".rol")) {
        char *tmp = g_strdup(filename);
        g_free(filename);
        filename = g_strconcat(tmp, ".rol", NULL);
        g_free(tmp);
    }

    errno = 0;
    pf = g_fopen(filename, "w");
    if (!pf) {
        outputerr(filename);
        g_free(filename);
        return;
    }
    SaveRolloutSettings(pf, "set rollout", &rcRollout);
    fclose(pf);
    if (errno)
        outputerr(filename);
    g_free(filename);
    return;
}

static void
gtk_load_rollout_settings(void)
{
    gchar *folder = g_build_filename(szHomeDirectory, "rol", NULL);
    gchar *filename =
        GTKFileSelect(_("Open rollout settings (*.rol)"), "*.rol", folder, NULL, GTK_FILE_CHOOSER_ACTION_OPEN);

    if (filename) {
        gchar *command = g_strconcat("load commands \"", filename, "\"", NULL);
        outputoff();
        UserCommand(command);
        outputon();
        g_free(command);
        g_free(filename);
    }
}


extern void
SetRollouts(gpointer UNUSED(p), guint UNUSED(n), GtkWidget * UNUSED(pwIgnore))
{
    int fOK = FALSE;
    int saveAs = TRUE;
    int loadRS = TRUE;
    rolloutwidget rw;
    rolloutpagegeneral RPGeneral;
    rolloutpagewidget RPPlayer0, RPPlayer1, RPPlayer0Late, RPPlayer1Late, RPTrunc;
    char sz[256];
    int i;
    const float epsilon = 1.0e-6f;

    while (saveAs || loadRS) {
        GtkWidget *pwDialog, *pwVBox;
#if GTK_CHECK_VERSION(3,0,0)
        GtkWidget *pwGrid;
        GtkWidget *pw;
#else
        GtkWidget *pwTable;
#endif
        GtkWidget *saveAsButton, *loadRSButton;

        memcpy(&rw.rcRollout, &rcRollout, sizeof(rcRollout));
        rw.prwGeneral = &RPGeneral;
        rw.prpwPages[0] = &RPPlayer0;
        rw.prpwPages[1] = &RPPlayer1;
        rw.prpwPages[2] = &RPPlayer0Late;
        rw.prpwPages[3] = &RPPlayer1Late;
        rw.prpwTrunc = &RPTrunc;
        rw.fCubeEqualChequer = fCubeEqualChequer;
        rw.fPlayersAreSame = fPlayersAreSame;
        rw.fTruncEqualPlayer0 = fTruncEqualPlayer0;
        rw.pfOK = &fOK;
        rw.psaveAs = &saveAs;
        rw.ploadRS = &loadRS;

        RPPlayer0.precCube = &rw.rcRollout.aecCube[0];
        RPPlayer0.precCheq = &rw.rcRollout.aecChequer[0];
        RPPlayer0.pmf = (movefilter *) rw.rcRollout.aaamfChequer[0];

        RPPlayer0Late.precCube = &rw.rcRollout.aecCubeLate[0];
        RPPlayer0Late.precCheq = &rw.rcRollout.aecChequerLate[0];
        RPPlayer0Late.pmf = (movefilter *) rw.rcRollout.aaamfLate[0];

        RPPlayer1.precCube = &rw.rcRollout.aecCube[1];
        RPPlayer1.precCheq = &rw.rcRollout.aecChequer[1];
        RPPlayer1.pmf = (movefilter *) rw.rcRollout.aaamfChequer[1];

        RPPlayer1Late.precCube = &rw.rcRollout.aecCubeLate[1];
        RPPlayer1Late.precCheq = &rw.rcRollout.aecChequerLate[1];
        RPPlayer1Late.pmf = (movefilter *) rw.rcRollout.aaamfLate[1];

        RPTrunc.precCube = &rw.rcRollout.aecCubeTrunc;
        RPTrunc.precCheq = &rw.rcRollout.aecChequerTrunc;

        saveAs = FALSE;
        loadRS = FALSE;
        pwDialog = GTKCreateDialog(_("GNU Backgammon - Rollouts"), DT_QUESTION,
                                   NULL, DIALOG_FLAG_MODAL, G_CALLBACK(SetRolloutsOK), &rw);

        gtk_container_add(GTK_CONTAINER(DialogArea(pwDialog, DA_BUTTONS)),
                          saveAsButton = gtk_button_new_with_label(_("Save As")));
        g_signal_connect(saveAsButton, "clicked", G_CALLBACK(save_rollout_as_clicked), &rw);

        gtk_container_add(GTK_CONTAINER(DialogArea(pwDialog, DA_BUTTONS)),
                          loadRSButton = gtk_button_new_with_label(_("Load")));
        g_signal_connect(loadRSButton, "clicked", G_CALLBACK(load_rs_clicked), &rw);

        gtk_container_add(GTK_CONTAINER(DialogArea(pwDialog, DA_MAIN)), rw.RolloutNotebook = gtk_notebook_new());
        gtk_container_set_border_width(GTK_CONTAINER(rw.RolloutNotebook), 4);


        gtk_notebook_append_page(GTK_NOTEBOOK(rw.RolloutNotebook),
                                 RolloutPageGeneral(rw.prwGeneral, &rw), gtk_label_new(_("General Settings")));

#if GTK_CHECK_VERSION(3,0,0)
        pwVBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
        pwVBox = gtk_vbox_new(FALSE, 0);
#endif
        gtk_notebook_append_page(GTK_NOTEBOOK(rw.RolloutNotebook), pwVBox, gtk_label_new(_("Play Settings")));

#if GTK_CHECK_VERSION(3,0,0)
        pwGrid = gtk_grid_new();
        gtk_box_pack_start(GTK_BOX(pwVBox), pwGrid, FALSE, FALSE, 0);
#else
        pwTable = gtk_table_new(3, 2, FALSE);
        gtk_box_pack_start(GTK_BOX(pwVBox), pwTable, FALSE, FALSE, 0);
#endif

        rw.analysisDetails[0] = RolloutPage(rw.prpwPages[0], _("First Play (0) "), TRUE, &rw.frame[0]);
#if GTK_CHECK_VERSION(3,0,0)
        gtk_grid_attach(GTK_GRID(pwGrid), pw = gtk_widget_get_parent(rw.analysisDetails[0]->pwSettingWidgets), 0, 0, 1, 1);
#if GTK_CHECK_VERSION(3,12,0)
        gtk_widget_set_margin_start(pw, 4);
        gtk_widget_set_margin_end(pw, 4);
#else
        gtk_widget_set_margin_left(pw, 4);
        gtk_widget_set_margin_right(pw, 4);
#endif
        gtk_widget_set_margin_top(pw, 4);
        gtk_widget_set_margin_bottom(pw, 4);
#else
        gtk_table_attach(GTK_TABLE(pwTable), gtk_widget_get_parent(rw.analysisDetails[0]->pwSettingWidgets), 0, 1, 0, 1,
                         (GtkAttachOptions) 0, (GtkAttachOptions) 0, 4, 4);
#endif

        rw.analysisDetails[1] = RolloutPage(rw.prpwPages[1], _("First Play (1) "), TRUE, NULL);
#if GTK_CHECK_VERSION(3,0,0)
        gtk_grid_attach(GTK_GRID(pwGrid), pw = gtk_widget_get_parent(rw.analysisDetails[1]->pwSettingWidgets), 1, 0, 1, 1);
#if GTK_CHECK_VERSION(3,12,0)
        gtk_widget_set_margin_start(pw, 4);
        gtk_widget_set_margin_end(pw, 4);
#else
        gtk_widget_set_margin_left(pw, 4);
        gtk_widget_set_margin_right(pw, 4);
#endif
        gtk_widget_set_margin_top(pw, 4);
        gtk_widget_set_margin_bottom(pw, 4);
#else
        gtk_table_attach(GTK_TABLE(pwTable), gtk_widget_get_parent(rw.analysisDetails[1]->pwSettingWidgets), 1, 2, 0, 1,
                         (GtkAttachOptions) 0, (GtkAttachOptions) 0, 4, 4);
#endif

        rw.analysisDetails[2] = RolloutPage(rw.prpwPages[2], _("Later Play (0) "), TRUE, &rw.frame[1]);
#if GTK_CHECK_VERSION(3,0,0)
        gtk_grid_attach(GTK_GRID(pwGrid), pw = gtk_widget_get_parent(rw.analysisDetails[2]->pwSettingWidgets), 0, 1, 1, 1);
#if GTK_CHECK_VERSION(3,12,0)
        gtk_widget_set_margin_start(pw, 4);
        gtk_widget_set_margin_end(pw, 4);
#else
        gtk_widget_set_margin_left(pw, 4);
        gtk_widget_set_margin_right(pw, 4);
#endif
        gtk_widget_set_margin_top(pw, 4);
        gtk_widget_set_margin_bottom(pw, 4);
#else
        gtk_table_attach(GTK_TABLE(pwTable), gtk_widget_get_parent(rw.analysisDetails[2]->pwSettingWidgets), 0, 1, 1, 2,
                         (GtkAttachOptions) 0, (GtkAttachOptions) 0, 4, 4);
#endif

        rw.analysisDetails[3] = RolloutPage(rw.prpwPages[3], _("Later Play (1) "), TRUE, NULL);
#if GTK_CHECK_VERSION(3,0,0)
        gtk_grid_attach(GTK_GRID(pwGrid), pw = gtk_widget_get_parent(rw.analysisDetails[3]->pwSettingWidgets), 1, 1, 1, 1);
#if GTK_CHECK_VERSION(3,12,0)
        gtk_widget_set_margin_start(pw, 4);
        gtk_widget_set_margin_end(pw, 4);
#else
        gtk_widget_set_margin_left(pw, 4);
        gtk_widget_set_margin_right(pw, 4);
#endif
        gtk_widget_set_margin_top(pw, 4);
        gtk_widget_set_margin_bottom(pw, 4);
#else
        gtk_table_attach(GTK_TABLE(pwTable), gtk_widget_get_parent(rw.analysisDetails[3]->pwSettingWidgets), 1, 2, 1, 2,
                         (GtkAttachOptions) 0, (GtkAttachOptions) 0, 4, 4);
#endif

        rw.prpwTrunc->pmf = NULL;
        rw.analysisDetails[4] = RolloutPage(rw.prpwTrunc, _("Eval. at Truncation Point"), FALSE, NULL);
#if GTK_CHECK_VERSION(3,0,0)
        gtk_grid_attach(GTK_GRID(pwGrid), pw = gtk_widget_get_parent(rw.analysisDetails[4]->pwSettingWidgets), 0, 2, 1, 1);
#if GTK_CHECK_VERSION(3,12,0)
        gtk_widget_set_margin_start(pw, 4);
        gtk_widget_set_margin_end(pw, 4);
#else
        gtk_widget_set_margin_left(pw, 4);
        gtk_widget_set_margin_right(pw, 4);
#endif
        gtk_widget_set_margin_top(pw, 4);
        gtk_widget_set_margin_bottom(pw, 4);
#else
        gtk_table_attach(GTK_TABLE(pwTable), gtk_widget_get_parent(rw.analysisDetails[4]->pwSettingWidgets), 0, 1, 2, 3,
                         (GtkAttachOptions) 0, (GtkAttachOptions) 0, 4, 4);
#endif

        RPGeneral.pwPlayersAreSame = gtk_check_button_new_with_label(_("Use same settings for both players"));
        gtk_box_pack_start(GTK_BOX(pwVBox), RPGeneral.pwPlayersAreSame, FALSE, FALSE, 0);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(RPGeneral.pwPlayersAreSame), rw.fPlayersAreSame);
        g_signal_connect(G_OBJECT(RPGeneral.pwPlayersAreSame), "toggled", G_CALLBACK(PlayersSameToggled), &rw);

        RPGeneral.pwCubeEqualChequer =
            gtk_check_button_new_with_label(_("Cube decisions use same settings as chequer play"));
        gtk_box_pack_start(GTK_BOX(pwVBox), RPGeneral.pwCubeEqualChequer, FALSE, FALSE, 0);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(RPGeneral.pwCubeEqualChequer), rw.fCubeEqualChequer);
        g_signal_connect(G_OBJECT(RPGeneral.pwCubeEqualChequer), "toggled", G_CALLBACK(CubeEqCheqToggled), &rw);

        RPGeneral.pwTruncEqualPlayer0 = gtk_check_button_new_with_label(_("Use player 0 setting for truncation point"));
        gtk_box_pack_start(GTK_BOX(pwVBox), RPGeneral.pwTruncEqualPlayer0, FALSE, FALSE, 0);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(RPGeneral.pwTruncEqualPlayer0), rw.fTruncEqualPlayer0);
        g_signal_connect(G_OBJECT(RPGeneral.pwTruncEqualPlayer0), "toggled", G_CALLBACK(TruncEqualPlayer0Toggled), &rw);

        /* Set things up correctly */
        LateEvalToggled(NULL, &rw);
        STDStopToggled(NULL, &rw);
        JsdStopToggled(NULL, &rw);
        TruncEnableToggled(NULL, &rw);
        CubeEqCheqToggled(NULL, &rw);
        PlayersSameToggled(NULL, &rw);
        CubefulToggled(NULL, &rw);

        GTKRunDialog(pwDialog);

        for (i = 0; i < 5; i++)
            free(rw.analysisDetails[i]);

        if (fOK || saveAs) {
            unsigned int fCubeful;
            outputoff();

            if ((fCubeful = rw.rcRollout.fCubeful) != rcRollout.fCubeful) {
                sprintf(sz, "set rollout cubeful %s", fCubeful ? "on" : "off");
                UserCommand(sz);

                for (i = 0; i < 2; ++i) {
                    /* replicate main page cubeful on/of in all evals */
                    rw.rcRollout.aecChequer[i].fCubeful = fCubeful;
                    rw.rcRollout.aecChequerLate[i].fCubeful = fCubeful;
                    rw.rcRollout.aecCube[i].fCubeful = fCubeful;
                    rw.rcRollout.aecCubeLate[i].fCubeful = fCubeful;
                }
                rw.rcRollout.aecCubeTrunc.fCubeful = rw.rcRollout.aecChequerTrunc.fCubeful = fCubeful;

            }

            for (i = 0; i < 2; ++i) {


                if (EvalCmp(&rw.rcRollout.aecCube[i], &rcRollout.aecCube[i], 1)) {
                    sprintf(sz, "set rollout player %d cubedecision", i);
                    SetEvalCommands(sz, &rw.rcRollout.aecCube[i], &rcRollout.aecCube[i]);
                }
                if (EvalCmp(&rw.rcRollout.aecChequer[i], &rcRollout.aecChequer[i], 1)) {
                    sprintf(sz, "set rollout player %d chequer", i);
                    SetEvalCommands(sz, &rw.rcRollout.aecChequer[i], &rcRollout.aecChequer[i]);
                }

                sprintf(sz, "set rollout player %d movefilter", i);
                SetMovefilterCommands(sz, rw.rcRollout.aaamfChequer[i], rcRollout.aaamfChequer[i]);

                if (EvalCmp(&rw.rcRollout.aecCubeLate[i], &rcRollout.aecCubeLate[i], 1)) {
                    sprintf(sz, "set rollout late player %d cube", i);
                    SetEvalCommands(sz, &rw.rcRollout.aecCubeLate[i], &rcRollout.aecCubeLate[i]);
                }

                if (EvalCmp(&rw.rcRollout.aecChequerLate[i], &rcRollout.aecChequerLate[i], 1)) {
                    sprintf(sz, "set rollout late player %d chequer", i);
                    SetEvalCommands(sz, &rw.rcRollout.aecChequerLate[i], &rcRollout.aecChequerLate[i]);
                }

                sprintf(sz, "set rollout late player %d movefilter", i);
                SetMovefilterCommands(sz, rw.rcRollout.aaamfLate[i], rcRollout.aaamfLate[i]);

            }

            if (EvalCmp(&rw.rcRollout.aecCubeTrunc, &rcRollout.aecCubeTrunc, 1)) {
                SetEvalCommands("set rollout truncation cube", &rw.rcRollout.aecCubeTrunc, &rcRollout.aecCubeTrunc);
            }

            if (EvalCmp(&rw.rcRollout.aecChequerTrunc, &rcRollout.aecChequerTrunc, 1)) {
                SetEvalCommands("set rollout truncation chequer",
                                &rw.rcRollout.aecChequerTrunc, &rcRollout.aecChequerTrunc);
            }

            if (rw.rcRollout.fCubeful != rcRollout.fCubeful) {
                sprintf(sz, "set rollout cubeful %s", rw.rcRollout.fCubeful ? "on" : "off");
                UserCommand(sz);
            }

            if (rw.rcRollout.fVarRedn != rcRollout.fVarRedn) {
                sprintf(sz, "set rollout varredn %s", rw.rcRollout.fVarRedn ? "on" : "off");
                UserCommand(sz);
            }

            if (rw.rcRollout.fInitial != rcRollout.fInitial) {
                sprintf(sz, "set rollout initial %s", rw.rcRollout.fInitial ? "on" : "off");
                UserCommand(sz);
            }

            if (rw.rcRollout.fRotate != rcRollout.fRotate) {
                sprintf(sz, "set rollout quasirandom %s", rw.rcRollout.fRotate ? "on" : "off");
                UserCommand(sz);
            }

            if (rw.rcRollout.fLateEvals != rcRollout.fLateEvals) {
                sprintf(sz, "set rollout late enable %s", rw.rcRollout.fLateEvals ? "on" : "off");
                UserCommand(sz);
            }

            if (rw.rcRollout.fDoTruncate != rcRollout.fDoTruncate) {
                sprintf(sz, "set rollout truncation enable %s", rw.rcRollout.fDoTruncate ? "on" : "off");
                UserCommand(sz);
            }

            if (rw.rcRollout.nTruncate != rcRollout.nTruncate) {
                sprintf(sz, "set rollout truncation plies %u", rw.rcRollout.nTruncate);
                UserCommand(sz);
            }

            if (rw.rcRollout.nTrials != rcRollout.nTrials) {
                sprintf(sz, "set rollout trials %u", rw.rcRollout.nTrials);
                UserCommand(sz);
            }

            if (rw.rcRollout.fStopOnSTD != rcRollout.fStopOnSTD) {
                sprintf(sz, "set rollout limit enable %s", rw.rcRollout.fStopOnSTD ? "on" : "off");
                UserCommand(sz);
            }

            if (rw.rcRollout.nMinimumGames != rcRollout.nMinimumGames) {
                sprintf(sz, "set rollout limit minimumgames %u", rw.rcRollout.nMinimumGames);
                UserCommand(sz);
            }

            if (fabsf(rw.rcRollout.rStdLimit - rcRollout.rStdLimit) > epsilon) {
                gchar buf[G_ASCII_DTOSTR_BUF_SIZE];
                sprintf(sz, "set rollout limit maxerr %s",
                        g_ascii_formatd(buf, G_ASCII_DTOSTR_BUF_SIZE, "%5.4f", rw.rcRollout.rStdLimit));
                UserCommand(sz);
            }

            /* ======================= */

            if (rw.rcRollout.fStopOnJsd != rcRollout.fStopOnJsd) {
                sprintf(sz, "set rollout jsd stop %s", rw.rcRollout.fStopOnJsd ? "on" : "off");
                UserCommand(sz);
            }

            if (rw.rcRollout.nMinimumJsdGames != rcRollout.nMinimumJsdGames) {
                sprintf(sz, "set rollout jsd minimumgames %u", rw.rcRollout.nMinimumJsdGames);
                UserCommand(sz);
            }

            if (fabsf(rw.rcRollout.rJsdLimit - rcRollout.rJsdLimit) > epsilon) {
                gchar buf[G_ASCII_DTOSTR_BUF_SIZE];
                sprintf(sz, "set rollout jsd limit %s",
                        g_ascii_formatd(buf, G_ASCII_DTOSTR_BUF_SIZE, "%5.4f", rw.rcRollout.rJsdLimit));
                UserCommand(sz);
            }


            if (rw.rcRollout.nLate != rcRollout.nLate) {
                sprintf(sz, "set rollout late plies %u", rw.rcRollout.nLate);
                UserCommand(sz);
            }

            if (rw.rcRollout.nSeed != rcRollout.nSeed) {
                sprintf(sz, "set rollout seed %lu", rw.rcRollout.nSeed);
                UserCommand(sz);
            }

            if (rw.rcRollout.fTruncBearoff2 != rcRollout.fTruncBearoff2) {
                sprintf(sz, "set rollout bearofftruncation exact %s", rw.rcRollout.fTruncBearoff2 ? "on" : "off");
                UserCommand(sz);
            }

            if (rw.rcRollout.fTruncBearoffOS != rcRollout.fTruncBearoffOS) {
                sprintf(sz, "set rollout bearofftruncation onesided %s", rw.rcRollout.fTruncBearoffOS ? "on" : "off");
                UserCommand(sz);
            }

            if (rw.fCubeEqualChequer != fCubeEqualChequer) {
                sprintf(sz, "set rollout cube-equal-chequer %s", rw.fCubeEqualChequer ? "on" : "off");
                UserCommand(sz);
            }

            if (rw.fPlayersAreSame != fPlayersAreSame) {
                sprintf(sz, "set rollout players-are-same %s", rw.fPlayersAreSame ? "on" : "off");
                UserCommand(sz);
            }

            if (rw.fTruncEqualPlayer0 != fTruncEqualPlayer0) {
                sprintf(sz, "set rollout truncate-equal-player0 %s", rw.fTruncEqualPlayer0 ? "on" : "off");
                UserCommand(sz);
            }

            UserCommand("save settings");
            outputon();
            if (saveAs)
                gtk_save_rollout_settings();
            else if (fOK)
                break;
        }
        if (loadRS)
            gtk_load_rollout_settings();
    }
}

void
GTKTextWindow(const char *szOutput, const char *title, const dialogtype type, GtkWidget * parent)
{

    GtkWidget *pwDialog = GTKCreateDialog(title, type, parent, 0, NULL, NULL);
    GtkWidget *pwText;
    GtkWidget *sw;
    GtkWidget *notice;
    GtkWidget *pwh, *pwv;
    GtkTextBuffer *buffer;
    GtkTextIter iter;
    GtkRequisition req;
    gchar *sz;

    GtkClipboard* clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    gtk_clipboard_set_text(clipboard, szOutput, -1);

    pwText = gtk_text_view_new();
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(pwText), GTK_WRAP_NONE);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(pwText), FALSE);

    buffer = gtk_text_buffer_new(NULL);
    gtk_text_buffer_create_tag(buffer, "monospace", "family", "monospace", NULL);
    gtk_text_buffer_get_end_iter(buffer, &iter);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, szOutput, -1, "monospace", NULL);
    gtk_text_view_set_buffer(GTK_TEXT_VIEW(pwText), buffer);

    sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(sw), pwText);

#if GTK_CHECK_VERSION(3,0,0)
    gtk_widget_get_preferred_size(GTK_WIDGET(pwText), &req, NULL);
#else
    gtk_widget_size_request(GTK_WIDGET(pwText), &req);
#endif

    sz = g_strdup_printf(_("%s copied to clipboard"), title);
    notice = gtk_label_new(sz);
    g_free(sz);

#if GTK_CHECK_VERSION(3,0,0)
    pwh = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    pwv = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
    pwh = gtk_hbox_new(FALSE, 0);
    pwv = gtk_vbox_new(FALSE, 0);
#endif
    gtk_box_pack_start(GTK_BOX(pwh), pwv, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(pwv), sw, TRUE, TRUE, 0);
    gtk_box_pack_end(GTK_BOX(pwv), notice, FALSE, FALSE, 0);

    gtk_window_set_default_size(GTK_WINDOW(pwDialog), -1, MIN(500, req.height + 200));

    gtk_box_pack_start(GTK_BOX(DialogArea(pwDialog, DA_MAIN)), pwh, TRUE, TRUE, 0);

    gtk_window_set_modal(GTK_WINDOW(pwDialog), TRUE);
    gtk_window_set_transient_for(GTK_WINDOW(pwDialog), GTK_WINDOW(pwMain));
    g_signal_connect(G_OBJECT(pwDialog), "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GTKRunDialog(pwDialog);
}

extern void
GTKEval(char *szOutput)
{
    GTKTextWindow(szOutput, _("GNU Backgammon - Evaluation"), DT_INFO, NULL);
}

static void
DestroyHint(gpointer p, GObject * UNUSED(obj))
{

    movelist *pml = p;

    if (pml) {
        if (pml->amMoves)
            g_free(pml->amMoves);

        g_free(pml);
    }

    SetPanelWidget(WINDOW_HINT, NULL);
}

static void
HintOK(GtkWidget * UNUSED(pw), void *UNUSED(unused))
{
    getWindowGeometry(WINDOW_HINT);
    DestroyPanel(WINDOW_HINT);
}

extern void
GTKCubeHint(moverecord * pmr, const matchstate * pms, int did_double, int did_take, int hist)
{

    GtkWidget *pw, *pwHint;

    if (GetPanelWidget(WINDOW_HINT))
        gtk_widget_destroy(GetPanelWidget(WINDOW_HINT));

    pwHint = GTKCreateDialog(_("GNU Backgammon - Hint"), DT_INFO, NULL, DIALOG_FLAG_NOTIDY, G_CALLBACK(HintOK), NULL);
    SetPanelWidget(WINDOW_HINT, pwHint);

    pw = CreateCubeAnalysis(pmr, pms, did_double, did_take, hist);

    gtk_container_add(GTK_CONTAINER(DialogArea(pwHint, DA_MAIN)), pw);

    gtk_widget_grab_focus(DialogArea(pwHint, DA_OK));

    setWindowGeometry(WINDOW_HINT);
    g_object_weak_ref(G_OBJECT(pwHint), DestroyHint, NULL);

    gtk_window_set_default_size(GTK_WINDOW(pwHint), 400, 300);

    gtk_widget_show_all(pwHint);
}

/*
 * Give hints for resignation
 *
 * Input:
 *    rEqBefore: equity before resignation
 *    rEqAfter: equity after resignation
 *    pci: cubeinfo
 *    fOUtputMWC: output in MWC or equity
 *
 * FIXME: Include arOutput in the dialog, so the the user
 *        can see how many gammons/backgammons she'll win.
 * FIXME: This only handles the acceptance / refusal side.
 *        Comment on the resignation itself as well.
 */

extern void
GTKResignHint(float UNUSED(arOutput[]), float rEqBefore, float rEqAfter, cubeinfo * pci, int fMWC)
{
    GtkWidget *pwDialog = GTKCreateDialog(_("GNU Backgammon - Hint"), DT_INFO,
                                          NULL, DIALOG_FLAG_MODAL, NULL, NULL);
    GtkWidget *pw;
#if GTK_CHECK_VERSION(3,0,0)
    GtkWidget *pwGrid;
#else
    GtkWidget *pwTable;
#endif
    char *pch, sz[16];

#if GTK_CHECK_VERSION(3,0,0)
    pwGrid = gtk_grid_new();
#else
    pwTable = gtk_table_new(2, 3, FALSE);
#endif

    /* equity before resignation */

#if GTK_CHECK_VERSION(3,0,0)
    gtk_grid_attach(GTK_GRID(pwGrid),  pw =
                     gtk_label_new(fMWC ? _("MWC before resignation") : _("Equity before resignation")), 0, 0, 1, 1);
    gtk_widget_set_hexpand(pw, TRUE);
    gtk_widget_set_vexpand(pw, TRUE);
#if GTK_CHECK_VERSION(3,12,0)
    gtk_widget_set_margin_start(pw, 4);
    gtk_widget_set_margin_end(pw, 4);
#else
    gtk_widget_set_margin_left(pw, 4);
    gtk_widget_set_margin_right(pw, 4);
#endif
    gtk_widget_set_halign(pw, GTK_ALIGN_START);
    gtk_widget_set_valign(pw, GTK_ALIGN_CENTER);
#else
    gtk_table_attach(GTK_TABLE(pwTable), pw =
                     gtk_label_new(fMWC ? _("MWC before resignation") : _("Equity before resignation")), 0, 1, 0, 1,
                     GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 4, 0);
    gtk_misc_set_alignment(GTK_MISC(pw), 0.0, 0.5);
#endif

    if (fMWC)
        sprintf(sz, "%6.2f%%", 100.0f * (eq2mwc(-rEqBefore, pci)));
    else
        sprintf(sz, "%+6.3f", -rEqBefore);

#if GTK_CHECK_VERSION(3,0,0)
    gtk_grid_attach(GTK_GRID(pwGrid), pw = gtk_label_new(sz), 1, 0, 1, 1);
    gtk_widget_set_hexpand(pw, TRUE);
    gtk_widget_set_vexpand(pw, TRUE);
#if GTK_CHECK_VERSION(3,12,0)
    gtk_widget_set_margin_start(pw, 4);
    gtk_widget_set_margin_end(pw, 4);
#else
    gtk_widget_set_margin_left(pw, 4);
    gtk_widget_set_margin_right(pw, 4);
#endif
    gtk_widget_set_halign(pw, GTK_ALIGN_END);
    gtk_widget_set_valign(pw, GTK_ALIGN_CENTER);
#else
    gtk_table_attach(GTK_TABLE(pwTable), pw = gtk_label_new(sz),
                     1, 2, 0, 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 4, 0);
    gtk_misc_set_alignment(GTK_MISC(pw), 1.0, 0.5);
#endif

    /* equity after resignation */

#if GTK_CHECK_VERSION(3,0,0)
    gtk_grid_attach(GTK_GRID(pwGrid),  pw =
                     gtk_label_new(fMWC ? _("MWC after resignation") : _("Equity after resignation")), 0, 1, 1, 1);
    gtk_widget_set_hexpand(pw, TRUE);
    gtk_widget_set_vexpand(pw, TRUE);
#if GTK_CHECK_VERSION(3,12,0)
    gtk_widget_set_margin_start(pw, 4);
    gtk_widget_set_margin_end(pw, 4);
#else
    gtk_widget_set_margin_left(pw, 4);
    gtk_widget_set_margin_right(pw, 4);
#endif
    gtk_widget_set_halign(pw, GTK_ALIGN_START);
    gtk_widget_set_valign(pw, GTK_ALIGN_CENTER);
#else
    gtk_table_attach(GTK_TABLE(pwTable), pw =
                     gtk_label_new(fMWC ? _("MWC after resignation") : _("Equity after resignation")), 0, 1, 1, 2,
                     GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 4, 0);
    gtk_misc_set_alignment(GTK_MISC(pw), 0.0, 0.5);
#endif

    if (fMWC)
        sprintf(sz, "%6.2f%%", 100.0f * eq2mwc(-rEqAfter, pci));
    else
        sprintf(sz, "%+6.3f", -rEqAfter);

#if GTK_CHECK_VERSION(3,0,0)
    gtk_grid_attach(GTK_GRID(pwGrid), pw = gtk_label_new(sz), 1, 1, 1, 1);
    gtk_widget_set_hexpand(pw, TRUE);
    gtk_widget_set_vexpand(pw, TRUE);
#if GTK_CHECK_VERSION(3,12,0)
    gtk_widget_set_margin_start(pw, 4);
    gtk_widget_set_margin_end(pw, 4);
#else
    gtk_widget_set_margin_left(pw, 4);
    gtk_widget_set_margin_right(pw, 4);
#endif
    gtk_widget_set_halign(pw, GTK_ALIGN_END);
    gtk_widget_set_valign(pw, GTK_ALIGN_CENTER);
#else
    gtk_table_attach(GTK_TABLE(pwTable), pw = gtk_label_new(sz),
                     1, 2, 1, 2, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 4, 0);
    gtk_misc_set_alignment(GTK_MISC(pw), 1.0, 0.5);
#endif

    if (-rEqAfter >= -rEqBefore)
        pch = _("You should accept the resignation!");
    else
        pch = _("You should reject the resignation!");

#if GTK_CHECK_VERSION(3,0,0)
    gtk_grid_attach(GTK_GRID(pwGrid), pw = gtk_label_new(pch), 0, 2, 2, 1);
    gtk_widget_set_hexpand(pw, TRUE);
    gtk_widget_set_vexpand(pw, TRUE);
    gtk_widget_set_margin_top(pw, 8);
    gtk_widget_set_margin_bottom(pw, 8);

    gtk_container_set_border_width(GTK_CONTAINER(pwGrid), 8);

    gtk_container_add(GTK_CONTAINER(DialogArea(pwDialog, DA_MAIN)), pwGrid);
#else
    gtk_table_attach(GTK_TABLE(pwTable), gtk_label_new(pch),
                     0, 2, 2, 3, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 8);

    gtk_container_set_border_width(GTK_CONTAINER(pwTable), 8);

    gtk_container_add(GTK_CONTAINER(DialogArea(pwDialog, DA_MAIN)), pwTable);
#endif

    GTKRunDialog(pwDialog);
}

extern void
GTKHint(moverecord * pmr, int hist)
{
    GtkWidget *pwMoves, *pwHint;
    if (!pmr || pmr->ml.cMoves < 1) {
        outputerrf(_("There are no legal moves. Figure it out yourself."));
        return;
    }

    if (GetPanelWidget(WINDOW_HINT))
        gtk_widget_destroy(GetPanelWidget(WINDOW_HINT));

    pwMoves = CreateMoveList(pmr, TRUE, TRUE, TRUE, hist);

    /* create dialog */

    pwHint = GTKCreateDialog(_("GNU Backgammon - Hint"), DT_INFO, NULL, DIALOG_FLAG_NONE, G_CALLBACK(HintOK), NULL);
    SetPanelWidget(WINDOW_HINT, pwHint);

    gtk_container_add(GTK_CONTAINER(DialogArea(pwHint, DA_MAIN)), pwMoves);

    setWindowGeometry(WINDOW_HINT);
    g_object_weak_ref(G_OBJECT(pwHint), DestroyHint, NULL);

    if (!IsPanelDocked(WINDOW_HINT))
        gtk_window_set_default_size(GTK_WINDOW(pwHint), 400, 300);

    gtk_widget_show_all(pwHint);
}

static void
SetMouseCursor(GdkCursorType cursorType)
{
    if (!GDK_IS_WINDOW(gtk_widget_get_window(pwMain))) {
        g_assert_not_reached();
        return;
    }
    if (cursorType) {
        GdkCursor *cursor;

        cursor = gdk_cursor_new_for_display(gtk_widget_get_display(pwMain), cursorType);
        gdk_window_set_cursor(gtk_widget_get_window(pwMain), cursor);
#if GTK_CHECK_VERSION(3,0,0)
        g_object_unref(cursor);
#else
        gdk_cursor_unref(cursor);
#endif
    } else
        gdk_window_set_cursor(gtk_widget_get_window(pwMain), NULL);
}

extern void
GTKProgressStart(const char *sz)
{
    GTKSuspendInput();

    if (sz)
        gtk_statusbar_push(GTK_STATUSBAR(pwStatus), idProgress, sz);

    SetMouseCursor(GDK_WATCH);
}

extern void
GTKProgressStartValue(char *sz, int UNUSED(iMax))
{
    GTKSuspendInput();

    if (sz)
        gtk_statusbar_push(GTK_STATUSBAR(pwStatus), idProgress, sz);

    SetMouseCursor(GDK_WATCH);
}

extern void
GTKProgressValue(int iValue, int iMax)
{
    gchar *gsz;
    gdouble frac = 1.0 * iValue / (1.0 * iMax);
    gsz = g_strdup_printf("%d/%d (%.0f%%)", iValue, iMax, 100 * frac);
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(pwProgress), gsz);
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(pwProgress), frac);
    g_free(gsz);

    ProcessEvents();
}

extern void
GTKProgress(void)
{
    gtk_progress_bar_pulse(GTK_PROGRESS_BAR(pwProgress));

    ProcessEvents();
}

extern void
GTKProgressEnd(void)
{
    GTKResumeInput();

    if (!pwProgress)            /*safe guard on language change */
        return;

    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(pwProgress), 0.0);
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(pwProgress), " ");
    gtk_statusbar_pop(GTK_STATUSBAR(pwStatus), idProgress);

    SetMouseCursor(GDK_X_CURSOR);
}

extern void
GTKShowScoreSheet(void)
{
    GtkWidget *pwDialog, *pwBox;
    GtkWidget *hbox;
    GtkWidget *view;
    GtkWidget *pwScrolled;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkListStore *store;
    char title[100];
    listOLD *pl;

    sprintf(title, "%s - ", _("Score Sheet"));
    if (ms.nMatchTo > 0)
        sprintf(title + strlen(title), ngettext("%d point match", "%d points match", ms.nMatchTo), ms.nMatchTo);
    else
        strcat(title, _("Money Session"));

    pwDialog = GTKCreateDialog(title, DT_INFO, NULL, DIALOG_FLAG_MODAL, NULL, NULL);
#if GTK_CHECK_VERSION(3,0,0)
    pwBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
    pwBox = gtk_vbox_new(FALSE, 0);
#endif
    gtk_container_set_border_width(GTK_CONTAINER(pwBox), 8);

    gtk_container_add(GTK_CONTAINER(DialogArea(pwDialog, DA_MAIN)), pwBox);

    gtk_container_set_border_width(GTK_CONTAINER(DialogArea(pwDialog, DA_MAIN)), 4);

#if GTK_CHECK_VERSION(3,0,0)
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
#else
    hbox = gtk_hbox_new(FALSE, 0);
#endif
    gtk_container_add(GTK_CONTAINER(DialogArea(pwDialog, DA_MAIN)), hbox);

    store = gtk_list_store_new(2, G_TYPE_INT, G_TYPE_INT);

    for (pl = lMatch.plNext; pl->p; pl = pl->plNext) {
        int score[2];
        GtkTreeIter iter;
        listOLD *plg = pl->plNext->p;

        if (plg) {
            moverecord *pmr = plg->plNext->p;
            score[0] = pmr->g.anScore[0];
            score[1] = pmr->g.anScore[1];
        } else {
            moverecord *pmr;

            plg = pl->p;
            if (!plg)
                continue;

            pmr = plg->plNext->p;
            score[0] = pmr->g.anScore[0];
            score[1] = pmr->g.anScore[1];
            if (pmr->g.fWinner == -1) {
                if (pl == lMatch.plNext) {      /* First game */
                    score[0] = score[1] = 0;
                } else
                    continue;
            } else
                score[pmr->g.fWinner] += pmr->g.nPoints;
        }
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter, 0, score[0], 1, score[1], -1);
    }

    pwScrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pwScrolled), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

    view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    g_object_unref(store);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(ap[0].szName, renderer, "text", 0, NULL);
    gtk_tree_view_column_set_min_width(column, 75);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(ap[1].szName, renderer, "text", 1, NULL);
    gtk_tree_view_column_set_min_width(column, 75);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);

    gtk_container_add(GTK_CONTAINER(pwScrolled), view);
    gtk_box_pack_start(GTK_BOX(hbox), pwScrolled, TRUE, TRUE, 0);

    GTKRunDialog(pwDialog);
}

static void
GtkShowCopying(GtkWidget * parent)
{
    ShowList(aszCopying, _("Copying"), parent);
}

static void
GtkShowWarranty(GtkWidget * parent)
{
    ShowList(aszWarranty, _("Warranty"), parent);
}

static void
GtkShowEngine(GtkWidget * parent)
{
    char *szBuffer[2];
    szBuffer[0] = g_new0(char, 4096);
    szBuffer[1] = NULL;
    EvalStatus(szBuffer[0]);
    ShowList(szBuffer, _("Evaluation Engine"), parent);
    g_free(szBuffer[0]);
}

extern void
GTKShowVersion(void)
{
    GtkWidget *pwDialog, *pwButtonBox, *pwButton;
    GtkWidget *image;
    gchar *fn;

    pwDialog =
        GTKCreateDialog(_("About GNU Backgammon"), DT_CUSTOM, NULL, DIALOG_FLAG_MODAL | DIALOG_FLAG_CLOSEBUTTON, NULL,
                        NULL);
    gtk_window_set_resizable(GTK_WINDOW(pwDialog), FALSE);

    fn = g_build_filename(getPkgDataDir(), "pixmaps", "gnubg-big.png", NULL);
    image = gtk_image_new_from_file(fn);
    g_free(fn);
#if GTK_CHECK_VERSION(3,0,0)
    g_object_set(image, "margin", 8, NULL);
#else
    gtk_misc_set_padding(GTK_MISC(image), 8, 8);
#endif
    gtk_box_pack_start(GTK_BOX(DialogArea(pwDialog, DA_MAIN)), image, FALSE, FALSE, 0);

    /* Buttons on right side */
#if GTK_CHECK_VERSION(3,0,0)
    pwButtonBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
    pwButtonBox = gtk_vbox_new(FALSE, 0);
#endif
    gtk_box_pack_start(GTK_BOX(DialogArea(pwDialog, DA_MAIN)), pwButtonBox, FALSE, FALSE, 8);

    gtk_box_pack_start(GTK_BOX(pwButtonBox), pwButton = gtk_button_new_with_label(_("Credits")), FALSE, FALSE, 8);
    g_signal_connect(G_OBJECT(pwButton), "clicked", G_CALLBACK(GTKCommandShowCredits), pwDialog);

    gtk_box_pack_start(GTK_BOX(pwButtonBox), pwButton = gtk_button_new_with_label(_("Build Info")), FALSE, FALSE, 8);
    g_signal_connect(G_OBJECT(pwButton), "clicked", G_CALLBACK(GTKShowBuildInfo), pwDialog);

    gtk_box_pack_start(GTK_BOX(pwButtonBox),
                       pwButton = gtk_button_new_with_label(_("Copying conditions")), FALSE, FALSE, 8);
    g_signal_connect(G_OBJECT(pwButton), "clicked", G_CALLBACK(GtkShowCopying), pwDialog);

    gtk_box_pack_start(GTK_BOX(pwButtonBox), pwButton = gtk_button_new_with_label(_("Warranty")), FALSE, FALSE, 8);
    g_signal_connect(G_OBJECT(pwButton), "clicked", G_CALLBACK(GtkShowWarranty), NULL);

    gtk_box_pack_start(GTK_BOX(pwButtonBox), pwButton = gtk_button_new_with_label(_("Report Bug")), FALSE, FALSE, 8);
    g_signal_connect(G_OBJECT(pwButton), "clicked", G_CALLBACK(ReportBug), NULL);

    gtk_box_pack_start(GTK_BOX(pwButtonBox),
                       pwButton = gtk_button_new_with_label(_("Evaluation Engine")), FALSE, FALSE, 8);
    g_signal_connect(G_OBJECT(pwButton), "clicked", G_CALLBACK(GtkShowEngine), pwDialog);
    GTKRunDialog(pwDialog);
}

static GtkWidget *
SelectableLabel(GtkWidget * UNUSED(reference), const char *text)
{
    GtkWidget *pwLabel = gtk_label_new(text);
    gtk_label_set_selectable(GTK_LABEL(pwLabel), TRUE);
    return pwLabel;
}

extern void
GTKShowBuildInfo(GtkWidget * UNUSED(pw), GtkWidget * pwParent)
{
    GtkWidget *pwDialog, *pwBox, *pwPrompt;
    const char *pch;

    pwDialog = GTKCreateDialog(_("GNU Backgammon - Build Info"), DT_INFO, pwParent, DIALOG_FLAG_MODAL, NULL, NULL);
#if GTK_CHECK_VERSION(3,0,0)
    pwBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
    pwBox = gtk_vbox_new(FALSE, 0);
#endif
    gtk_container_set_border_width(GTK_CONTAINER(pwBox), 8);

    gtk_container_add(GTK_CONTAINER(DialogArea(pwDialog, DA_MAIN)), pwBox);

    gtk_box_pack_start(GTK_BOX(pwBox), SelectableLabel(pwDialog, VERSION_STRING), FALSE, FALSE, 4);

#if GTK_CHECK_VERSION(3,0,0)
    gtk_box_pack_start(GTK_BOX(pwBox), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, 4);
#else
    gtk_box_pack_start(GTK_BOX(pwBox), gtk_hseparator_new(), FALSE, FALSE, 4);
#endif

    while ((pch = GetBuildInfoString()))
        gtk_box_pack_start(GTK_BOX(pwBox), gtk_label_new(gettext(pch)), FALSE, FALSE, 0);

#if GTK_CHECK_VERSION(3,0,0)
    gtk_box_pack_start(GTK_BOX(pwBox), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, 4);
#else
    gtk_box_pack_start(GTK_BOX(pwBox), gtk_hseparator_new(), FALSE, FALSE, 4);
#endif

    gtk_box_pack_start(GTK_BOX(pwBox), gtk_label_new(_(aszCOPYRIGHT)), FALSE, FALSE, 4);

    pwPrompt = gtk_label_new(_(intro_string));
    gtk_box_pack_start(GTK_BOX(pwBox), pwPrompt, FALSE, FALSE, 4);
    gtk_label_set_line_wrap(GTK_LABEL(pwPrompt), TRUE);

    GTKRunDialog(pwDialog);
}

/* Stores names in credits so not duplicated in list at bottom */
static listOLD names;

static void
AddTitle(GtkWidget * pwBox, char *Title)
{
    GtkRcStyle *ps = gtk_rc_style_new();
    GtkWidget *pwTitle = gtk_label_new(Title), *pwHBox;

#if GTK_CHECK_VERSION(3,0,0)
    pwHBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
#else
    pwHBox = gtk_hbox_new(FALSE, 0);
#endif
    gtk_box_pack_start(GTK_BOX(pwBox), pwHBox, FALSE, FALSE, 4);

    ps->font_desc = pango_font_description_new();
    pango_font_description_set_family_static(ps->font_desc, "serif");
    pango_font_description_set_size(ps->font_desc, 16 * PANGO_SCALE);
    gtk_widget_modify_style(pwTitle, ps);
    g_object_unref(ps);

    gtk_box_pack_start(GTK_BOX(pwHBox), pwTitle, TRUE, FALSE, 0);
}

static void
AddName(GtkWidget * pwBox, char *name, const char *type)
{
    char buf[255];
    if (type)
        sprintf(buf, "%s: %s", type, name);
    else
        strcpy(buf, name);

    gtk_box_pack_start(GTK_BOX(pwBox), gtk_label_new(buf), FALSE, FALSE, 0);
    ListInsert(&names, name);
}

static int
FindName(listOLD * pList, const char *name)
{
    listOLD *pl;
    for (pl = pList->plNext; pl != pList; pl = pl->plNext) {
        if (!strcmp(pl->p, name))
            return TRUE;
    }
    return FALSE;
}

extern void
GTKCommandShowCredits(GtkWidget * UNUSED(pw), GtkWidget * pwParent)
{
    GtkWidget *pwDialog;
    GtkWidget *pwBox;
    GtkWidget *pwMainHBox;
    GtkWidget *pwHBox = 0;
    GtkWidget *pwVBox;
    GtkWidget *pwScrolled;
    GtkWidget *treeview;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkListStore *store;
    GtkTreeIter iter;
    int i = 0;
    credits *credit = &creditList[0];

    pwScrolled = gtk_scrolled_window_new(NULL, NULL);
    ListCreate(&names);

    pwDialog = GTKCreateDialog(_("GNU Backgammon - Credits"), DT_INFO, pwParent, DIALOG_FLAG_MODAL, NULL, NULL);

#if GTK_CHECK_VERSION(3,0,0)
    pwMainHBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
#else
    pwMainHBox = gtk_hbox_new(FALSE, 0);
#endif

    gtk_container_add(GTK_CONTAINER(DialogArea(pwDialog, DA_MAIN)), pwMainHBox);

#if GTK_CHECK_VERSION(3,0,0)
    pwBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
    pwBox = gtk_vbox_new(FALSE, 0);
#endif
    gtk_box_pack_start(GTK_BOX(pwMainHBox), pwBox, FALSE, FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(pwBox), 8);

    while (credit->Title) {
        credEntry *ce;

        /* Two columns, so new hbox every-other one */
        if (i % 2 == 0) {
#if GTK_CHECK_VERSION(3,0,0)
            pwHBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
#else
            pwHBox = gtk_hbox_new(FALSE, 0);
#endif
            gtk_box_pack_start(GTK_BOX(pwBox), pwHBox, TRUE, FALSE, 0);
        }
#if GTK_CHECK_VERSION(3,0,0)
        pwVBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
        pwVBox = gtk_vbox_new(FALSE, 0);
#endif
        gtk_box_pack_start(GTK_BOX(pwHBox), pwVBox, TRUE, FALSE, 0);

        AddTitle(pwVBox, _(credit->Title));

        ce = credit->Entry;
        while (ce->Name) {
            AddName(pwVBox, ce->Name, _(ce->Type));
            ce++;
        }
        if (i == 1)
#if GTK_CHECK_VERSION(3,0,0)
            gtk_box_pack_start(GTK_BOX(pwBox), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, 4);
#else
            gtk_box_pack_start(GTK_BOX(pwBox), gtk_hseparator_new(), FALSE, FALSE, 4);
#endif
        credit++;
        i++;
    }

#if GTK_CHECK_VERSION(3,0,0)
    pwVBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
    pwVBox = gtk_vbox_new(FALSE, 0);
#endif
    gtk_box_pack_start(GTK_BOX(pwMainHBox), pwVBox, FALSE, FALSE, 0);

    AddTitle(pwVBox, _("Special thanks"));

    /* create list store */
    store = gtk_list_store_new(1, G_TYPE_STRING);

    /* add data to the list store */
    for (i = 0; ceCredits[i].Name; i++) {
        if (!FindName(&names, ceCredits[i].Name)) {
            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store, &iter, 0, ceCredits[i].Name, -1);
        }
    }

    /* create tree view */
    treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    g_object_unref(store);
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(_("Contributors"), renderer, "text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

    while (names.plNext->p)
        ListDelete(names.plNext);
    gtk_container_set_border_width(GTK_CONTAINER(pwVBox), 8);
    gtk_box_pack_start(GTK_BOX(pwVBox), pwScrolled, TRUE, TRUE, 0);
    gtk_widget_set_size_request(pwScrolled, 150, -1);
#if GTK_CHECK_VERSION(3, 8, 0)
    gtk_container_add(GTK_CONTAINER(pwScrolled), treeview);
#else
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(pwScrolled), treeview);
#endif
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pwScrolled), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);

    GTKRunDialog(pwDialog);
}

static void
GTKHelpAdd(GtkTreeStore * pts, GtkTreeIter * ptiParent, command * pc)
{
    GtkTreeIter ti;

    for (; pc->sz; pc++)
        if (pc->szHelp) {
            gtk_tree_store_append(pts, &ti, ptiParent);
            gtk_tree_store_set(pts, &ti, 0, pc->sz, 1, Q_(pc->szHelp), 2, pc, -1);
            if (pc->pc && pc->pc->sz)
                GTKHelpAdd(pts, &ti, pc->pc);
        }
}

static void
GTKHelpSelect(GtkTreeSelection * pts, gpointer UNUSED(p))
{
    GtkTreeModel *ptm;
    GtkTreeIter ti;
    command **apc;
    char szCommand[128], *pchCommand = szCommand, szUsage[128], *pchUsage = szUsage;

    if (gtk_tree_selection_get_selected(pts, &ptm, &ti)) {
        char *pLabel;

        GtkTreePath *ptp = gtk_tree_model_get_path(ptm, &ti);
        int c = gtk_tree_path_get_depth(ptp);

        apc = g_malloc(c * sizeof(command *));
        for (int i = c - 1;; i--) {
            gtk_tree_model_get(ptm, &ti, 2, apc + i, -1);
            if (!i)
                break;
            gtk_tree_path_up(ptp);
            gtk_tree_model_get_iter(ptm, &ti, ptp);
        }

        for (int i = 0; i < c; i++) {
            /* accumulate command and usage strings from path */
            /* FIXME use markup a la gtk_label_set_markup for this */
            const char *pch = apc[i]->sz;
            while (*pch)
                *pchCommand++ = *pchUsage++ = *pch++;
            *pchCommand++ = ' ';
            *pchCommand = 0;
            *pchUsage++ = ' ';
            *pchUsage = 0;

            if ((pch = apc[i]->szUsage)) {
                while (*pch)
                    *pchUsage++ = *pch++;
                *pchUsage++ = ' ';
                *pchUsage = 0;
            }
        }

/* Use szUsage as is, at this point it has already been translated by GTKHelpAdd() */

        pLabel = g_strdup_printf("%s - %s\n\n%s %s%s\n", szCommand,
                                 gettext(apc[c - 1]->szHelp), _("Usage:"), szUsage,
                                 (apc[c - 1]->pc && apc[c - 1]->pc->sz) ? _(" <subcommand>") : "");
        gtk_label_set_text(GTK_LABEL(pwHelpLabel), pLabel);
        g_free(pLabel);

        g_free(apc);
        gtk_tree_path_free(ptp);
    } else
        gtk_label_set_text(GTK_LABEL(pwHelpLabel), NULL);
}

extern void
GTKHelp(char *sz)
{
    static GtkWidget *pw = NULL;
    GtkWidget *pwPaned, *pwScrolled;
    GtkTreeStore *pts;
    GtkTreeIter ti, tiSearch;
    GtkTreePath *ptp, *ptpExpand;
    GtkTreeSelection *treeSelection;
    char *pch;
    command *pc, *pcTest, *pcStart;
    int i, c, *pn;
    void (*pf) (char *);

    if (pw) {
        gtk_window_present(GTK_WINDOW(pw));
        return;
    }

    pts = gtk_tree_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER);

    GTKHelpAdd(pts, NULL, acTop);

    pw = GTKCreateDialog(_("Help - command reference"), DT_INFO, NULL, DIALOG_FLAG_NONE, NULL, NULL);
    g_object_add_weak_pointer(G_OBJECT(pw), (void *) &pw);
    gtk_window_set_title(GTK_WINDOW(pw), _("Help - command reference"));
    gtk_window_set_default_size(GTK_WINDOW(pw), 500, 400);

    g_signal_connect_swapped(pw, "response", G_CALLBACK(gtk_widget_destroy), pw);

#if GTK_CHECK_VERSION(3,0,0)
    pwPaned = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
#else
    pwPaned = gtk_vpaned_new();
#endif

    gtk_container_add(GTK_CONTAINER(DialogArea(pw, DA_MAIN)), pwPaned);

    gtk_paned_pack1(GTK_PANED(pwPaned), pwScrolled = gtk_scrolled_window_new(NULL, NULL), TRUE, FALSE);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(pwScrolled), GTK_SHADOW_IN);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pwScrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(pwScrolled), pwHelpTree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(pts)));
    treeSelection = gtk_tree_view_get_selection(GTK_TREE_VIEW(pwHelpTree));

    g_object_unref(G_OBJECT(pts));
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(pwHelpTree), FALSE);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(pwHelpTree), 0, NULL, gtk_cell_renderer_text_new(),
                                                "text", 0, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(pwHelpTree), 1, NULL, gtk_cell_renderer_text_new(),
                                                "text", 1, NULL);
    g_signal_connect(G_OBJECT(treeSelection), "changed", G_CALLBACK(GTKHelpSelect), NULL);

    gtk_paned_pack2(GTK_PANED(pwPaned), pwHelpLabel = gtk_label_new(NULL), FALSE, FALSE);
    gtk_label_set_selectable(GTK_LABEL(pwHelpLabel), TRUE);

    gtk_widget_show_all(pw);

    gtk_tree_model_get_iter_first(GTK_TREE_MODEL(pts), &ti);
    tiSearch = ti;
    pc = acTop;
    c = 0;
    while (pc && sz && (pch = NextToken(&sz))) {
        size_t cch;

        pcStart = pc;
        cch = strlen(pch);
        for (; pc->sz; pc++)
            if (!StrNCaseCmp(pch, pc->sz, cch))
                break;

        if (!pc->sz)
            break;

        if (!pc->szHelp) {
            /* they gave a synonym; find the canonical version */
            pf = pc->pf;
            for (pc = pcStart; pc->sz; pc++)
                if (pc->pf == pf && pc->szHelp)
                    break;

            if (!pc->sz)
                break;
        }

        do
            gtk_tree_model_get(GTK_TREE_MODEL(pts), &tiSearch, 2, &pcTest, -1);
        while (pcTest != pc && gtk_tree_model_iter_next(GTK_TREE_MODEL(pts), &tiSearch));

        if (pcTest == pc) {
            /* found!  now try the next level down... */
            c++;
            ti = tiSearch;
            pc = pc->pc;
            gtk_tree_model_iter_children(GTK_TREE_MODEL(pts), &tiSearch, &ti);
        } else
            break;
    }

    ptp = gtk_tree_model_get_path(GTK_TREE_MODEL(pts), &ti);
    pn = gtk_tree_path_get_indices(ptp);
    ptpExpand = gtk_tree_path_new();
    for (i = 0; i < c; i++) {
        gtk_tree_path_append_index(ptpExpand, pn[i]);
        gtk_tree_view_expand_row(GTK_TREE_VIEW(pwHelpTree), ptpExpand, FALSE);
    }
    gtk_tree_selection_select_iter(treeSelection, &ti);
    gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(pwHelpTree), ptp, NULL, TRUE, 0.5, 0);
    gtk_tree_view_set_cursor(GTK_TREE_VIEW(pwHelpTree), ptp, NULL, FALSE);
    gtk_tree_path_free(ptp);
    gtk_tree_path_free(ptpExpand);
}

static void
GTKBearoffProgressCancel(void)
{
    raise(SIGINT);
    exit(EXIT_FAILURE);
}

/* Show a dialog box with a progress bar to be used during initialisation
 * if a heuristic bearoff database must be created.  Most of GNUbg hasn't
 * been initialised yet, so this function is restricted in many ways. */
extern void
GTKBearoffProgress(int i)
{

    static GtkWidget *pwDialog, *pw;
    gchar *gsz;

    if (!pwDialog) {
#if !GTK_CHECK_VERSION(3,0,0)
        static GtkWidget *pwAlign;
#endif

        pwDialog =
            GTKCreateDialog(_("GNU Backgammon"), DT_INFO, NULL, DIALOG_FLAG_MODAL | DIALOG_FLAG_NOTIDY, NULL, NULL);
        gtk_window_set_role(GTK_WINDOW(pwDialog), "progress");
        gtk_window_set_type_hint(GTK_WINDOW(pwDialog), GDK_WINDOW_TYPE_HINT_DIALOG);
        g_signal_connect(G_OBJECT(pwDialog), "destroy", G_CALLBACK(GTKBearoffProgressCancel), NULL);

#if GTK_CHECK_VERSION(3,0,0)
        gtk_box_pack_start(GTK_BOX(DialogArea(pwDialog, DA_MAIN)),
		           pw = gtk_progress_bar_new(), TRUE, TRUE, 8);
        gtk_widget_set_halign(pw, GTK_ALIGN_FILL);
        gtk_widget_set_valign(pw, GTK_ALIGN_CENTER);
#else
        gtk_box_pack_start(GTK_BOX(DialogArea(pwDialog, DA_MAIN)),
                           pwAlign = gtk_alignment_new(0.5, 0.5, 1, 0), TRUE, TRUE, 8);
        gtk_container_add(GTK_CONTAINER(pwAlign), pw = gtk_progress_bar_new());
#endif

        gtk_widget_show_all(pwDialog);
    }

    gsz = g_strdup_printf(_("Generating bearoff database (%.0f %%)"), i / 542.64);
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(pw), gsz);
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(pw), i / 54264.0);
    g_free(gsz);

    if (i >= 54000) {
        g_signal_handlers_disconnect_by_func(G_OBJECT(pwDialog), (gpointer) G_CALLBACK(GTKBearoffProgressCancel), NULL);

        gtk_widget_destroy(pwDialog);
    }

    ProcessEvents();
}

static void enable_sub_menu(GtkWidget * pw, int f);     /* for recursion */

static void
enable_menu(GtkWidget * pw, int f)
{

    GtkMenuItem *pmi = GTK_MENU_ITEM(pw);

    if (gtk_menu_item_get_submenu(pmi))
        enable_sub_menu(gtk_menu_item_get_submenu(pmi), f);
    else
        gtk_widget_set_sensitive(pw, f);
}

static inline void
my_enable_menu(gpointer data, gpointer user_data)
{
    enable_menu((GtkWidget *) data, GPOINTER_TO_INT(user_data));
}

static void
enable_sub_menu(GtkWidget * pw, int f)
{
    GList *pl;

    GtkMenuShell *pms = GTK_MENU_SHELL(pw);
    g_list_foreach(pl = gtk_container_get_children(GTK_CONTAINER(pms)), my_enable_menu, GINT_TO_POINTER(f));

    g_list_free(pl);
}

/* A global setting has changed; update entry in Settings menu if necessary. */
extern void
GTKSet(void *p)
{

    BoardData *bd = BOARD(pwBoard)->board_data;

    if (p == ap) {
        /* Handle the player names. */
#if !defined(USE_GTKITEMFACTORY)
        gtk_label_set_text(GTK_LABEL
                           (gtk_bin_get_child
                            (GTK_BIN(gtk_ui_manager_get_widget(puim, "/MainMenu/GameMenu/SetTurnMenu/SetTurnPlayer0")
                             ))), (ap[0].szName));
        gtk_label_set_text(GTK_LABEL
                           (gtk_bin_get_child
                            (GTK_BIN(gtk_ui_manager_get_widget(puim, "/MainMenu/GameMenu/SetTurnMenu/SetTurnPlayer1")
                             ))), (ap[1].szName));
#else
        gtk_label_set_text(GTK_LABEL
                           (gtk_bin_get_child(GTK_BIN(gtk_item_factory_get_widget_by_action(pif, CMD_SET_TURN_0)
                                              ))), (ap[0].szName));
        gtk_label_set_text(GTK_LABEL
                           (gtk_bin_get_child(GTK_BIN(gtk_item_factory_get_widget_by_action(pif, CMD_SET_TURN_1)
                                              ))), (ap[1].szName));
#endif
        GL_SetNames();

        GTKRegenerateGames();

    } else if (p == &ms.fJacoby) {
        bd->jacoby_flag = ms.fJacoby;
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(bd->jacoby), bd->jacoby_flag);
        ShowBoard();

    } else if (p == &ms.fTurn) {
        /* Handle the player on roll. */
        fAutoCommand = TRUE;
#if !defined(USE_GTKITEMFACTORY)
        if (ms.fTurn >= 0) {
            if (ms.fTurn)
                gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_ui_manager_get_widget(puim,
                                                                                             "/MainMenu/GameMenu/SetTurnMenu/SetTurnPlayer0")),
                                               TRUE);
            else
                gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_ui_manager_get_widget(puim,
                                                                                             "/MainMenu/GameMenu/SetTurnMenu/SetTurnPlayer1")),
                                               TRUE);
            enable_menu(gtk_ui_manager_get_widget(puim, "/MainMenu/GameMenu/Roll"), ms.fMove == ms.fTurn
                        && ap[ms.fMove].pt == PLAYER_HUMAN);
        }
#else

        if (ms.fTurn >= 0) {
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM
                                           (gtk_item_factory_get_widget_by_action(pif, CMD_SET_TURN_0 + ms.fTurn)),
                                           TRUE);

            enable_menu(gtk_item_factory_get_widget(pif, "/Game/Roll"),
                        ms.fMove == ms.fTurn && ap[ms.fMove].pt == PLAYER_HUMAN);
        }
#endif
        fAutoCommand = FALSE;
    } else if (p == &ms.gs) {
        /* Handle the game state. */
        fAutoCommand = TRUE;

        board_set_playing(BOARD(pwBoard), plGame != NULL);
        ToolbarSetPlaying(pwToolbar, plGame != NULL);

#if !defined(USE_GTKITEMFACTORY)
        gtk_widget_set_sensitive(gtk_ui_manager_get_widget(puim, "/MainMenu/FileMenu/Save"), plGame != NULL);
        enable_menu(gtk_ui_manager_get_widget(puim, "/MainMenu/GameMenu"), ms.gs == GAME_PLAYING);
        if (ms.fTurn >= 0)
            enable_menu(gtk_ui_manager_get_widget(puim,
                                                  "/MainMenu/GameMenu/Roll"),
                        ms.fMove == ms.fTurn && ap[ms.fMove].pt == PLAYER_HUMAN);

        gtk_widget_set_sensitive(gtk_ui_manager_get_widget(puim, "/MainMenu/GoMenu/NextRoll"), plGame != NULL);
        gtk_widget_set_sensitive(gtk_ui_manager_get_widget(puim, "/MainMenu/GoMenu/PreviousRoll"), plGame != NULL);
        gtk_widget_set_sensitive(gtk_ui_manager_get_widget(puim, "/MainMenu/GoMenu/NextMarkedMove"), plGame != NULL);
        gtk_widget_set_sensitive(gtk_ui_manager_get_widget(puim,
                                                           "/MainMenu/GoMenu/PreviousMarkedMove"), plGame != NULL);
        gtk_widget_set_sensitive(gtk_ui_manager_get_widget(puim, "/MainMenu/GoMenu/NextGame"), plGame != NULL);
        gtk_widget_set_sensitive(gtk_ui_manager_get_widget(puim, "/MainMenu/GoMenu/PreviousGame"), plGame != NULL);
        gtk_widget_set_sensitive(gtk_ui_manager_get_widget(puim, "/MainMenu/GoMenu/NextCMarkedMove"), plGame != NULL);
        gtk_widget_set_sensitive(gtk_ui_manager_get_widget(puim,
                                                           "/MainMenu/GoMenu/PreviousCMarkedMove"), plGame != NULL);

        gtk_widget_set_sensitive(gtk_ui_manager_get_widget(puim, "/MainMenu/FileMenu/MatchInfo"), !ListEmpty(&lMatch));
        enable_menu(gtk_ui_manager_get_widget(puim, "/MainMenu/AnalyseMenu"), ms.gs == GAME_PLAYING);
        gtk_widget_set_sensitive(gtk_ui_manager_get_widget(puim, "/MainMenu/AnalyseMenu/AnalyseFile"), TRUE);
        gtk_widget_set_sensitive(gtk_ui_manager_get_widget(puim, "/MainMenu/AnalyseMenu/BatchAnalyse"), TRUE);

        gtk_widget_set_sensitive(gtk_ui_manager_get_widget(puim,
                                                           "/MainMenu/AnalyseMenu/AnalyseMove"),
                                 plLastMove && plLastMove->plNext && plLastMove->plNext->p);
        gtk_widget_set_sensitive(gtk_ui_manager_get_widget(puim, "/MainMenu/AnalyseMenu/AnalyseGame"), plGame != NULL);
        gtk_widget_set_sensitive(gtk_ui_manager_get_widget(puim,
                                                           "/MainMenu/AnalyseMenu/AnalyseMatch"), !ListEmpty(&lMatch));

        gtk_widget_set_sensitive(gtk_ui_manager_get_widget(puim,
                                                           "/MainMenu/AnalyseMenu/ClearAnalysisMenu/Move"),
                                 plLastMove && plLastMove->plNext && plLastMove->plNext->p);
        gtk_widget_set_sensitive(gtk_ui_manager_get_widget(puim,
                                                           "/MainMenu/AnalyseMenu/ClearAnalysisMenu/Game"),
                                 plGame != NULL);
        gtk_widget_set_sensitive(gtk_ui_manager_get_widget
                                 (puim, "/MainMenu/AnalyseMenu/ClearAnalysisMenu/MatchOrSession"), !ListEmpty(&lMatch));

        gtk_widget_set_sensitive(gtk_ui_manager_get_widget(puim,
                                                           "/MainMenu/AnalyseMenu/MatchOrSessionStats"),
                                 !ListEmpty(&lMatch));
        gtk_widget_set_sensitive(gtk_ui_manager_get_widget(puim, "/MainMenu/AnalyseMenu/MatchEquityTable"), TRUE);
        gtk_widget_set_sensitive(gtk_ui_manager_get_widget(puim, "/MainMenu/AnalyseMenu/EvaluationSpeed"), TRUE);
        gtk_widget_set_sensitive(gtk_ui_manager_get_widget(puim,
                                                           "/MainMenu/GameMenu/SwapPlayers"), !ListEmpty(&lMatch));

        gtk_widget_set_sensitive(gtk_ui_manager_get_widget(puim,
                                                           "/MainMenu/AnalyseMenu/CMarkMenu/CMarkCubeMenu/Clear"),
                                 !ListEmpty(&lMatch));
        gtk_widget_set_sensitive(gtk_ui_manager_get_widget(puim,
                                                           "/MainMenu/AnalyseMenu/CMarkMenu/CMarkCubeMenu/Show"),
                                 !ListEmpty(&lMatch));
        gtk_widget_set_sensitive(gtk_ui_manager_get_widget(puim,
                                                           "/MainMenu/AnalyseMenu/CMarkMenu/CMarkMoveMenu/Clear"),
                                 !ListEmpty(&lMatch));
        gtk_widget_set_sensitive(gtk_ui_manager_get_widget(puim,
                                                           "/MainMenu/AnalyseMenu/CMarkMenu/CMarkMoveMenu/Show"),
                                 !ListEmpty(&lMatch));
        gtk_widget_set_sensitive(gtk_ui_manager_get_widget(puim,
                                                           "/MainMenu/AnalyseMenu/CMarkMenu/CMarkGameMenu/Clear"),
                                 !ListEmpty(&lMatch));
        gtk_widget_set_sensitive(gtk_ui_manager_get_widget(puim,
                                                           "/MainMenu/AnalyseMenu/CMarkMenu/CMarkGameMenu/Show"),
                                 !ListEmpty(&lMatch));
        gtk_widget_set_sensitive(gtk_ui_manager_get_widget(puim,
                                                           "/MainMenu/AnalyseMenu/CMarkMenu/CMarkMatchMenu/Clear"),
                                 !ListEmpty(&lMatch));
        gtk_widget_set_sensitive(gtk_ui_manager_get_widget(puim,
                                                           "/MainMenu/AnalyseMenu/CMarkMenu/CMarkMatchMenu/Show"),
                                 !ListEmpty(&lMatch));

        gtk_widget_set_sensitive(gtk_ui_manager_get_widget(puim,
                                                           "/MainMenu/AnalyseMenu/RolloutMenu/Cube"),
                                 !ListEmpty(&lMatch));
        gtk_widget_set_sensitive(gtk_ui_manager_get_widget(puim,
                                                           "/MainMenu/AnalyseMenu/RolloutMenu/Move"),
                                 !ListEmpty(&lMatch));
        gtk_widget_set_sensitive(gtk_ui_manager_get_widget(puim,
                                                           "/MainMenu/AnalyseMenu/RolloutMenu/Game"),
                                 !ListEmpty(&lMatch));
        gtk_widget_set_sensitive(gtk_ui_manager_get_widget(puim,
                                                           "/MainMenu/AnalyseMenu/RolloutMenu/Match"),
                                 !ListEmpty(&lMatch));

        gtk_widget_set_sensitive(gtk_ui_manager_get_widget(puim,
                                                           "/MainMenu/AnalyseMenu/AddMatchOrSessionStatsToDB"),
                                 !ListEmpty(&lMatch));
        gtk_widget_set_sensitive(gtk_ui_manager_get_widget(puim, "/MainMenu/AnalyseMenu/ShowRecords"), TRUE);
	gtk_widget_set_sensitive(gtk_ui_manager_get_widget(puim, "/MainMenu/AnalyseMenu/PlotHistory"), TRUE);
         
        /*disabling everything when we analyze a game in the background*/
        
        if(fBackgroundAnalysis){
            gtk_widget_set_sensitive(gtk_ui_manager_get_widget(puim, "/MainMenu/FileMenu/"), !fAnalysisRunning);
            gtk_widget_set_sensitive(gtk_ui_manager_get_widget(puim, "/MainMenu/EditMenu/"), !fAnalysisRunning);
            // gtk_widget_set_sensitive(gtk_ui_manager_get_widget(puim, "/MainMenu/ViewMenu/"), !fAnalysisRunning);
            gtk_widget_set_sensitive(gtk_ui_manager_get_widget(puim, "/MainMenu/GameMenu/"), !fAnalysisRunning);
            gtk_widget_set_sensitive(gtk_ui_manager_get_widget(puim, "/MainMenu/AnalyseMenu/"), !fAnalysisRunning);
            gtk_widget_set_sensitive(gtk_ui_manager_get_widget(puim, "/MainMenu/SettingsMenu/"), !fAnalysisRunning);
            // gtk_widget_set_sensitive(gtk_ui_manager_get_widget(puim, "/MainMenu/GoMenu/"), !fAnalysisRunning);
            // gtk_widget_set_sensitive(gtk_ui_manager_get_widget(puim, "/MainMenu/HelpMenu/"), !fAnalysisRunning);
        }

#else
        gtk_widget_set_sensitive(gtk_item_factory_get_widget(pif, "/File/Save..."), plGame != NULL);

        enable_sub_menu(gtk_item_factory_get_widget(pif, "/Game"), ms.gs == GAME_PLAYING);

        if (ms.fTurn >= 0)
            enable_menu(gtk_item_factory_get_widget(pif, "/Game/Roll"),
                        ms.fMove == ms.fTurn && ap[ms.fMove].pt == PLAYER_HUMAN);

        gtk_widget_set_sensitive(gtk_item_factory_get_widget_by_action(pif, CMD_NEXT_ROLL), plGame != NULL);
        gtk_widget_set_sensitive(gtk_item_factory_get_widget_by_action(pif, CMD_PREV_ROLL), plGame != NULL);
        gtk_widget_set_sensitive(gtk_item_factory_get_widget_by_action(pif, CMD_NEXT_MARKED), plGame != NULL);
        gtk_widget_set_sensitive(gtk_item_factory_get_widget_by_action(pif, CMD_PREV_MARKED), plGame != NULL);
        gtk_widget_set_sensitive(gtk_item_factory_get_widget_by_action(pif, CMD_NEXT_GAME), !ListEmpty(&lMatch));
        gtk_widget_set_sensitive(gtk_item_factory_get_widget_by_action(pif, CMD_PREV_GAME), !ListEmpty(&lMatch));
        gtk_widget_set_sensitive(gtk_item_factory_get_widget(pif, "/File/Match information..."), !ListEmpty(&lMatch));

        if (!fAnalysisRunning)
            enable_sub_menu(gtk_item_factory_get_widget(pif, "/Analyse"), ms.gs == GAME_PLAYING);

        gtk_widget_set_sensitive(gtk_item_factory_get_widget(pif, "/Analyse/Analyse File"), TRUE);
        gtk_widget_set_sensitive(gtk_item_factory_get_widget(pif, "/Analyse/Batch analyse..."), TRUE);

        gtk_widget_set_sensitive(gtk_item_factory_get_widget_by_action(pif, CMD_ANALYSE_MOVE),
                                 plLastMove && plLastMove->plNext && plLastMove->plNext->p);
        gtk_widget_set_sensitive(gtk_item_factory_get_widget_by_action(pif, CMD_ANALYSE_GAME), plGame != NULL);
        gtk_widget_set_sensitive(gtk_item_factory_get_widget_by_action(pif, CMD_ANALYSE_MATCH), !ListEmpty(&lMatch));

        gtk_widget_set_sensitive(gtk_item_factory_get_widget_by_action(pif, CMD_ANALYSE_CLEAR_MOVE),
                                 plLastMove && plLastMove->plNext && plLastMove->plNext->p);
        gtk_widget_set_sensitive(gtk_item_factory_get_widget_by_action(pif, CMD_ANALYSE_CLEAR_GAME), plGame != NULL);
        gtk_widget_set_sensitive(gtk_item_factory_get_widget_by_action(pif, CMD_ANALYSE_CLEAR_MATCH),
                                 !ListEmpty(&lMatch));
        gtk_widget_set_sensitive(gtk_item_factory_get_widget_by_action(pif, CMD_SHOW_STATISTICS_MATCH),
                                 !ListEmpty(&lMatch));
        gtk_widget_set_sensitive(gtk_item_factory_get_widget_by_action(pif, CMD_SHOW_HISTORY),
                                 !ListEmpty(&lMatch));
        gtk_widget_set_sensitive(gtk_item_factory_get_widget_by_action(pif, CMD_SHOW_MATCHEQUITYTABLE), TRUE);
        gtk_widget_set_sensitive(gtk_item_factory_get_widget_by_action(pif, CMD_SHOW_CALIBRATION), TRUE);
        gtk_widget_set_sensitive(gtk_item_factory_get_widget_by_action(pif, CMD_SWAP_PLAYERS), !ListEmpty(&lMatch));
        gtk_widget_set_sensitive(gtk_item_factory_get_widget_by_action(pif, CMD_CMARK_CUBE_CLEAR), !ListEmpty(&lMatch));
        gtk_widget_set_sensitive(gtk_item_factory_get_widget_by_action(pif, CMD_CMARK_CUBE_SHOW), !ListEmpty(&lMatch));
        gtk_widget_set_sensitive(gtk_item_factory_get_widget_by_action(pif, CMD_CMARK_MOVE_CLEAR), !ListEmpty(&lMatch));
        gtk_widget_set_sensitive(gtk_item_factory_get_widget_by_action(pif, CMD_CMARK_MOVE_SHOW), !ListEmpty(&lMatch));
        gtk_widget_set_sensitive(gtk_item_factory_get_widget_by_action(pif, CMD_CMARK_GAME_CLEAR), !ListEmpty(&lMatch));
        gtk_widget_set_sensitive(gtk_item_factory_get_widget_by_action(pif, CMD_CMARK_GAME_SHOW), !ListEmpty(&lMatch));
        gtk_widget_set_sensitive(gtk_item_factory_get_widget_by_action(pif, CMD_CMARK_MATCH_CLEAR),
                                 !ListEmpty(&lMatch));
        gtk_widget_set_sensitive(gtk_item_factory_get_widget_by_action(pif, CMD_CMARK_MATCH_SHOW), !ListEmpty(&lMatch));
        gtk_widget_set_sensitive(gtk_item_factory_get_widget_by_action(pif, CMD_ANALYSE_ROLLOUT_CUBE),
                                 !ListEmpty(&lMatch));
        gtk_widget_set_sensitive(gtk_item_factory_get_widget_by_action(pif, CMD_ANALYSE_ROLLOUT_MOVE),
                                 !ListEmpty(&lMatch));
        gtk_widget_set_sensitive(gtk_item_factory_get_widget_by_action(pif, CMD_ANALYSE_ROLLOUT_GAME),
                                 !ListEmpty(&lMatch));
        gtk_widget_set_sensitive(gtk_item_factory_get_widget_by_action(pif, CMD_ANALYSE_ROLLOUT_MATCH),
                                 !ListEmpty(&lMatch));

        gtk_widget_set_sensitive(gtk_item_factory_get_widget(pif,
                                                             "/Analyse/Add match or session to database"),
                                 !ListEmpty(&lMatch));

        gtk_widget_set_sensitive(gtk_item_factory_get_widget(pif, "/Analyse/Show Records"), TRUE);
        gtk_widget_set_sensitive(gtk_item_factory_get_widget(pif, "/Analyse/Plot History"), TRUE);
#endif
        fAutoCommand = FALSE;
    } else if (p == &ms.fCrawford) {
        bd->crawford_game = ms.fCrawford;
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(bd->crawford), ms.fCrawford);
        ShowBoard();            /* this is overkill, but it works */
    } else if (p == &ms.nCube) {
        ShowBoard();            /* this is overkill, but it works */
    } else if (IsPanelShowVar(WINDOW_ANNOTATION, p)) {
        ShowHidePanel(WINDOW_ANNOTATION);
    } else if (IsPanelShowVar(WINDOW_GAME, p)) {
        ShowHidePanel(WINDOW_GAME);
    } else if (IsPanelShowVar(WINDOW_ANALYSIS, p)) {
        ShowHidePanel(WINDOW_ANALYSIS);
    } else if (IsPanelShowVar(WINDOW_MESSAGE, p)) {
        ShowHidePanel(WINDOW_MESSAGE);
    } else if (IsPanelShowVar(WINDOW_THEORY, p)) {
        ShowHidePanel(WINDOW_THEORY);
    } else if (IsPanelShowVar(WINDOW_COMMAND, p)) {
        ShowHidePanel(WINDOW_COMMAND);
    } else if (p == &bd->rd->fDiceArea) {
        if (gtk_widget_get_realized(pwBoard)) {
#if defined(USE_BOARD3D)
            /* If in 3d mode may need to update sizes */
			if (display_is_3d(bd->rd))
				RecalcViewingVolume(bd);
            else
#endif
            {
                if (gtk_widget_get_realized(pwBoard)) {
                    if (gtk_widget_get_visible(bd->dice_area) && !bd->rd->fDiceArea)
                        gtk_widget_hide(bd->dice_area);
                    else if (!gtk_widget_get_visible(bd->dice_area) && bd->rd->fDiceArea)
                        gtk_widget_show_all(bd->dice_area);
                }
            }
        }
    } else if (p == &fShowIDs) {
        inCallback = TRUE;
#if !defined(USE_GTKITEMFACTORY)
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_ui_manager_get_widget(puim,
                                                                                     "/MainMenu/ViewMenu/ShowIDStatusBar")),
                                       fShowIDs);
#else
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM
                                       (gtk_item_factory_get_widget(pif, "/View/Show ID in status bar")), fShowIDs);
#endif
        inCallback = FALSE;

        if (!fShowIDs)
            gtk_widget_hide(pwIDBox);
        else
            gtk_widget_show_all(pwIDBox);
    } else if (p == &gui_show_pips)
        ShowBoard();            /* this is overkill, but it works */
    else if (p == &fOutputWinPC) {
        MoveListRefreshSize();
    } else if (p == &showMoveListDetail) {
        if (pwMoveAnalysis && pwDetails)
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pwDetails), showMoveListDetail);
    }
}


/* Match stats variables */
#define NUM_STAT_TYPES (FORMATGS_OVERALL - FORMATGS_ALL)

static char *aszStatHeading[NUM_STAT_TYPES] = {
    N_("Chequer play Statistics:"),
    N_("Cube Statistics:"),
    N_("Luck Statistics:"),
    N_("Overall Statistics:")
};

static GtkWidget *statViews[NUM_STAT_TYPES], *statView;
static int numStatGames;
static GtkWidget *pwStatDialog;
int fGUIUseStatsPanel = TRUE;
static GtkWidget *pswList;
static GtkWidget *pwNotebook;

static void
AddList(char *pStr, GtkWidget * view, const char *pTitle)
{
    gchar *sz;
    GtkTreeIter iter;
    GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(view));

    sprintf(strchr(pStr, 0), "%s\n", pTitle);
    if (gtk_tree_model_get_iter_first(model, &iter))
        do {
            gtk_tree_model_get(model, &iter, 0, &sz, -1);
            sprintf(strchr(pStr, 0), "%-37s ", sz ? sz : "");
            gtk_tree_model_get(model, &iter, 1, &sz, -1);
            sprintf(strchr(pStr, 0), "%-20s ", sz ? sz : "");
            gtk_tree_model_get(model, &iter, 2, &sz, -1);
            sprintf(strchr(pStr, 0), "%-20s\n", sz ? sz : "");
        } while (gtk_tree_model_iter_next(model, &iter));
    sprintf(strchr(pStr, 0), "\n");
}

static void
CopyData(GtkWidget * UNUSED(pwNotebook), int page)
{
    char szOutput[4096];

    sprintf(szOutput, "%-37s %-20s %-20s\n", "", ap[0].szName, ap[1].szName);

    if (page == FORMATGS_CHEQUER || page == FORMATGS_ALL)
        AddList(szOutput, statViews[FORMATGS_CHEQUER], gettext(aszStatHeading[FORMATGS_CHEQUER]));
    if (page == FORMATGS_LUCK || page == FORMATGS_ALL)
        AddList(szOutput, statViews[FORMATGS_LUCK], gettext(aszStatHeading[FORMATGS_LUCK]));
    if (page == FORMATGS_CUBE || page == FORMATGS_ALL)
        AddList(szOutput, statViews[FORMATGS_CUBE], gettext(aszStatHeading[FORMATGS_CUBE]));
    if (page == FORMATGS_OVERALL || page == FORMATGS_ALL)
        AddList(szOutput, statViews[FORMATGS_OVERALL], gettext(aszStatHeading[FORMATGS_OVERALL]));

    TextToClipboard(szOutput);
}

static void
CopyPage(GtkWidget * UNUSED(pwWidget), GtkWidget * pwNotebook)
{
    switch (gtk_notebook_get_current_page(GTK_NOTEBOOK(pwNotebook))) {
    case 0:
        CopyData(pwNotebook, FORMATGS_OVERALL);
        break;
    case 1:
        CopyData(pwNotebook, FORMATGS_CHEQUER);
        break;
    case 2:
        CopyData(pwNotebook, FORMATGS_CUBE);
        break;
    case 3:
        CopyData(pwNotebook, FORMATGS_LUCK);
        break;
    }
}

static void
CopyAll(GtkWidget * UNUSED(pwWidget), GtkWidget * pwNotebook)
{
    CopyData(pwNotebook, FORMATGS_ALL);
}

static void
FillStats(const statcontext * psc, const matchstate * pms, const formatgs gs, GtkWidget * statView)
{

    GList *list = formatGS(psc, pms->nMatchTo, gs);
    GList *pl;
    GtkListStore *store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(statView)));

    for (pl = g_list_first(list); pl; pl = g_list_next(pl)) {
        GtkTreeIter iter;
        char **aasz = pl->data;
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter, 0, aasz[0], 1, aasz[1], 2, aasz[2], -1);
    }
    freeGS(list);
}

static void
SetStats(const statcontext * psc)
{
    char *aszLine[] = { NULL, NULL, NULL };
    int i;
    GtkListStore *store;
    GtkTreeIter iter;

    for (i = 0; i < NUM_STAT_TYPES; ++i) {
        store = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
        gtk_tree_view_set_model(GTK_TREE_VIEW(statViews[i]), GTK_TREE_MODEL(store));
        FillStats(psc, &ms, i, statViews[i]);
        g_object_unref(store);
    }

    store = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    gtk_tree_view_set_model(GTK_TREE_VIEW(statView), GTK_TREE_MODEL(store));
    g_object_unref(store);
    aszLine[0] = gettext(aszStatHeading[FORMATGS_CHEQUER]);
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, aszLine[0], -1);
    FillStats(psc, &ms, FORMATGS_CHEQUER, statView);
    FillStats(psc, &ms, FORMATGS_LUCK, statView);

    aszLine[0] = gettext(aszStatHeading[FORMATGS_CUBE]);
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, aszLine[0], -1);
    FillStats(psc, &ms, FORMATGS_CUBE, statView);

    aszLine[0] = gettext(aszStatHeading[FORMATGS_OVERALL]);
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, aszLine[0], -1);
    FillStats(psc, &ms, FORMATGS_OVERALL, statView);
}

static const statcontext *
GetStatContext(int game)
{
    xmovegameinfo *pmgi;

    if (!game)
        return &scMatch;
    else {
        listOLD *plg, *pl = lMatch.plNext;
        for (int i = 1; i < game; i++)
            pl = pl->plNext;

        plg = pl->p;
        pmgi = &((moverecord *) plg->plNext->p)->g;
        return &pmgi->sc;
    }
}

static void
StatsSelectGame(GtkWidget * box, int UNUSED(i))
{
    int curStatGame = gtk_combo_box_get_active(GTK_COMBO_BOX(box));
    if (!curStatGame) {
        gtk_window_set_title(GTK_WINDOW(pwStatDialog), _("Statistics for all games"));
    } else {
        char sz[100];
        sprintf(sz, _("Statistics for game %d"), curStatGame);
        gtk_window_set_title(GTK_WINDOW(pwStatDialog), sz);
    }
    SetStats(GetStatContext(curStatGame));
}

static void
StatsPreviousGame(GtkWidget * UNUSED(button), GtkWidget * combo)
{
    int i = gtk_combo_box_get_active(GTK_COMBO_BOX(combo));
    if (i > 0)
        gtk_combo_box_set_active(GTK_COMBO_BOX(combo), i - 1);
}

static void
StatsNextGame(GtkWidget * UNUSED(button), GtkWidget * combo)
{
    int i = gtk_combo_box_get_active(GTK_COMBO_BOX(combo));
    if (i < numStatGames)
        gtk_combo_box_set_active(GTK_COMBO_BOX(combo), gtk_combo_box_get_active(GTK_COMBO_BOX(combo)) + 1);
}

static GtkWidget *
AddNavigation(GtkWidget * pvbox)
{
    GtkWidget *phbox, *pw, *box;
    char sz[128];
    int anFinalScore[2];
    listOLD *pl;

    box = gtk_combo_box_text_new();

    if (getFinalScore(anFinalScore))
        sprintf(sz, _("All games: %s %d, %s %d"), ap[0].szName, anFinalScore[0], ap[1].szName, anFinalScore[1]);
    else
        sprintf(sz, _("All games: %s, %s"), ap[0].szName, ap[1].szName);
#if GTK_CHECK_VERSION(3,0,0)
    phbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
#else
    phbox = gtk_hbox_new(FALSE, 0);
#endif
    gtk_box_pack_start(GTK_BOX(pvbox), phbox, FALSE, FALSE, 4);
    pw = button_from_image(gtk_image_new_from_stock(GNUBG_STOCK_GO_PREV_GAME, GTK_ICON_SIZE_LARGE_TOOLBAR));
    g_signal_connect(G_OBJECT(pw), "clicked", G_CALLBACK(StatsPreviousGame), box);
    gtk_box_pack_start(GTK_BOX(phbox), pw, FALSE, FALSE, 0);
    gtk_widget_set_tooltip_text(pw, _("Move back to the previous game"));

    pw = button_from_image(gtk_image_new_from_stock(GNUBG_STOCK_GO_NEXT_GAME, GTK_ICON_SIZE_LARGE_TOOLBAR));
    g_signal_connect(G_OBJECT(pw), "clicked", G_CALLBACK(StatsNextGame), box);
    gtk_box_pack_start(GTK_BOX(phbox), pw, FALSE, FALSE, 4);
    gtk_widget_set_tooltip_text(pw, _("Move ahead to the next game"));

    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(box), sz);
    numStatGames = 0;
    for (pl = lMatch.plNext; pl->p; pl = pl->plNext) {
        listOLD *plg = pl->p;
        moverecord *pmr = plg->plNext->p;
        numStatGames++;

        sprintf(sz, _("Game %d: %s %d, %s %d"), pmr->g.i + 1, ap[0].szName,
                pmr->g.anScore[0], ap[1].szName, pmr->g.anScore[1]);
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(box), sz);
    }
    g_signal_connect(G_OBJECT(box), "changed", G_CALLBACK(StatsSelectGame), NULL);
    gtk_box_pack_start(GTK_BOX(phbox), box, TRUE, TRUE, 4);

    return box;
}

static void
toggle_fGUIUseStatsPanel(GtkWidget * widget, GtkWidget * UNUSED(pw))
{
    fGUIUseStatsPanel = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

    if (fGUIUseStatsPanel) {
        gtk_widget_hide(pswList);
        gtk_widget_show(pwNotebook);
    } else {
        gtk_widget_hide(pwNotebook);
        gtk_widget_show(pswList);
    }
}

static void
StatcontextCopy(GtkWidget * UNUSED(pw), GtkTreeView * view)
{
    static char szOutput[4096];
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GList *row;
    GList *rows;

    selection = gtk_tree_view_get_selection(view);

    if (gtk_tree_selection_count_selected_rows(selection) < 1)
        return;

    sprintf(szOutput, "%-37s %-20s %-20s\n", "", ap[0].szName, ap[1].szName);

    rows = gtk_tree_selection_get_selected_rows(selection, &model);

    for (row = rows; row; row = row->next) {
        GtkTreeIter iter;
        gchar *sz;
        GtkTreePath *path = row->data;
        gtk_tree_model_get_iter(model, &iter, path);

        gtk_tree_model_get(model, &iter, 0, &sz, -1);
        sprintf(strchr(szOutput, 0), "%-37s ", sz ? sz : "");
        g_free(sz);

        gtk_tree_model_get(model, &iter, 1, &sz, -1);
        sprintf(strchr(szOutput, 0), "%-20s ", sz ? sz : "");
        g_free(sz);

        gtk_tree_model_get(model, &iter, 2, &sz, -1);
        sprintf(strchr(szOutput, 0), "%-20s\n", sz ? sz : "");
        g_free(sz);

        gtk_tree_path_free(path);
    }
    g_list_free(rows);
    GTKTextToClipboard(szOutput);
}


static GtkWidget *
CreateList(void)
{
    int i;
    GtkWidget *view;
    GtkWidget *copyMenu;
    GtkWidget *menu_item;

    view = gtk_tree_view_new();

    for (i = 0; i < 3; i++) {
        GtkCellRenderer *renderer;
        GtkTreeViewColumn *column;
        renderer = gtk_cell_renderer_text_new();
        g_object_set(renderer, "xalign", 1.0, NULL);
        column = gtk_tree_view_column_new_with_attributes("", renderer, "text", i, NULL);
        gtk_tree_view_column_set_alignment(column, 0.97f);
        gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);
    }
    gtk_tree_view_column_set_title(gtk_tree_view_get_column(GTK_TREE_VIEW(view), 1), ap[0].szName);
    gtk_tree_view_column_set_title(gtk_tree_view_get_column(GTK_TREE_VIEW(view), 2), ap[1].szName);
    gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(view)), GTK_SELECTION_MULTIPLE);
    /* list view (selections) */
    copyMenu = gtk_menu_new();

    menu_item = gtk_menu_item_new_with_label(_("Copy selection"));
    gtk_menu_shell_append(GTK_MENU_SHELL(copyMenu), menu_item);
    gtk_widget_show(menu_item);
    g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(StatcontextCopy), view);

    menu_item = gtk_menu_item_new_with_label(_("Copy all"));
    gtk_menu_shell_append(GTK_MENU_SHELL(copyMenu), menu_item);
    gtk_widget_show(menu_item);
    g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(CopyAll), pwNotebook);

    g_signal_connect(G_OBJECT(view), "button-press-event", G_CALLBACK(ContextMenu), copyMenu);

    return view;
}

static void
stat_dialog_map(GtkWidget * UNUSED(window), GtkWidget * pwUsePanels)
{
    toggle_fGUIUseStatsPanel(pwUsePanels, 0);
}

/* Credit: partly based on http://kapo-cpp.blogspot.com */

extern void drawArrow(cairo_t *cr, double start_x, double start_y, double end_x, double end_y)
{
    double angle = atan2(end_y - start_y, end_x - start_x) + M_PI;
    double dist = sqrt((start_x - end_x) * (start_x - end_x) + (start_y - end_y) * (start_y - end_y));
    double side = MIN(6.0, 0.5 * dist);
    double degrees = 0.6;

    double x1 = end_x + side * cos(angle - degrees);
    double y1 = end_y + side * sin(angle - degrees);
    double x2 = end_x + side * cos(angle + degrees);
    double y2 = end_y + side * sin(angle + degrees);

    cairo_move_to(cr, start_x, start_y);
    cairo_line_to(cr, end_x,end_y);
    cairo_stroke(cr);

    cairo_move_to(cr, end_x, end_y);
    cairo_line_to(cr, x1, y1);
    cairo_line_to(cr, x2, y2);
    cairo_line_to(cr, end_x, end_y);
    cairo_fill(cr);
    // cairo_stroke (cr);

    // g_message("arrow: %f %f %f %f",x1,y1,x2,y2);
}

extern void
GTKDumpStatcontext(int game)
{
    GtkWidget *copyMenu, *menu_item, *pvbox, *pwUsePanels;
    GtkWidget *navi_combo;
    GtkWidget *addToDbButton;

    /* the 3D graph seems buggy and makes gnubg crash, removing for now */

#if defined(USE_BOARD3D)
    int graph3d = 1;
    int i;
    GtkWidget *pw;
    listOLD *pl;
    GraphData *gd = NULL;
    if (graph3d) {
        gd = CreateGraphData();
    }
#endif

    /* Here we had many issues. This statistics window calls the MWC window.
     * - Making both non-modal was causing crashes when closing them.
     * - Making both modal was causing a "blackhole" whereby we could open
     *   the stats window once but not twice
     * - Using different modalities causes usability problems
     * - A solution was found by making both non-modal, but replacing here
     *   the "GTKRunDialog()" at the end by "gtk_widget_show_all()".
     * - Alas, it turns out it prevents the graph from displaying.
     * - Finally (for now), another solution consists of making both
     *   windows modal, and using "GTKRunDialog()" in both. Not sure why,
     *   but it finally works.
     */

    pwStatDialog = GTKCreateDialog("", DT_INFO, NULL, DIALOG_FLAG_MODAL, G_CALLBACK(gtk_widget_destroy), NULL);
    // pwStatDialog = GTKCreateDialog("", DT_INFO, NULL, DIALOG_FLAG_NONE, G_CALLBACK(gtk_widget_destroy), NULL);

    if (!fAutoDB) {
        gtk_container_add(GTK_CONTAINER(DialogArea(pwStatDialog, DA_BUTTONS)),
                        addToDbButton = gtk_button_new_with_label(_("Add to DB")));
        g_signal_connect(addToDbButton, "clicked", G_CALLBACK(GtkRelationalAddMatch), pwStatDialog);
    }

    pwNotebook = gtk_notebook_new();
    gtk_notebook_set_scrollable(GTK_NOTEBOOK(pwNotebook), TRUE);
    gtk_notebook_popup_disable(GTK_NOTEBOOK(pwNotebook));

#if GTK_CHECK_VERSION(3,0,0)
    pvbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
    pvbox = gtk_vbox_new(FALSE, 0);
#endif
    gtk_box_pack_start(GTK_BOX(pvbox), pwNotebook, TRUE, TRUE, 0);

    gtk_notebook_append_page(GTK_NOTEBOOK(pwNotebook), statViews[FORMATGS_OVERALL] = CreateList(),
                             gtk_label_new(_("Overall")));

    gtk_notebook_append_page(GTK_NOTEBOOK(pwNotebook), statViews[FORMATGS_CHEQUER] = CreateList(),
                             gtk_label_new(_("Chequer play")));

    gtk_notebook_append_page(GTK_NOTEBOOK(pwNotebook), statViews[FORMATGS_CUBE] = CreateList(),
                             gtk_label_new(_("Cube decisions")));

    gtk_notebook_append_page(GTK_NOTEBOOK(pwNotebook), statViews[FORMATGS_LUCK] = CreateList(),
                             gtk_label_new(_("Luck")));

    statView = CreateList();

    pswList = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pswList), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(pswList), statView);

    gtk_box_pack_start(GTK_BOX(pvbox), pswList, TRUE, TRUE, 0);

    navi_combo = AddNavigation(pvbox);
    gtk_container_add(GTK_CONTAINER(DialogArea(pwStatDialog, DA_MAIN)), pvbox);

#if defined(USE_BOARD3D)
    SetNumGames(gd, numStatGames);

    pl = lMatch.plNext;
    for (i = 0; i < numStatGames; i++) {
        listOLD *plg = pl->p;
        moverecord *mr = plg->plNext->p;
        xmovegameinfo *pmgi = &mr->g;
        AddGameData(gd, i, &pmgi->sc);
        pl = pl->plNext;
    }
    /* Total values */
    AddGameData(gd, i, &scMatch);

    pw = StatGraph(gd);
    if (pw != NULL) {
        gtk_notebook_append_page(GTK_NOTEBOOK(pwNotebook), pw, gtk_label_new(_("Graph")));
        gtk_widget_set_tooltip_text(pw, _("This graph shows the total error rates per game for each player."
                                          " The games are along the bottom and the error rates up the side."
                                          " Chequer error in green, cube error in blue."));
    }
#endif

    pwUsePanels = gtk_check_button_new_with_label(_("Split statistics into panels"));
    gtk_widget_set_tooltip_text(pwUsePanels, _("Show data in a single list or split into several panels"));
    gtk_box_pack_start(GTK_BOX(pvbox), pwUsePanels, FALSE, FALSE, 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pwUsePanels), fGUIUseStatsPanel);
    g_signal_connect(G_OBJECT(pwUsePanels), "toggled", G_CALLBACK(toggle_fGUIUseStatsPanel), NULL);

    /* list view (selections) */
    copyMenu = gtk_menu_new();

    menu_item = gtk_menu_item_new_with_label(_("Copy selection"));
    gtk_menu_shell_append(GTK_MENU_SHELL(copyMenu), menu_item);
    gtk_widget_show(menu_item);
    g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(StatcontextCopy), statView);

    menu_item = gtk_menu_item_new_with_label(_("Copy all"));
    gtk_menu_shell_append(GTK_MENU_SHELL(copyMenu), menu_item);
    gtk_widget_show(menu_item);
    g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(CopyAll), pwNotebook);

    g_signal_connect(G_OBJECT(statView), "button-press-event", G_CALLBACK(ContextMenu), copyMenu);

    /* dialog size */
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pwUsePanels)))
        gtk_window_set_default_size(GTK_WINDOW(pwStatDialog), 0, 300);
    else {
        GtkRequisition req;
#if GTK_CHECK_VERSION(3,0,0)
        gtk_widget_get_preferred_size(GTK_WIDGET(pwStatDialog), &req, NULL);
#else
        gtk_widget_size_request(GTK_WIDGET(pwStatDialog), &req);
#endif
        if (req.height < 600)
            gtk_window_set_default_size(GTK_WINDOW(pwStatDialog), 0, 600);
    }

    copyMenu = gtk_menu_new();

    menu_item = gtk_menu_item_new_with_label(_("Copy page"));
    gtk_menu_shell_append(GTK_MENU_SHELL(copyMenu), menu_item);
    gtk_widget_show(menu_item);
    g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(CopyPage), pwNotebook);

    menu_item = gtk_menu_item_new_with_label(_("Copy all pages"));
    gtk_menu_shell_append(GTK_MENU_SHELL(copyMenu), menu_item);
    gtk_widget_show(menu_item);
    g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(CopyAll), pwNotebook);

    g_signal_connect(G_OBJECT(pwNotebook), "button-press-event", G_CALLBACK(ContextMenu), copyMenu);

    gtk_combo_box_set_active(GTK_COMBO_BOX(navi_combo), game);

    g_signal_connect(pwStatDialog, "map", G_CALLBACK(stat_dialog_map), pwUsePanels);

    // gtk_widget_show_all (pwStatDialog);
    GTKRunDialog(pwStatDialog); // <-- causes issues! 


#if defined(USE_BOARD3D)
    TidyGraphData(gd);
#endif
}

extern int
GTKGetMove(int anMove[8])
{

    BoardData *bd = BOARD(pwBoard)->board_data;

    if (!bd->valid_move)
        return 0;

    memcpy(anMove, bd->valid_move->anMove, 8 * sizeof(int));

    return 1;

}

static void
UpdateMatchinfo(const char *pch, const char *szParam, char **ppch)
{

    char *szCommand;
    const char *pchOld = *ppch ? *ppch : "";

    if (!strcmp(pch, pchOld))
        /* no change */
        return;

    szCommand = g_strdup_printf("set matchinfo %s %s", szParam, pch);
    UserCommand(szCommand);
    g_free(szCommand);
}

/* Variables for match info dialog */
static GtkWidget *apwRating[2], *pwDate, *pwEvent, *pwRound, *pwPlace, *pwAnnotator;
static GtkTextBuffer *txtComment;

static void
MatchInfoOK(GtkWidget * pw, int *UNUSED(pf))
{
    GtkTextIter begin, end;
    unsigned int nYear, nMonth, nDay;
    char *pch;

    outputpostpone();

    UpdateMatchinfo(gtk_entry_get_text(GTK_ENTRY(apwRating[0])), "rating 0", &mi.pchRating[0]);
    UpdateMatchinfo(gtk_entry_get_text(GTK_ENTRY(apwRating[1])), "rating 1", &mi.pchRating[1]);

    gtk_calendar_get_date(GTK_CALENDAR(pwDate), &nYear, &nMonth, &nDay);
    nMonth++;
    if (mi.nYear && !nDay)
        UserCommand("set matchinfo date");
    else if (nDay && (!mi.nYear || mi.nYear != nYear || mi.nMonth != nMonth || mi.nDay != nDay)) {
        char sz[64];
        sprintf(sz, "set matchinfo date %04u-%02u-%02u", nYear, nMonth, nDay);
        UserCommand(sz);
    }

    UpdateMatchinfo(gtk_entry_get_text(GTK_ENTRY(pwEvent)), "event", &mi.pchEvent);
    UpdateMatchinfo(gtk_entry_get_text(GTK_ENTRY(pwRound)), "round", &mi.pchRound);
    UpdateMatchinfo(gtk_entry_get_text(GTK_ENTRY(pwPlace)), "place", &mi.pchPlace);
    UpdateMatchinfo(gtk_entry_get_text(GTK_ENTRY(pwAnnotator)), "annotator", &mi.pchAnnotator);

    gtk_text_buffer_get_bounds(txtComment, &begin, &end);
    pch = gtk_text_buffer_get_text(txtComment, &begin, &end, FALSE);
    UpdateMatchinfo(pch, "comment", &mi.pchComment);
    g_free(pch);

    outputresume();

    gtk_widget_destroy(gtk_widget_get_toplevel(pw));
}

static void
AddToTable(GtkWidget * pwTable, char *str, int x, int y)
{
    GtkWidget *pw = gtk_label_new(str);
    /* Right align */
#if GTK_CHECK_VERSION(3,0,0)
    gtk_widget_set_halign(pw, GTK_ALIGN_END);
    gtk_widget_set_valign(pw, GTK_ALIGN_CENTER);
    gtk_grid_attach(GTK_GRID(pwTable), pw, x, y, 1, 1);
    gtk_widget_set_hexpand(pw, TRUE);
    gtk_widget_set_vexpand(pw, TRUE);
#else
    gtk_misc_set_alignment(GTK_MISC(pw), 1.0, 0.5);
    gtk_table_attach(GTK_TABLE(pwTable), pw, x, x + 1, y, y + 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
#endif
}

extern void
GTKMatchInfo(void)
{
    int fOK = FALSE;
    GtkWidget *pwDialog, *pwScrolled, *pwComment;
    GtkWidget *pwTable;
    char sz[128];

    pwDialog = GTKCreateDialog(_("GNU Backgammon - Match information"),
                               DT_QUESTION, NULL, DIALOG_FLAG_MODAL, G_CALLBACK(MatchInfoOK), &fOK);
#if GTK_CHECK_VERSION(3,0,0)
    pwTable = gtk_grid_new();
#else
    pwTable = gtk_table_new(3, 7, FALSE);
#endif
    gtk_container_add(GTK_CONTAINER(DialogArea(pwDialog, DA_MAIN)), pwTable);

    sprintf(sz, _("%s's rating:"), ap[0].szName);
    AddToTable(pwTable, sz, 0, 0);

    sprintf(sz, _("%s's rating:"), ap[1].szName);
    AddToTable(pwTable, sz, 0, 1);

    AddToTable(pwTable, _("Date:"), 0, 2);
    AddToTable(pwTable, _("Event:"), 0, 3);
    AddToTable(pwTable, _("Round:"), 0, 4);
    AddToTable(pwTable, _("Place:"), 0, 5);
    AddToTable(pwTable, _("Annotator:"), 0, 6);

    sprintf(sz, "%-78s", _("Comments:"));
#if GTK_CHECK_VERSION(3,0,0)
    gtk_grid_attach(GTK_GRID(pwTable), gtk_label_new(sz), 2, 0, 1, 1);
#else
    gtk_table_attach_defaults(GTK_TABLE(pwTable), gtk_label_new(sz), 2, 3, 0, 1);
#endif

    apwRating[0] = gtk_entry_new();
    if (mi.pchRating[0])
        gtk_entry_set_text(GTK_ENTRY(apwRating[0]), mi.pchRating[0]);
#if GTK_CHECK_VERSION(3,0,0)
    gtk_grid_attach(GTK_GRID(pwTable), apwRating[0], 1, 0, 1, 1);
#else
    gtk_table_attach_defaults(GTK_TABLE(pwTable), apwRating[0], 1, 2, 0, 1);
#endif

    apwRating[1] = gtk_entry_new();
    if (mi.pchRating[1])
        gtk_entry_set_text(GTK_ENTRY(apwRating[1]), mi.pchRating[1]);
#if GTK_CHECK_VERSION(3,0,0)
    gtk_grid_attach(GTK_GRID(pwTable), apwRating[1], 1, 1, 1, 1);
#else
    gtk_table_attach_defaults(GTK_TABLE(pwTable), apwRating[1], 1, 2, 1, 2);
#endif

    pwDate = gtk_calendar_new();
    if (mi.nYear) {
        gtk_calendar_select_month(GTK_CALENDAR(pwDate), mi.nMonth - 1, mi.nYear);
        gtk_calendar_select_day(GTK_CALENDAR(pwDate), mi.nDay);
    } else
        gtk_calendar_select_day(GTK_CALENDAR(pwDate), 0);
#if GTK_CHECK_VERSION(3,0,0)
    gtk_grid_attach(GTK_GRID(pwTable), pwDate, 1, 2, 1, 1);
#else
    gtk_table_attach_defaults(GTK_TABLE(pwTable), pwDate, 1, 2, 2, 3);
#endif

    pwEvent = gtk_entry_new();
    if (mi.pchEvent)
        gtk_entry_set_text(GTK_ENTRY(pwEvent), mi.pchEvent);
#if GTK_CHECK_VERSION(3,0,0)
    gtk_grid_attach(GTK_GRID(pwTable), pwEvent, 1, 3, 1, 1);
#else
    gtk_table_attach_defaults(GTK_TABLE(pwTable), pwEvent, 1, 2, 3, 4);
#endif

    pwRound = gtk_entry_new();
    if (mi.pchRound)
        gtk_entry_set_text(GTK_ENTRY(pwRound), mi.pchRound);
#if GTK_CHECK_VERSION(3,0,0)
    gtk_grid_attach(GTK_GRID(pwTable), pwRound, 1, 4, 1, 1);
#else
    gtk_table_attach_defaults(GTK_TABLE(pwTable), pwRound, 1, 2, 4, 5);
#endif

    pwPlace = gtk_entry_new();
    if (mi.pchPlace)
        gtk_entry_set_text(GTK_ENTRY(pwPlace), mi.pchPlace);
#if GTK_CHECK_VERSION(3,0,0)
    gtk_grid_attach(GTK_GRID(pwTable), pwPlace, 1, 5, 1, 1);
#else
    gtk_table_attach_defaults(GTK_TABLE(pwTable), pwPlace, 1, 2, 5, 6);
#endif

    pwAnnotator = gtk_entry_new();
    if (mi.pchAnnotator)
        gtk_entry_set_text(GTK_ENTRY(pwAnnotator), mi.pchAnnotator);
#if GTK_CHECK_VERSION(3,0,0)
    gtk_grid_attach(GTK_GRID(pwTable), pwAnnotator, 1, 6, 1, 1);
#else
    gtk_table_attach_defaults(GTK_TABLE(pwTable), pwAnnotator, 1, 2, 6, 7);
#endif

    pwComment = gtk_text_view_new();
    txtComment = gtk_text_view_get_buffer(GTK_TEXT_VIEW(pwComment));
    if (mi.pchComment) {
        gtk_text_buffer_set_text(txtComment, mi.pchComment, -1);
    }
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(pwComment), GTK_WRAP_WORD);

    pwScrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pwScrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(pwScrolled), pwComment);

#if GTK_CHECK_VERSION(3,0,0)
    gtk_grid_attach(GTK_GRID(pwTable), pwScrolled, 2, 1, 1, 6);
#else
    gtk_table_attach_defaults(GTK_TABLE(pwTable), pwScrolled, 2, 3, 1, 7);
#endif

    GTKRunDialog(pwDialog);
}

static void
CalibrationOK(GtkWidget * pw, GtkWidget * ppw)
{
    GtkAdjustment *padj = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(ppw));

    if (gtk_widget_is_sensitive(ppw)) {
        if (gtk_adjustment_get_value(padj) != (gdouble) rEvalsPerSec) {
            char sz[128];

            sprintf(sz, "set calibration %.0f", gtk_adjustment_get_value(padj));
            UserCommand(sz);
        }
    } else if (rEvalsPerSec > 0)
        UserCommand("set calibration");

    UserCommand("save settings");
    gtk_widget_destroy(gtk_widget_get_toplevel(pw));
}

static void
CalibrationEnable(GtkWidget * pw, GtkWidget * pwspin)
{
    gtk_widget_set_sensitive(pwspin, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pw)));
}

static void
CalibrationGo(GtkWidget * pw, GtkWidget * apw[2])
{
    GTKSetCurrentParent(pw);
    UserCommand("calibrate");

    fInterrupt = FALSE;

    if (rEvalsPerSec > 0) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(apw[0]), TRUE);
        gtk_adjustment_set_value(gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(apw[1])), rEvalsPerSec);
    }
}

extern void
GTKShowCalibration(void)
{
    GtkAdjustment *padj;
    GtkWidget *pwDialog, *pwvbox, *pwhbox, *pwenable, *pwspin, *pwbutton, *apw[2];

    padj = GTK_ADJUSTMENT(gtk_adjustment_new(rEvalsPerSec > 0 ? rEvalsPerSec : 10000, 2, G_MAXFLOAT, 100, 1000, 0));
    pwspin = gtk_spin_button_new(padj, 100, 0);
    /* FIXME should be modal but presently causes crash and/or killing of the main window */
    pwDialog = GTKCreateDialog(_("GNU Backgammon - Speed estimate"), DT_QUESTION,
                               NULL, 0, G_CALLBACK(CalibrationOK), pwspin);
#if GTK_CHECK_VERSION(3,0,0)
    pwvbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
#else
    pwvbox = gtk_vbox_new(FALSE, 8);
#endif
    gtk_container_add(GTK_CONTAINER(DialogArea(pwDialog, DA_MAIN)), pwvbox);
    gtk_container_set_border_width(GTK_CONTAINER(pwvbox), 8);
#if GTK_CHECK_VERSION(3,0,0)
    pwhbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
#else
    pwhbox = gtk_hbox_new(FALSE, 8);
#endif
    gtk_container_add(GTK_CONTAINER(pwvbox), pwhbox);
    gtk_container_add(GTK_CONTAINER(pwhbox), pwenable = gtk_check_button_new_with_label(_("Speed recorded:")));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pwenable), rEvalsPerSec > 0);

    gtk_container_add(GTK_CONTAINER(pwhbox), pwspin);
    gtk_widget_set_sensitive(pwspin, rEvalsPerSec > 0);

    gtk_container_add(GTK_CONTAINER(pwhbox), gtk_label_new(_("static evaluations/second")));

    gtk_container_add(GTK_CONTAINER(pwvbox), pwbutton = gtk_button_new_with_label(_("Calibrate")));
    gtk_widget_set_tooltip_text(pwbutton, _("Estimate speed, in 0-ply evaluations/second"));
    apw[0] = pwenable;
    apw[1] = pwspin;
    g_signal_connect(G_OBJECT(pwbutton), "clicked", G_CALLBACK(CalibrationGo), apw);

    g_signal_connect(G_OBJECT(pwenable), "toggled", G_CALLBACK(CalibrationEnable), pwspin);

    GTKRunDialog(pwDialog);
}

static gboolean
CalibrationCancel(GObject * UNUSED(po), gpointer UNUSED(p))
{

    fInterrupt = TRUE;

    return TRUE;
}

extern void *
GTKCalibrationStart(void)
{

    GtkWidget *pwDialog, *pwhbox, *pwResult;

    pwDialog = GTKCreateDialog(_("GNU Backgammon - Calibration"), DT_INFO,
                               NULL, DIALOG_FLAG_MODAL | DIALOG_FLAG_NOTIDY, G_CALLBACK(CalibrationCancel), NULL);
#if GTK_CHECK_VERSION(3,0,0)
    pwhbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
#else
    pwhbox = gtk_hbox_new(FALSE, 8);
#endif
    gtk_container_add(GTK_CONTAINER(DialogArea(pwDialog, DA_MAIN)), pwhbox);
    gtk_container_set_border_width(GTK_CONTAINER(pwhbox), 8);
    gtk_container_add(GTK_CONTAINER(pwhbox), gtk_label_new(_("Calibrating:")));
    gtk_container_add(GTK_CONTAINER(pwhbox), pwResult = gtk_label_new(_("       (n/a)       ")));
    gtk_container_add(GTK_CONTAINER(pwhbox), gtk_label_new(_("static evaluations/second")));

    pwOldGrab = pwGrab;
    pwGrab = pwDialog;

    g_signal_connect(G_OBJECT(pwDialog), "delete_event", G_CALLBACK(CalibrationCancel), NULL);

    gtk_widget_show_all(pwDialog);

    ProcessEvents();

    g_object_ref(G_OBJECT(pwResult));

    return pwResult;
}

extern void
GTKCalibrationUpdate(void *context, float rEvalsPerSec)
{

    gchar sz[32];

    sprintf(sz, "%.0f", rEvalsPerSec);
    gtk_label_set_text(GTK_LABEL(context), sz);

    ProcessEvents();
}

extern void
GTKCalibrationEnd(void *context)
{

    g_object_unref(G_OBJECT(context));

    gtk_widget_destroy(gtk_widget_get_toplevel(GTK_WIDGET(context)));

    pwGrab = pwOldGrab;
}

static void
CallbackResign(GtkWidget * pw, gpointer data)
{
    int i = GPOINTER_TO_INT(data);
    const char *asz[3] = { "normal", "gammon", "backgammon" };
    char sz[20];

    ms.anDice[0] = ms.anDice[1] = 0;
    sprintf(sz, "resign %s", asz[i]);
    gtk_widget_destroy(gtk_widget_get_toplevel(pw));
    UserCommand(sz);

    return;
}

extern void
GTKResign(gpointer UNUSED(p), guint UNUSED(n), GtkWidget * UNUSED(pw))
{
    GtkWidget *pwDialog, *pwVbox;
    int i;
    const char *asz[3] = { N_("Resign normal"),
        N_("Resign gammon"),
        N_("Resign backgammon")
    };
    const char *resign_stocks[3] = { GNUBG_STOCK_RESIGNSN, GNUBG_STOCK_RESIGNSG, GNUBG_STOCK_RESIGNSB };

    if (ap[!ms.fTurn].pt != PLAYER_HUMAN && check_resigns(NULL) != -1 && GTKShowWarning(WARN_RESIGN, NULL)) {   /* Automatically resign for computer */
        UserCommand("resign -1");
        while (nNextTurn)
            NextTurnNotify(NULL);
        if (!ms.fResignationDeclined)
            return;
    }
    pwDialog = GTKCreateDialog(_("Resign"), DT_QUESTION, NULL, DIALOG_FLAG_MODAL | DIALOG_FLAG_NOOK, NULL, NULL);

#if GTK_CHECK_VERSION(3,0,0)
    pwVbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    /* FIXME: how do we set homogenous to TRUE in gtk3 ? */
#else
    pwVbox = gtk_vbox_new(TRUE, 5);
#endif

    for (i = 0; i < 3; i++) {
        GtkWidget *pwButtons, *pwHbox;

        pwButtons = gtk_button_new();
#if GTK_CHECK_VERSION(3,0,0)
        pwHbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
#else
        pwHbox = gtk_hbox_new(FALSE, 0);
#endif
        gtk_container_add(GTK_CONTAINER(pwButtons), pwHbox);
        gtk_box_pack_start(GTK_BOX(pwHbox), gtk_image_new_from_stock(resign_stocks[i], GTK_ICON_SIZE_LARGE_TOOLBAR),
                           FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(pwHbox), gtk_label_new(_(asz[i])), TRUE, TRUE, 10);
        gtk_container_add(GTK_CONTAINER(pwVbox), pwButtons);
        g_signal_connect(G_OBJECT(pwButtons), "clicked", G_CALLBACK(CallbackResign), GINT_TO_POINTER(i));
    }

    gtk_container_add(GTK_CONTAINER(DialogArea(pwDialog, DA_MAIN)), pwVbox);

    GTKRunDialog(pwDialog);
}

extern void
MoveListDestroy(void)
{
    if (pwMoveAnalysis) {
        gtk_widget_destroy(pwMoveAnalysis);
        pwMoveAnalysis = NULL;
    }
}

#if defined(USE_BOARD3D)
extern gboolean
display_is_3d(const renderdata* prd)
{
	g_assert(prd->fDisplayType == DT_2D || (prd->fDisplayType == DT_3D && widget3dValid));
	return (prd->fDisplayType == DT_3D);
}

extern gboolean
display_is_2d(const renderdata* prd)
{
	displaytype fdt = prd->fDisplayType;
	g_assert(fdt == DT_2D || fdt == DT_3D);
	return (fdt == DT_2D ? TRUE : FALSE);
}
#endif
