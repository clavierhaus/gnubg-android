/*
 * Copyright (C) 2002-2003 Joern Thyssen <jthyssen@dk.ibm.com>
 * Copyright (C) 2021 Aaron Tikuisis and Isaac Keslassy (MoneyEval)
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
 * $Id: gtkcube.c,v 1.107 2023/03/07 22:29:54 plm Exp $
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "backgammon.h"
#include "eval.h"
#include "export.h"
#include "rollout.h"
#include "gtkgame.h"
#include "gtkcube.h"
#include "gtkwindows.h"
#include "progress.h"
#include "format.h"
#include "gtklocdefs.h"

//  -------------------------------------------------------------------------
//   new functionality with a button performing an Eval in Money Play	    |
//      (i.e. hypothetical eval independent of score)			            |
//  -------------------------------------------------------------------------

/* Notes:
- One minor problem is left: When running a rollout in MoneyEval, the "JSDs" column displays "NaN"
*/

typedef struct {
    GtkWidget *pwFrame;         /* the table */
    GtkWidget *pw;              /* the box */
    GtkWidget *pwTools;         /* the tools */
    moverecord *pmr;
    matchstate ms;
    int did_double;
    int did_take;
    int hist;
    GtkWidget *pwMWC;       // Remember the pwMWC and pwCmark buttons because they should be deactivated during money eval
    GtkWidget *pwCmark; 
    int evalAtMoney;        // Whether evalAtMoney is toggled
} cubehintdata;

// These two variables are extern, and used for the temperature map when the MoneyEval button is toggled on.
// The temperature map can then provide an eval at money play rather than at the current score
// The second variable determines whether to use a Jacoby rule or not
int cubeTempMapAtMoney = 0;   
int cubeTempMapJacoby = 0;     

// Text to display when using MoneyEval
static const char *MONEY_EVAL_TEXT=N_("(Hypothetical money game)");

// here we can set an initial default preference for Jacoby -- else we use the system preference fJacoby
#define UNDEF (-1000) // Different from TRUE
static int moneyEvalJacoby=UNDEF;

static void
EvalCube(cubehintdata * pchd, const evalcontext * pec);

static void
JacobyToggled(GtkWidget *pwJ, cubehintdata *pchd)
{
/* This is called by gtk when the user clicks on the Jacoby check button during money eval.
If the Jacoby setting is changed, it reruns the EvalCube()
*/
    g_assert(pchd->evalAtMoney);
    if (moneyEvalJacoby!=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pwJ)) ) {
        moneyEvalJacoby = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pwJ));
        EvalCube(pchd, &GetEvalCube()->ec);
    }
}

static GtkWidget *
OutputPercentsTable(const float ar[])
{
/* Creates the table showing win% etc. */
#if GTK_CHECK_VERSION(3,0,0)
    GtkWidget *pwGrid;
#else
    GtkWidget *pwTable;
#endif
    GtkWidget *pw;
    int i;
    static gchar *headings[] = { N_("Win"), N_("W(g)"), N_("W(bg)"),
        "-", N_("Lose"), N_("L(g)"), N_("L(bg)")
    };

#if GTK_CHECK_VERSION(3,0,0)
    pwGrid = gtk_grid_new();
#else
    pwTable = gtk_table_new(2, 7, FALSE);
#endif

    for (i = 0; i < 7; i++) {
        pw = gtk_label_new(gettext(headings[i]));
#if GTK_CHECK_VERSION(3,0,0)
        gtk_grid_attach(GTK_GRID(pwGrid), pw, i, 0, 1, 1);
        gtk_widget_set_hexpand(pw, TRUE);
        gtk_widget_set_vexpand(pw, TRUE);
#if GTK_CHECK_VERSION(3,12,0)
        gtk_widget_set_margin_start(pw, 2);
        gtk_widget_set_margin_end(pw, 2);
#else
        gtk_widget_set_margin_left(pw, 2);
        gtk_widget_set_margin_right(pw, 2);
#endif
#else
        gtk_table_attach(GTK_TABLE(pwTable), pw, i, i + 1, 0, 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 2, 0);
#endif
    }

    pw = gtk_label_new(OutputPercent(ar[OUTPUT_WIN]));
#if GTK_CHECK_VERSION(3,0,0)
    gtk_grid_attach(GTK_GRID(pwGrid), pw, 0, 1, 1, 1);
    gtk_widget_set_hexpand(pw, TRUE);
    gtk_widget_set_vexpand(pw, TRUE);
#if GTK_CHECK_VERSION(3,12,0)
    gtk_widget_set_margin_start(pw, 2);
    gtk_widget_set_margin_end(pw, 2);
#else
    gtk_widget_set_margin_left(pw, 2);
    gtk_widget_set_margin_right(pw, 2);
#endif
#else
    gtk_table_attach(GTK_TABLE(pwTable), pw, 0, 1, 1, 2, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 2, 0);
#endif

    pw = gtk_label_new(OutputPercent(ar[OUTPUT_WINGAMMON]));
#if GTK_CHECK_VERSION(3,0,0)
    gtk_grid_attach(GTK_GRID(pwGrid), pw, 1, 1, 1, 1);
    gtk_widget_set_hexpand(pw, TRUE);
    gtk_widget_set_vexpand(pw, TRUE);
#if GTK_CHECK_VERSION(3,12,0)
    gtk_widget_set_margin_start(pw, 2);
    gtk_widget_set_margin_end(pw, 2);
#else
    gtk_widget_set_margin_left(pw, 2);
    gtk_widget_set_margin_right(pw, 2);
#endif
#else
    gtk_table_attach(GTK_TABLE(pwTable), pw, 1, 2, 1, 2, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 2, 0);
#endif

    pw = gtk_label_new(OutputPercent(ar[OUTPUT_WINBACKGAMMON]));
#if GTK_CHECK_VERSION(3,0,0)
    gtk_grid_attach(GTK_GRID(pwGrid), pw, 2, 1, 1, 1);
    gtk_widget_set_hexpand(pw, TRUE);
    gtk_widget_set_vexpand(pw, TRUE);
#if GTK_CHECK_VERSION(3,12,0)
    gtk_widget_set_margin_start(pw, 2);
    gtk_widget_set_margin_end(pw, 2);
#else
    gtk_widget_set_margin_left(pw, 2);
    gtk_widget_set_margin_right(pw, 2);
#endif
#else
    gtk_table_attach(GTK_TABLE(pwTable), pw, 2, 3, 1, 2, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 2, 0);
#endif

    pw = gtk_label_new(" - ");
#if GTK_CHECK_VERSION(3,0,0)
    gtk_grid_attach(GTK_GRID(pwGrid), pw, 3, 1, 1, 1);
    gtk_widget_set_hexpand(pw, TRUE);
    gtk_widget_set_vexpand(pw, TRUE);
#if GTK_CHECK_VERSION(3,12,0)
    gtk_widget_set_margin_start(pw, 2);
    gtk_widget_set_margin_end(pw, 2);
#else
    gtk_widget_set_margin_left(pw, 2);
    gtk_widget_set_margin_right(pw, 2);
#endif
#else
    gtk_table_attach(GTK_TABLE(pwTable), pw, 3, 4, 1, 2, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 2, 0);
#endif

    pw = gtk_label_new(OutputPercent(1.0f - ar[OUTPUT_WIN]));
#if GTK_CHECK_VERSION(3,0,0)
    gtk_grid_attach(GTK_GRID(pwGrid), pw, 4, 1, 1, 1);
    gtk_widget_set_hexpand(pw, TRUE);
    gtk_widget_set_vexpand(pw, TRUE);
#if GTK_CHECK_VERSION(3,12,0)
    gtk_widget_set_margin_start(pw, 2);
    gtk_widget_set_margin_end(pw, 2);
#else
    gtk_widget_set_margin_left(pw, 2);
    gtk_widget_set_margin_right(pw, 2);
#endif
#else
    gtk_table_attach(GTK_TABLE(pwTable), pw, 4, 5, 1, 2, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 2, 0);
#endif

    pw = gtk_label_new(OutputPercent(ar[OUTPUT_LOSEGAMMON]));
#if GTK_CHECK_VERSION(3,0,0)
    gtk_grid_attach(GTK_GRID(pwGrid), pw, 5, 1, 1, 1);
    gtk_widget_set_hexpand(pw, TRUE);
    gtk_widget_set_vexpand(pw, TRUE);
#if GTK_CHECK_VERSION(3,12,0)
    gtk_widget_set_margin_start(pw, 2);
    gtk_widget_set_margin_end(pw, 2);
#else
    gtk_widget_set_margin_left(pw, 2);
    gtk_widget_set_margin_right(pw, 2);
#endif
#else
    gtk_table_attach(GTK_TABLE(pwTable), pw, 5, 6, 1, 2, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 2, 0);
#endif

    pw = gtk_label_new(OutputPercent(ar[OUTPUT_LOSEBACKGAMMON]));
#if GTK_CHECK_VERSION(3,0,0)
    gtk_grid_attach(GTK_GRID(pwGrid), pw, 6, 1, 1, 1);
    gtk_widget_set_hexpand(pw, TRUE);
    gtk_widget_set_vexpand(pw, TRUE);
#if GTK_CHECK_VERSION(3,12,0)
    gtk_widget_set_margin_start(pw, 2);
    gtk_widget_set_margin_end(pw, 2);
#else
    gtk_widget_set_margin_left(pw, 2);
    gtk_widget_set_margin_right(pw, 2);
#endif
#else
    gtk_table_attach(GTK_TABLE(pwTable), pw, 6, 7, 1, 2, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 2, 0);
#endif

#if GTK_CHECK_VERSION(3,0,0)
    return pwGrid;
#else
    return pwTable;
#endif
}

static void
GetMoneyCubeInfo(cubeinfo * pci, const matchstate * pms) {
/* Fills pci with cube info for a money game, i.e., same as given match state except with nMatchTo=0. */
    matchstate moneyms = *pms; // Copy
    moneyms.nMatchTo=0;
    moneyms.fJacoby=moneyEvalJacoby;
    GetMatchStateCubeInfo(pci, &moneyms);
}

// this function is used for the beavers in money play for example, and
//      for the take decision frame; see the next CubeAnalysis function for 
//      explanations, as the two functions are very similar 
static GtkWidget *
TakeAnalysis(cubehintdata * pchd)
{
    cubeinfo ci;
    GtkWidget *pw;
#if GTK_CHECK_VERSION(3,0,0)
    GtkWidget *pwGrid;
#else
    GtkWidget *pwTable;
#endif
    GtkWidget *pwFrame;
    int iRow;
    int i;
    cubedecision cd;
    int ai[2];
    const char *aszCube[] = {
        NULL, NULL,
        N_("Take"),
        N_("Pass")
    };
    float arDouble[4];
    gchar *sz;
    cubedecisiondata *cdec = (pchd->evalAtMoney ? pchd->pmr->MoneyCubeDecPtr : pchd->pmr->CubeDecPtr);
    const evalsetup *pes = &cdec->esDouble;

    if (pes->et == EVAL_NONE)
        return NULL;

    if (!pchd->evalAtMoney)
        GetMatchStateCubeInfo(&ci, &pchd->ms);
    else
        GetMoneyCubeInfo(&ci, &pchd->ms);

  
    cd = FindCubeDecision(arDouble, cdec->aarOutput, &ci); 
    
    /* header */

    sz = g_strdup_printf("%s %s", _("Take analysis"), pchd->evalAtMoney ? Q_(MONEY_EVAL_TEXT) : "");
    pwFrame = gtk_frame_new(sz);
    g_free(sz);
    gtk_container_set_border_width(GTK_CONTAINER(pwFrame), 8);

#if GTK_CHECK_VERSION(3,0,0)
    pwGrid = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(pwFrame), pwGrid);
#else
    pwTable = gtk_table_new(6, 4, FALSE);
    gtk_container_add(GTK_CONTAINER(pwFrame), pwTable);
#endif

    /* if EVAL_EVAL include cubeless equity and winning percentages */

    iRow = 0;
    InvertEvaluationR(cdec->aarOutput[0], &ci);
    InvertEvaluationR(cdec->aarOutput[1], &ci);

    switch (pes->et) {
    case EVAL_EVAL:
        if (ci.nMatchTo)
            sz = g_strdup_printf(_("Cubeless %d-ply %s: %s (Money: %s)"),
                                 pes->ec.nPlies,
                                 fOutputMWC ? _("MWC") : _("equity"),
                                 OutputEquity(cdec->aarOutput[0][OUTPUT_EQUITY],
                                             &ci, TRUE),
                                 OutputMoneyEquity(cdec->aarOutput[0], TRUE));
        else
            sz = g_strdup_printf(_("Cubeless %d-ply equity: %s"),
                                 pes->ec.nPlies, OutputMoneyEquity(cdec->aarOutput[0], TRUE));
        break;

    case EVAL_ROLLOUT:
        if (ci.nMatchTo)
            sz = g_strdup_printf(_("Cubeless rollout %s: %s (Money: %s)"),
                                fOutputMWC ? _("MWC") : _("equity"),
                                OutputEquity(cdec->aarOutput[0][OUTPUT_EQUITY],
                                            &ci, TRUE),
                                OutputMoneyEquity(cdec->aarOutput[0], TRUE));
        else
            sz = g_strdup_printf(_("Cubeless rollout equity: %s"), OutputMoneyEquity(cdec->aarOutput[0], TRUE));
        break;

    default:
        sz = g_strdup("");
    }

    pw = gtk_label_new(sz);
#if GTK_CHECK_VERSION(3,0,0)
    gtk_widget_set_halign(pw, GTK_ALIGN_START);
    gtk_widget_set_valign(pw, GTK_ALIGN_CENTER);
    gtk_grid_attach(GTK_GRID(pwGrid), pw, 0, iRow, 4, 1);
    gtk_widget_set_hexpand(pw, TRUE);
    gtk_widget_set_vexpand(pw, TRUE);
#if GTK_CHECK_VERSION(3,12,0)
    gtk_widget_set_margin_start(pw, 8);
    gtk_widget_set_margin_end(pw, 8);
#else
    gtk_widget_set_margin_left(pw, 8);
    gtk_widget_set_margin_right(pw, 8);
#endif
#else
    gtk_table_attach(GTK_TABLE(pwTable), pw, 0, 4, iRow, iRow + 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 8, 0);
    gtk_misc_set_alignment(GTK_MISC(pw), 0, 0.5);
#endif
    g_free(sz);

    iRow++;

    /* winning percentages */

    switch (pes->et) {

    case EVAL_EVAL:
        pw = OutputPercentsTable(cdec->aarOutput[0]);
#if GTK_CHECK_VERSION(3,0,0)
        gtk_grid_attach(GTK_GRID(pwGrid), pw, 0, iRow, 4, 1);
        gtk_widget_set_hexpand(pw, TRUE);
        gtk_widget_set_vexpand(pw, TRUE);
#if GTK_CHECK_VERSION(3,12,0)
        gtk_widget_set_margin_start(pw, 8);
        gtk_widget_set_margin_end(pw, 8);
#else
        gtk_widget_set_margin_left(pw, 8);
        gtk_widget_set_margin_right(pw, 8);
#endif
        gtk_widget_set_margin_top(pw, 4);
        gtk_widget_set_margin_bottom(pw, 4);
#else
        gtk_table_attach(GTK_TABLE(pwTable), pw,
                         0, 4, iRow, iRow + 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 8, 4);
#endif
        iRow++;

        break;

    case EVAL_ROLLOUT:
        pw = OutputPercentsTable(cdec->aarOutput[1]);
#if GTK_CHECK_VERSION(3,0,0)
        gtk_grid_attach(GTK_GRID(pwGrid), pw, 0, iRow, 4, 1);
        gtk_widget_set_hexpand(pw, TRUE);
        gtk_widget_set_vexpand(pw, TRUE);
#if GTK_CHECK_VERSION(3,12,0)
        gtk_widget_set_margin_start(pw, 8);
        gtk_widget_set_margin_end(pw, 8);
#else
        gtk_widget_set_margin_left(pw, 8);
        gtk_widget_set_margin_right(pw, 8);
#endif
        gtk_widget_set_margin_top(pw, 4);
        gtk_widget_set_margin_bottom(pw, 4);
#else
        gtk_table_attach(GTK_TABLE(pwTable), pw,
                         0, 4, iRow, iRow + 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 8, 4);
#endif
        iRow++;

        break;

    default:

        g_assert_not_reached();

    }

    InvertEvaluationR(cdec->aarOutput[0], &ci);
    InvertEvaluationR(cdec->aarOutput[1], &ci);

    /* sub-header */

    pw = gtk_label_new(_("Cubeful equities:"));
#if GTK_CHECK_VERSION(3,0,0)
    gtk_widget_set_halign(pw, GTK_ALIGN_START);
    gtk_widget_set_valign(pw, GTK_ALIGN_CENTER);
    gtk_grid_attach(GTK_GRID(pwGrid), pw, 0, iRow, 4, 1);
    gtk_widget_set_hexpand(pw, TRUE);
    gtk_widget_set_vexpand(pw, TRUE);
#if GTK_CHECK_VERSION(3,12,0)
    gtk_widget_set_margin_start(pw, 8);
    gtk_widget_set_margin_end(pw, 8);
#else
    gtk_widget_set_margin_left(pw, 8);
    gtk_widget_set_margin_right(pw, 8);
#endif
    gtk_widget_set_margin_top(pw, 4);
    gtk_widget_set_margin_bottom(pw, 4);
#else
    gtk_misc_set_alignment(GTK_MISC(pw), 0, 0.5);
    gtk_table_attach(GTK_TABLE(pwTable), pw, 0, 4, iRow, iRow + 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 8, 4);
#endif
    iRow++;

    /* ordered take actions with equities */

    if (arDouble[OUTPUT_TAKE] < arDouble[OUTPUT_DROP]) {
        ai[0] = OUTPUT_TAKE;
        ai[1] = OUTPUT_DROP;
    } else {
        ai[0] = OUTPUT_DROP;
        ai[1] = OUTPUT_TAKE;
    }

    for (i = 0; i < 2; i++) {

        /* numbering */

        sz = g_strdup_printf("%d.", i + 1);
        pw = gtk_label_new(sz);
#if GTK_CHECK_VERSION(3,0,0)
        gtk_widget_set_halign(pw, GTK_ALIGN_START);
        gtk_widget_set_valign(pw, GTK_ALIGN_CENTER);
        gtk_grid_attach(GTK_GRID(pwGrid), pw, 0, iRow, 1, 1);
        gtk_widget_set_hexpand(pw, TRUE);
        gtk_widget_set_vexpand(pw, TRUE);
#if GTK_CHECK_VERSION(3,12,0)
        gtk_widget_set_margin_start(pw, 8);
        gtk_widget_set_margin_end(pw, 8);
#else
        gtk_widget_set_margin_left(pw, 8);
        gtk_widget_set_margin_right(pw, 8);
#endif
#else
        gtk_misc_set_alignment(GTK_MISC(pw), 0, 0.5);
        gtk_table_attach(GTK_TABLE(pwTable), pw,
                         0, 1, iRow, iRow + 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 8, 0);
#endif
        g_free(sz);

        /* label */

        pw = gtk_label_new(gettext(aszCube[ai[i]]));
#if GTK_CHECK_VERSION(3,0,0)
        gtk_widget_set_halign(pw, GTK_ALIGN_START);
        gtk_widget_set_valign(pw, GTK_ALIGN_CENTER);
        gtk_grid_attach(GTK_GRID(pwGrid), pw, 1, iRow, 1, 1);
        gtk_widget_set_hexpand(pw, TRUE);
        gtk_widget_set_vexpand(pw, TRUE);
#if GTK_CHECK_VERSION(3,12,0)
        gtk_widget_set_margin_start(pw, 8);
        gtk_widget_set_margin_end(pw, 8);
#else
        gtk_widget_set_margin_left(pw, 8);
        gtk_widget_set_margin_right(pw, 8);
#endif
#else
        gtk_misc_set_alignment(GTK_MISC(pw), 0, 0.5);
        gtk_table_attach(GTK_TABLE(pwTable), pw,
                         1, 2, iRow, iRow + 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 8, 0);
#endif

        /* equity */

        if (!ci.nMatchTo || !fOutputMWC)
            sz = g_strdup_printf("%+7.3f", -arDouble[ai[i]]);
        else
            sz = g_strdup_printf("%7.3f%%", 100.0f * (1.0f - eq2mwc(arDouble[ai[i]], &ci)));

        pw = gtk_label_new(sz);
#if GTK_CHECK_VERSION(3,0,0)
        gtk_widget_set_halign(pw, GTK_ALIGN_END);
        gtk_widget_set_valign(pw, GTK_ALIGN_CENTER);
        gtk_grid_attach(GTK_GRID(pwGrid), pw, 2, iRow, 1, 1);
        gtk_widget_set_hexpand(pw, TRUE);
        gtk_widget_set_vexpand(pw, TRUE);
#if GTK_CHECK_VERSION(3,12,0)
        gtk_widget_set_margin_start(pw, 8);
        gtk_widget_set_margin_end(pw, 8);
#else
        gtk_widget_set_margin_left(pw, 8);
        gtk_widget_set_margin_right(pw, 8);
#endif
#else
        gtk_misc_set_alignment(GTK_MISC(pw), 1, 0.5);
        gtk_table_attach(GTK_TABLE(pwTable), pw,
                         2, 3, iRow, iRow + 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 8, 0);
#endif
        g_free(sz);

        /* difference */

        if (i) {

            if (!ci.nMatchTo || !fOutputMWC)
                sz = g_strdup_printf("%+7.3f", arDouble[ai[0]] - arDouble[ai[i]]);
            else
                sz = g_strdup_printf("%+7.3f%%",
                                     100.0f * eq2mwc(arDouble[ai[0]], &ci) - 100.0f * eq2mwc(arDouble[ai[i]], &ci));

            pw = gtk_label_new(sz);
#if GTK_CHECK_VERSION(3,0,0)
            gtk_widget_set_halign(pw, GTK_ALIGN_END);
            gtk_widget_set_valign(pw, GTK_ALIGN_CENTER);
            gtk_grid_attach(GTK_GRID(pwGrid), pw, 3, iRow, 1, 1);
            gtk_widget_set_hexpand(pw, TRUE);
            gtk_widget_set_vexpand(pw, TRUE);
#if GTK_CHECK_VERSION(3,12,0)
            gtk_widget_set_margin_start(pw, 8);
            gtk_widget_set_margin_end(pw, 8);
#else
            gtk_widget_set_margin_left(pw, 8);
            gtk_widget_set_margin_right(pw, 8);
#endif
#else
            gtk_misc_set_alignment(GTK_MISC(pw), 1, 0.5);
            gtk_table_attach(GTK_TABLE(pwTable), pw,
                             3, 4, iRow, iRow + 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 8, 0);
#endif
            g_free(sz);

        }

        iRow++;

    }

    /* proper cube action */

    pw = gtk_label_new(_("Correct response: "));
#if GTK_CHECK_VERSION(3,0,0)
    gtk_widget_set_halign(pw, GTK_ALIGN_START);
    gtk_widget_set_valign(pw, GTK_ALIGN_CENTER);
    gtk_grid_attach(GTK_GRID(pwGrid), pw, 0, iRow, 2, 1);
    gtk_widget_set_hexpand(pw, TRUE);
    gtk_widget_set_vexpand(pw, TRUE);
#if GTK_CHECK_VERSION(3,12,0)
    gtk_widget_set_margin_start(pw, 8);
    gtk_widget_set_margin_end(pw, 8);
#else
    gtk_widget_set_margin_left(pw, 8);
    gtk_widget_set_margin_right(pw, 8);
#endif
    gtk_widget_set_margin_top(pw, 8);
    gtk_widget_set_margin_bottom(pw, 8);
#else
    gtk_misc_set_alignment(GTK_MISC(pw), 0, 0.5);
    gtk_table_attach(GTK_TABLE(pwTable), pw, 0, 2, iRow, iRow + 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 8, 8);
#endif

    switch (cd) {

    case DOUBLE_TAKE:
    case NODOUBLE_TAKE:
    case TOOGOOD_TAKE:
    case REDOUBLE_TAKE:
    case NO_REDOUBLE_TAKE:
    case TOOGOODRE_TAKE:
    case NODOUBLE_DEADCUBE:
    case NO_REDOUBLE_DEADCUBE:
    case OPTIONAL_DOUBLE_TAKE:
    case OPTIONAL_REDOUBLE_TAKE:
        pw = gtk_label_new(_("Take"));
        break;

    case DOUBLE_PASS:
    case TOOGOOD_PASS:
    case REDOUBLE_PASS:
    case TOOGOODRE_PASS:
    case OPTIONAL_DOUBLE_PASS:
    case OPTIONAL_REDOUBLE_PASS:
        pw = gtk_label_new(_("Pass"));
        break;

    case DOUBLE_BEAVER:
    case NODOUBLE_BEAVER:
    case NO_REDOUBLE_BEAVER:
    case OPTIONAL_DOUBLE_BEAVER:
        pw = gtk_label_new(_("Beaver!"));
        break;

    case NOT_AVAILABLE:
        pw = gtk_label_new(_("Eat it!"));
        break;

    }

#if GTK_CHECK_VERSION(3,0,0)
    gtk_widget_set_halign(pw, GTK_ALIGN_START);
    gtk_widget_set_valign(pw, GTK_ALIGN_CENTER);
    gtk_grid_attach(GTK_GRID(pwGrid), pw, 2, iRow, 2, 1);
    gtk_widget_set_hexpand(pw, TRUE);
    gtk_widget_set_vexpand(pw, TRUE);
#if GTK_CHECK_VERSION(3,12,0)
    gtk_widget_set_margin_start(pw, 8);
    gtk_widget_set_margin_end(pw, 8);
#else
    gtk_widget_set_margin_left(pw, 8);
    gtk_widget_set_margin_right(pw, 8);
#endif
    gtk_widget_set_margin_top(pw, 8);
    gtk_widget_set_margin_bottom(pw, 8);
#else
    gtk_misc_set_alignment(GTK_MISC(pw), 0, 0.5);
    gtk_table_attach(GTK_TABLE(pwTable), pw, 2, 4, iRow, iRow + 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 8, 8);
#endif

    iRow++;

    /* Jacoby rule */ 
    if (pchd->ms.nMatchTo && pchd->evalAtMoney) {

        GtkWidget *pwJ = gtk_check_button_new_with_label(_("Jacoby"));

#if GTK_CHECK_VERSION(3,0,0)
        gtk_grid_attach(GTK_GRID(pwGrid), pwJ, 0, iRow, 2, 1);
        gtk_widget_set_hexpand(pwJ, TRUE);
        gtk_widget_set_vexpand(pwJ, TRUE);
#if GTK_CHECK_VERSION(3,12,0)
        gtk_widget_set_margin_start(pwJ, 8);
        gtk_widget_set_margin_end(pwJ, 8);
#else
        gtk_widget_set_margin_left(pwJ, 8);
        gtk_widget_set_margin_right(pwJ, 8);
#endif
        gtk_widget_set_margin_top(pwJ, 8);
        gtk_widget_set_margin_bottom(pwJ, 8);
#else
        gtk_table_attach(GTK_TABLE(pwTable), pwJ, 0, 2, iRow, iRow + 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 8, 8);
#endif
        g_signal_connect(G_OBJECT (pwJ), "toggled", G_CALLBACK(JacobyToggled), pchd); 
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pwJ), moneyEvalJacoby);
        gtk_widget_set_tooltip_text(pwJ, _("Toggle Jacoby rule for money play"));
        // gtk_widget_set_sensitive(pwJ, !pchd->ms.nMatchTo && pchd->pmr->evalAtMoney);
    }

    return pwFrame;

}


/*
 * Make cube analysis widget. Used for doubling decisions.
 *
 * Input:
 *   aarOutput, aarStdDev: evaluations
 *   pes: evaluation setup
 *
 * Returns:
 *   nice and pretty widget with cube analysis
 *
 */
static GtkWidget *
CubeAnalysis(cubehintdata * pchd)
{
    cubeinfo ci;
    GtkWidget *pw;
#if GTK_CHECK_VERSION(3,0,0)
    GtkWidget *pwGrid;
#else
    GtkWidget *pwTable;
#endif
    GtkWidget *pwFrame;
    int iRow;
    int i;
    cubedecision cd;
    float r;
    int ai[3];
    const char *aszCube[] = {
        NULL,
        N_("No double"),
        N_("Double, take"),
        N_("Double, pass")
    };
    float arDouble[4];
    gchar *sz;
    cubedecisiondata *cdec = (pchd->evalAtMoney ? pchd->pmr->MoneyCubeDecPtr : pchd->pmr->CubeDecPtr);
    const evalsetup *pes = &cdec->esDouble;

    if (pes->et == EVAL_NONE)
        return NULL;

    if (!pchd->evalAtMoney)
        GetMatchStateCubeInfo(&ci, &pchd->ms);
    else
        GetMoneyCubeInfo(&ci, &pchd->ms);

    //  FindCubeDecision(arDouble, aarOutput, pci) is defined in eval.c:
    //  extern cubedecision FindCubeDecision(float arDouble[], float aarOutput[][NUM_ROLLOUT_OUTPUTS], const cubeinfo * pci)
    // cubedecision is from eval.h: "DOUBLE_TAKE", "DOUBLE_PASS", etc.
    // It calls FindBestCubeDecision, which calculates the optimal cube decision and equity/mwc for this.
    //  *
    //  * Input:
    //  *    arDouble    - array with equities or mwc's:
    //  *                      arDouble[ 1 ]: no double,
    //  *                      arDouble[ 2 ]: double take
    //  *                      arDouble[ 3 ]: double pass
    //  *    pci         - pointer to cube info
    //  *
    //  * Output:
    //  *    arDouble    - array with equities or mwc's
    //  *                      arDouble[ 0 ]: equity for optimal cube decision
    //  *
    //  * Returns:
    //  *    cube decision
    // Namely, the function takes aarOutput to get the 3 equities of ND,DT and DP, 
    // (eg from aarOutput[5,6])
    // then converts into mwc (when NMatchTo!=0) then stores them in arDouble[1,2,3]
    // and stores the best in arDouble[0]
    //reminder: typedef struct _cubedecisiondata {float aarOutput[2][NUM_ROLLOUT_OUTPUTS]; float aarStdDev[2][NUM_ROLLOUT_OUTPUTS];
    //                      evalsetup esDouble; CMark cmark; } cubedecisiondata;
    cd = FindCubeDecision(arDouble, cdec->aarOutput, &ci); 

    if (!GetDPEq(NULL, NULL, &ci) && !(pchd->pmr->mt == MOVE_DOUBLE))
        /* No cube action possible */
        return NULL;

    /* header */

    sz = g_strdup_printf("%s %s", _("Cube analysis"), pchd->evalAtMoney ? Q_(MONEY_EVAL_TEXT) : "");
    pwFrame = gtk_frame_new(sz);
    g_free(sz);
    gtk_container_set_border_width(GTK_CONTAINER(pwFrame), 8);

#if GTK_CHECK_VERSION(3,0,0)
    pw = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
    pw = gtk_vbox_new(FALSE, 0);
#endif
    gtk_container_add(GTK_CONTAINER(pwFrame), pw);

#if GTK_CHECK_VERSION(3,0,0)
    pwGrid = gtk_grid_new();
    gtk_box_pack_start(GTK_BOX(pw), pwGrid, FALSE, FALSE, 0);
#else
    pwTable = gtk_table_new(8, 4, FALSE);
    gtk_box_pack_start(GTK_BOX(pw), pwTable, FALSE, FALSE, 0);
#endif

    iRow = 0;

    /* cubeless equity */

    /* Define first row of text (in sz) */
    switch (pes->et) {
    case EVAL_EVAL:
        if (ci.nMatchTo)
        {
            // OutputEquity: in format.c:
            // extern char * OutputEquity(const float r, const cubeinfo * pci, const int f)
            sz = g_strdup_printf(_("Cubeless %d-ply %s: %s (Money: %s)"),
                                 pes->ec.nPlies,
                                 fOutputMWC ? _("MWC") : _("equity"),
                                 OutputEquity(cdec->aarOutput[0][OUTPUT_EQUITY],
                                              &ci, TRUE),
                                 OutputMoneyEquity(cdec->aarOutput[0], TRUE));
        } else
            sz = g_strdup_printf(_("Cubeless %d-ply equity: %s"),
                                 pes->ec.nPlies, OutputMoneyEquity(cdec->aarOutput[0], TRUE));

        break;

    case EVAL_ROLLOUT:
        if (ci.nMatchTo)
            sz = g_strdup_printf(_("Cubeless rollout %s: %s (Money: %s)"),
                                 fOutputMWC ? _("MWC") : _("equity"),
                                 OutputEquity(cdec->aarOutput[0][OUTPUT_EQUITY],
                                              &ci, TRUE),
                                 OutputMoneyEquity(cdec->aarOutput[0], TRUE));
        else
            sz = g_strdup_printf(_("Cubeless rollout equity: %s"), OutputMoneyEquity(cdec->aarOutput[0], TRUE));

        break;

    default:

        sz = g_strdup("");

    }

    pw = gtk_label_new(sz);
#if GTK_CHECK_VERSION(3,0,0)
    gtk_widget_set_halign(pw, GTK_ALIGN_START);
    gtk_widget_set_valign(pw, GTK_ALIGN_CENTER);
    gtk_grid_attach(GTK_GRID(pwGrid), pw, 0, iRow, 4, 1);
    gtk_widget_set_hexpand(pw, TRUE);
    gtk_widget_set_vexpand(pw, TRUE);
#if GTK_CHECK_VERSION(3,12,0)
    gtk_widget_set_margin_start(pw, 8);
    gtk_widget_set_margin_end(pw, 8);
#else
    gtk_widget_set_margin_left(pw, 8);
    gtk_widget_set_margin_right(pw, 8);
#endif
#else
    gtk_misc_set_alignment(GTK_MISC(pw), 0, 0.5);
    gtk_table_attach(GTK_TABLE(pwTable), pw, 0, 4, iRow, iRow + 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 8, 0);
#endif
    g_free(sz);

    iRow++;


    /* if EVAL_EVAL include cubeless equity and winning percentages */

    if (pes->et == EVAL_EVAL) {

        pw = OutputPercentsTable(cdec->aarOutput[0]);
#if GTK_CHECK_VERSION(3,0,0)
        gtk_grid_attach(GTK_GRID(pwGrid), pw, 0, iRow, 4, 1);
        gtk_widget_set_hexpand(pw, TRUE);
        gtk_widget_set_vexpand(pw, TRUE);
#if GTK_CHECK_VERSION(3,12,0)
        gtk_widget_set_margin_start(pw, 8);
        gtk_widget_set_margin_end(pw, 8);
#else
        gtk_widget_set_margin_left(pw, 8);
        gtk_widget_set_margin_right(pw, 8);
#endif
        gtk_widget_set_margin_top(pw, 4);
        gtk_widget_set_margin_bottom(pw, 4);
#else
        gtk_table_attach(GTK_TABLE(pwTable), pw,
                         0, 4, iRow, iRow + 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 8, 4);
#endif

        iRow++;
    }

    pw = gtk_label_new(_("Cubeful equities:"));
#if GTK_CHECK_VERSION(3,0,0)
    gtk_widget_set_halign(pw, GTK_ALIGN_START);
    gtk_widget_set_valign(pw, GTK_ALIGN_CENTER);
    gtk_grid_attach(GTK_GRID(pwGrid), pw, 0, iRow, 4, 1);
    gtk_widget_set_hexpand(pw, TRUE);
    gtk_widget_set_vexpand(pw, TRUE);
#if GTK_CHECK_VERSION(3,12,0)
    gtk_widget_set_margin_start(pw, 8);
    gtk_widget_set_margin_end(pw, 8);
#else
    gtk_widget_set_margin_left(pw, 8);
    gtk_widget_set_margin_right(pw, 8);
#endif
    gtk_widget_set_margin_top(pw, 4);
    gtk_widget_set_margin_bottom(pw, 4);
#else
    gtk_misc_set_alignment(GTK_MISC(pw), 0, 0.5);
    gtk_table_attach(GTK_TABLE(pwTable), pw, 0, 4, iRow, iRow + 1, GTK_EXPAND |
GTK_FILL, GTK_EXPAND | GTK_FILL, 8, 4);
#endif
    iRow++;

    getCubeDecisionOrdering(ai, arDouble, cdec->aarOutput, &ci);

    for (i = 0; i < 3; i++) {

        /* numbering */

        sz = g_strdup_printf("%d.", i + 1);
        pw = gtk_label_new(sz);
#if GTK_CHECK_VERSION(3,0,0)
        gtk_widget_set_halign(pw, GTK_ALIGN_START);
        gtk_widget_set_valign(pw, GTK_ALIGN_CENTER);
        gtk_grid_attach(GTK_GRID(pwGrid), pw, 0, iRow, 1, 1);
        gtk_widget_set_hexpand(pw, TRUE);
        gtk_widget_set_vexpand(pw, TRUE);
#if GTK_CHECK_VERSION(3,12,0)
        gtk_widget_set_margin_start(pw, 8);
        gtk_widget_set_margin_end(pw, 8);
#else
        gtk_widget_set_margin_left(pw, 8);
        gtk_widget_set_margin_right(pw, 8);
#endif
#else
        gtk_misc_set_alignment(GTK_MISC(pw), 0, 0.5);
        gtk_table_attach(GTK_TABLE(pwTable), pw,
                         0, 1, iRow, iRow + 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 8, 0);
#endif
        g_free(sz);

        /* label */

        pw = gtk_label_new(gettext(aszCube[ai[i]]));
#if GTK_CHECK_VERSION(3,0,0)
        gtk_widget_set_halign(pw, GTK_ALIGN_START);
        gtk_widget_set_valign(pw, GTK_ALIGN_CENTER);
        gtk_grid_attach(GTK_GRID(pwGrid), pw, 1, iRow, 1, 1);
        gtk_widget_set_hexpand(pw, TRUE);
        gtk_widget_set_vexpand(pw, TRUE);
#if GTK_CHECK_VERSION(3,12,0)
        gtk_widget_set_margin_start(pw, 8);
        gtk_widget_set_margin_end(pw, 8);
#else
        gtk_widget_set_margin_left(pw, 8);
        gtk_widget_set_margin_right(pw, 8);
#endif
#else
        gtk_misc_set_alignment(GTK_MISC(pw), 0, 0.5);
        gtk_table_attach(GTK_TABLE(pwTable), pw,
                         1, 2, iRow, iRow + 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 8, 0);
#endif

        /* equity */

        sz = OutputEquity(arDouble[ai[i]], &ci, TRUE);

        pw = gtk_label_new(sz);
#if GTK_CHECK_VERSION(3,0,0)
        gtk_widget_set_halign(pw, GTK_ALIGN_END);
        gtk_widget_set_valign(pw, GTK_ALIGN_CENTER);
        gtk_grid_attach(GTK_GRID(pwGrid), pw, 2, iRow, 1, 1);
        gtk_widget_set_hexpand(pw, TRUE);
        gtk_widget_set_vexpand(pw, TRUE);
#if GTK_CHECK_VERSION(3,12,0)
        gtk_widget_set_margin_start(pw, 8);
        gtk_widget_set_margin_end(pw, 8);
#else
        gtk_widget_set_margin_left(pw, 8);
        gtk_widget_set_margin_right(pw, 8);
#endif
#else
        gtk_misc_set_alignment(GTK_MISC(pw), 1, 0.5);
        gtk_table_attach(GTK_TABLE(pwTable), pw,
                         2, 3, iRow, iRow + 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 8, 0);
#endif

        /* difference */

        if (i)
        {
            pw = gtk_label_new(OutputEquityDiff(arDouble[ai[i]], arDouble[OUTPUT_OPTIMAL], &ci)); //function from format.c

#if GTK_CHECK_VERSION(3,0,0)
            gtk_widget_set_halign(pw, GTK_ALIGN_END);
            gtk_widget_set_valign(pw, GTK_ALIGN_CENTER);
            gtk_grid_attach(GTK_GRID(pwGrid), pw, 3, iRow, 1, 1);
            gtk_widget_set_hexpand(pw, TRUE);
            gtk_widget_set_vexpand(pw, TRUE);
#if GTK_CHECK_VERSION(3,12,0)
            gtk_widget_set_margin_start(pw, 8);
            gtk_widget_set_margin_end(pw, 8);
#else
            gtk_widget_set_margin_left(pw, 8);
            gtk_widget_set_margin_right(pw, 8);
#endif
#else
            gtk_misc_set_alignment(GTK_MISC(pw), 1, 0.5);
            gtk_table_attach(GTK_TABLE(pwTable), pw,
                             3, 4, iRow, iRow + 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 8, 0);
#endif
        }

        iRow++;

        /* rollout details */

        if (pes->et == EVAL_ROLLOUT && (ai[i] == OUTPUT_TAKE || ai[i] == OUTPUT_NODOUBLE)) {

            /* FIXME: output cubeless equities and percentages for rollout */
            /*        probably along with rollout details */

            pw = gtk_label_new(OutputPercents(cdec->aarOutput[ai[i] - 1], TRUE));
            
#if GTK_CHECK_VERSION(3,0,0)
            gtk_widget_set_halign(pw, GTK_ALIGN_CENTER);
            gtk_widget_set_valign(pw, GTK_ALIGN_CENTER);
            gtk_grid_attach(GTK_GRID(pwGrid), pw, 0, iRow, 4, 1);
            gtk_widget_set_hexpand(pw, TRUE);
            gtk_widget_set_vexpand(pw, TRUE);
#if GTK_CHECK_VERSION(3,12,0)
            gtk_widget_set_margin_start(pw, 8);
            gtk_widget_set_margin_end(pw, 8);
#else
            gtk_widget_set_margin_left(pw, 8);
            gtk_widget_set_margin_right(pw, 8);
#endif
            gtk_widget_set_margin_top(pw, 4);
            gtk_widget_set_margin_bottom(pw, 4);
#else
            gtk_misc_set_alignment(GTK_MISC(pw), 0.5, 0.5);
            gtk_table_attach(GTK_TABLE(pwTable), pw,
                             0, 4, iRow, iRow + 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 8, 4);
#endif
            iRow++;

        }

    }

    /* proper cube action */

    pw = gtk_label_new(_("Proper cube action: "));
#if GTK_CHECK_VERSION(3,0,0)
    gtk_widget_set_halign(pw, GTK_ALIGN_START);
    gtk_widget_set_valign(pw, GTK_ALIGN_CENTER);
    gtk_grid_attach(GTK_GRID(pwGrid), pw, 0, iRow, 2, 1);
    gtk_widget_set_hexpand(pw, TRUE);
    gtk_widget_set_vexpand(pw, TRUE);
#if GTK_CHECK_VERSION(3,12,0)
    gtk_widget_set_margin_start(pw, 8);
    gtk_widget_set_margin_end(pw, 8);
#else
    gtk_widget_set_margin_left(pw, 8);
    gtk_widget_set_margin_right(pw, 8);
#endif
    gtk_widget_set_margin_top(pw, 8);
    gtk_widget_set_margin_bottom(pw, 8);
#else
    gtk_misc_set_alignment(GTK_MISC(pw), 0, 0.5);
    gtk_table_attach(GTK_TABLE(pwTable), pw, 0, 2, iRow, iRow + 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 8, 8);
#endif

    pw = gtk_label_new(GetCubeRecommendation(cd));
#if GTK_CHECK_VERSION(3,0,0)
    gtk_widget_set_halign(pw, GTK_ALIGN_START);
    gtk_widget_set_valign(pw, GTK_ALIGN_CENTER);
    gtk_grid_attach(GTK_GRID(pwGrid), pw, 2, iRow, 1, 1);
    gtk_widget_set_hexpand(pw, TRUE);
    gtk_widget_set_vexpand(pw, TRUE);
#if GTK_CHECK_VERSION(3,12,0)
    gtk_widget_set_margin_start(pw, 8);
    gtk_widget_set_margin_end(pw, 8);
#else
    gtk_widget_set_margin_left(pw, 8);
    gtk_widget_set_margin_right(pw, 8);
#endif
    gtk_widget_set_margin_top(pw, 8);
    gtk_widget_set_margin_bottom(pw, 8);
#else
    gtk_misc_set_alignment(GTK_MISC(pw), 0, 0.5);
    gtk_table_attach(GTK_TABLE(pwTable), pw, 2, 3, iRow, iRow + 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 8, 8);
#endif

    /* percent */

    if ((r = getPercent(cd, arDouble)) >= 0.0f) {

        sz = g_strdup_printf("(%.1f%%)", 100.0f * r);
        pw = gtk_label_new(sz);
        g_free(sz);
#if GTK_CHECK_VERSION(3,0,0)
        gtk_widget_set_halign(pw, GTK_ALIGN_END);
        gtk_widget_set_valign(pw, GTK_ALIGN_CENTER);
        gtk_grid_attach(GTK_GRID(pwGrid), pw, 3, iRow, 1, 1);
        gtk_widget_set_hexpand(pw, TRUE);
        gtk_widget_set_vexpand(pw, TRUE);
#if GTK_CHECK_VERSION(3,12,0)
        gtk_widget_set_margin_start(pw, 8);
        gtk_widget_set_margin_end(pw, 8);
#else
        gtk_widget_set_margin_left(pw, 8);
        gtk_widget_set_margin_right(pw, 8);
#endif
        gtk_widget_set_margin_top(pw, 8);
        gtk_widget_set_margin_bottom(pw, 8);
#else
        gtk_misc_set_alignment(GTK_MISC(pw), 1, 0.5);
        gtk_table_attach(GTK_TABLE(pwTable), pw,
                         3, 4, iRow, iRow + 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 8, 8);
#endif
    }
    iRow++;

    /* Jacoby option - only appears when using money eval */
    if (pchd->evalAtMoney) {

        GtkWidget *pwJ = gtk_check_button_new_with_label(_("Jacoby"));

#if GTK_CHECK_VERSION(3,0,0)
        gtk_grid_attach(GTK_GRID(pwGrid), pwJ, 0, iRow, 2, 1);
        gtk_widget_set_hexpand(pwJ, TRUE);
        gtk_widget_set_vexpand(pwJ, TRUE);
#if GTK_CHECK_VERSION(3,12,0)
        gtk_widget_set_margin_start(pwJ, 8);
        gtk_widget_set_margin_end(pwJ, 8);
#else
        gtk_widget_set_margin_left(pwJ, 8);
        gtk_widget_set_margin_right(pwJ, 8);
#endif
        gtk_widget_set_margin_top(pwJ, 8);
        gtk_widget_set_margin_bottom(pwJ, 8);
#else
        gtk_table_attach(GTK_TABLE(pwTable), pwJ, 0, 2, iRow, iRow + 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 8, 8);
#endif
        g_signal_connect(G_OBJECT (pwJ), "toggled", G_CALLBACK(JacobyToggled), pchd); 
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pwJ), moneyEvalJacoby);
        gtk_widget_set_tooltip_text(pwJ, _("Toggle Jacoby rule for money play"));
    }

    return pwFrame;
}

static void
UpdateCubeAnalysis(cubehintdata * pchd)
/* This function destroys the existing analysis widget, replacing it with a new one.
It is called after something in pchd has changed, e.g., eval at different ply.
*/
{
    GtkWidget *pw = 0;
    //doubletype dt = DoubleType(ms.fDoubled, ms.fMove, ms.fTurn);//for the beaver etc; ms is extern (global) 
    /* ^ this is the original code. It does not make sense that we should be looking at the double type of ms, instead of pchd->ms!!
    */
    doubletype dt = DoubleType(pchd->ms.fDoubled, pchd->ms.fMove, pchd->ms.fTurn);

    if(!pchd->evalAtMoney) 
        find_skills(pchd->pmr, &pchd->ms, pchd->did_double, pchd->did_take);//gnubg.c / backgammon.h: 
    // Don't find skills for hypothetical money decision, because this is not relevant.

    switch (pchd->pmr->mt) {
    case MOVE_NORMAL:
    case MOVE_SETDICE:
        /*
         * We're stepping back in the game record after rolling and
         * asking for a hint on the missed cube decision, for instance
         */
    case MOVE_SETCUBEPOS:
    case MOVE_SETCUBEVAL:
        /*
         * These two happen in cube decisions loaded from a .sgf file
         */
    case MOVE_DOUBLE:
        if (dt == DT_NORMAL)
            pw = CubeAnalysis(pchd);
        else if (dt == DT_BEAVER)
            pw = TakeAnalysis(pchd);
        break;

    case MOVE_DROP:
    case MOVE_TAKE:
        pw = TakeAnalysis(pchd);
        break;

    default:
        g_assert_not_reached();
        break;

    }
   
    g_assert(pchd->pw != NULL);

    /* Remove old analysis */
    if (pchd->pwFrame != NULL) // This is null when first called from CreateCubeAnalysis()
        gtk_container_remove(GTK_CONTAINER(pchd->pw), pchd->pwFrame);

    /* Insert new analysis */
    if (pw != NULL) // This is null if there is no cube decision (see TakeAnalysis() and CubeAnalysis())
        gtk_box_pack_start(GTK_BOX(pchd->pw), pchd->pwFrame = pw, FALSE, FALSE, 0);
    
    gtk_widget_show_all(pchd->pw);
}

static void
CubeAnalysisRollout(GtkWidget * pw, cubehintdata * pchd)
/* Called by GTK when the rollout button is clicked. */
{

    cubeinfo ci;
    float aarOutput[2][NUM_ROLLOUT_OUTPUTS];
    float aarStdDev[2][NUM_ROLLOUT_OUTPUTS];
    cubedecisiondata *cdec = (pchd->evalAtMoney ? pchd->pmr->MoneyCubeDecPtr : pchd->pmr->CubeDecPtr);
    rolloutstat aarsStatistics[2][2];
    evalsetup *pes;
    char asz[2][FORMATEDMOVESIZE];
    void *p;

#if 0
    if (cdec==NULL) { // Should not happen, but just in case
        g_assert(pchd->evalAtMoney);
        cdec=pchd->pmr->MoneyCubeDecPtr=g_malloc(sizeof(cubedecisiondata));
        cdec->esDouble.et = EVAL_NONE;
        cdec->cmark = CMARK_NONE;
    }
#else
    g_assert(cdec != NULL);
#endif 

    pes = &cdec->esDouble;   // see function EvalCube for explanations on esDouble

    if (pes->et != EVAL_ROLLOUT) {
        pes->rc = rcRollout;
        pes->rc.nGamesDone = 0;
    } else {
        pes->rc.nTrials = rcRollout.nTrials;
        pes->rc.fStopOnSTD = rcRollout.fStopOnSTD;
        pes->rc.nMinimumGames = rcRollout.nMinimumGames;
        pes->rc.rStdLimit = rcRollout.rStdLimit;
        memcpy(aarOutput, cdec->aarOutput, 2 * NUM_ROLLOUT_OUTPUTS * sizeof(float));
        memcpy(aarStdDev, cdec->aarStdDev, 2 * NUM_ROLLOUT_OUTPUTS * sizeof(float));
    }

    if (!pchd->evalAtMoney)
        GetMatchStateCubeInfo(&ci, &pchd->ms);
    else 
        GetMoneyCubeInfo(&ci, &pchd->ms);

    FormatCubePositions(&ci, asz);
    GTKSetCurrentParent(pw);
    RolloutProgressStart(&ci, 2, aarsStatistics, &pes->rc, asz, FALSE, &p); // from progress.c

    if (GeneralCubeDecisionR(aarOutput, aarStdDev, aarsStatistics,
                             (ConstTanBoard) pchd->ms.anBoard, &ci, &pes->rc, pes, RolloutProgress, p) < 0) {
        RolloutProgressEnd(&p, FALSE);
        return;
    }

    RolloutProgressEnd(&p, FALSE);
    memcpy(cdec->aarOutput, aarOutput, 2 * NUM_ROLLOUT_OUTPUTS * sizeof(float));
    memcpy(cdec->aarStdDev, aarStdDev, 2 * NUM_ROLLOUT_OUTPUTS * sizeof(float));

    if (pes->et != EVAL_ROLLOUT)
        memcpy(&pes->rc, &rcRollout, sizeof(rcRollout)); //rcRollout: extrern rolloutcontext: includes rollout parameters (truncation etc)

    pes->et = EVAL_ROLLOUT;

    /* If the source widget parent has been destroyed do not attempt
     * to update the hint window */
    if (!GDK_IS_WINDOW(gtk_widget_get_parent_window(pw)))
        return;

    UpdateCubeAnalysis(pchd);
    if (pchd->hist && !pchd->evalAtMoney) // What is the point of this behaviour? I don't like it!
        ChangeGame(NULL);
}

static void
EvalCube(cubehintdata * pchd, const evalcontext * pec)
{
/* Evaluate the cube action based on the given eval context. */

    cubeinfo ci;
    decisionData dd;
    cubedecisiondata *cdec = (pchd->evalAtMoney ? pchd->pmr->MoneyCubeDecPtr : pchd->pmr->CubeDecPtr);
    evalsetup *pes;
    
    if (cdec==NULL) {  
        /* This happens if money eval is toggled, and it hasn't been toggled before (for this move record).
           Initialize the MoneyCubeDecPtr for this move record. */
        g_assert(pchd->evalAtMoney);
        cdec=pchd->pmr->MoneyCubeDecPtr=g_malloc(sizeof(cubedecisiondata));
        cdec->esDouble.et = EVAL_NONE;
        cdec->cmark = CMARK_NONE;
    }
    
    pes = &cdec->esDouble;   //eval.h: 
                // evalsetup: struct {evaltype et; evalcontext ec; rolloutcontext rc; } 
                // evaltype: enum {EVAL_NONE, EVAL_EVAL, EVAL_ROLLOUT}
                // likewise, evalcontext: fCubeful, nPlies, etc.; rolloutcontext for rollout
                // => they only contain int, float, etc, no pointers
    if (!pchd->evalAtMoney)
        GetMatchStateCubeInfo(&ci, &pchd->ms);
    else
        GetMoneyCubeInfo(&ci, &pchd->ms);

    dd.pboard = (ConstTanBoard) pchd->ms.anBoard;
    dd.pci = &ci;
    dd.pec = pec;
    dd.pes = NULL;
    if (RunAsyncProcess((AsyncFun) asyncCubeDecisionE, &dd, _("Considering cube action...")) != 0)
        return;

    /* save evaluation */
    memcpy(cdec->aarOutput, dd.aarOutput, 2 * NUM_ROLLOUT_OUTPUTS * sizeof(float));

    pes->et = EVAL_EVAL;
    memcpy(&pes->ec, dd.pec, sizeof(evalcontext)); //here the n-ply may change

    UpdateCubeAnalysis(pchd);
    if (pchd->hist && !pchd->evalAtMoney) // What is the point of this behaviour? I don't like it!
        ChangeGame(NULL);
}

static void
CubeAnalysisEvalPly(GtkWidget * pw, cubehintdata * pchd)
{
/* Called by GTK when one of the ply-evaluation buttons are clicked.
   Evaluates the cube action at the given ply and updates the widget.
*/

    int *pi = (int *)g_object_get_data(G_OBJECT(pw), "ply");
    evalcontext ec = { 0, 0, 0, TRUE, 0.0 };

    ec.fCubeful = esAnalysisCube.ec.fCubeful; //from backgammon.h: extern evalsetup esAnalysisCube;
    ec.fUsePrune = esAnalysisCube.ec.fUsePrune;
    ec.nPlies = *pi;

    EvalCube(pchd, &ec);
}

static void
CubeAnalysisEval(GtkWidget * UNUSED(pw), cubehintdata * pchd)
{
/* Called by GTK when one of the "eval" button is clicked.
   Evaluates the cube action at default settings, and updates the widget.
*/
    EvalCube(pchd, &GetEvalCube()->ec);
}

static void
CubeAnalysisEvalSettings(GtkWidget * pw, void *UNUSED(data))
{
/* Called by GTK when the eval settings button is clicked.
*/
    GTKSetCurrentParent(pw);
    SetAnalysis(NULL, 0, NULL);

    /* bring the dialog holding this button to the top */
    gtk_window_present(GTK_WINDOW(gtk_widget_get_toplevel(pw)));
}

static void
CubeAnalysisRolloutSettings(GtkWidget * pw, void *UNUSED(data))
{
/* Called by GTK when the rollout settings button is clicked.
*/
    GTKSetCurrentParent(pw);
    SetRollouts(NULL, 0, NULL);

    /* bring the dialog holding this button to the top */
    gtk_window_present(GTK_WINDOW(gtk_widget_get_toplevel(pw)));

}

static void
CubeRolloutPresets(GtkWidget * pw, cubehintdata * pchd)
{
/* Called by GTK when one of the rollout presents buttons are clicked.
Changes the rollout settings to the given preset and does a rollout.
Note: settings remain changed.
Note: settings are stored as commands in a file.
*/
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
        CubeAnalysisRollout(pw, pchd);
    } else {
        outputerrf(_("You need to save a preset as \"%s\""), file);
        CubeAnalysisRolloutSettings(pw, NULL);
    }
    g_free(file);
    g_free(path);
    g_free(command);
}

static void
CubeAnalysisMWC(GtkWidget * pw, cubehintdata * pchd)
{
/* Called by GTK when the MWC button is toggled. Switches output between MWC and equity. Only applicable during match play.
*/
    int f;

    g_assert(pchd->ms.nMatchTo);
    g_assert(!pchd->evalAtMoney);

    f = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pw));

    if (f != fOutputMWC) {
        gchar *sz = g_strdup_printf("set output mwc %s", fOutputMWC ? "off" : "on");

        UserCommand(sz);
        UserCommand("save settings");

        g_free(sz);
    }

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pw), fOutputMWC);

    UpdateCubeAnalysis(pchd);

}

static char *
GetContent(cubehintdata * pchd)
{
/* Returns a string containing the information visible in this widget.
*/
    static char *pc;
    cubeinfo ci;
    cubedecisiondata *cdec = (pchd->evalAtMoney ? pchd->pmr->MoneyCubeDecPtr : pchd->pmr->CubeDecPtr);
    int fTake;


    if(!pchd->evalAtMoney)
        GetMatchStateCubeInfo(&ci, &pchd->ms);
    else
        GetMoneyCubeInfo(&ci, &pchd->ms);
    
    // g_message("move type: %d, player: %d",pchd->pmr->mt, pchd->pmr->fPlayer);

    switch (pchd->pmr->mt) {
    case MOVE_NORMAL:    
    case MOVE_DOUBLE:
        fTake = -1;
        break;
    case MOVE_DROP:
        fTake = 0;
        break;
    case MOVE_TAKE:
        fTake = 1;
        break;
    default: //IK: problem: we get here with a regular {ND|move} ("MOVE_NORMAL") when we click on the cube tab
        g_assert_not_reached();
        return NULL;
    }

    pc = OutputCubeAnalysis(cdec->aarOutput, cdec->aarStdDev, &cdec->esDouble, &ci, fTake);

    return pc;
}

static void
CubeAnalysisMoneyEval(GtkWidget *pw, cubehintdata *pchd) 
{
/* Called by GTK when the money eval button is toggled. Should only be possible during match play.
*/
    g_assert(pchd->ms.nMatchTo);
    pchd->evalAtMoney = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pw));

    //we want to hide (resp. unhide) the MWC and cmark buttons when money eval is toggled
    gtk_widget_set_sensitive(pchd->pwMWC, !pchd->evalAtMoney); 
    gtk_widget_set_sensitive(pchd->pwCmark, !pchd->evalAtMoney); 

    if (pchd->evalAtMoney && (pchd->pmr->MoneyCubeDecPtr==NULL)) // Test if money eval has been toggled before for this move record
        EvalCube(pchd, &GetEvalCube()->ec); // If not, eval at current settings - this forces MoneyCubeDecPtr to be created.
    else
        UpdateCubeAnalysis(pchd);            
}

static void
CubeAnalysisCopy(GtkWidget * UNUSED(pw), cubehintdata * pchd)
{
/* Called by GTK when the copy button is clicked.
*/

    char *pc = GetContent(pchd);

    if (pc)
        GTKTextWindow(pc, _("Cube details"), DT_INFO, NULL);

}

static void
CubeAnalysisTempMap(GtkWidget * UNUSED(pw), cubehintdata * pchd)
{
/* Called by GTK when the temp map button is clicked.  */

    gchar *sz = g_strdup("show temperaturemap =cube");
    cubeTempMapAtMoney = pchd->evalAtMoney;
    if (cubeTempMapAtMoney)
        cubeTempMapJacoby = moneyEvalJacoby;
    UserCommand(sz);
    g_free(sz);

}

static void
CubeAnalysisScoreMap(GtkWidget * UNUSED(pw), cubehintdata * UNUSED(pchd))
{
/* Called by GTK when the score map button is clicked.
*/

    UserCommand("show scoremap"); //cf keyword -> gtkgame.c: CMD_SHOW_SCORE_MAP_CUBE

}

static void
CubeAnalysisCmark(GtkWidget * pw, cubehintdata * pchd)
{
/* Called by GTK when the Cmark button is clicked.
*/
    pchd->pmr->CubeDecPtr->cmark = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pw));
}

static GtkWidget *
CreateCubeAnalysisTools(cubehintdata * pchd)
{
/* Creates the buttons that appear at the bottom of the cube widget, and places them into the gtk table which is returned.
*/
    GtkWidget *pwTools;
    GtkWidget *pwEval = gtk_button_new_with_label(_("Eval"));
    GtkWidget *pwEvalSettings = gtk_button_new_with_label(_("..."));
    GtkWidget *pwRollout = gtk_button_new_with_label(_("Rollout"));
    GtkWidget *pwRolloutPresets;
    GtkWidget *pwRolloutSettings = gtk_button_new_with_label(_("..."));
    GtkWidget *pwMWC = pchd->pwMWC = gtk_toggle_button_new_with_label(_("MWC"));
    GtkWidget *pwCopy = gtk_button_new_with_label(_("Copy"));
    GtkWidget *pwTempMap = gtk_button_new_with_label(_("Temp. Map"));
    GtkWidget *pwCmark = pchd->pwCmark = gtk_toggle_button_new_with_label(_("Cmark"));
    GtkWidget *pwMoneyEval = gtk_toggle_button_new_with_label(_("Money Eval"));   
    GtkWidget *pwScoreMap = gtk_button_new_with_label(_("ScoreMap"));     
    GtkWidget *pw;
    int i;

#if GTK_CHECK_VERSION(3,0,0)
    gtk_style_context_add_class(gtk_widget_get_style_context(pwEval), "gnubg-analysis-button");
    gtk_style_context_add_class(gtk_widget_get_style_context(pwEvalSettings), "gnubg-analysis-button");
    gtk_style_context_add_class(gtk_widget_get_style_context(pwRollout), "gnubg-analysis-button");
    gtk_style_context_add_class(gtk_widget_get_style_context(pwRolloutSettings), "gnubg-analysis-button");
    gtk_style_context_add_class(gtk_widget_get_style_context(pwMWC), "gnubg-analysis-button");
    gtk_style_context_add_class(gtk_widget_get_style_context(pwCopy), "gnubg-analysis-button");
    gtk_style_context_add_class(gtk_widget_get_style_context(pwTempMap), "gnubg-analysis-button");
    gtk_style_context_add_class(gtk_widget_get_style_context(pwCmark), "gnubg-analysis-button");
    gtk_style_context_add_class(gtk_widget_get_style_context(pwMoneyEval), "gnubg-analysis-button");  
    gtk_style_context_add_class(gtk_widget_get_style_context(pwScoreMap), "gnubg-analysis-button");      
#endif

    /* toolbox on the left with buttons for eval, rollout and more */

#if GTK_CHECK_VERSION(3,0,0)
    pchd->pwTools = pwTools = gtk_grid_new();

    gtk_grid_attach(GTK_GRID(pwTools), pwEval, 0, 0, 1, 1);

    gtk_grid_attach(GTK_GRID(pwTools), pwEvalSettings, 1, 0, 1, 1);
#else
    pchd->pwTools = pwTools = gtk_table_new(2, 6, FALSE);

    gtk_table_attach(GTK_TABLE(pwTools), pwEval, 0, 1, 0, 1,
                     (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);

    gtk_table_attach(GTK_TABLE(pwTools), pwEvalSettings, 1, 2, 0, 1,
                     (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);
#endif

#if GTK_CHECK_VERSION(3,0,0)
    pw = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_grid_attach(GTK_GRID(pwTools), pw, 2, 0, 1, 1);
#else
    pw = gtk_hbox_new(FALSE, 0);
    gtk_table_attach(GTK_TABLE(pwTools), pw, 2, 3, 0, 1, (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);
#endif

    for (i = 0; i < 5; ++i) {

        gchar *sz;  
        int *pi = g_malloc(sizeof(int));

        sz = g_strdup_printf("%d", i);
        GtkWidget *pwply = gtk_button_new_with_label(sz);
        g_free(sz);

#if GTK_CHECK_VERSION(3,0,0)
        gtk_style_context_add_class(gtk_widget_get_style_context(pwply), "gnubg-analysis-button");
#endif
        gtk_box_pack_start(GTK_BOX(pw), pwply, TRUE, TRUE, 0);

        g_signal_connect(G_OBJECT(pwply), "clicked", G_CALLBACK(CubeAnalysisEvalPly), pchd);

        *pi = i;
        g_object_set_data_full(G_OBJECT(pwply), "ply", pi, g_free);

        sz = g_strdup_printf(_("Evaluate play on cubeful %d-ply"), i);
        gtk_widget_set_tooltip_text(pwply, sz);
        g_free(sz);

    }

#if GTK_CHECK_VERSION(3,0,0)
    pwRolloutPresets = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
#else
    pwRolloutPresets = gtk_hbox_new(FALSE, 0);
#endif

#if GTK_CHECK_VERSION(3,0,0)
    gtk_grid_attach(GTK_GRID(pwTools), pwMWC, 3, 0, 1, 1);
#else
    gtk_table_attach(GTK_TABLE(pwTools), pwMWC, 3, 4, 0, 1,
                     (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);
#endif

#if GTK_CHECK_VERSION(3,0,0)
    gtk_grid_attach(GTK_GRID(pwTools), pwTempMap, 4, 0, 1, 1);
#else
    gtk_table_attach(GTK_TABLE(pwTools), pwTempMap, 4, 5, 0, 1,
                     (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);
#endif

#if GTK_CHECK_VERSION(3,0,0)
    gtk_grid_attach(GTK_GRID(pwTools), pwCmark, 4, 1, 1, 1);
#else
    gtk_table_attach(GTK_TABLE(pwTools), pwCmark, 4, 5, 1, 2,
                     (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);
#endif

#if GTK_CHECK_VERSION(3,0,0)
    gtk_grid_attach(GTK_GRID(pwTools), pwRollout, 0, 1, 1, 1);
#else
    gtk_table_attach(GTK_TABLE(pwTools), pwRollout, 0, 1, 1, 2,
                     (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);
#endif

#if GTK_CHECK_VERSION(3,0,0)
    gtk_grid_attach(GTK_GRID(pwTools), pwRolloutSettings, 1, 1, 1, 1);
#else
    gtk_table_attach(GTK_TABLE(pwTools), pwRolloutSettings, 1, 2, 1, 2,
                     (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);
#endif

#if GTK_CHECK_VERSION(3,0,0)
    gtk_grid_attach(GTK_GRID(pwTools), pwRolloutPresets, 2, 1, 1, 1);
#else
    gtk_table_attach(GTK_TABLE(pwTools), pwRolloutPresets, 2, 3, 1, 2,
                     (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);
#endif

    for (i = 0; i < 5; ++i) {
        GtkWidget *ro_preset;
        gchar *sz = g_strdup_printf("%c", i + 'a');    /* string is freed by set_data_full */

        ro_preset = gtk_button_new_with_label(sz);
#if GTK_CHECK_VERSION(3,0,0)
        gtk_style_context_add_class(gtk_widget_get_style_context(ro_preset), "gnubg-analysis-button");
#endif
        gtk_box_pack_start(GTK_BOX(pwRolloutPresets), ro_preset, TRUE, TRUE, 0);

        g_signal_connect(G_OBJECT(ro_preset), "clicked", G_CALLBACK(CubeRolloutPresets), pchd);

        g_object_set_data_full(G_OBJECT(ro_preset), "user_data", sz, g_free);

        sz = g_strdup_printf(_("Rollout preset %c"), i + 'a');
        gtk_widget_set_tooltip_text(ro_preset, sz);
        g_free(sz);

    }

#if GTK_CHECK_VERSION(3,0,0)
    gtk_grid_attach(GTK_GRID(pwTools), pwCopy, 3, 1, 1, 1);
#else
    gtk_table_attach(GTK_TABLE(pwTools), pwCopy, 3, 4, 1, 2,
                     (GtkAttachOptions)(GTK_FILL), (GtkAttachOptions)(0), 0, 0);
#endif

#if GTK_CHECK_VERSION(3,0,0)
    gtk_grid_attach(GTK_GRID(pwTools), pwMoneyEval, 5, 0, 1, 1);
#else
    gtk_table_attach(GTK_TABLE(pwTools), pwMoneyEval, 5, 6, 0, 1,
                     (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);
#endif

#if GTK_CHECK_VERSION(3,0,0)
    gtk_grid_attach(GTK_GRID(pwTools), pwScoreMap, 5, 1, 1, 1);
#else
    gtk_table_attach(GTK_TABLE(pwTools), pwScoreMap, 5, 6, 1, 2,
                     (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);
#endif

    /* We want to disable some buttons particularly when we are in the middle
       of running an analysis in the background */

    gtk_widget_set_sensitive(pwRollout, !fAnalysisRunning);
    gtk_widget_set_sensitive(pwEval, !fAnalysisRunning);
    gtk_widget_set_sensitive(pwEvalSettings, !fAnalysisRunning);
    gtk_widget_set_sensitive(pw, !fAnalysisRunning);
    gtk_widget_set_sensitive(pwRolloutSettings, !fAnalysisRunning);
    gtk_widget_set_sensitive(pwRolloutPresets, !fAnalysisRunning);
    gtk_widget_set_sensitive(pwCopy, !fAnalysisRunning);
    gtk_widget_set_sensitive(pwTempMap, !fAnalysisRunning);
    gtk_widget_set_sensitive(pwScoreMap, !fAnalysisRunning);


    // Note: evalAtMoney should always be false here.
    gtk_widget_set_sensitive(pwMWC, pchd->ms.nMatchTo && !pchd->evalAtMoney); //MWC not available in money play, i.e.
                                                // either in true money play or in a hypothetical one
    gtk_widget_set_sensitive(pwMoneyEval, pchd->ms.nMatchTo && !fAnalysisRunning); //i.e., the Money Eval
                                                          // button is not available at money play since it doesn't help there
    //gtk_widget_set_sensitive(pwTempMap, !pchd->evalAtMoney);  
    gtk_widget_set_sensitive(pwCmark, !pchd->evalAtMoney);   


    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pwCmark), pchd->pmr->CubeDecPtr->cmark);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pwMWC), fOutputMWC);
    
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pwMoneyEval), pchd->evalAtMoney);   

    /* signals */

    g_signal_connect(G_OBJECT(pwRollout), "clicked", G_CALLBACK(CubeAnalysisRollout), pchd);
    g_signal_connect(G_OBJECT(pwEval), "clicked", G_CALLBACK(CubeAnalysisEval), pchd);
    g_signal_connect(G_OBJECT(pwEvalSettings), "clicked", G_CALLBACK(CubeAnalysisEvalSettings), NULL);
    g_signal_connect(G_OBJECT(pwRolloutSettings), "clicked", G_CALLBACK(CubeAnalysisRolloutSettings), NULL);
    g_signal_connect(G_OBJECT(pwMWC), "toggled", G_CALLBACK(CubeAnalysisMWC), pchd);
    g_signal_connect(G_OBJECT(pwCopy), "clicked", G_CALLBACK(CubeAnalysisCopy), pchd);
    g_signal_connect(G_OBJECT(pwTempMap), "clicked", G_CALLBACK(CubeAnalysisTempMap), pchd);
    g_signal_connect(G_OBJECT(pwCmark), "toggled", G_CALLBACK(CubeAnalysisCmark), pchd);
    // clicking pwMoneyEval launches CubeAnalysisMoneyEval
    g_signal_connect(G_OBJECT(pwMoneyEval), "toggled", G_CALLBACK(CubeAnalysisMoneyEval), pchd);
    g_signal_connect(G_OBJECT(pwScoreMap), "clicked", G_CALLBACK(CubeAnalysisScoreMap), pchd);    
  
    /* tool tips */

    // gtk_widget_set_tooltip_text(pwRollout, _("Rollout cube decision with current settings"));

    // gtk_widget_set_tooltip_text(pwEval, _("Evaluate cube decision with current settings"));

    gtk_widget_set_tooltip_text(pwRolloutSettings, _("Modify rollout settings"));

    gtk_widget_set_tooltip_text(pwEvalSettings, _("Modify evaluation settings"));

    gtk_widget_set_tooltip_text(pwMWC, _("Toggle output as MWC or equity"));

    // gtk_widget_set_tooltip_text(pwCopy, _("Copy cube decision to clipboard"));

    gtk_widget_set_tooltip_text(pwCmark, _("Mark cube decision for later rollout"));

    // gtk_widget_set_tooltip_text(pwTempMap, _("Show Sho Sengoku Temperature Map of position"));

    gtk_widget_set_tooltip_text(pwMoneyEval, _("Provide hypothetical cube decision in Money Play"));
    
    gtk_widget_set_tooltip_text(pwScoreMap, _("Provide cube decision at different scores"));

    if (!pchd->evalAtMoney) {
        gtk_widget_set_tooltip_text(pwRollout, _("Rollout cube decision with current settings"));

        gtk_widget_set_tooltip_text(pwEval, _("Evaluate cube decision with current settings"));

        gtk_widget_set_tooltip_text(pwCopy, _("Copy cube decision to clipboard"));

        gtk_widget_set_tooltip_text(pwTempMap, _("Show Sho Sengoku Temperature Map of position"));


    } else {
        gtk_widget_set_tooltip_text(pwRollout, _("Rollout cube decision in Money Play with current settings"));

        gtk_widget_set_tooltip_text(pwEval, _("Evaluate cube decision in Money Play with current settings"));

        gtk_widget_set_tooltip_text(pwCopy, _("Copy cube decision in Money Play to clipboard"));

        gtk_widget_set_tooltip_text(pwTempMap, _("Temperature Map in Money Play of different rolls"));
     
    }

// g_print("matchstate.evalAtMoney EndOf CubeAnalysisTools:%d\n",pchd->evalAtMoney);

    return pwTools;

}


extern GtkWidget *
CreateCubeAnalysis(moverecord * pmr, const matchstate * pms, int did_double, int did_take, int hist)
{
/* Returns pwx, a box layed out as follows:
-- pwx ---------------
| --pchd->pw-------- |
| | pchd->pwdFrame | |
| ------------------ |
| --pwhb------------ |
| | pchd->pwTools  | |
| ------------------ |
---------------------|
*/
    cubehintdata *pchd = g_new0(cubehintdata, 1);

    GtkWidget *pw, *pwhb, *pwx;

    pchd->pmr = pmr;
    pchd->ms = *pms;
    pchd->did_double = did_double;
    pchd->did_take = did_take;
    pchd->hist = hist;
    pchd->evalAtMoney=FALSE;
    if (moneyEvalJacoby==UNDEF)
        moneyEvalJacoby=fJacoby; // Initially use value from preferences
        

#if GTK_CHECK_VERSION(3,0,0)
    pchd->pw = pw = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_widget_set_halign(pchd->pw, GTK_ALIGN_START);
#else
    pchd->pw = pw = gtk_hbox_new(FALSE, 2);
#endif

    UpdateCubeAnalysis(pchd);
    
    if (pchd->pwFrame == NULL) {
        g_free(pchd);
        return NULL;
    }

//    gtk_box_pack_start(GTK_BOX(pw), pchd->pwFrame, FALSE, FALSE, 0);

#if GTK_CHECK_VERSION(3,0,0)
    pwx = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
    pwx = gtk_vbox_new(FALSE, 0);
#endif

    gtk_box_pack_start(GTK_BOX(pwx), pw, FALSE, FALSE, 0);

#if GTK_CHECK_VERSION(3,0,0)
    pwhb = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
#else
    pwhb = gtk_hbox_new(FALSE, 0);
#endif
    gtk_box_pack_start(GTK_BOX(pwhb), CreateCubeAnalysisTools(pchd), FALSE, FALSE, 8); // calls CreateCubeAnalysisTools

    gtk_box_pack_start(GTK_BOX(pwx), pwhb, FALSE, FALSE, 0);

    /* hook to free cubehintdata on widget destruction */
    g_object_set_data_full(G_OBJECT(pw), "cubehintdata", pchd, g_free);

    return pwx;

}
