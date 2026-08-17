/*
 * Copyright (C) 2003 Joern Thyssen <jth@gnubg.org>
 * Copyright (C) 2003-2011 the AUTHORS
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
 * $Id: gtktoolbar.h,v 1.16 2021/06/30 21:11:44 plm Exp $
 */

#ifndef GTKTOOLBAR_H
#define GTKTOOLBAR_H

#include "gtkboard.h"

#if defined(USE_GTKITEMFACTORY)
extern GtkItemFactory *pif;
#endif

typedef enum {
    C_NONE,
    C_ROLLDOUBLE,
    C_TAKEDROP,
    C_AGREEDECLINE,
    C_PLAY
} toolbarcontrol;

extern GtkWidget *ToolbarNew(void);

extern toolbarcontrol
ToolbarUpdate(GtkWidget * pwToolbar,
              const matchstate * pms, const DiceShown diceShown, const int fComputerTurn, const int fPlaying);

extern int
 ToolbarIsEditing(GtkWidget * pwToolbar);

extern void
 ToolbarActivateEdit(GtkWidget * pwToolbar);

extern void
 ToolbarSetPlaying(GtkWidget * pwToolbar, const int f);

extern void
 ToolbarSetClockwise(GtkWidget * pwToolbar, const int f);

extern GtkWidget *image_from_xpm_d(char **xpm, GtkWidget * pw);

extern void click_edit(void);
extern void click_swapdirection(void);

#endif                          /* GTKTOOLBAR_H */
