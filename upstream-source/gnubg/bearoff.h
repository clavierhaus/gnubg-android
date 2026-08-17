/*
 * Copyright (C) 2002-2004 Joern Thyssen <jthyssen@dk.ibm.com>
 * Copyright (C) 2003-2018 the AUTHORS
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
 * $Id: bearoff.h,v 1.36 2019/12/02 12:46:26 plm Exp $
 */

#ifndef BEAROFF_H
#define BEAROFF_H

#include "gnubg-types.h"

#include <glib.h>

typedef enum {
    BEAROFF_INVALID,
    BEAROFF_ONESIDED,
    BEAROFF_TWOSIDED,
    BEAROFF_HYPERGAMMON
} bearofftype;

typedef struct {
    bearofftype bt;             /* type of bearoff database */
    unsigned int nPoints;       /* number of points covered by database */
    unsigned int nChequers;     /* number of chequers for one-sided database */
    /* one sided dbs */
    int fCompressed;            /* is database compressed? */
    int fGammon;                /* gammon probs included */
    int fND;                    /* normal distibution instead of exact dist? */
    int fHeuristic;             /* heuristic database? */
    /* two sided dbs */
    int fCubeful;               /* cubeful equities included */
    FILE *pf;                   /* file pointer */
    char *szFilename;           /* filename */
    GMappedFile *map;
    unsigned char *p;           /* pointer to data in memory */
} bearoffcontext;

enum bearoffoptions {
    BO_NONE = 0,
    BO_IN_MEMORY = 1,
    BO_MUST_BE_ONE_SIDED = 2,
    BO_MUST_BE_TWO_SIDED = 4,
    BO_HEURISTIC = 8
};

extern bearoffcontext *BearoffInit(const char *szFilename, const unsigned int bo, void (*p) (unsigned int));

extern int
 BearoffEval(const bearoffcontext * pbc, const TanBoard anBoard, float arOutput[]);

extern void
 BearoffStatus(const bearoffcontext * pbc, char *sz);

extern int
 BearoffDump(const bearoffcontext * pbc, const TanBoard anBoard, char *sz);

extern int


BearoffDist(const bearoffcontext * pbc, const unsigned int nPosID,
            float arProb[32], float arGammonProb[32],
            float ar[4], unsigned short int ausProb[32], unsigned short int ausGammonProb[32]);

extern int
 BearoffCubeful(const bearoffcontext * pbc, const unsigned int iPos, float ar[4], unsigned short int aus[4]);

extern void BearoffClose(bearoffcontext * pbc);

extern int
 isBearoff(const bearoffcontext * pbc, const TanBoard anBoard);

extern float
 fnd(const float x, const float mu, const float sigma);

extern int
 BearoffHyper(const bearoffcontext * pbc, const unsigned int iPos, float arOutput[], float arEquity[]);

#endif                          /* BEAROFF_H */
