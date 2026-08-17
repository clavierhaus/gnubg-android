/*
 * Copyright (C) 2005-2009 Jon Kinsey <jonkinsey@gmail.com>
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
 * $Id: gtkmovelistctrl.h,v 1.8 2019/12/08 20:55:55 plm Exp $
 */

#ifndef GTKMOVELISTCTRL_H
#define GTKMOVELISTCTRL_H


/* Some boilerplate GObject type check and type cast macros.
 *  'klass' is used here instead of 'class', because 'class'
 *  is a c++ keyword */

#define CUSTOM_TYPE_CELL_RENDERER_MOVELIST             (custom_cell_renderer_movelist_get_type())
#define CUSTOM_CELL_RENDERER_MOVELIST(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),  CUSTOM_TYPE_CELL_RENDERER_MOVELIST, CustomCellRendererMovelist))
#define CUSTOM_CELL_RENDERER_MOVELIST_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass),  CUSTOM_TYPE_CELL_RENDERER_MOVELIST, CustomCellRendererMovelistClass))
#define CUSTOM_IS_CELL_MOVELIST_MOVELIST(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CUSTOM_TYPE_CELL_RENDERER_MOVELIST))
#define CUSTOM_IS_CELL_MOVELIST_MOVELIST_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass),  CUSTOM_TYPE_CELL_RENDERER_MOVELIST))
#define CUSTOM_CELL_RENDERER_MOVELIST_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj),  CUSTOM_TYPE_CELL_RENDERER_MOVELIST, CustomCellRendererMovelistClass))

/* CustomCellRendererMovelist: Our custom cell renderer
 *   structure. Extend according to need */

typedef struct {
    GtkCellRenderer parent;
    move *pml;
    unsigned int rank;
} CustomCellRendererMovelist;


typedef struct {
    GtkCellRendererClass parent_class;
} CustomCellRendererMovelistClass;

extern GtkCellRenderer *custom_cell_renderer_movelist_new(void);

extern GtkStyle *psHighlight;
extern float rBest;

extern void custom_cell_renderer_invalidate_size(void);
#endif                          /* _custom_cell_renderer_movelistbar_included_ */
