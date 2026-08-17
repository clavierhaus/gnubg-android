/*
 * Copyright (C) 1999-2001 Gary Wong <gtw@gnu.org>
 * Copyright (C) 2000-2015 the AUTHORS
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
 * $Id: drawboard.h,v 1.26 2019/11/29 19:29:46 plm Exp $
 */

#ifndef DRAWBOARD_H
#define DRAWBOARD_H

#include "gnubg-types.h"
#include "external.h"

extern int fClockwise;          /* Player 1 moves clockwise */

extern char *DrawBoard(char *pch, const TanBoard anBoard, int fRoll, char *asz[], char *szMatchID, int nChequers);
/* Fill the buffer pch with a representation of the move anMove, assuming
 * the board looks like anBoard.  pch must have room for 28 characters plus
 * a trailing 0 (consider the move `bar/24* 23/22* 21/20* 19/18*'). */
#define FORMATEDMOVESIZE 29
extern char *FormatMove(char *pch, const TanBoard anBoard, const int anMove[8]);
extern char *FormatMovePlain(char *pch, const TanBoard anBoard, const int anMove[8]);
extern int ParseMove(char *pch, int an[8]);
extern void CanonicalMoveOrder(int an[]);
/* Fill the buffer pch with a FIBS "boardstyle 3" description of the game. */
extern char *FIBSBoard(char *pch, TanBoard anBoard, int fRoll,
                       const char *szPlayer, const char *szOpp, int nMatchTo,
                       int nScore, int nOpponent, int nDice0, int nDice1,
                       int nCube, int fCubeOwner, int fDoubled, int fTurn, int fCrawford, int nChequers,
					   int fPostCrawford);

/* Process a board info structure from external interface */
extern int ProcessFIBSBoardInfo(FIBSBoardInfo * brdInfo, ProcessedFIBSBoard * procBrd);

#endif
