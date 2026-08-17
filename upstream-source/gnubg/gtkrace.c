/*
 * Copyright (C) 2002-2003 Joern Thyssen <jthyssen@dk.ibm.com>
 * Copyright (C) 2003-2022 the AUTHORS
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
 * $Id: gtkrace.c,v 1.52 2022/09/03 21:12:09 plm Exp $
 */

#include "config.h"
#include <gtk/gtk.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "backgammon.h"
#include "gtkrace.h"
#include "osr.h"
#include "format.h"
#include "gtkwindows.h"

typedef struct {
    GtkWidget *apwEPC[2];
    GtkWidget *apwWastage[2];
} epcwidget;

typedef struct {
    GtkAdjustment *padjTrials;
    GtkWidget *pwRollout, *pwOutput;
    TanBoard anBoard;
    epcwidget epcwOSR;
    int fMove;
} racewidget;

static gint current_page_num = 0;

static GtkWidget *
monospace_text(const char *szOutput)
{
    GtkWidget *pwText;
    GtkTextBuffer *buffer;
    GtkTextIter iter;

    pwText = gtk_text_view_new();
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(pwText), GTK_WRAP_NONE);

    buffer = gtk_text_buffer_new(NULL);
    gtk_text_buffer_create_tag(buffer, "monospace", "family", "monospace", NULL);
    gtk_text_buffer_get_end_iter(buffer, &iter);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, szOutput, -1, "monospace", NULL);
    gtk_text_view_set_buffer(GTK_TEXT_VIEW(pwText), buffer);
    return pwText;
}

static GtkWidget *
KleinmanPage(TanBoard anBoard, const int UNUSED(fMove))
{
    char sz[500];
    show_kleinman(anBoard, sz);
    return monospace_text(sz);
}

static GtkWidget *
TwoSidedPage(TanBoard anBoard, const int UNUSED(fMove))
{
    char sz[500];
    show_bearoff(anBoard, sz);
    return monospace_text(sz);
}

static GtkWidget *
KeithPage(TanBoard anBoard, const int UNUSED(fMove))
{
    char sz[500];
    show_keith(anBoard, sz);
    return monospace_text(sz);
}

static GtkWidget *
IsightPage(TanBoard anBoard, const int UNUSED(fMove))
{
    char sz[500];
    show_isight(anBoard, sz);
    return monospace_text(sz);
}

static GtkWidget *
Pip8912Page(TanBoard anBoard, const int UNUSED(fMove))
{
    char sz[500];
    show_8912(anBoard, sz);
    return monospace_text(sz);
}

static GtkWidget *
ThorpPage(TanBoard anBoard, const int UNUSED(fMove))
{
    char sz[500];
    show_thorp(anBoard, sz);
    return monospace_text(sz);
}

static GtkWidget *
EffectivePipCount(const float arPips[2], const float arWastage[2], const int fInvert, epcwidget * pepcw)
{
#if GTK_CHECK_VERSION(3,0,0)
    GtkWidget *pwGrid = gtk_grid_new();
#else
    GtkWidget *pwTable = gtk_table_new(3, 4, FALSE);
#endif
    GtkWidget *pwvbox;
    GtkWidget *pw;
    GtkWidget *pwFrame;
    unsigned int i;

    pwFrame = gtk_frame_new(_("Effective pip count"));

#if GTK_CHECK_VERSION(3,0,0)
    pwvbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
    pwvbox = gtk_vbox_new(FALSE, 0);
#endif

    gtk_container_add(GTK_CONTAINER(pwFrame), pwvbox);
    gtk_container_set_border_width(GTK_CONTAINER(pwvbox), 4);

    /* table */

#if GTK_CHECK_VERSION(3,0,0)
    gtk_box_pack_start(GTK_BOX(pwvbox), pwGrid, FALSE, FALSE, 4);

    gtk_grid_attach(GTK_GRID(pwGrid), pw = gtk_label_new(_("EPC")), 1, 0, 1, 1);
#if GTK_CHECK_VERSION(3,12,0)
    gtk_widget_set_margin_start(pw, 4);
    gtk_widget_set_margin_end(pw, 4);
#else
    gtk_widget_set_margin_left(pw, 4);
    gtk_widget_set_margin_right(pw, 4);
#endif
    gtk_widget_set_margin_top(pw, 4);
    gtk_widget_set_margin_bottom(pw, 4);

    gtk_grid_attach(GTK_GRID(pwGrid), pw = gtk_label_new(_("Wastage")), 2, 0, 1, 1);
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
    gtk_box_pack_start(GTK_BOX(pwvbox), pwTable, FALSE, FALSE, 4);

    gtk_table_attach(GTK_TABLE(pwTable), gtk_label_new(_("EPC")), 1, 2, 0, 1, 0, 0, 4, 4);
    gtk_table_attach(GTK_TABLE(pwTable), gtk_label_new(_("Wastage")), 2, 3, 0, 1, 0, 0, 4, 4);
#endif

    for (i = 0; i < 2; ++i) {

        gchar *sz = g_strdup_printf(_("Player %s"), ap[i].szName);

#if GTK_CHECK_VERSION(3,0,0)
        gtk_grid_attach(GTK_GRID(pwGrid), pw = gtk_label_new(sz), 0, i + 1, 1, 1);
#if GTK_CHECK_VERSION(3,12,0)
        gtk_widget_set_margin_start(pw, 4);
        gtk_widget_set_margin_end(pw, 4);
#else
        gtk_widget_set_margin_left(pw, 4);
        gtk_widget_set_margin_right(pw, 4);
#endif
        gtk_widget_set_margin_top(pw, 4);
        gtk_widget_set_margin_bottom(pw, 4);

        gtk_widget_set_halign(pw, GTK_ALIGN_START);
        gtk_widget_set_valign(pw, GTK_ALIGN_CENTER);
#else
        gtk_table_attach(GTK_TABLE(pwTable), pw = gtk_label_new(sz), 0, 1, i + 1, i + 2, 0, 0, 4, 4);
        gtk_misc_set_alignment(GTK_MISC(pw), 0, 0.5);
#endif

        g_free(sz);

        sz = g_strdup_printf("%7.3f", arPips[fInvert ? !i : i]);

#if GTK_CHECK_VERSION(3,0,0)
        gtk_grid_attach(GTK_GRID(pwGrid), pw = gtk_label_new(sz), 1, i + 1, 1, 1);
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
        gtk_table_attach(GTK_TABLE(pwTable), pw = gtk_label_new(sz), 1, 2, i + 1, i + 2, 0, 0, 4, 4);
#endif

        g_free(sz);
        if (pepcw)
            pepcw->apwEPC[i] = pw;

        sz = g_strdup_printf("%7.3f", arWastage[fInvert ? !i : i]);

#if GTK_CHECK_VERSION(3,0,0)
        gtk_grid_attach(GTK_GRID(pwGrid), pw = gtk_label_new(sz), 2, i + 1, 1, 1);
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
        gtk_table_attach(GTK_TABLE(pwTable), pw = gtk_label_new(sz), 2, 3, i + 1, i + 2, 0, 0, 4, 4);
#endif

        g_free(sz);
        if (pepcw)
            pepcw->apwWastage[i] = pw;

    }

    gtk_box_pack_start(GTK_BOX(pwvbox),
                       pw = gtk_label_new(_("EPC = Effective pip count = Avg. rolls * 8.167")), FALSE, FALSE, 0);
#if GTK_CHECK_VERSION(3,0,0)
    gtk_widget_set_halign(pw, GTK_ALIGN_START);
    gtk_widget_set_valign(pw, GTK_ALIGN_CENTER);
#else
    gtk_misc_set_alignment(GTK_MISC(pw), 0, 0.5);
#endif

    gtk_box_pack_start(GTK_BOX(pwvbox), pw = gtk_label_new(_("Wastage = EPC - Pips")), FALSE, FALSE, 0);
#if GTK_CHECK_VERSION(3,0,0)
    gtk_widget_set_halign(pw, GTK_ALIGN_START);
    gtk_widget_set_valign(pw, GTK_ALIGN_CENTER);
#else
    gtk_misc_set_alignment(GTK_MISC(pw), 0, 0.5);
#endif

    return pwFrame;
}

static void
PerformOSR(GtkWidget * UNUSED(pw), racewidget * prw)
{

    unsigned int nTrials = (unsigned int) gtk_adjustment_get_value(prw->padjTrials);
    float ar[5];
    int i;
    char sz[16];
    unsigned int anPips[2];
    const float x = (2 * 3 + 3 * 4 + 4 * 5 + 4 * 6 + 6 * 7 +
                     5 * 8 + 4 * 9 + 2 * 10 + 2 * 11 + 1 * 12 + 1 * 16 + 1 * 20 + 1 * 24) / 36.0f;
    float arMu[2];
    GtkTreeIter iter;
    GtkTreeModel *store;

    raceProbs((ConstTanBoard) prw->anBoard, nTrials, ar, arMu);

    PipCount((ConstTanBoard) prw->anBoard, anPips);

    store = gtk_tree_view_get_model(GTK_TREE_VIEW(prw->pwOutput));
    gtk_tree_model_get_iter_first(store, &iter);
    for (i = 0; i < 5; ++i) {
        if (fOutputWinPC)
            sprintf(sz, "%5.1f%%", ar[i] * 100.0f);
        else
            sprintf(sz, "%5.3f", ar[i]);
        gtk_list_store_set(GTK_LIST_STORE(store), &iter, i + 1, sz, -1);
    }

    /* effective pip count */

    for (i = 0; i < 2; ++i) {
        gchar *pch;
        int j = prw->fMove ? i : !i;

        pch = g_strdup_printf("%7.3f", arMu[j] * x);
        gtk_label_set_text(GTK_LABEL(prw->epcwOSR.apwEPC[i]), pch);
        g_free(pch);

        pch = g_strdup_printf("%7.3f", arMu[j] * x - (float) anPips[j]);
        gtk_label_set_text(GTK_LABEL(prw->epcwOSR.apwWastage[i]), pch);
        g_free(pch);

    }

}

static GtkWidget *
do_rollout_view(void)
{
    GtkWidget *view;
    GtkListStore *store;
    GtkCellRenderer *renderer;
    GtkTreeIter iter;
    int i;

    const char *aszTitle[] = {
        " ",                    /* neither NULL (undefined behaviour as gettext argument) nor "" (special meaning) */
        N_("Win"),
        N_("W g"),
        N_("W bg"),
        N_("L g"),
        N_("L bg")
    };
    store = gtk_list_store_new(6, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
                               G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(GTK_LIST_STORE(store), &iter, 0, _("Rollout"), -1);
    view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    g_object_unref(store);
    renderer = gtk_cell_renderer_text_new();
    for (i = 0; i < 6; i++) {
        gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), -1, gettext(aszTitle[i]),
                                                    renderer, "text", i, NULL);
    }
    return view;
}

static GtkWidget *
OSRPage(TanBoard UNUSED(anBoard), racewidget * prw)
{

    GtkWidget *pwvbox;
    GtkWidget *pw;
    float ar0[2] = { 0, 0 };
    char *pch;

#if GTK_CHECK_VERSION(3,0,0)
    pwvbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_widget_set_halign(pwvbox, GTK_ALIGN_START);
    gtk_widget_set_valign(pwvbox, GTK_ALIGN_START);
    gtk_container_set_border_width(GTK_CONTAINER(pwvbox), 4);
#else
    GtkWidget *align;

    align = gtk_alignment_new(0, 0, 0, 0);
    pwvbox = gtk_vbox_new(FALSE, 4);
    gtk_container_add(GTK_CONTAINER(align), pwvbox);
    gtk_container_set_border_width(GTK_CONTAINER(align), 4);
#endif

    prw->padjTrials = GTK_ADJUSTMENT(gtk_adjustment_new(5760, 1, 1296 * 1296, 36, 36, 0));
#if GTK_CHECK_VERSION(3,0,0)
    pw = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
#else
    pw = gtk_hbox_new(FALSE, 0);
#endif
    gtk_box_pack_start(GTK_BOX(pwvbox), pw, FALSE, FALSE, 4);

    gtk_box_pack_start(GTK_BOX(pw), gtk_label_new(_("Trials:")), FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(pw), gtk_spin_button_new(prw->padjTrials, 36, 0), TRUE, TRUE, 4);
    gtk_box_pack_start(GTK_BOX(pw), prw->pwRollout = gtk_button_new_with_label(_("Roll out")), TRUE, TRUE, 4);

    g_signal_connect(G_OBJECT(prw->pwRollout), "clicked", G_CALLBACK(PerformOSR), prw);

    /* separator */

#if GTK_CHECK_VERSION(3,0,0)
    gtk_box_pack_start(GTK_BOX(pwvbox), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, 4);
#else
    gtk_box_pack_start(GTK_BOX(pwvbox), gtk_hseparator_new(), FALSE, FALSE, 4);
#endif

    /* result */

    pch = g_strdup_printf(_("%s on roll:"), ap[prw->fMove].szName);
    gtk_box_pack_start(GTK_BOX(pwvbox), pw = gtk_label_new(pch), FALSE, FALSE, 4);
#if GTK_CHECK_VERSION(3,0,0)
    gtk_widget_set_halign(pw, GTK_ALIGN_START);
    gtk_widget_set_valign(pw, GTK_ALIGN_CENTER);
#else
    gtk_misc_set_alignment(GTK_MISC(pw), 0, 0.5);
#endif
    g_free(pch);

    prw->pwOutput = do_rollout_view();
    gtk_box_pack_start(GTK_BOX(pwvbox), prw->pwOutput, FALSE, FALSE, 4);

    /* effective pip count */

#if GTK_CHECK_VERSION(3,0,0)
    gtk_box_pack_start(GTK_BOX(pwvbox), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, 4);
#else
    gtk_box_pack_start(GTK_BOX(pwvbox), gtk_hseparator_new(), FALSE, FALSE, 4);
#endif

    gtk_box_pack_start(GTK_BOX(pwvbox), EffectivePipCount(ar0, ar0, !prw->fMove, &prw->epcwOSR), FALSE, FALSE, 4);

#if GTK_CHECK_VERSION(3,0,0)
    return pwvbox;
#else
    return align;
#endif

}

static void
set_current_page(GtkWidget * UNUSED(pw), gpointer UNUSED(page), guint page_num, gpointer * UNUSED(data))
{
    current_page_num = page_num;

    return;
}

/*
 * Display widget with various race metrics
 */

extern void
GTKShowRace(TanBoard anBoard)
{

    GtkWidget *pwDialog;
    GtkWidget *pwNotebook;

    racewidget *prw;

    prw = g_malloc(sizeof(racewidget));

    memcpy(prw->anBoard, anBoard, 2 * 25 * sizeof(int));
    prw->fMove = ms.fMove;

    /* create dialog */

    pwDialog = GTKCreateDialog(_("GNU Backgammon - Race Theory"), DT_INFO, NULL, DIALOG_FLAG_MODAL, NULL, NULL);

    /* add notebook pages */

    pwNotebook = gtk_notebook_new();
    gtk_container_add(GTK_CONTAINER(DialogArea(pwDialog, DA_MAIN)), pwNotebook);

    gtk_container_set_border_width(GTK_CONTAINER(pwNotebook), 4);

    /* 8-9-12 */

    gtk_notebook_append_page(GTK_NOTEBOOK(pwNotebook), Pip8912Page(anBoard, prw->fMove), gtk_label_new(_("8912 Rule")));

    /* Kleinman */

    gtk_notebook_append_page(GTK_NOTEBOOK(pwNotebook),
                             KleinmanPage(anBoard, prw->fMove), gtk_label_new(_("Kleinman Formula")));

    /* Thorp */

    gtk_notebook_append_page(GTK_NOTEBOOK(pwNotebook), ThorpPage(anBoard, prw->fMove), gtk_label_new(_("Thorp Count")));

    /* Keith */

    gtk_notebook_append_page(GTK_NOTEBOOK(pwNotebook), KeithPage(anBoard, prw->fMove), gtk_label_new(_("Keith Count")));

    /* Isight */

    gtk_notebook_append_page(GTK_NOTEBOOK(pwNotebook), IsightPage(anBoard, prw->fMove),
                             gtk_label_new(_("Isight Count")));

    /* One sided rollout */

    gtk_notebook_append_page(GTK_NOTEBOOK(pwNotebook),
                             OSRPage(anBoard, prw), gtk_label_new(_("One-Sided Rollout/Database")));

    /* Two-sided database */
    gtk_notebook_append_page(GTK_NOTEBOOK(pwNotebook),
                             TwoSidedPage(anBoard, prw->fMove), gtk_label_new(_("Two-Sided Database")));

    g_signal_connect_after(G_OBJECT(pwNotebook), "switch-page", G_CALLBACK(set_current_page), pwNotebook);

    g_object_set_data_full(G_OBJECT(pwDialog), "racewidget", prw, g_free);

    /* show dialog */

    /* OSR can take a long time for non-race positions */
    if (ClassifyPosition(msBoard(), ms.bgv) <= CLASS_RACE)
        PerformOSR(NULL, prw);

    gtk_widget_show_all(pwNotebook);
    gtk_notebook_set_current_page(GTK_NOTEBOOK(pwNotebook), current_page_num);

    GTKRunDialog(pwDialog);
}
