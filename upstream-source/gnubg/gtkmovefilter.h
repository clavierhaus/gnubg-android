/*
 * Copyright (C) 2002-2003 Joern Thyssen <jthyssen@dk.ibm.com>
 * Copyright (C) 2008-2011 the AUTHORS
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
 * $Id: gtkmovefilter.h,v 1.7 2013/06/16 02:16:15 mdpetch Exp $
 */

#ifndef GTKMOVEFILTER_H
#define GTKMOVEFILTER_H

extern GtkWidget *MoveFilterWidget(movefilter * pmf, int *pfOK, GCallback pfChanged, gpointer userdata);

extern void


SetMovefilterCommands(const char *sz,
                      movefilter aamfNew[MAX_FILTER_PLIES][MAX_FILTER_PLIES],
                      movefilter aamfOld[MAX_FILTER_PLIES][MAX_FILTER_PLIES]);

extern void
 MoveFilterSetPredefined(GtkWidget * pwMoveFilter, const int i);

#endif                          /* GTKMOVEFILTER_H */
