/*
 * Copyright (C) 2003 Joern Thyssen <jth@gnubg.org>
 * Copyright (C) 2007-2008 the AUTHORS
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
 * $Id: boardpos.h,v 1.10 2018/04/21 16:14:51 plm Exp $
 */

#ifndef BOARDPOS_H
#define BOARDPOS_H

#define POINT_UNUSED0 28        /* the top unused bearoff tray */
#define POINT_UNUSED1 29        /* the bottom unused bearoff tray */
#define POINT_DICE 30
#define POINT_CUBE 31
#define POINT_RIGHT 32
#define POINT_LEFT 33
#define POINT_RESIGN 34

extern int positions[2][30][3];

extern void
 ChequerPosition(const int clockwise, const int point, const int chequer, int *px, int *py);

extern void
 PointArea(const int fClockwise, const int nSize, const int n, int *px, int *py, int *pcx, int *pcy);


extern void


CubePosition(const int crawford_game, const int cube_use,
             const int doubled, const int cube_owner, int fClockwise, int *px, int *py, int *porient);

extern void ArrowPosition(const int clockwise, int turn, const int nSize, int *px, int *py);


extern void
 ResignPosition(const int resigned, int *px, int *py, int *porient);


#endif
