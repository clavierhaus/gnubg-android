/*
 * Copyright (C) 2002 Joern Thyssen <jthyssen@dk.ibm.com>
 * Copyright (C) 2008-2013 the AUTHORS
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
 * $Id: matchid.h,v 1.18 2021/07/14 19:21:45 plm Exp $
 */

#ifndef MATCHID_H
#define MATCHID_H

#define L_MATCHID 12

typedef struct {
    int anDice[2];
    int fTurn;
    int fResigned;
    int fDoubled;
    gamestate gs;
} posinfo;

extern int LogCube(int n);

extern char *MatchID(const unsigned int anDice[2],
                     const int fTurn,
                     const int fResigned,
                     const int fDoubled,
                     const int fMove,
                     const int fCubeOwner,
                     const int fCrawford, const int nMatchTo, const int anScore[2], const int nCube,
                     const int fJacoby, const gamestate gs);

extern int MatchFromID(unsigned int anDice[2],
            int *pfTurn,
            int *pfResigned,
            int *pfDoubled, int *pfMove, int *pfCubeOwner, int *pfCrawford, int *pnMatchTo, int anScore[2], int *pnCube,
            int *pfJacoby, gamestate * pgs, const char *szMatchID);

extern char *MatchIDFromMatchState(const matchstate * pms);

#endif
