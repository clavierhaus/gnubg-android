/*
 * Copyright (C) 2002-2003 Joern Thyssen <jthyssen@dk.ibm.com>
 * Copyright (C) 2009 the AUTHORS
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
 * $Id: gtkcube.h,v 1.12 2020/01/25 21:40:13 plm Exp $
 */

#ifndef GTKCUBE_H
#define GTKCUBE_H

extern GtkWidget *CreateCubeAnalysis(moverecord * pmr, const matchstate * pms, int did_double, int did_take, int hist);

extern int cubeTempMapAtMoney;   // extern: for the temperature map when the MoneyEval button is toggled on
extern int cubeTempMapJacoby;    // extern: also for the temperature map when the MoneyEval button is toggled on 

#endif
