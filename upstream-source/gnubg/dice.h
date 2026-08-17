/*
 * Copyright (C) 1999-2002 Gary Wong <gtw@gnu.org>
 * Copyright (C) 1999-2016 the AUTHORS
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
 * $Id: dice.h,v 1.37 2019/12/09 19:59:49 plm Exp $
 */

#ifndef DICE_H
#define DICE_H

#include <stdio.h>

typedef enum {
    RNG_BBS, RNG_ISAAC, RNG_MD5, RNG_MERSENNE,
    RNG_MANUAL, RNG_RANDOM_DOT_ORG, RNG_FILE,
    NUM_RNGS
} rng;

typedef struct rngcontext rngcontext;

extern const char *aszRNG[NUM_RNGS];
extern const char *aszRNGTip[NUM_RNGS];
extern char szDiceFilename[];
extern rng rngCurrent;
extern rngcontext *rngctxCurrent;

rngcontext *CopyRNGContext(rngcontext * rngctx);

extern void free_rngctx(rngcontext * rngctx);
extern void *InitRNG(unsigned long *pnSeed, int *pfInitFrom, const int fSet, const rng rngx);
extern void CloseRNG(const rng rngx, rngcontext * rngctx);
extern void DestroyRNG(const rng rngx, rngcontext ** rngctx);
extern void PrintRNGSeed(const rng rngx, rngcontext * rngctx);
extern void PrintRNGCounter(const rng rngx, rngcontext * rngctx);
extern void InitRNGSeed(unsigned int n, const rng rngx, rngcontext * rngctx);
extern int RNGSystemSeed(const rng rngx, void *p, unsigned long *pnSeed);

extern int RollDice(unsigned int anDice[2], rng * prng, rngcontext * rngctx);

#if defined(HAVE_LIBGMP)
extern int InitRNGSeedLong(char *sz, rng rng, rngcontext * rngctx);
extern int InitRNGBBSModulus(const char *sz, rngcontext * rngctx);
extern int InitRNGBBSFactors(char *sz0, char *sz1, rngcontext * rngctx);
#endif

extern FILE *OpenDiceFile(rngcontext * rngctx, const char *sz);

extern char *GetDiceFileName(rngcontext * rngctx);

#endif
