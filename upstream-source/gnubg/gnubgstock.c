/*
 * Copyright (C) 2008-2011 Christian Anthon <anthon@kiku.dk>
 * Copyright (C) 2016 Jeremy Moore <jmoore@ieee.org>
 *
 * Initial revision derived from The GIMP Library / gimpstock.c 
 * Copyright (C) 2001 Michael Natterer <mitch@gimp.org>
 *
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
 * $Id: gnubgstock.c,v 1.12 2019/11/05 22:01:25 plm Exp $
 */

#include "config.h"
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include "gnubgstock.h"

#ifdef USE_GRESOURCE
#include "gnubg-stock-resources.h"
#else
#include "gtklocdefs.h"
#endif

static GtkIconFactory *gnubg_stock_factory = NULL;

static void
icon_set_from_resource_path(GtkIconSet * set,
                            const char *resource_path, GtkIconSize size, GtkTextDirection direction, gboolean fallback)
{
    GtkIconSource *source;
    GdkPixbuf *pixbuf;
    GError *pixbuf_error = NULL;

    source = gtk_icon_source_new();

    if (direction != GTK_TEXT_DIR_NONE) {
        gtk_icon_source_set_direction(source, direction);
        gtk_icon_source_set_direction_wildcarded(source, FALSE);
    }

    gtk_icon_source_set_size(source, size);
    gtk_icon_source_set_size_wildcarded(source, FALSE);

    pixbuf = gdk_pixbuf_new_from_resource(resource_path, &pixbuf_error);

    if (pixbuf == NULL) {
        g_print(_("Failed to create pixbuf from %s, %s\n"), resource_path, pixbuf_error->message);
        g_assert_not_reached();
        return;
    }

    gtk_icon_source_set_pixbuf(source, pixbuf);

    g_object_unref(pixbuf);

    gtk_icon_set_add_source(set, source);

    if (fallback) {
        gtk_icon_source_set_size_wildcarded(source, TRUE);
        gtk_icon_set_add_source(set, source);
    }

    gtk_icon_source_free(source);
}

static void
add_sized_with_same_fallback(GtkIconFactory * factory,
                             const char *resource_path,
                             const char *resource_path_rtl, GtkIconSize size, const gchar *stock_id)
{
    GtkIconSet *set;
    gboolean fallback = FALSE;

    set = gtk_icon_factory_lookup(factory, stock_id);

    if (!set) {
        set = gtk_icon_set_new();
        gtk_icon_factory_add(factory, stock_id, set);
        gtk_icon_set_unref(set);

        fallback = TRUE;
    }

    icon_set_from_resource_path(set, resource_path, size, GTK_TEXT_DIR_NONE, fallback);

    if (resource_path_rtl)
        icon_set_from_resource_path(set, resource_path_rtl, size, GTK_TEXT_DIR_RTL, fallback);
}

static const GtkStockItem gnubg_stock_items[] = {
    {GNUBG_STOCK_ACCEPT, N_("_Accept"), 0, 0, NULL},
    {GNUBG_STOCK_ANTI_CLOCKWISE, N_("Play in Anti-clockwise Direction"), 0, 0, NULL},
    {GNUBG_STOCK_CLOCKWISE, N_("Play in _Clockwise Direction"), 0, 0, NULL},
    {GNUBG_STOCK_DOUBLE, N_("_Double"), 0, 0, NULL},
    {GNUBG_STOCK_END_GAME, N_("_End Game"), 0, 0, NULL},
    {GNUBG_STOCK_GO_NEXT_CMARKED, N_("Next CMarked"), 0, 0, NULL},
    {GNUBG_STOCK_GO_NEXT_GAME, N_("Next _Game"), 0, 0, NULL},
    {GNUBG_STOCK_GO_NEXT_MARKED, N_("Next _Marked"), 0, 0, NULL},
    {GNUBG_STOCK_GO_NEXT, N_("_Next"), 0, 0, NULL},
    {GNUBG_STOCK_GO_PREV_CMARKED, N_("Previous CMarked"), 0, 0, NULL},
    {GNUBG_STOCK_GO_PREV_GAME, N_("Previous _Game"), 0, 0, NULL},
    {GNUBG_STOCK_GO_PREV_MARKED, N_("Previous _Marked"), 0, 0, NULL},
    {GNUBG_STOCK_GO_PREV, N_("_Previous"), 0, 0, NULL},
    {GNUBG_STOCK_HINT, N_("_Hint"), 0, 0, NULL},
    {GNUBG_STOCK_REJECT, N_("_Reject"), 0, 0, NULL},
    {GNUBG_STOCK_RESIGN, N_("_Resign"), 0, 0, NULL},
};

static const struct {
    const gchar *stock_id;
    const char *ltr;
    const char *rtl;
    GtkIconSize size;
} gnubg_stock_pixbufs[] = {
    {GNUBG_STOCK_ACCEPT, "/org/gnubg/16x16/actions/ok_16.png", NULL, GTK_ICON_SIZE_MENU},
    {GNUBG_STOCK_ACCEPT, "/org/gnubg/24x24/actions/ok_24.png", NULL, GTK_ICON_SIZE_LARGE_TOOLBAR},
    {GNUBG_STOCK_ANTI_CLOCKWISE, "/org/gnubg/24x24/actions/anti_clockwise_24.png", NULL, GTK_ICON_SIZE_LARGE_TOOLBAR},
    {GNUBG_STOCK_CLOCKWISE, "/org/gnubg/24x24/actions/clockwise_24.png", NULL, GTK_ICON_SIZE_LARGE_TOOLBAR},
    {GNUBG_STOCK_DOUBLE, "/org/gnubg/16x16/actions/double_16.png", NULL, GTK_ICON_SIZE_MENU},
    {GNUBG_STOCK_DOUBLE, "/org/gnubg/24x24/actions/double_24.png", NULL, GTK_ICON_SIZE_LARGE_TOOLBAR},
    {GNUBG_STOCK_END_GAME, "/org/gnubg/16x16/actions/runit_16.png", NULL, GTK_ICON_SIZE_MENU},
    {GNUBG_STOCK_END_GAME, "/org/gnubg/24x24/actions/runit_24.png", NULL, GTK_ICON_SIZE_LARGE_TOOLBAR},
    {GNUBG_STOCK_GO_NEXT_CMARKED, "/org/gnubg/16x16/actions/go_next_cmarked_16.png", NULL, GTK_ICON_SIZE_MENU},
    {GNUBG_STOCK_GO_NEXT_CMARKED, "/org/gnubg/24x24/actions/go_next_cmarked_24.png", NULL, GTK_ICON_SIZE_LARGE_TOOLBAR},
    {GNUBG_STOCK_GO_NEXT_GAME, "/org/gnubg/16x16/actions/go_next_game_16.png", NULL, GTK_ICON_SIZE_MENU},
    {GNUBG_STOCK_GO_NEXT_GAME, "/org/gnubg/24x24/actions/go_next_game_24.png", NULL, GTK_ICON_SIZE_LARGE_TOOLBAR},
    {GNUBG_STOCK_GO_NEXT, "/org/gnubg/16x16/actions/go_next_16.png", NULL, GTK_ICON_SIZE_MENU},
    {GNUBG_STOCK_GO_NEXT, "/org/gnubg/24x24/actions/go_next_24.png", NULL, GTK_ICON_SIZE_LARGE_TOOLBAR},
    {GNUBG_STOCK_GO_NEXT_MARKED, "/org/gnubg/16x16/actions/go_next_marked_16.png", NULL, GTK_ICON_SIZE_MENU},
    {GNUBG_STOCK_GO_NEXT_MARKED, "/org/gnubg/24x24/actions/go_next_marked_24.png", NULL, GTK_ICON_SIZE_LARGE_TOOLBAR},
    {GNUBG_STOCK_GO_PREV_CMARKED, "/org/gnubg/16x16/actions/go_prev_cmarked_16.png", NULL, GTK_ICON_SIZE_MENU},
    {GNUBG_STOCK_GO_PREV_CMARKED, "/org/gnubg/24x24/actions/go_prev_cmarked_24.png", NULL, GTK_ICON_SIZE_LARGE_TOOLBAR},
    {GNUBG_STOCK_GO_PREV_GAME, "/org/gnubg/16x16/actions/go_prev_game_16.png", NULL, GTK_ICON_SIZE_MENU},
    {GNUBG_STOCK_GO_PREV_GAME, "/org/gnubg/24x24/actions/go_prev_game_24.png", NULL, GTK_ICON_SIZE_LARGE_TOOLBAR},
    {GNUBG_STOCK_GO_PREV, "/org/gnubg/16x16/actions/go_prev_16.png", NULL, GTK_ICON_SIZE_MENU},
    {GNUBG_STOCK_GO_PREV, "/org/gnubg/24x24/actions/go_prev_24.png", NULL, GTK_ICON_SIZE_LARGE_TOOLBAR},
    {GNUBG_STOCK_GO_PREV_MARKED, "/org/gnubg/16x16/actions/go_prev_marked_16.png", NULL, GTK_ICON_SIZE_MENU},
    {GNUBG_STOCK_GO_PREV_MARKED, "/org/gnubg/24x24/actions/go_prev_marked_24.png", NULL, GTK_ICON_SIZE_LARGE_TOOLBAR},
    {GNUBG_STOCK_HINT, "/org/gnubg/16x16/actions/hint_16.png", NULL, GTK_ICON_SIZE_MENU},
    {GNUBG_STOCK_HINT, "/org/gnubg/24x24/actions/hint_24.png", NULL, GTK_ICON_SIZE_LARGE_TOOLBAR},
    {GNUBG_STOCK_NEW0, "/org/gnubg/24x24/actions/new0_24.png", NULL, GTK_ICON_SIZE_LARGE_TOOLBAR},
    {GNUBG_STOCK_NEW11, "/org/gnubg/24x24/actions/new11_24.png", NULL, GTK_ICON_SIZE_LARGE_TOOLBAR},
    {GNUBG_STOCK_NEW13, "/org/gnubg/24x24/actions/new13_24.png", NULL, GTK_ICON_SIZE_LARGE_TOOLBAR},
    {GNUBG_STOCK_NEW15, "/org/gnubg/24x24/actions/new15_24.png", NULL, GTK_ICON_SIZE_LARGE_TOOLBAR},
    {GNUBG_STOCK_NEW17, "/org/gnubg/24x24/actions/new17_24.png", NULL, GTK_ICON_SIZE_LARGE_TOOLBAR},
    {GNUBG_STOCK_NEW1, "/org/gnubg/24x24/actions/new1_24.png", NULL, GTK_ICON_SIZE_LARGE_TOOLBAR},
    {GNUBG_STOCK_NEW3, "/org/gnubg/24x24/actions/new3_24.png", NULL, GTK_ICON_SIZE_LARGE_TOOLBAR},
    {GNUBG_STOCK_NEW5, "/org/gnubg/24x24/actions/new5_24.png", NULL, GTK_ICON_SIZE_LARGE_TOOLBAR},
    {GNUBG_STOCK_NEW7, "/org/gnubg/24x24/actions/new7_24.png", NULL, GTK_ICON_SIZE_LARGE_TOOLBAR},
    {GNUBG_STOCK_NEW9, "/org/gnubg/24x24/actions/new9_24.png", NULL, GTK_ICON_SIZE_LARGE_TOOLBAR},
    {GNUBG_STOCK_REJECT, "/org/gnubg/16x16/actions/cancel_16.png", NULL, GTK_ICON_SIZE_MENU},
    {GNUBG_STOCK_REJECT, "/org/gnubg/24x24/actions/cancel_24.png", NULL, GTK_ICON_SIZE_LARGE_TOOLBAR},
    {GNUBG_STOCK_RESIGN, "/org/gnubg/16x16/actions/resign_16.png", NULL, GTK_ICON_SIZE_MENU},
    {GNUBG_STOCK_RESIGN, "/org/gnubg/24x24/actions/resign_24.png", NULL, GTK_ICON_SIZE_LARGE_TOOLBAR},
    {GNUBG_STOCK_RESIGNSB, "/org/gnubg/24x24/actions/resignsb_24.png", NULL, GTK_ICON_SIZE_LARGE_TOOLBAR},
    {GNUBG_STOCK_RESIGNSG, "/org/gnubg/24x24/actions/resignsg_24.png", NULL, GTK_ICON_SIZE_LARGE_TOOLBAR},
    {GNUBG_STOCK_RESIGNSN, "/org/gnubg/24x24/actions/resignsn_24.png", NULL, GTK_ICON_SIZE_LARGE_TOOLBAR}
};

/**
 * gnubg_stock_init:
 *
 * Initializes the GNUbg stock icon factory.
 */
void
gnubg_stock_init(void)
{
    guint i;
    gnubg_stock_factory = gtk_icon_factory_new();
    gnubg_stock_register_resource();

    for (i = 0; i < G_N_ELEMENTS(gnubg_stock_pixbufs); i++) {
        add_sized_with_same_fallback(gnubg_stock_factory,
                                     gnubg_stock_pixbufs[i].ltr,
                                     gnubg_stock_pixbufs[i].rtl,
                                     gnubg_stock_pixbufs[i].size, gnubg_stock_pixbufs[i].stock_id);
    }

    gtk_icon_factory_add_default(gnubg_stock_factory);
    gtk_stock_add_static(gnubg_stock_items, G_N_ELEMENTS(gnubg_stock_items));
}
