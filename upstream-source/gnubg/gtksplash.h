/*
 * Copyright (C) 2002 Joern Thyssen <jth@gnubg.org>
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
 * $Id: gtksplash.h,v 1.7 2013/06/16 02:16:16 mdpetch Exp $
 */

#ifndef GTKSPLASH_H
#define GTKSPLASH_H

extern GtkWidget *CreateSplash(void);

extern void
 DestroySplash(GtkWidget * pwSplash);

extern void
 PushSplash(GtkWidget * pwSplash, const gchar * szText0, const gchar * szText1);

#endif                          /* GTKSPLASH_H */
