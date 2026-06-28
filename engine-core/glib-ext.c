/*
 * Copyright (C) 2014-2015 Michael Petch <mpetch@gnubg.org>
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
 * $Id: glib-ext.c,v 1.17 2022/12/30 11:59:35 plm Exp $
 */

/* Map/GList extensions and utility functions for GLIB */

#include "glib-ext.h"

void
glib_ext_init(void)
{
#if !GLIB_CHECK_VERSION (2,32,0)
    if (!g_thread_supported())
        g_thread_init(NULL);
    g_assert(g_thread_supported());
#endif
    return;
}

/* GNU indent misformats the next 3 lines, but adding ; is inadequate */

GLIBEXT_DEFINE_BOXED_TYPE(GListBoxed, g_list_boxed, g_list_copy, g_list_gv_boxed_free)
GLIBEXT_DEFINE_BOXED_TYPE(GMapBoxed, g_map_boxed, g_list_copy, g_list_gv_boxed_free)
GLIBEXT_DEFINE_BOXED_TYPE(GMapEntryBoxed, g_mapentry_boxed, g_list_copy, g_list_gv_boxed_free)

#if ! GLIB_CHECK_VERSION(2,28,0)
void
g_list_free_full(GList * list, GDestroyNotify free_func)
{
    g_list_foreach(list, (GFunc) free_func, NULL);
    g_list_free(list);
}
#endif

void
g_value_unsetfree(GValue * gv)
{
    g_value_unset(gv);
    g_free(gv);
}

GMapEntry *
str2gv_map_has_key(GMap * map, GString * key)
{
    guint item;

    for (item = 0; item < g_list_length(map); item++) {
        GString *cmpkey = g_value_get_gstring(g_list_nth_data(g_value_get_boxed(g_list_nth_data(map, item)), 0));
        if (g_string_equal(cmpkey, key))
            return g_list_nth(map, item);
    }

    return NULL;
}

GValue *
str2gv_map_get_key_value(GMap * map, gchar * key, GValue * defaultgv)
{
    GString *gstrkey = g_string_new(key);
    GMapEntry *entry = str2gv_map_has_key(map, gstrkey);
    GValue *gv;

    if (entry)
        gv = g_list_nth_data(g_value_get_boxed(entry->data), 1);
    else
        gv = defaultgv;

    g_string_free(gstrkey, TRUE);

    return gv;
}

void
g_list_gv_free_full(gpointer data)
{
    g_assert(G_IS_VALUE(data));

    if (G_IS_VALUE(data))
        g_value_unsetfree(data);
}

void
g_list_gv_boxed_free(GList * list)
{
    g_list_free_full(list, (GDestroyNotify) g_list_gv_free_full);
}

GList *
create_str2int_tuple(char *str, int value)
{
    GString *tmpstr = g_string_new(str);
    GVALUE_CREATE(G_TYPE_INT, int, value, gvint);
    GVALUE_CREATE(G_TYPE_GSTRING, boxed, tmpstr, gvstr);
    g_string_free(tmpstr, TRUE);
    return g_list_prepend(g_list_prepend(NULL, gvint), gvstr);
}

GList *
create_str2gvalue_tuple(char *str, GValue * gv)
{
    GString *tmpstr = g_string_new(str);
    GVALUE_CREATE(G_TYPE_GSTRING, boxed, tmpstr, gvstr);
    g_string_free(tmpstr, TRUE);
    return g_list_prepend(g_list_prepend(NULL, gv), gvstr);
}

void
g_value_list_tostring(GString * str, GList * list, int depth)
{
    GList *cur = list;
    while (cur != NULL) {
        g_value_tostring(str, cur->data, depth);
        if ((cur = g_list_next(cur)))
            g_string_append(str, ", ");
    }
}

void
g_value_tostring(GString * str, GValue * gv, int depth)
{
    if (!gv)
        return;
    g_assert(G_IS_VALUE(gv));

    if (G_VALUE_HOLDS_INT(gv)) {
        g_string_append_printf(str, "%d", g_value_get_int(gv));
    } else if (G_VALUE_HOLDS_DOUBLE(gv)) {
        g_string_append_printf(str, "%lf", g_value_get_double(gv));
    } else if (G_VALUE_HOLDS_GSTRING(gv)) {
        g_string_append_printf(str, "\"%s\"", g_value_get_gstring_gchar(gv));
    } else if (G_VALUE_HOLDS_BOXED_GLIST_GV(gv)) {
        g_string_append_c(str, '(');
        g_value_list_tostring(str, g_value_get_boxed(gv), depth + 1);
        g_string_append_c(str, ')');
    } else if (G_VALUE_HOLDS_BOXED_MAP_GV(gv)) {
        g_string_append_c(str, '[');
        g_value_list_tostring(str, g_value_get_boxed(gv), depth + 1);
        g_string_append_c(str, ']');
    } else if (G_VALUE_HOLDS_BOXED_MAPENTRY_GV(gv)) {
        g_value_tostring(str, g_list_nth_data(g_value_get_boxed(gv), 0), depth + 1);
        g_string_append(str, " : ");
        g_value_tostring(str, g_list_nth_data(g_value_get_boxed(gv), 1), depth + 1);
    }
}
