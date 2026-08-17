/*
 * Copyright (C) 2002-2003 Joern Thyssen <jthyssen@dk.ibm.com>
 * Copyright (C) 2002-2023 the AUTHORS
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
 * $Id: gtkchequer.c,v 1.135 2023/01/18 21:49:36 plm Exp $
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "backgammon.h"
#include "drawboard.h"
#include "eval.h"
#include "positionid.h"
#include "rollout.h"
#include "gtkboard.h"
#include "gtkgame.h"
#include "gtkchequer.h"
#include "gtktempmap.h"
#include "gtkscoremap.h"
#include "gtkwindows.h"
#include "progress.h"
#include "format.h"
#include "gtklocdefs.h"

int showMoveListDetail = 1;
moverecord *pmrCurAnn;

static void
MoveListRolloutClicked(GtkWidget * pw, hintdata * phd)
{
    cubeinfo ci;
    int *ai;
    void *p;
    GList *pl, *plSelList = MoveListGetSelectionList(phd);
    int c;

    if (!plSelList)
        return;

    GetMatchStateCubeInfo(&ci, &ms);

    c = g_list_length(plSelList);

    /* setup rollout dialog */
    {
        move **ppm = (move **) g_malloc(c * sizeof(move *));
        cubeinfo **ppci = (cubeinfo **) g_malloc(c * sizeof(cubeinfo *));
        char (*asz)[FORMATEDMOVESIZE] = (char (*)[FORMATEDMOVESIZE]) g_malloc(FORMATEDMOVESIZE * c);
        int i, res;

        for (i = 0, pl = plSelList; i < c; pl = pl->next, i++) {
            move *m = ppm[i] = MoveListGetMove(phd, pl);

            ppci[i] = &ci;
            FormatMove(asz[i], msBoard(), m->anMove);
        }
        MoveListFreeSelectionList(plSelList);

        GTKSetCurrentParent(pw);
        RolloutProgressStart(&ci, c, NULL, &rcRollout, asz, FALSE, &p);

        res = ScoreMoveRollout(ppm, ppci, c, RolloutProgress, p);

        RolloutProgressEnd(&p, FALSE);

        g_free(asz);
        g_free(ppm);
        g_free(ppci);

        if (res < 0)
            return;

        /* If the source widget parent has been destroyed do not attempt
         * to update the hint window */
        if (!GDK_IS_WINDOW(gtk_widget_get_parent_window(pw)))
            return;

        /* Calling RefreshMoveList here requires some extra work, as
         * it may reorder moves */
        MoveListUpdate(phd);
    }

    MoveListClearSelection(0, 0, phd);

    ai = (int *) g_malloc(phd->pml->cMoves * sizeof(int));
    RefreshMoveList(phd->pml, ai);

    if (phd->piHighlight && phd->pml->cMoves)
        *phd->piHighlight = ai[*phd->piHighlight];

    g_free(ai);

    find_skills(phd->pmr, &ms, -1, -1);
    MoveListUpdate(phd);
    if (phd->hist) {
        SetAnnotation(pmrCurAnn);
        ChangeGame(NULL);
    }
}

extern void
ShowMove(hintdata * phd, const int f)
{
    if (f) {
        move *pm;
        GList *plSelList = MoveListGetSelectionList(phd);
        TanBoard anBoard;

        if (!plSelList)
            return;

        /* the button is toggled */
        pm = MoveListGetMove(phd, plSelList);

        MoveListFreeSelectionList(plSelList);

        memcpy(anBoard, msBoard(), sizeof(TanBoard));
        ApplyMove(anBoard, pm->anMove, FALSE);

        UpdateMove((BOARD(pwBoard))->board_data, anBoard);
    } else {

        gchar *sz = g_strdup("show board");
        UserCommand(sz);
        g_free(sz);

    }
}

static void
MoveListTempMapClicked(GtkWidget * pw, hintdata * phd)
{
    GList *pl;
    char szMove[FORMATEDMOVESIZE];
    matchstate *ams;
    int i, c;
    gchar **asz;

    GList *plSelList = MoveListGetSelectionList(phd);
    if (!plSelList)
        return;

    c = g_list_length(plSelList);

    ams = (matchstate *) g_malloc(c * sizeof(matchstate));
    asz = (gchar **) g_malloc(c * sizeof(gchar *));

    for (i = 0, pl = plSelList; pl; pl = pl->next, ++i) {

        move *m = MoveListGetMove(phd, pl);

        /* Apply move to get board */

        memcpy(&ams[i], &ms, sizeof(matchstate));

        FormatMove(szMove, (ConstTanBoard) ams[i].anBoard, m->anMove);
        ApplyMove(ams[i].anBoard, m->anMove, FALSE);

        /* Swap sides */

        SwapSides(ams[i].anBoard);
        ams[i].fMove = !ams[i].fMove;
        ams[i].fTurn = !ams[i].fTurn;

        /* Show temp map dialog */

        asz[i] = g_strdup(szMove);

    }
    MoveListFreeSelectionList(plSelList);

    GTKSetCurrentParent(pw);
    GTKShowTempMap(ams, c, asz, TRUE);

    g_free(ams);
    for (i = 0; i < c; ++i)
        g_free(asz[i]);
    g_free(asz);
}

static void
MoveListScoreMapClicked(GtkWidget * UNUSED(pw), hintdata * UNUSED(phd))
{

    char *sz = g_strdup("show scoremap =move"); //cf keyword -> gtkgame.c: CMD_SHOW_SCORE_MAP_MOVE
    UserCommand(sz);
    g_free(sz);  

}

static void
MoveListCmarkClicked(GtkWidget * UNUSED(pw), hintdata * phd)
{
    CMark new_mark;
    guint all_marked = TRUE;
    GList *pl;
    GList *plSelList = MoveListGetSelectionList(phd);
    if (!plSelList)
        return;
    for (pl = plSelList; pl; pl = pl->next) {

        move *m = MoveListGetMove(phd, pl);
        if (m->cmark == CMARK_NONE)
            all_marked = FALSE;
    }
    new_mark = all_marked ? CMARK_NONE : CMARK_ROLLOUT;
    for (pl = plSelList; pl; pl = pl->next) {

        move *m = MoveListGetMove(phd, pl);
        m->cmark = new_mark;
    }
    MoveListFreeSelectionList(plSelList);
    MoveListUpdate(phd);
}

static void
MoveListMWC(GtkWidget * pw, hintdata * phd)
{
    int f = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pw));

    if (f != fOutputMWC) {
        gchar *sz = g_strdup_printf("set output mwc %s", fOutputMWC ? "off" : "on");

        UserCommand(sz);
        UserCommand("save settings");

        g_free(sz);
    }

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pw), fOutputMWC);

    /* Make sure display is up to date */
    MoveListUpdate(phd);
    SetAnnotation(pmrCurAnn);

}

static void
EvalMoves(hintdata * phd, evalcontext * pec)
{
    GList *pl;
    cubeinfo ci;
    int *ai;
    GList *plSelList = MoveListGetSelectionList(phd);

    if (!plSelList)
        return;

    GetMatchStateCubeInfo(&ci, &ms);

    for (pl = plSelList; pl; pl = pl->next) {
        scoreData sd;
        sd.pm = MoveListGetMove(phd, pl);
        sd.pci = &ci;
        sd.pec = pec;

        if (RunAsyncProcess((AsyncFun) asyncScoreMove, &sd, _("Evaluating positions...")) != 0) {
            MoveListFreeSelectionList(plSelList);
            return;
        }

        /* Calling RefreshMoveList here requires some extra work, as
         * it may reorder moves */
        MoveListUpdate(phd);
    }
    MoveListFreeSelectionList(plSelList);

    MoveListClearSelection(0, 0, phd);

    ai = (int *) g_malloc(phd->pml->cMoves * sizeof(int));
    RefreshMoveList(phd->pml, ai);

    if (phd->piHighlight && phd->pml->cMoves)
        *phd->piHighlight = ai[*phd->piHighlight];

    g_free(ai);

    find_skills(phd->pmr, &ms, -1, -1);
    MoveListUpdate(phd);
    if (phd->hist) {
        SetAnnotation(pmrCurAnn);
        ChangeGame(NULL);
    }
}

static void
MoveListEval(GtkWidget * UNUSED(pw), hintdata * phd)
{
    EvalMoves(phd, &GetEvalChequer()->ec);
}

static void
MoveListEvalPly(GtkWidget * pw, hintdata * phd)
{
    char *szPly = (char *) g_object_get_data(G_OBJECT(pw), "user_data");
    evalcontext ec = { TRUE, 0, TRUE, TRUE, 0.0 };
    /* Reset interrupt flag */
    fInterrupt = FALSE;

    ec.nPlies = atoi(szPly);

    EvalMoves(phd, &ec);
}

static void
MoveListEvalSettings(GtkWidget * pw, void *UNUSED(unused))
{
    GTKSetCurrentParent(pw);
    SetAnalysis(NULL, 0, NULL);

    /* bring the dialog holding this button to the top */
    gtk_window_present(GTK_WINDOW(gtk_widget_get_toplevel(pw)));
}

static void
MoveListRolloutSettings(GtkWidget * pw, void *UNUSED(unused))
{
    GTKSetCurrentParent(pw);
    SetRollouts(NULL, 0, NULL);

    /* bring the dialog holding this button to the top */
    gtk_window_present(GTK_WINDOW(gtk_widget_get_toplevel(pw)));
}

static void
MoveListRolloutPresets(GtkWidget * pw, hintdata * phd)
{
    const gchar *preset;
    gchar *file = NULL;
    gchar *path = NULL;
    gchar *command = NULL;

    preset = (const gchar *) g_object_get_data(G_OBJECT(pw), "user_data");
    file = g_strdup_printf("%s.rol", preset);
    path = g_build_filename(szHomeDirectory, "rol", file, NULL);
    if (g_file_test(path, G_FILE_TEST_IS_REGULAR)) {
        command = g_strdup_printf("load commands \"%s\"", path);
        outputoff();
        UserCommand(command);
        outputon();
        MoveListRolloutClicked(pw, phd);
    } else {
        outputerrf(_("You need to save a preset as \"%s\""), file);
        MoveListRolloutSettings(pw, NULL);
    }
    g_free(file);
    g_free(path);
    g_free(command);
}

typedef int (*cfunc) (const void *, const void *);

static int
CompareInts(const int *p0, const int *p1)
{
    return *p0 - *p1;
}

static char *
MoveListCopyData(hintdata * phd)
{
    int c, i;
    GList *pl;
    char *sz, *pch;
    int *an;
    GList *plSelList = MoveListGetSelectionList(phd);
    c = g_list_length(plSelList);

    an = (int *) g_malloc(c * sizeof(an[0]));
    /* TODO: This needs to be cleaned up since the maximum number of
     * lines or length of a string can vary depending on settings */
    sz = (char *) g_malloc(c * 25 * 80);

    *sz = 0;

    for (i = 0, pl = plSelList; pl; pl = pl->next, i++) {
        move *m = MoveListGetMove(phd, pl);
        int rank = 0;
        while (&phd->pml->amMoves[rank] != m)
            rank++;
        an[i] = rank;
    }

    MoveListFreeSelectionList(plSelList);

    qsort(an, c, sizeof(an[0]), (cfunc) CompareInts);

    for (i = 0, pch = sz; i < c; i++, pch = strchr(pch, 0))
        FormatMoveHint(pch, &ms, phd->pml, an[i], TRUE, TRUE, TRUE);

    g_free(an);

    return sz;
}

static void
MoveListMove(GtkWidget * pw, hintdata * phd)
{
    move m;
    move *pm;
    char szMove[40];
    GList *plSelList = MoveListGetSelectionList(phd);
    if (!plSelList)
        return;

    ShowMove(phd, TRUE);
    pm = MoveListGetMove(phd, plSelList);
    MoveListFreeSelectionList(plSelList);

    memcpy(&m, pm, sizeof(move));

    if (phd->fDestroyOnMove)
        /* Destroy widget on exit */
        gtk_widget_destroy(gtk_widget_get_toplevel(pw));

    FormatMove(szMove, msBoard(), m.anMove);
    UserCommand(szMove);
}

static void
MoveListDetailsClicked(GtkWidget * pw, hintdata * UNUSED(phd))
{
    showMoveListDetail = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pw));
    /* Reshow list */
    SetAnnotation(pmrCurAnn);
}

GtkWidget *pwDetails;

static void
MoveListCopy(GtkWidget * UNUSED(pw), hintdata * phd)
{
    char *pc = MoveListCopyData(phd);
    if (pc) {
        GTKTextWindow(pc, _("Move details"), DT_INFO, NULL);
        g_free(pc);
    }
}

static GtkWidget *
CreateMoveListTools(hintdata * phd)
{
    GtkWidget *pwTools;
    GtkWidget *pwEval = gtk_button_new_with_label(_("Eval"));
    GtkWidget *pwEvalSettings = gtk_button_new_with_label(_("..."));
    GtkWidget *pwRollout = gtk_button_new_with_label(_("Rollout"));
    GtkWidget *pwRolloutSettings = gtk_button_new_with_label(_("..."));
    GtkWidget *pwMWC = gtk_toggle_button_new_with_label(_("MWC"));
    GtkWidget *pwMove = gtk_button_new_with_label(Q_("verb|Move"));
    GtkWidget *pwShow = gtk_toggle_button_new_with_label(_("Show"));
    GtkWidget *pwCopy = gtk_button_new_with_label(_("Copy"));
    GtkWidget *pwTempMap = gtk_button_new_with_label(_("TM"));
    GtkWidget *pwCmark = gtk_button_new_with_label(_("Cmark"));
    GtkWidget *pwScoreMap = gtk_button_new_with_label(_("ScoreMap"));     
    int i;

    pwDetails = phd->fDetails ? NULL : gtk_toggle_button_new_with_label(_("Details"));
    phd->pwRollout = pwRollout;
    phd->pwRolloutSettings = pwRolloutSettings;
    phd->pwEval = pwEval;
    phd->pwEvalSettings = pwEvalSettings;
    phd->pwMove = pwMove;
    phd->pwShow = pwShow;
    phd->pwCopy = pwCopy;
    phd->pwTempMap = pwTempMap;
    phd->pwCmark = pwCmark;
    phd->pwScoreMap = pwScoreMap;

#if GTK_CHECK_VERSION(3,0,0)
    gtk_style_context_add_class(gtk_widget_get_style_context(pwEval), "gnubg-analysis-button");
    gtk_style_context_add_class(gtk_widget_get_style_context(pwEvalSettings), "gnubg-analysis-button");
    gtk_style_context_add_class(gtk_widget_get_style_context(pwRollout), "gnubg-analysis-button");
    gtk_style_context_add_class(gtk_widget_get_style_context(pwRolloutSettings), "gnubg-analysis-button");
    gtk_style_context_add_class(gtk_widget_get_style_context(pwMWC), "gnubg-analysis-button");
    gtk_style_context_add_class(gtk_widget_get_style_context(pwMove), "gnubg-analysis-button");
    gtk_style_context_add_class(gtk_widget_get_style_context(pwShow), "gnubg-analysis-button");
    gtk_style_context_add_class(gtk_widget_get_style_context(pwCopy), "gnubg-analysis-button");
    gtk_style_context_add_class(gtk_widget_get_style_context(pwTempMap), "gnubg-analysis-button");
    gtk_style_context_add_class(gtk_widget_get_style_context(pwCmark), "gnubg-analysis-button");
#endif

    /* toolbox on the left with buttons for eval, rollout and more */

#if GTK_CHECK_VERSION(3,0,0)
    pwTools = gtk_grid_new();

    gtk_grid_attach(GTK_GRID(pwTools), pwEval, 0, 0, 1, 1);

    gtk_grid_attach(GTK_GRID(pwTools), pwEvalSettings, 1, 0, 1, 1);
#else
    pwTools = gtk_table_new(2, 7, FALSE);

    gtk_table_attach(GTK_TABLE(pwTools), pwEval, 0, 1, 0, 1,
                     (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);

    gtk_table_attach(GTK_TABLE(pwTools), pwEvalSettings, 1, 2, 0, 1,
                     (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);
#endif

#if GTK_CHECK_VERSION(3,0,0)
    phd->pwEvalPly = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

    gtk_grid_attach(GTK_GRID(pwTools), phd->pwEvalPly, 2, 0, 1, 1);
#else
    phd->pwEvalPly = gtk_hbox_new(FALSE, 0);

    gtk_table_attach(GTK_TABLE(pwTools), phd->pwEvalPly, 2, 3, 0, 1,
                     (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);
#endif

    for (i = 0; i < 5; ++i) {

        gchar *sz = g_strdup_printf("%d", i);  /* string is freed by set_data_full */
        GtkWidget *pwply = gtk_button_new_with_label(sz);

#if GTK_CHECK_VERSION(3,0,0)
        gtk_style_context_add_class(gtk_widget_get_style_context(pwply), "gnubg-analysis-button");
#endif
        gtk_box_pack_start(GTK_BOX(phd->pwEvalPly), pwply, TRUE, TRUE, 0);

        g_signal_connect(G_OBJECT(pwply), "clicked", G_CALLBACK(MoveListEvalPly), phd);

        g_object_set_data_full(G_OBJECT(pwply), "user_data", sz, g_free);

        sz = g_strdup_printf(_("Evaluate play on cubeful %d-ply"), i);
        gtk_widget_set_tooltip_text(pwply, sz);
        g_free(sz);

    }

#if GTK_CHECK_VERSION(3,0,0)
    gtk_grid_attach(GTK_GRID(pwTools), pwShow, 3, 0, 1, 1);

    gtk_grid_attach(GTK_GRID(pwTools), pwMWC, 4, 0, 1, 1);

    if (!phd->fDetails)
        gtk_grid_attach(GTK_GRID(pwTools), pwDetails, 5, 0, 1, 1);

    gtk_grid_attach(GTK_GRID(pwTools), pwRollout, 0, 1, 1, 1);

    gtk_grid_attach(GTK_GRID(pwTools), pwRolloutSettings, 1, 1, 1, 1);
#else
    gtk_table_attach(GTK_TABLE(pwTools), pwShow, 3, 4, 0, 1,
                     (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);

    gtk_table_attach(GTK_TABLE(pwTools), pwMWC, 4, 5, 0, 1,
                     (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);

    if (!phd->fDetails)
        gtk_table_attach(GTK_TABLE(pwTools), pwDetails, 5, 6, 0, 1,
                         (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 0, 0);

    gtk_table_attach(GTK_TABLE(pwTools), pwRollout, 0, 1, 1, 2,
                     (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);

    gtk_table_attach(GTK_TABLE(pwTools), pwRolloutSettings, 1, 2, 1, 2,
                     (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);
#endif

#if GTK_CHECK_VERSION(3,0,0)
    phd->pwRolloutPresets = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_grid_attach(GTK_GRID(pwTools), phd->pwRolloutPresets, 2, 1, 1, 1);
#else
    phd->pwRolloutPresets = gtk_hbox_new(FALSE, 0);
    gtk_table_attach(GTK_TABLE(pwTools), phd->pwRolloutPresets, 2, 3, 1, 2,
                     (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);
#endif

    for (i = 0; i < 5; ++i) {
        gchar *sz = g_strdup_printf("%c", i + 'a');    /* string is freed by set_data_full */
        GtkWidget *ro_preset = gtk_button_new_with_label(sz);

#if GTK_CHECK_VERSION(3,0,0)
        gtk_style_context_add_class(gtk_widget_get_style_context(ro_preset), "gnubg-analysis-button");
#endif
        gtk_box_pack_start(GTK_BOX(phd->pwRolloutPresets), ro_preset, TRUE, TRUE, 0);

        g_signal_connect(G_OBJECT(ro_preset), "clicked", G_CALLBACK(MoveListRolloutPresets), phd);

        g_object_set_data_full(G_OBJECT(ro_preset), "user_data", sz, g_free);

        sz = g_strdup_printf(_("Rollout preset %c"), i + 'a');
        gtk_widget_set_tooltip_text(ro_preset, sz);
        g_free(sz);

    }

#if GTK_CHECK_VERSION(3,0,0)
    gtk_grid_attach(GTK_GRID(pwTools), pwMove, 3, 1, 1, 1);

    gtk_grid_attach(GTK_GRID(pwTools), pwCopy, 4, 1, 1, 1);

    gtk_grid_attach(GTK_GRID(pwTools), pwCmark, 5, 1, 1, 1);

    gtk_grid_attach(GTK_GRID(pwTools), pwTempMap, 6, 0, 1, 1);

    gtk_grid_attach(GTK_GRID(pwTools), pwScoreMap, 6, 1, 1, 1);
#else
    gtk_table_attach(GTK_TABLE(pwTools), pwMove, 3, 4, 1, 2,
                     (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);

    gtk_table_attach(GTK_TABLE(pwTools), pwCopy, 4, 5, 1, 2,
                     (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);

    gtk_table_attach(GTK_TABLE(pwTools), pwCmark, 5, 6, 1, 2,
                     (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);

    gtk_table_attach(GTK_TABLE(pwTools), pwTempMap, 6, 7, 0, 1,
                     (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);

    gtk_table_attach(GTK_TABLE(pwTools), pwScoreMap, 6, 7, 1, 2,
                     (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0); 
#endif

            // g_message("GTK chequer: fAnalysisRunning=%d",fAnalysisRunning);
    /* some buttons are always disabled anyway */
        gtk_widget_set_sensitive(pwMove, FALSE);
    gtk_widget_set_sensitive(pwCopy, FALSE);
    gtk_widget_set_sensitive(pwTempMap, FALSE);

    /* We want to disable particularly when we are in the middle of running an analysis
    in the background*/
    gtk_widget_set_sensitive(pwMWC, ms.nMatchTo && !fAnalysisRunning);

    gtk_widget_set_sensitive(pwRollout, !fAnalysisRunning);
    gtk_widget_set_sensitive(pwRollout, !fAnalysisRunning);
    gtk_widget_set_sensitive(pwRolloutSettings, !fAnalysisRunning);
    gtk_widget_set_sensitive(pwEval, !fAnalysisRunning);
    gtk_widget_set_sensitive(pwEvalSettings, !fAnalysisRunning);
    gtk_widget_set_sensitive(pwMove, !fAnalysisRunning);
    gtk_widget_set_sensitive(pwCopy, !fAnalysisRunning);
    gtk_widget_set_sensitive(pwScoreMap, !fAnalysisRunning);



    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pwMWC), fOutputMWC);

    if (pwDetails)
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pwDetails), showMoveListDetail);
    /* signals */

    g_signal_connect(G_OBJECT(pwRollout), "clicked", G_CALLBACK(MoveListRolloutClicked), phd);
    g_signal_connect(G_OBJECT(pwEval), "clicked", G_CALLBACK(MoveListEval), phd);
    g_signal_connect(G_OBJECT(pwEvalSettings), "clicked", G_CALLBACK(MoveListEvalSettings), NULL);
    g_signal_connect(G_OBJECT(pwRolloutSettings), "clicked", G_CALLBACK(MoveListRolloutSettings), NULL);
    g_signal_connect(G_OBJECT(pwMWC), "toggled", G_CALLBACK(MoveListMWC), phd);
    g_signal_connect(G_OBJECT(pwMove), "clicked", G_CALLBACK(MoveListMove), phd);
    g_signal_connect(G_OBJECT(pwShow), "toggled", G_CALLBACK(MoveListShowToggledClicked), phd);
    g_signal_connect(G_OBJECT(pwCopy), "clicked", G_CALLBACK(MoveListCopy), phd);
    g_signal_connect(G_OBJECT(pwTempMap), "clicked", G_CALLBACK(MoveListTempMapClicked), phd);
    g_signal_connect(G_OBJECT(pwScoreMap), "clicked", G_CALLBACK(MoveListScoreMapClicked), phd);
    g_signal_connect(G_OBJECT(pwCmark), "clicked", G_CALLBACK(MoveListCmarkClicked), phd);
    if (!phd->fDetails)
        g_signal_connect(G_OBJECT(pwDetails), "clicked", G_CALLBACK(MoveListDetailsClicked), phd);

    /* tool tips */

    gtk_widget_set_tooltip_text(pwRollout, _("Rollout selected moves with current settings"));

    gtk_widget_set_tooltip_text(pwEval, _("Evaluate selected moves with current settings"));

    gtk_widget_set_tooltip_text(pwRolloutSettings, _("Modify rollout settings"));

    gtk_widget_set_tooltip_text(pwEvalSettings, _("Modify evaluation settings"));

    gtk_widget_set_tooltip_text(pwShow, _("Show position after selected move"));

    gtk_widget_set_tooltip_text(pwMWC, _("Toggle output as MWC or equity"));

    gtk_widget_set_tooltip_text(pwCopy, _("Copy selected moves to clipboard"));

    gtk_widget_set_tooltip_text(pwMove, _("Play the selected move"));

    gtk_widget_set_tooltip_text(pwCmark, _("Mark selected moves for later rollout"));

    gtk_widget_set_tooltip_text(pwTempMap, _("Show Sho Sengoku Temperature Map of position" " after selected move"));

    gtk_widget_set_tooltip_text(pwScoreMap, _("Show map of best moves at different scores"));


    return pwTools;
}

extern void
HintDoubleClick(GtkTreeView * UNUSED(treeview),
                GtkTreePath * UNUSED(path), GtkTreeViewColumn * UNUSED(col), hintdata * phd)
{
    gtk_button_clicked(GTK_BUTTON(phd->pwMove));
}

extern void
HintSelect(GtkTreeSelection * selection, hintdata * phd)
{
    CheckHintButtons(phd);
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(phd->pwShow))) {
        int c = gtk_tree_selection_count_selected_rows(selection);
        switch (c) {
        case 0:
        case 1:
            ShowMove(phd, c);
            break;

        default:
            ShowMove(phd, FALSE);
            break;
        }
    }
}

extern int
CheckHintButtons(hintdata * phd)
{
    BoardData *bd;

    GList *plSelList = MoveListGetSelectionList(phd);
    int c = g_list_length(plSelList);
    MoveListFreeSelectionList(plSelList);

    /* We want to disable particularly when we are in the middle of running an analysis
    in the background*/

    gtk_widget_set_sensitive(phd->pwMove, c == 1 && phd->fButtonsValid && !fAnalysisRunning);
    gtk_widget_set_sensitive(phd->pwCopy, c && phd->fButtonsValid && !fAnalysisRunning);
    gtk_widget_set_sensitive(phd->pwTempMap, c && phd->fButtonsValid && !fAnalysisRunning);
    gtk_widget_set_sensitive(phd->pwCmark, c && phd->fButtonsValid && !fAnalysisRunning);
    gtk_widget_set_sensitive(phd->pwRollout, c && phd->fButtonsValid && !fAnalysisRunning);
    gtk_widget_set_sensitive(phd->pwRolloutPresets, c && phd->fButtonsValid && !fAnalysisRunning);
    gtk_widget_set_sensitive(phd->pwEval, c && phd->fButtonsValid && !fAnalysisRunning);
    gtk_widget_set_sensitive(phd->pwEvalPly, c && phd->fButtonsValid && !fAnalysisRunning);

    bd = BOARD(pwBoard)->board_data;
    gtk_widget_set_sensitive(phd->pwScoreMap, (bd->diceShown == DICE_ON_BOARD) && !fAnalysisRunning);

    return c;
}

extern GtkWidget *
CreateMoveList(moverecord * pmr, const int fButtonsValid, const int fDestroyOnMove, const int fDetails, int hist)
{
    GtkWidget *pw;
    GtkWidget *pwVBox, *mlt;

    hintdata *phd = (hintdata *) g_malloc(sizeof(hintdata));

    /* This function should only be called when the game state matches
     * the move list. */
    g_assert(ms.fMove == 0 || ms.fMove == 1);

    if (pmr->n.iMove < pmr->ml.cMoves)
        phd->piHighlight = &pmr->n.iMove;
    else
        phd->piHighlight = NULL;
    phd->pmr = pmr;
    phd->pml = &pmr->ml;
    phd->fButtonsValid = fButtonsValid;
    phd->fDestroyOnMove = fDestroyOnMove;
    phd->pwMove = NULL;
    phd->fDetails = fDetails;
    phd->hist = hist;
    mlt = CreateMoveListTools(phd);
    MoveListCreate(phd);

    pw = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pw), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(pw), phd->pwMoves);

#if GTK_CHECK_VERSION(3,0,0)
    pwVBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
    pwVBox = gtk_vbox_new(FALSE, 0);
#endif
    gtk_box_pack_start(GTK_BOX(pwVBox), pw, TRUE, TRUE, 0);
    gtk_box_pack_end(GTK_BOX(pwVBox), mlt, FALSE, FALSE, 0);

    g_object_set_data_full(G_OBJECT(pwVBox), "user_data", phd, g_free);

    CheckHintButtons(phd);

    return pwVBox;
}
