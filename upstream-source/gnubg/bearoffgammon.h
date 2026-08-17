/*
 * Copyright (C) 2000-2002 Joseph Heled <joseph@gnubg.org>
 * Copyright (C) 2006-2008 the AUTHORS
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
 * $Id: bearoffgammon.h,v 1.12 2018/06/16 14:54:30 plm Exp $
 */

#if !defined( BEAROFFGAMMON_H )
#define BEAROFFGAMMON_H

/* pack for space */
struct GammonProbs {
    unsigned int p1:16;         /* 0 - 36^2 */
    unsigned int p2:16;         /* 0 - 36^3 */
    unsigned int p3:24;         /* 0 - 36^4 */
    unsigned int p0:8;          /* 0 - 36   */
};

extern struct GammonProbs *getBearoffGammonProbs(const unsigned int board[6]);

extern long *getRaceBGprobs(const unsigned int board[6]);

#define RBG_NPROBS 5

#endif
