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
 * $Id: glib-ext.h,v 1.16 2021/08/22 15:20:33 plm Exp $
 */

/* Map/GList extensions and utility functions for GLIB */

#ifndef GLIB_EXT_H
#define GLIB_EXT_H

#include <glib.h>
#include <glib-object.h>

#define GLIBEXT_MERGE(a,b)  a##b
#define GLIBEXT_LABEL_(a,b) GLIBEXT_MERGE(a, b)

#if ! GLIB_CHECK_VERSION(2,28,0)
extern void g_list_free_full(GList * list, GDestroyNotify free_func);
#endif

typedef GList GMap;
typedef GList GMapEntry;

extern void glib_ext_init(void);
extern GType g_list_boxed_get_type(void);
extern GType g_map_boxed_get_type(void);
extern GType g_mapentry_boxed_get_type(void);
extern void g_value_unsetfree(GValue * gv);
extern void g_value_print(GValue * gv, int depth);
extern GMapEntry *str2gv_map_has_key(GMap * map, GString * key);
extern GValue *str2gv_map_get_key_value(GMap * map, gchar * key, GValue * defaultgv);
extern void g_list_gv_boxed_free(GList * list);
extern void g_list_gv_free_full(gpointer data);
extern GList *create_str2int_tuple(char *str, int value);
extern GList *create_str2gvalue_tuple(char *str, GValue * gv);
extern void g_value_list_tostring(GString * str, GList * list, int depth);
extern void g_value_tostring(GString * str, GValue * gv, int depth);

#define G_TYPE_BOXED_GLIST_GV (g_list_boxed_get_type ())
#define G_VALUE_HOLDS_BOXED_GLIST_GV(value)	 (G_TYPE_CHECK_VALUE_TYPE ((value), G_TYPE_BOXED_GLIST_GV))

#define G_TYPE_BOXED_MAP_GV (g_map_boxed_get_type ())
#define G_VALUE_HOLDS_BOXED_MAP_GV(value) (G_TYPE_CHECK_VALUE_TYPE ((value), G_TYPE_BOXED_MAP_GV))

#define G_TYPE_BOXED_MAPENTRY_GV (g_mapentry_boxed_get_type ())
#define G_VALUE_HOLDS_BOXED_MAPENTRY_GV(value) (G_TYPE_CHECK_VALUE_TYPE ((value), G_TYPE_BOXED_MAPENTRY_GV))

#define G_VALUE_HOLDS_GSTRING(gvalue) G_TYPE_CHECK_VALUE_TYPE ((gvalue), G_TYPE_GSTRING)
#define g_value_get_gstring_gchar(gvalue) ((GString *)g_value_get_boxed(gvalue))->str
#define g_value_get_gstring(gvalue) ((GString *)g_value_get_boxed(gvalue))


#if ! GLIB_CHECK_VERSION(2,67,1)
#define GLIBEXT_DEFINE_BOXED_TYPE(TypeName, type_name, copy_func, free_func) \
        GType \
        type_name##_get_type (void) \
        { \
            static volatile gsize g_define_type_id__volatile = 0; \
            if (g_once_init_enter (&g_define_type_id__volatile)) { \
                    GType g_define_type_id = \
                        g_boxed_type_register_static (g_intern_static_string (#TypeName), \
                                                     (GBoxedCopyFunc) copy_func, \
                                                     (GBoxedFreeFunc) free_func); \
                g_once_init_leave (&g_define_type_id__volatile, g_define_type_id); \
            } \
            return g_define_type_id__volatile; \
        }
#else
#define GLIBEXT_DEFINE_BOXED_TYPE(TypeName, type_name, copy_func, free_func) \
        GType \
        type_name##_get_type (void) \
        { \
            static gsize static_g_define_type_id = 0; \
            if (g_once_init_enter (&static_g_define_type_id)) { \
                    GType g_define_type_id = \
                        g_boxed_type_register_static (g_intern_static_string (#TypeName), \
                                                     (GBoxedCopyFunc) copy_func, \
                                                     (GBoxedFreeFunc) free_func); \
                g_once_init_leave (&static_g_define_type_id, g_define_type_id); \
            } \
            return static_g_define_type_id; \
        }
#endif

#define GVALUE_CREATE(typeval,typename,value,gvobject) \
            GValue * gvobject = (GValue *)g_malloc0(sizeof(GValue)); \
            g_value_init(gvobject, typeval); \
            g_value_set_##typename(gvobject, value);

#define STR2GV_MAPENTRY_CREATE(str,value,typeval,typename,gvmap) \
            GString *GLIBEXT_LABEL_(tmpStr,__LINE__) = g_string_new(str); \
            GVALUE_CREATE(typeval, typename, value, gvmap##_value); \
            GVALUE_CREATE(G_TYPE_GSTRING, boxed, \
                          GLIBEXT_LABEL_(tmpStr,__LINE__), gvmap##_string); \
            GMap *GLIBEXT_LABEL_(tmpMap,__LINE__) = \
                g_list_prepend(g_list_prepend(NULL, gvmap##_value), \
                gvmap##_string); \
            GVALUE_CREATE(G_TYPE_BOXED_MAPENTRY_GV, boxed, GLIBEXT_LABEL_(tmpMap,__LINE__), gvmap); \
            g_string_free (GLIBEXT_LABEL_(tmpStr,__LINE__), TRUE); \
            g_list_free(GLIBEXT_LABEL_(tmpMap,__LINE__));

#define STR2GV_MAP_ADD_ENTRY(map,mapentry,outmap) \
            GMapEntry *GLIBEXT_LABEL_(tmpMap,__LINE__) = str2gv_map_has_key(map, g_value_get_gstring(g_list_nth_data(mapentry, 0))); \
            if (GLIBEXT_LABEL_(tmpMap,__LINE__)) { \
                /* If key exists delete option */ \
                map = g_list_remove_link (map, GLIBEXT_LABEL_(tmpMap,__LINE__)); \
                g_list_gv_boxed_free(GLIBEXT_LABEL_(tmpMap,__LINE__)); \
            } \
            /* Add the entry */ \
            GVALUE_CREATE(G_TYPE_BOXED_MAPENTRY_GV, boxed, mapentry, gvptr); \
            g_list_free(mapentry);\
            outmap = g_list_prepend(map, gvptr); \

#endif
