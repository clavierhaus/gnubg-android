/*
 * Copyright (C) 2001-2003 Gary Wong <gtw@gnu.org>
 * Copyright (C) 2002-2015 the AUTHORS
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
 * $Id: gtkprefs.h,v 1.18 2019/11/03 15:25:18 plm Exp $
 */

#ifndef GTKPREFS_H
#define GTKPREFS_H

#include "gtkboard.h"

extern void BoardPreferences(GtkWidget * pwBoard);
extern void SetBoardPreferences(GtkWidget * pwBoard, char *sz);
extern void Default3dSettings(BoardData * bd);
extern void UpdatePreview(void);
extern void gtk_color_button_get_array(GtkColorButton * button, float array[4]);
extern void gtk_color_button_set_from_array(GtkColorButton * button, float const array[4]);

extern GtkWidget *pwPrevBoard;

#endif
