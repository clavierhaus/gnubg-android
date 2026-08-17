/*
 * Copyright (C) 1999-2002 Joern Thyssen <jth@gnubg.org>
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
 * $Id: mec.h,v 1.4 2013/06/16 02:16:19 mdpetch Exp $
 */


#ifndef MEC_H
#define MEC_H

#include "matchequity.h"

extern void
 mec(const float rGammonRate, const float rWinRate,
     /* const */ float aarMetPC[2][MAXSCORE],
     float aarMet[MAXSCORE][MAXSCORE]);

extern void


mec_pc(const float rGammonRate,
       const float rFreeDrop2Away, const float rFreeDrop4Away, const float rWinRate, float arMetPC[MAXSCORE]);

#endif                          /* MEC_H */
