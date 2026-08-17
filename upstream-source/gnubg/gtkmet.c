/*
 * Copyright (C) 2002-2003 Joern Thyssen <jthyssen@dk.ibm.com>
 * Copyright (C) 2002-2022 the AUTHORS
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
 * $Id: gtkmet.c,v 1.36 2022/08/30 18:38:35 plm Exp $
 */

#include "config.h"

#include "gtkgame.h"
#include "matchequity.h"
#include "gtkmet.h"
#include "gtkwindows.h"
#include "gtklocdefs.h"

typedef struct {
#if GTK_CHECK_VERSION(3,0,0)
    GtkWidget *pwGrid;
#else
    GtkWidget *pwTable;
#endif
    GtkWidget *pwName;
    GtkWidget *pwFileName;
    GtkWidget *pwDescription;
    GtkWidget *aapwLabel[MAXSCORE][MAXSCORE];
} mettable;

typedef struct {
    GtkWidget *apwPostCrawford[2];
    GtkWidget *pwPreCrawford;
    unsigned int nMatchTo;
    unsigned int anAway[2];
} metwidget;


static void
UpdateTable(const mettable * pmt,
            /*lint -e{818} */ float met[][MAXSCORE],
            const metinfo * pmi, const unsigned int nRows, const unsigned int nCols, const int fInvert)
{

    unsigned int i, j;
    char sz[9];

    /* set labels */

    gtk_label_set_text(GTK_LABEL(pmt->pwName), (char *) pmi->szName);
    gtk_label_set_text(GTK_LABEL(pmt->pwFileName), (char *) pmi->szFileName);
    gtk_label_set_text(GTK_LABEL(pmt->pwDescription), (char *) pmi->szDescription);

    /* fill out table */

    for (i = 0; i < nRows; i++)
        for (j = 0; j < nCols; j++) {

            if (fInvert) {
                g_assert(met[j][i] >= 0.0f && met[j][i] <= 1.0f);
                sprintf(sz, "%8.4f", met[j][i] * 100.0f);
            }
            else {
                g_assert(met[i][j] >= 0.0f && met[i][j] <= 1.0f);
                sprintf(sz, "%8.4f", met[i][j] * 100.0f);
            }
            gtk_label_set_text(GTK_LABEL(pmt->aapwLabel[i][j]), sz);

        }

}


static void
UpdateAllTables(const metwidget * pmw)
{
    const mettable *pmt;
    int i;

    pmt = (const mettable *) g_object_get_data(G_OBJECT(pmw->pwPreCrawford), "mettable");
    UpdateTable(pmt, aafMET, &miCurrent, pmw->nMatchTo, pmw->nMatchTo, FALSE);

    for (i = 0; i < 2; ++i) {
        pmt = (const mettable *) g_object_get_data(G_OBJECT(pmw->apwPostCrawford[i]), "mettable");
        /* Ugly cast to pass the the aafMETPostCrawford[i] vector as a [1][] array */
        UpdateTable(pmt, (float (*)[MAXSCORE]) (void *) aafMETPostCrawford[i], &miCurrent, pmw->nMatchTo, 1, TRUE);
    }
}

static GtkWidget *
GTKWriteMET(const unsigned int nRows, const unsigned int nCols, const unsigned int nAway0, const unsigned int nAway1)
{
    GtkWidget *pwScrolledWindow = gtk_scrolled_window_new(NULL, NULL);
#if GTK_CHECK_VERSION(3,0,0)
    GtkWidget *pwGrid = gtk_grid_new();
    gtk_grid_set_column_homogeneous(GTK_GRID(pwGrid), TRUE);
    gtk_grid_set_row_homogeneous(GTK_GRID(pwGrid), TRUE);
#else
    GtkWidget *pwTable = gtk_table_new(nRows + 1, nCols + 1, TRUE);
#endif
    GtkWidget *pw, *pwBox;
    mettable *pmt;

#if GTK_CHECK_VERSION(3,0,0)
    g_object_set(G_OBJECT(pwScrolledWindow), "expand", TRUE, NULL);
    pwBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
    pwBox = gtk_vbox_new(FALSE, 0);
#endif

    pmt = (mettable *) g_malloc(sizeof(mettable));
#if GTK_CHECK_VERSION(3,0,0)
    pmt->pwGrid = pwGrid;
#else
    pmt->pwTable = pwTable;
#endif

    gtk_box_pack_start(GTK_BOX(pwBox), pmt->pwName = gtk_label_new((char *) miCurrent.szName), FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(pwBox), pmt->pwFileName = gtk_label_new((char *) miCurrent.szFileName), FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(pwBox),
                       pmt->pwDescription = gtk_label_new((char *) miCurrent.szDescription), FALSE, FALSE, 4);

    gtk_box_pack_start(GTK_BOX(pwBox), pwScrolledWindow, TRUE, TRUE, 0);

#if GTK_CHECK_VERSION(3,0,0)
    gtk_container_add(GTK_CONTAINER(pwScrolledWindow), pwGrid);
#else
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(pwScrolledWindow), pwTable);
#endif

    /* header for rows */

    for (unsigned int i = 0; i < nCols; i++) {
        gchar *sz = g_strdup_printf(_("%u-away"), i + 1);

#if GTK_CHECK_VERSION(3,0,0)
        gtk_grid_attach(GTK_GRID(pwGrid), pw = gtk_label_new(sz), i + 1, 0, 1, 1);
#else
        gtk_table_attach_defaults(GTK_TABLE(pwTable), pw = gtk_label_new(sz), i + 1, i + 2, 0, 1);
#endif
        if (i == nAway1) {
            gtk_widget_set_name(GTK_WIDGET(pw), "gnubg-met-matching-score");
        }
	g_free(sz);
    }

    /* header for columns */

    for (unsigned int i = 0; i < nRows; i++) {
	gchar *sz = g_strdup_printf(_("%u-away"), i + 1);

#if GTK_CHECK_VERSION(3,0,0)
        gtk_grid_attach(GTK_GRID(pwGrid), pw = gtk_label_new(sz), 0, i + 1, 1, 1);
#else
        gtk_table_attach_defaults(GTK_TABLE(pwTable), pw = gtk_label_new(sz), 0, 1, i + 1, i + 2);
#endif
        if (i == nAway0) {
            gtk_widget_set_name(GTK_WIDGET(pw), "gnubg-met-matching-score");
        }
        g_free(sz);
    }


    /* fill out table */

    for (unsigned int i = 0; i < nRows; i++)
        for (unsigned int j = 0; j < nCols; j++) {

            pmt->aapwLabel[i][j] = gtk_label_new(NULL);

#if GTK_CHECK_VERSION(3,0,0)
            gtk_grid_attach(GTK_GRID(pmt->pwGrid), pmt->aapwLabel[i][j], j + 1, i + 1, 1, 1);
#else
            gtk_table_attach_defaults(GTK_TABLE(pmt->pwTable), pmt->aapwLabel[i][j], j + 1, j + 2, i + 1, i + 2);
#endif

            if (i == nAway0 && j == nAway1) {
                gtk_widget_set_name(GTK_WIDGET(pmt->aapwLabel[i][j]), "gnubg-met-the-score");
            } else if (i == nAway0 || j == nAway1) {
                gtk_widget_set_name(GTK_WIDGET(pmt->aapwLabel[i][j]), "gnubg-met-matching-score");
            }
        }

#if GTK_CHECK_VERSION(3,0,0)
    gtk_grid_set_column_spacing(GTK_GRID(pwGrid), 4);
#else
    gtk_table_set_col_spacings(GTK_TABLE(pwTable), 4);
#endif

    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pwScrolledWindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    g_object_set_data_full(G_OBJECT(pwBox), "mettable", pmt, g_free);

    return pwBox;
}

static void
invertMETlocal(GtkWidget * UNUSED(widget), const metwidget * pmw)
{

    if (fInvertMET)
        UserCommand("set invert met off");
    else
        UserCommand("set invert met on");

    UserCommand("save settings");
    UpdateAllTables(pmw);
}


static void
loadMET(GtkWidget * UNUSED(widget), const metwidget * pmw)
{

    SetMET(NULL, NULL);

    UpdateAllTables(pmw);

}


extern void
GTKShowMatchEquityTable(const unsigned int nMatchTo, const int anScore[2])
{
    /* FIXME: Widget should update after 'Invert' or 'Load ...' */
    metwidget mw;
    GtkWidget *pwDialog = GTKCreateDialog(_("GNU Backgammon - Match equity table"),
                                          DT_INFO, NULL, DIALOG_FLAG_MODAL, NULL, NULL);
    GtkWidget *pwNotebook = gtk_notebook_new();
    GtkWidget *pwLoad = gtk_button_new_with_label(_("Load table..."));

    GtkWidget *pwInvertButton = gtk_toggle_button_new_with_label(_("Invert table"));

    /* similar tooltip is used in gtkoptions.c:append_match_options() */
    gtk_widget_set_tooltip_text(pwInvertButton,
                                _("Use the specified match equity table "
                                  "around the other way (i.e., swap the players before "
                                  "looking up equities in the table)."));

    mw.nMatchTo = nMatchTo;
    mw.anAway[0] = (nMatchTo - (unsigned) anScore[0]) - 1;
    mw.anAway[1] = (nMatchTo - (unsigned) anScore[1]) - 1;

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pwInvertButton), fInvertMET);

    gtk_container_set_border_width(GTK_CONTAINER(pwNotebook), 4);

    gtk_container_add(GTK_CONTAINER(DialogArea(pwDialog, DA_MAIN)), pwNotebook);
    gtk_container_add(GTK_CONTAINER(DialogArea(pwDialog, DA_BUTTONS)), pwInvertButton);
    gtk_container_add(GTK_CONTAINER(DialogArea(pwDialog, DA_BUTTONS)), pwLoad);

    mw.pwPreCrawford = GTKWriteMET((unsigned) mw.nMatchTo, (unsigned) mw.nMatchTo, mw.anAway[0], mw.anAway[1]);
    gtk_notebook_append_page(GTK_NOTEBOOK(pwNotebook), mw.pwPreCrawford, gtk_label_new(_("Pre-Crawford")));

    for (int i = 0; i < 2; i++) {

        gchar *sz = g_strdup_printf(_("Post-Crawford for player %s"), ap[i].szName);

        mw.apwPostCrawford[i] = GTKWriteMET(nMatchTo, 1, mw.anAway[i], mw.anAway[!i]);
        gtk_notebook_append_page(GTK_NOTEBOOK(pwNotebook), mw.apwPostCrawford[i], gtk_label_new(sz));

	g_free(sz);
    }

    gtk_window_set_default_size(GTK_WINDOW(pwDialog), 500, 300);
    g_signal_connect(G_OBJECT(pwInvertButton), "toggled", G_CALLBACK(invertMETlocal), &mw);
    g_signal_connect(G_OBJECT(pwLoad), "clicked", G_CALLBACK(loadMET), &mw);

    UpdateAllTables(&mw);

    GTKRunDialog(pwDialog);
}
