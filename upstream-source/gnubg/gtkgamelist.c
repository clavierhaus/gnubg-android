/*
 * Copyright (C) 2004-2019 Jon Kinsey <jonkinsey@gmail.com>
 * Copyright (C) 2004-2018 the AUTHORS
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
 * $Id: gtkgamelist.c,v 1.52 2022/10/02 12:43:20 plm Exp $
 */

#include "config.h"
#include "gtklocdefs.h"

#include <gtk/gtk.h>
#include <glib.h>
#include <stdlib.h>
#include <string.h>

#include "backgammon.h"
#include "gtkboard.h"
#include "drawboard.h"
#include "positionid.h"
#include "gtkgame.h"
#include "util.h"

static GtkListStore *plsGameList;
static GtkWidget *pwGameList;
static GtkStyle *psGameList, *psCurrent, *psCubeErrors[3], *psChequerErrors[3], *psLucky[N_LUCKS];

static int
gtk_compare_fonts(GtkStyle * psOne, GtkStyle * psTwo)
{
    return pango_font_description_equal(psOne->font_desc, psTwo->font_desc);
}

static void
gtk_set_font(GtkStyle * psStyle, GtkStyle * psValue)
{
    pango_font_description_free(psStyle->font_desc);
    psStyle->font_desc = pango_font_description_copy(psValue->font_desc);
}

enum ListStoreColumns {
    GL_COL_MOVE_NUMBER,
    GL_COL_MOVE_STRING_0,
    GL_COL_MOVE_STRING_1,
    GL_COL_MOVE_RECORD_0,
    GL_COL_MOVE_RECORD_1,
    GL_COL_STYLE_0,
    GL_COL_STYLE_1,
    GL_COL_COMBINED,
    GL_COL_NUM_COLUMNS
};

extern void
GTKClearMoveRecord(void)
{
    gtk_list_store_clear(plsGameList);
}

static void
GameListSelectRow(GtkTreeView * tree_view, gpointer UNUSED(p))
{
#if defined(USE_BOARD3D)
    BoardData *bd = BOARD(pwBoard)->board_data;
#endif
    moverecord *apmr[2], *pmr, *pmrPrev = NULL;
    gboolean fCombined;
    GtkTreePath *path;
    GtkTreeViewColumn *column;
    GtkTreeIter iter;
    int *pPlayer;
    listOLD *pl;

    gtk_tree_view_get_cursor(tree_view, &path, &column);
    if (!path)
        return;

    /* This didn't seem to happen with GTK2 but has been noticed with GTK3 */
    if (!column) {
        gtk_tree_path_free(path);
        return;
    }

    pPlayer = g_object_get_data(G_OBJECT(column), "player");
    if (!pPlayer) {
        gtk_tree_path_free(path);
        return;
    }

    gtk_tree_model_get_iter(GTK_TREE_MODEL(plsGameList), &iter, path);
    gtk_tree_model_get(GTK_TREE_MODEL(plsGameList), &iter, GL_COL_MOVE_RECORD_0, &apmr[0],
                       GL_COL_MOVE_RECORD_1, &apmr[1], GL_COL_COMBINED, &fCombined, -1);
    pmr = apmr[fCombined ? 0 : *pPlayer];

    /* Get previous move record */
    if (!fCombined && *pPlayer == 1)
        pmrPrev = apmr[0];
    else {
        if (!gtk_tree_path_prev(path))
            pmrPrev = NULL;
        else {
            gtk_tree_model_get_iter(GTK_TREE_MODEL(plsGameList), &iter, path);
            gtk_tree_model_get(GTK_TREE_MODEL(plsGameList), &iter, GL_COL_MOVE_RECORD_0, &apmr[0],
                               GL_COL_MOVE_RECORD_1, &apmr[1], GL_COL_COMBINED, &fCombined, -1);
            pmrPrev = apmr[fCombined ? 0 : 1];
        }
    }

    gtk_tree_path_free(path);

    if (!pmr && !pmrPrev)
        return;

    for (pl = plGame->plPrev; pl != plGame; pl = pl->plPrev) {
        g_assert(pl->p);
        if (pl == plGame->plPrev && pl->p == pmr && pmr && pmr->mt == MOVE_SETDICE)
            break;

        if (pl->p == pmrPrev && pmr != pmrPrev) {
            /* pmr = pmrPrev; */
            break;
        } else if (pl->plNext->p == pmr) {
            /* pmr = pl->p; */
            break;
        }
    }

    if (pl == plGame)
        /* couldn't find the moverecord they selected */
        return;

    plLastMove = pl;

    CalculateBoard();

    if (pmr && (pmr->mt == MOVE_NORMAL || pmr->mt == MOVE_SETDICE)) {
        /* roll dice */
        ms.gs = GAME_PLAYING;
        ms.anDice[0] = pmr->anDice[0];
        ms.anDice[1] = pmr->anDice[1];
    }
#if defined(USE_BOARD3D)
    if (display_is_3d(bd->rd)) {        /* Make sure dice are shown (and not rolled) */
        bd->diceShown = DICE_ON_BOARD;
        bd->diceRoll[0] = !ms.anDice[0];
    }
#endif

    UpdateSetting(&ms.nCube);
    UpdateSetting(&ms.fCubeOwner);
    UpdateSetting(&ms.fTurn);
    UpdateSetting(&ms.gs);

    SetMoveRecord(pl->p);

    ShowBoard();
}

extern void
GL_SetNames(void)
{
    gtk_tree_view_column_set_title(gtk_tree_view_get_column(GTK_TREE_VIEW(pwGameList), 1), ap[0].szName);
    gtk_tree_view_column_set_title(gtk_tree_view_get_column(GTK_TREE_VIEW(pwGameList), 2), ap[1].szName);
}

static void
SetColourIfDifferent(GdkColor cOrg[5], GdkColor cNew[5], GdkColor cDefault[5])
{
    int i;
    for (i = 0; i < 5; i++) {
        if (memcmp(&cNew[i], &cDefault[i], sizeof(GdkColor)))
            memcpy(&cOrg[i], &cNew[i], sizeof(GdkColor));
    }
}

static void
UpdateStyle(GtkStyle * psStyle, GtkStyle * psNew, GtkStyle * psDefault)
{
    /* Only set changed attributes */
    SetColourIfDifferent(psStyle->fg, psNew->fg, psDefault->fg);
    SetColourIfDifferent(psStyle->bg, psNew->bg, psDefault->bg);
    SetColourIfDifferent(psStyle->base, psNew->base, psDefault->base);

    if (!gtk_compare_fonts(psNew, psDefault))
        gtk_set_font(psStyle, psNew);
}

void
GetStyleFromRCFile(GtkStyle ** ppStyle, const char *name, GtkStyle * psBase)
{                               /* Note gtk 1.3 doesn't seem to have a nice way to do this... */
    BoardData *bd = BOARD(pwBoard)->board_data;
    GtkStyle *psDefault, *psNew;
    GtkWidget *dummy, *temp;
    char styleName[100];
    /* Get default style so only changes to this are applied */
    temp = gtk_button_new();
    gtk_widget_ensure_style(temp);
    psDefault = gtk_widget_get_style(temp);

    /* Get Style from rc file */
    strcpy(styleName, "gnubg-");
    strcat(styleName, name);
    dummy = gtk_label_new("");
    gtk_widget_ensure_style(dummy);
    gtk_widget_set_name(dummy, styleName);
    /* Pack in box to make sure style is loaded */
    gtk_box_pack_start(GTK_BOX(bd->table), dummy, FALSE, FALSE, 0);
    psNew = gtk_widget_get_style(dummy);

    /* Base new style on base style passed in */
    *ppStyle = gtk_style_copy(psBase);
    /* Make changes to fg+bg and copy to selected states */
    if (memcmp(&psNew->fg[GTK_STATE_ACTIVE], &psDefault->fg[GTK_STATE_ACTIVE], sizeof(GdkColor)))
        memcpy(&(*ppStyle)->fg[GTK_STATE_NORMAL], &psNew->fg[GTK_STATE_ACTIVE], sizeof(GdkColor));
    if (memcmp(&psNew->fg[GTK_STATE_NORMAL], &psDefault->fg[GTK_STATE_NORMAL], sizeof(GdkColor)))
        memcpy(&(*ppStyle)->fg[GTK_STATE_NORMAL], &psNew->fg[GTK_STATE_NORMAL], sizeof(GdkColor));
    memcpy(&(*ppStyle)->fg[GTK_STATE_SELECTED], &(*ppStyle)->fg[GTK_STATE_NORMAL], sizeof(GdkColor));

    if (memcmp(&psNew->base[GTK_STATE_NORMAL], &psDefault->base[GTK_STATE_NORMAL], sizeof(GdkColor)))
        memcpy(&(*ppStyle)->base[GTK_STATE_NORMAL], &psNew->base[GTK_STATE_NORMAL], sizeof(GdkColor));
    memcpy(&(*ppStyle)->bg[GTK_STATE_SELECTED], &(*ppStyle)->base[GTK_STATE_NORMAL], sizeof(GdkColor));
    /* Update the font if different */
    if (!gtk_compare_fonts(psNew, psDefault))
        gtk_set_font(*ppStyle, psNew);

    /* Remove useless widgets */
    gtk_widget_destroy(dummy);
    g_object_ref_sink(G_OBJECT(temp));
    g_object_unref(G_OBJECT(temp));
}

static void
RenderMoveString(GtkTreeViewColumn * tree_column, GtkCellRenderer * cell, GtkTreeModel * tree_model,
                 GtkTreeIter * iter, gpointer UNUSED(p))
{
    gchar *moveString;
    GtkStyle *style;
    int *pPlayer = g_object_get_data(G_OBJECT(tree_column), "player");

    gtk_tree_model_get(tree_model, iter, GL_COL_MOVE_STRING_0 + *pPlayer, &moveString,
                       GL_COL_STYLE_0 + *pPlayer, &style, -1);
    if (!style) {
        style = psGameList;
        g_object_ref(style);
    }

    g_object_set(cell, "text", moveString, "background-gdk", &style->base[GTK_STATE_NORMAL],
                 "foreground-gdk", &style->fg[GTK_STATE_NORMAL], "font-desc", style->font_desc, NULL);

    if (moveString)
        g_free(moveString);
    g_object_unref(style);
}

static void
CreateStyles(GtkWidget * UNUSED(widget), gpointer UNUSED(p))
{
    GtkStyle *ps;

    gtk_widget_ensure_style(pwGameList);
    GetStyleFromRCFile(&ps, "gnubg", gtk_widget_get_style(pwGameList));
    ps->base[GTK_STATE_SELECTED] =
        ps->base[GTK_STATE_ACTIVE] =
        ps->base[GTK_STATE_NORMAL] = gtk_widget_get_style(pwGameList)->base[GTK_STATE_NORMAL];
    ps->fg[GTK_STATE_SELECTED] =
        ps->fg[GTK_STATE_ACTIVE] = ps->fg[GTK_STATE_NORMAL] = gtk_widget_get_style(pwGameList)->fg[GTK_STATE_NORMAL];
    gtk_widget_set_style(pwGameList, ps);

    psGameList = gtk_style_copy(ps);
    psGameList->bg[GTK_STATE_SELECTED] = psGameList->bg[GTK_STATE_NORMAL] = ps->base[GTK_STATE_NORMAL];

    psCurrent = gtk_style_copy(psGameList);
    psCurrent->bg[GTK_STATE_SELECTED] = psCurrent->bg[GTK_STATE_NORMAL] =
        psCurrent->base[GTK_STATE_SELECTED] = psCurrent->base[GTK_STATE_NORMAL] = psGameList->fg[GTK_STATE_NORMAL];
    psCurrent->fg[GTK_STATE_SELECTED] = psCurrent->fg[GTK_STATE_NORMAL] = psGameList->bg[GTK_STATE_NORMAL];

    GetStyleFromRCFile(&psCubeErrors[SKILL_VERYBAD], "gamelist-cube-blunder", psGameList);
    GetStyleFromRCFile(&psCubeErrors[SKILL_BAD], "gamelist-cube-error", psGameList);
    GetStyleFromRCFile(&psCubeErrors[SKILL_DOUBTFUL], "gamelist-cube-doubtful", psGameList);

    GetStyleFromRCFile(&psChequerErrors[SKILL_VERYBAD], "gamelist-chequer-blunder", psGameList);
    GetStyleFromRCFile(&psChequerErrors[SKILL_BAD], "gamelist-chequer-error", psGameList);
    GetStyleFromRCFile(&psChequerErrors[SKILL_DOUBTFUL], "gamelist-chequer-doubtful", psGameList);

    GetStyleFromRCFile(&psLucky[LUCK_VERYBAD], "gamelist-luck-bad", psGameList);
    GetStyleFromRCFile(&psLucky[LUCK_VERYGOOD], "gamelist-luck-good", psGameList);
}

extern GtkWidget *
GL_Create(void)
{
    static int player[] = { 0, 1 };
    GtkTreeViewColumn *column;
    GtkCellRenderer *renderer;
    gint nMaxWidth;
    PangoRectangle logical_rect;
    PangoLayout *layout;
    int i;

    plsGameList = gtk_list_store_new(GL_COL_NUM_COLUMNS, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING,
                                     G_TYPE_POINTER, G_TYPE_POINTER, G_TYPE_OBJECT, G_TYPE_OBJECT, G_TYPE_BOOLEAN);

    pwGameList = gtk_tree_view_new_with_model(GTK_TREE_MODEL(plsGameList));
    gtk_tree_view_set_headers_clickable(GTK_TREE_VIEW(pwGameList), FALSE);
    gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(pwGameList)), GTK_SELECTION_NONE);
    gtk_tree_view_set_fixed_height_mode(GTK_TREE_VIEW(pwGameList), TRUE);

    renderer = gtk_cell_renderer_text_new();
    g_object_set(renderer, "ypad", 0, NULL);
    gtk_cell_renderer_set_alignment(renderer, 1.0, 0.5);
    column = gtk_tree_view_column_new_with_attributes(_("#"), renderer, "text", GL_COL_MOVE_NUMBER, NULL);
    gtk_tree_view_column_set_alignment(column, 1.0);
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
    gtk_tree_view_append_column(GTK_TREE_VIEW(pwGameList), column);

    for (i = 0; i < 2; i++) {
        renderer = gtk_cell_renderer_text_new();
        g_object_set(renderer, "ypad", 0, NULL);
        column = gtk_tree_view_column_new();
        g_object_set_data(G_OBJECT(column), "player", &player[i]);
        gtk_tree_view_column_pack_start(column, renderer, FALSE);
        gtk_tree_view_column_set_cell_data_func(column, renderer, RenderMoveString, NULL, NULL);
        gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
        gtk_tree_view_column_set_expand(column, TRUE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(pwGameList), column);
    }

    GL_SetNames();

    layout = gtk_widget_create_pango_layout(pwGameList, "99");
    pango_layout_get_pixel_extents(layout, NULL, &logical_rect);
    g_object_unref(layout);
    nMaxWidth = logical_rect.width;
    gtk_tree_view_column_set_fixed_width(gtk_tree_view_get_column(GTK_TREE_VIEW(pwGameList), 0), nMaxWidth + 8);

    layout = gtk_widget_create_pango_layout(pwGameList, " (set board AAAAAAAAAAAAAA)");
    pango_layout_get_pixel_extents(layout, NULL, &logical_rect);
    g_object_unref(layout);
    nMaxWidth = logical_rect.width;
    gtk_tree_view_column_set_fixed_width(gtk_tree_view_get_column(GTK_TREE_VIEW(pwGameList), 1), nMaxWidth - 22);
    gtk_tree_view_column_set_fixed_width(gtk_tree_view_get_column(GTK_TREE_VIEW(pwGameList), 2), nMaxWidth - 22);

    g_signal_connect(G_OBJECT(pwGameList), "cursor-changed", G_CALLBACK(GameListSelectRow), NULL);
#if GTK_CHECK_VERSION(3,0,0)
    /* Set up styles after the widget's base GtkStyleContext has been created */
    g_signal_connect(G_OBJECT(pwGameList), "style-updated", G_CALLBACK(CreateStyles), NULL);
#else
    CreateStyles(pwGameList, NULL);
#endif

    return pwGameList;
}

static void
AddStyle(GtkStyle ** ppsComb, GtkStyle * psNew)
{
    if (!*ppsComb) {
        *ppsComb = psNew;
        g_object_ref(*ppsComb);
    } else {
        GtkStyle *copy = gtk_style_copy(*ppsComb);
        g_object_unref(*ppsComb);
        *ppsComb = copy;
        UpdateStyle(*ppsComb, psNew, psGameList);
    }
}

static void
SetCellStyle(GtkTreeIter * iter, int fPlayer, moverecord * pmr)
{
    GtkStyle *pStyle = NULL;

    if (fStyledGamelist) {
        if (pmr->lt == LUCK_VERYGOOD) {
            pStyle = psLucky[LUCK_VERYGOOD];
            g_object_ref(pStyle);
        } else if (pmr->lt == LUCK_VERYBAD) {
            pStyle = psLucky[LUCK_VERYBAD];
            g_object_ref(pStyle);
        }

        if (pmr->n.stMove == SKILL_DOUBTFUL)
            AddStyle(&pStyle, psChequerErrors[SKILL_DOUBTFUL]);
        if (pmr->stCube == SKILL_DOUBTFUL)
            AddStyle(&pStyle, psCubeErrors[SKILL_DOUBTFUL]);

        if (pmr->n.stMove == SKILL_BAD)
            AddStyle(&pStyle, psChequerErrors[SKILL_BAD]);
        if (pmr->stCube == SKILL_BAD)
            AddStyle(&pStyle, psCubeErrors[SKILL_BAD]);

        if (pmr->n.stMove == SKILL_VERYBAD)
            AddStyle(&pStyle, psChequerErrors[SKILL_VERYBAD]);
        if (pmr->stCube == SKILL_VERYBAD)
            AddStyle(&pStyle, psCubeErrors[SKILL_VERYBAD]);
    }

    if (!pStyle) {
        pStyle = psGameList;
        g_object_ref(pStyle);
    }

    gtk_list_store_set(plsGameList, iter, GL_COL_STYLE_0 + fPlayer, pStyle, -1);
    g_object_unref(pStyle);
}

/* Add a moverecord to the game list window.  NOTE: This function must be
 * called _before_ applying the moverecord, so it can be displayed
 * correctly. */
extern void
GTKAddMoveRecord(moverecord * pmr)
{
    moverecord *apmr[2] = { NULL, NULL };
    gboolean fCombined = TRUE;
    int fPlayer, moveNum = 0;
    GtkTreeIter iter;
    const char *pch = GetMoveString(pmr, &fPlayer, TRUE);
    if (!pch)
        return;

    if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(plsGameList), &iter)) {
        GtkTreeIter next = iter;
        while (gtk_tree_model_iter_next(GTK_TREE_MODEL(plsGameList), &next))
            iter = next;

        gtk_tree_model_get(GTK_TREE_MODEL(plsGameList), &iter, GL_COL_MOVE_NUMBER, &moveNum, GL_COL_MOVE_RECORD_0,
                           &apmr[0], GL_COL_MOVE_RECORD_1, &apmr[1], GL_COL_COMBINED, &fCombined, -1);
    }

    if (fCombined || apmr[1] || (fPlayer != 1 && apmr[0])) {
        /* Add new row */
        gtk_list_store_append(plsGameList, &iter);
        ++moveNum;
    }

    if (fPlayer == -1) {
        fCombined = TRUE;
        fPlayer = 0;
    } else
        fCombined = FALSE;

    gtk_list_store_set(plsGameList, &iter, GL_COL_MOVE_NUMBER, moveNum, GL_COL_MOVE_STRING_0 + fPlayer, pch,
                       GL_COL_MOVE_RECORD_0 + fPlayer, pmr, GL_COL_COMBINED, fCombined, -1);

    SetCellStyle(&iter, fPlayer, pmr);
}


/* Select a moverecord as the "current" one.  NOTE: This function must be
 * called _after_ applying the moverecord. */
extern void
GTKSetMoveRecord(moverecord * pmr)
{

    /* highlighted row/col in game record */
    static GtkTreePath *path = NULL;
    static int fPlayer = -1;

    GtkTreeIter iter;
    moverecord *apmr[2];
    gboolean iterValid, lastRow = FALSE;
    /* Avoid lots of screen updates */
    if (!frozen)
        SetAnnotation(pmr);

#ifdef UNDEF
    {
        GtkWidget *pwWin = GetPanelWidget(WINDOW_HINT);
        if (pwWin) {
            hintdata *phd = g_object_get_data(G_OBJECT(pwWin), "user_data");
            phd->fButtonsValid = FALSE;
            CheckHintButtons(phd);
        }
    }
#endif

    if (path && fPlayer != -1) {
        moverecord *pmrLast = NULL;
        if (gtk_tree_model_get_iter(GTK_TREE_MODEL(plsGameList), &iter, path)) {
            gtk_tree_model_get(GTK_TREE_MODEL(plsGameList), &iter, GL_COL_MOVE_RECORD_0, &apmr[0],
                               GL_COL_MOVE_RECORD_1, &apmr[1], -1);
            pmrLast = apmr[fPlayer];
            if (pmrLast)
                SetCellStyle(&iter, fPlayer, pmrLast);
            else
                gtk_list_store_set(plsGameList, &iter, GL_COL_STYLE_0 + fPlayer, psGameList, -1);
        }
    }

    if (path)
        gtk_tree_path_free(path);
    path = NULL;
    fPlayer = -1;

    if (!pmr)
        return;

    if (pmr == plGame->plNext->p) {
        g_assert(pmr->mt == MOVE_GAMEINFO);
        path = gtk_tree_path_new_first();

        if (plGame->plNext->plNext->p) {
            moverecord *pmrNext = plGame->plNext->plNext->p;

            if (pmrNext->mt == MOVE_NORMAL && pmrNext->fPlayer == 1)
                fPlayer = 1;
            else
                fPlayer = 0;
        } else
            fPlayer = 0;
    } else {
        iterValid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(plsGameList), &iter);
        while (iterValid) {
            gtk_tree_model_get(GTK_TREE_MODEL(plsGameList), &iter, GL_COL_MOVE_RECORD_0, &apmr[0],
                               GL_COL_MOVE_RECORD_1, &apmr[1], -1);
            if (apmr[1] == pmr) {
                fPlayer = 1;
                break;
            } else if (apmr[0] == pmr) {
                fPlayer = 0;
                break;
            }
            iterValid = gtk_tree_model_iter_next(GTK_TREE_MODEL(plsGameList), &iter);
        }

        if (iterValid) {
            path = gtk_tree_model_get_path(GTK_TREE_MODEL(plsGameList), &iter);
            lastRow = !gtk_tree_model_iter_next(GTK_TREE_MODEL(plsGameList), &iter);
            gtk_tree_model_get_iter(GTK_TREE_MODEL(plsGameList), &iter, path);
        }

        if (path && !(pmr->mt == MOVE_SETDICE && lastRow)) {
            do {
                if (++fPlayer > 1) {
                    fPlayer = 0;
                    gtk_tree_path_next(path);
                    iterValid = gtk_tree_model_iter_next(GTK_TREE_MODEL(plsGameList), &iter);
                }

                if (iterValid) {
                    GtkTreeIter tmpIter = iter;
                    lastRow = !gtk_tree_model_iter_next(GTK_TREE_MODEL(plsGameList), &tmpIter);
                    gtk_tree_model_get(GTK_TREE_MODEL(plsGameList), &iter, GL_COL_MOVE_RECORD_0, &apmr[0],
                                       GL_COL_MOVE_RECORD_1, &apmr[1], -1);
                }
            } while (iterValid && !lastRow && !apmr[fPlayer]);

            if (!iterValid) {
                int *moveIndex = gtk_tree_path_get_indices(path);
                gtk_list_store_append(plsGameList, &iter);
                gtk_list_store_set(plsGameList, &iter, GL_COL_MOVE_NUMBER, *moveIndex + 1, -1);
            }
        }
    }

    /* Highlight current move */
    if (path && gtk_tree_model_get_iter(GTK_TREE_MODEL(plsGameList), &iter, path)) {
        gtk_list_store_set(plsGameList, &iter, GL_COL_STYLE_0 + fPlayer, psCurrent, -1);
        gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(pwGameList), path, NULL, TRUE, 0.8f, 0.5f);
    }
}

extern void
GTKPopMoveRecord(moverecord * const pmr)
{
    GtkTreeIter iter;
    gboolean iterValid, recordFound = FALSE;
    moverecord *apmr[2];

    iterValid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(plsGameList), &iter);
    while (iterValid) {
        gtk_tree_model_get(GTK_TREE_MODEL(plsGameList), &iter, GL_COL_MOVE_RECORD_0, &apmr[0],
                           GL_COL_MOVE_RECORD_1, &apmr[1], -1);
        if (apmr[0] == pmr || recordFound) {
            /* the left column matches; delete the row */
            iterValid = gtk_list_store_remove(plsGameList, &iter);
            recordFound = TRUE;
        } else if (apmr[1] == pmr) {
            /* the right column matches; delete that column only */
            gtk_list_store_set(plsGameList, &iter, GL_COL_MOVE_STRING_1, NULL,
                               GL_COL_MOVE_RECORD_1, NULL, GL_COL_STYLE_1, NULL, -1);
            iterValid = gtk_tree_model_iter_next(GTK_TREE_MODEL(plsGameList), &iter);
            recordFound = TRUE;
        } else
            iterValid = gtk_tree_model_iter_next(GTK_TREE_MODEL(plsGameList), &iter);
    }
}
