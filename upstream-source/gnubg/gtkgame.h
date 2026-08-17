/*
 * Copyright (C) 2000-2003 Gary Wong <gtw@gnu.org>
 * Copyright (C) 2001-2016 the AUTHORS
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
 *
 * $Id: gtkgame.h,v 1.165 2023/11/20 21:00:06 plm Exp $
 */

#ifndef GTKGAME_H
#define GTKGAME_H

#include <gtk/gtk.h>
#include "backgammon.h"
#include "gtkwindows.h"
#include "gtkpanels.h"
#include "gtkchequer.h"

#define TOOLBAR_ACTION_OFFSET 10000

#if !defined(USE_GTKITEMFACTORY)
extern GtkUIManager *puim;
#endif

extern GdkColor wlCol;
extern GtkAccelGroup *pagMain;

extern GtkWidget *pwAnalysis;
extern GtkWidget *pwBoard;
extern GtkWidget *pwCommentary;
extern GtkWidget *pwGrab;
extern GtkWidget *pwMain;
extern GtkWidget *pwMessageText;
extern GtkWidget *pwMoveAnalysis;
extern GtkWidget *pwOldGrab;
extern GtkWidget *pwPanelVbox;
extern GtkWidget *pwToolbar;
extern GtkWidget *pwGnubgID;
extern guint nNextTurn;         /* GTK idle function */
extern int fEndDelay;
extern int fNeedPrompt;
extern int frozen;
extern int fTTY;
extern int fX;
extern int fToolbarShowing;
extern int inCallback;
extern unsigned int nDelay;
extern GtkWidget *hpaned;
extern GtkWidget *pom;
extern moverecord *pmrCurAnn;

extern char *ReturnHits(TanBoard anBoard);
extern gboolean ShowGameWindow(void);
extern gint MoveListClearSelection(GtkWidget * pw, GdkEventSelection * pes, hintdata * phd);
extern gint NextTurnNotify(gpointer p);
extern GList *MoveListGetSelectionList(const hintdata * phd);
extern GtkWidget *GetPanelWidget(gnubgwindow window);
extern GtkWidget *GL_Create(void);
extern int ArePanelsDocked(void);
extern int ArePanelsShowing(void);
extern int DockedPanelsShowing(void);
extern int edit_new(unsigned int length);
#if ! GTK_CHECK_VERSION(3,0,0)
extern int GetPanelWidth(gnubgwindow panel);
#endif
extern int GTKGetManualDice(unsigned int an[2]);
extern int GTKGetMove(int anMove[8]);
extern int GtkTutor(char *sz);
extern int IsPanelDocked(gnubgwindow window);
extern int IsPanelShowVar(gnubgwindow panel, void *p);
extern int SetMainWindowSize(void);
extern move *MoveListGetMove(const hintdata * phd, GList * pl);
extern void CommentaryChanged(GtkWidget * pw, GtkTextBuffer * buffer);
extern void DisplayWindows(void);
extern void DockPanels(void);
extern void FullScreenMode(int state);
extern void GetFullscreenWindowSettings(int *panels, int *ids, int *maxed);
extern void GetStyleFromRCFile(GtkStyle ** ppStyle, const char *name, GtkStyle * psBase);
extern void GL_SetNames(void);
extern void GTKAddGame(moverecord * pmr);
extern void GTKAddMoveRecord(moverecord * pmr);
extern void GTKAllowStdin(void);
extern void GTKBearoffProgress(int i);
extern void GTKCalibrationEnd(void *context);
extern void *GTKCalibrationStart(void);
extern void GTKCalibrationUpdate(void *context, float rEvalsPerSec);
extern void GtkChangeLanguage(void);
extern void GTKClearMoveRecord(void);
extern void GTKCommandShowCredits(GtkWidget * pw, GtkWidget * parent);
extern void GTKCubeHint(moverecord * pmr, const matchstate * pms, int did_double, int did_take, int hist);
extern void GTKDelay(void);
extern void GTKDisallowStdin(void);
extern void GTKDumpStatcontext(int game);
extern void GTKEval(char *szOutput);
extern void GTKFreeze(void);
extern void GTKHelp(char *sz);
extern void GTKHint(moverecord * pmr, int hist);
extern void GTKMatchInfo(void);
extern void GTKNew(void);
extern void GTKOutput(const char *sz);
extern void GTKOutputErr(const char *sz);
extern void GTKOutputNew(void);
extern void GTKOutputX(void);
extern void GTKPopGame(int i);
extern void GTKPopMoveRecord(moverecord * pmr);
extern void GTKProgressEnd(void);
extern void GTKProgressStart(const char *sz);
extern void GTKProgressStartValue(char *sz, int iMax);
extern void GTKProgressValue(int iValue, int iMax);
extern void GTKProgress(void);
extern void GTKRecordShow(FILE * pfIn, char *sz, char *szPlayer);
extern void GTKRegenerateGames(void);
extern void GTKResign(gpointer p, guint n, GtkWidget * pw);
extern void GTKResignHint(float arOutput[], float rEqBefore, float rEqAfter, cubeinfo * pci, int fMWC);
extern void GTKResumeInput(void);
extern void GTKSaveSettings(void);
extern void GTKSetCube(gpointer p, guint n, GtkWidget * pw);
extern void GTKSetDice(gpointer p, guint n, GtkWidget * pw);
extern void GTKSetGame(int i);
extern void GTKSetMoveRecord(moverecord * pmr);
extern void GTKSet(void *p);
extern void GTKShowBuildInfo(GtkWidget * pw, GtkWidget * pwParent);
extern void GTKShowCalibration(void);
extern void GTKShowScoreSheet(void);
extern void GTKShowVersion(void);
extern void GTKSuspendInput(void);
extern void GTKTextToClipboard(const char *text);
extern void GTKTextWindow(const char *szOutput, const char *title, const dialogtype type, GtkWidget * parent);
extern void GTKThaw(void);
extern void HideAllPanels(gpointer p, guint n, GtkWidget * pw);
extern void HintDoubleClick(GtkTreeView * treeview, GtkTreePath * path, GtkTreeViewColumn * col, hintdata * phd);
extern void HintSelect(GtkTreeSelection * selection, hintdata * phd);
extern void InitGTK(int *argc, char ***argv);
extern void MoveListCreate(hintdata * phd);
extern void MoveListFreeSelectionList(GList * pl);
extern void MoveListShowToggledClicked(GtkWidget * pw, hintdata * phd);
extern void MoveListUpdate(const hintdata * phd);
extern void OK(GtkWidget * pw, int *pf);
extern void PanelHide(gnubgwindow panel);
extern void PanelShow(gnubgwindow panel);
extern void RefreshGeometries(void);
extern void RunGTK(GtkWidget * pwSplash, char *commands, char *python_script, char *match);
extern void SetAnnotation(moverecord * pmr);
extern void SetAnalysis(gpointer p, guint n, GtkWidget * pw);
extern void SetMET(GtkWidget * pw, gpointer p);
extern void SetPanelWidget(gnubgwindow window, GtkWidget * pWin);
extern void SetRollouts(gpointer p, guint n, GtkWidget * pw);
extern void SetToolbarStyle(int value);
extern void setWindowGeometry(gnubgwindow window);
extern void ShowAllPanels(gpointer p, guint n, GtkWidget * pw);
extern void ShowHidePanel(gnubgwindow panel);
extern void ShowList(char *asz[], const char *szTitle, GtkWidget * parent);
extern void ShowMove(hintdata * phd, const int f);
extern void SwapBoardToPanel(int ToPanel, int updateEvents);
extern void DoHideAllPanels(int updateEvents);
#if defined(USE_GTKITEMFACTORY)
extern void ToggleDockPanels(gpointer p, guint n, GtkWidget * pw);
#else
extern void ToggleEdit(GtkToggleAction * action, gpointer user_data);
extern void ToggleClockwise(GtkToggleAction * action, gpointer user_data);
extern void ToggleDockPanels(GtkToggleAction * action, gpointer user_data);
#endif
extern void GTKUndo(void);
extern void ShowToolbar(void);
extern void HideToolbar(void);
extern void MoveListDestroy(void);

#if defined(HAVE_LIB_READLINE)
extern int fReadingCommand;
extern void ProcessInput(char *sz);
#endif

#if defined(USE_BOARD3D)
extern void SetSwitchModeMenuText(void);
#endif

extern void AddText(GtkWidget* pwBox, char* Text);
extern void drawArrow (cairo_t *cr, double start_x, double start_y, double end_x, double end_y);

#endif
