/*
 * Copyright (C) 2002-2003 Joern Thyssen <jthyssen@dk.ibm.com>
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
 * $Id: osr.h,v 1.9 2018/04/23 21:49:59 plm Exp $
 */

#ifndef OSR_H
#define OSR_H

extern void
 raceProbs(const TanBoard anBoard, const unsigned int nGames, float arOutput[NUM_OUTPUTS], float arMu[2]);


#endif                          /* OSR_H */
