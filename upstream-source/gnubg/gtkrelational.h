/*
 * Copyright (C) 2006-2008 Christian Anthon <anthon@kiku.dk>
 * Copyright (C) 2007-2024 the AUTHORS
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
 */

#include "relational.h"

#include <gtk/gtk.h>

#ifndef GTKRELATIONAL_H
#define GTKRELATIONAL_H
extern void GtkRelationalAddMatch(gpointer p, guint n, GtkWidget * pw);
extern void GtkShowRelational(gpointer p, guint n, GtkWidget * pw);
extern GtkWidget *RelationalOptions(void);
extern void RelationalOptionsShown(void);
extern void RelationalSaveOptions(void);
extern void GtkShowQuery(RowSet * pRow);

extern void ComputeHistory(int usePlayerName); /* for history plot*/
#endif
