/*
 * Copyright (C) 2002-2004 Joern Thyssen <jthyssen@dk.ibm.com>
 * Copyright (C) 2004-2023 the AUTHORS
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
 *
 * Inspired from osr.cc from fibs2html <fibs2html.sourceforge.net>
 *
 * $Id: osr.c,v 1.45 2022/10/22 18:27:59 plm Exp $
 */

/*lint -e514 */

#include "config.h"

#include <glib.h>
#include <string.h>

#include "eval.h"
#include "positionid.h"
#include "SFMT.h"
#include "osr.h"

#define MAX_PROBS        32
#define MAX_GAMMON_PROBS 15

static sfmt_t sfmt;

static void
OSRQuasiRandomDice(const unsigned int iTurn, const unsigned int iGame, const unsigned int cGames, unsigned int anDice[2])
{
    if (!iTurn && !(cGames % 36)) {
        anDice[0] = (iGame % 6) + 1;
        anDice[1] = ((iGame / 6) % 6) + 1;
    } else if (iTurn == 1 && !(cGames % 1296)) {
        anDice[0] = ((iGame / 36) % 6) + 1;
        anDice[1] = ((iGame / 216) % 6) + 1;
    } else {
        anDice[0] = (unsigned int) (sfmt_genrand_uint32(&sfmt) % 6) + 1;
        anDice[1] = (unsigned int) (sfmt_genrand_uint32(&sfmt) % 6) + 1;
    }
}

/* Fill aaProb with one sided bearoff probabilities for position with */
/* bearoff id n.                                                      */

static void
getBearoffProbs(const unsigned int n, unsigned short int aaProb[32])
{
    if (BearoffDist(pbc1, n, NULL, NULL, NULL, aaProb, NULL))
        g_assert_not_reached();
}


static int
isCrossOver(const unsigned int from, const unsigned int to)
{
    return (from / 6) != (to / 6);
}


/*
 * Find (and move) best move in one side rollout.
 *
 * Input
 *    anBoard: the board (reversed compared to normal convention)
 *    anDice: the roll (dice 1 <> dice 2)
 *    pnOut: current number of chequers outside home quadrant
 *
 * Output:
 *    anBoard: the updated board after the move
 *    pnOut: the number of chequers outside home quadrant after move
 *
 */

#if !defined(G_DISABLE_ASSERT)

static unsigned int
chequersout(const unsigned int anBoard[25])
{
    unsigned int i, n = 0;

    for (i = 6; i < 25; i++)
        n += anBoard[i];

    return n;
}

#endif

/*! \brief checks that we haven't moved too many checkers of any point on the board
 * but board need not contain all 15 checkers */
#if !defined(G_DISABLE_ASSERT)

static int
checkboard(const unsigned int anBoard[25])
{
    unsigned int i;

    for (i = 0; i < 25; i++)
        if (anBoard[i] > 15)
            return 0;
    return 1;
}

#endif

static void
FindBestMoveOSR2(unsigned int anBoard[25], const unsigned int anDice[2], unsigned int *pnOut)
{
    unsigned int ifar, inear, iboth;
    unsigned int iused = 0;
    unsigned int i, j, lc;
    unsigned int found;

    ifar = 5 + anDice[0];
    inear = 5 + anDice[1];

    if (anBoard[ifar] && anBoard[inear]) {

        /* two chequers move exactly into the home quadrant */

        /* move chequers */

        --anBoard[ifar];
        --anBoard[inear];
        g_assert(checkboard(anBoard));
        anBoard[5] += 2;

        g_assert(*pnOut >= 2);
        *pnOut -= 2;
        g_assert(*pnOut == chequersout(anBoard));

        return;
    }

    iboth = 5 + (anDice[0] + anDice[1]);

    if (anBoard[iboth]) {

        /* one chequer move exactly into the home quadrant */

        /* move chequer */

        --anBoard[iboth];
        ++anBoard[5];
        g_assert(checkboard(anBoard));

        g_assert(*pnOut > 0);
        --*pnOut;
        g_assert(*pnOut == chequersout(anBoard));

        return;

    }


    /* loop through dice */

    for (i = 0; i < 2 && *pnOut; ++i) {

        /* check for exact cross over */

        if (anBoard[5 + anDice[i]]) {

            /* move chequer */

            --anBoard[5 + anDice[i]];
            ++anBoard[5];
            g_assert(checkboard(anBoard));

            g_assert(*pnOut > 0);
            --*pnOut;
            g_assert(*pnOut == chequersout(anBoard));

            ++iused;

            /* next die */

            continue;

        }

        /* find chequer furthest away */

        lc = 24;
        while (lc > 5 && !anBoard[lc])
            --lc;

        /* try to make cross over from the back */

        found = FALSE;
        for (j = lc; j - anDice[i] > 5; --j) {

            if (anBoard[j] && isCrossOver(j, j - anDice[i])) {

                /* move chequer */

                --anBoard[j];
                ++anBoard[j - anDice[i]];
                g_assert(checkboard(anBoard));
                ++iused;
                found = TRUE;
                /* FIXME: increment lc if needed */

                break;

            }

        }


        if (!found) {

            /* no move with cross-over was found */

            /* move chequer from the rear */

            for (j = lc; j > 5; --j) {

                if (anBoard[j]) {

                    --anBoard[j];
                    ++anBoard[j - anDice[i]];
                    g_assert(checkboard(anBoard));
                    ++iused;

                    if (j - anDice[i] < 6) {    /* we've moved inside home quadrant */
                        g_assert(*pnOut > 0);
                        --*pnOut;
                    }

                    g_assert(*pnOut == chequersout(anBoard));

                    break;

                }

            }


        }

    }

    if (!*pnOut && iused < 2) {

        /* die 2 still left, and all chequers inside home quadrant */

        if (anBoard[anDice[1] - 1]) {
            /* bear-off */
            --anBoard[anDice[1] - 1];
            g_assert(checkboard(anBoard));
            return;
        }

        /* try filling rearest empty space */

        for (i = 0; i < 6 - anDice[1]; i++) {
            j = 5 - i;
            if (anBoard[j] && !anBoard[j - anDice[1]]) {
                /* empty space found */

                --anBoard[j];
                ++anBoard[j - anDice[1]];

                g_assert(checkboard(anBoard));
                return;

            }
        }

        /* move chequer from the rear */
        for (i = 0; i < 6; i++) {
            j = 5 - i;

            if (anBoard[j]) {

                /* move chequer from the rear */

                --anBoard[j];

                if (j >= anDice[1])
                    /* add chequer to point */
                    ++anBoard[j - anDice[1]];
                /* else */
                /*   bearoff */

                g_assert(checkboard(anBoard));
                return;

            }

        }

    }

    g_assert(iused == 2);
    return;

}

/*
 * Find (and move) best move in one side rollout.
 *
 * Input
 *    anBoard: the board 
 *    anDice: the roll (dice 1 = dice 2)
 *    pnOut: current number of chequers outside home quadrant
 *
 * Output:
 *    anBoard: the updated board after the move
 *    pnOut: the number of chequers outside home quadrant after move
 *
 */

static void
FindBestMoveOSR4(unsigned int anBoard[25], const unsigned int nDice, unsigned int *pnOut)
{
    unsigned int nd = 4;
    unsigned int i, n = 0;

    /* check for exact bear-ins */

    while (nd > 0 && *pnOut > 0 && anBoard[5 + nDice]) {
        --anBoard[5 + nDice];
        ++anBoard[5];
        g_assert(checkboard(anBoard));

        --nd;
        g_assert(*pnOut > 0);
        --*pnOut;
        g_assert(*pnOut == chequersout(anBoard));

    }

#if 0
    /* this is broken or very very slow when the player has chequers in the
     * 3rd or 4th quadrant */
    if (*pnOut > 0) {
        /* check for 4, 3, or 2 chequers move exactly into home quadrant */

        for (n = nd; n > 1; --n) {

            i = 5 + n * nDice;

            if (i < 25 && anBoard[i]) {

                --anBoard[i];
                ++anBoard[5];
                g_assert(checkboard(anBoard));

                nd -= n;
                g_assert(*pnOut > 0);
                --*pnOut;
                g_assert(*pnOut == chequersout(anBoard));

                n = nd;         /* restart loop */
            }
        }
    }
#endif

    if (*pnOut > 0 && nd > 0) {
        unsigned int first;
        unsigned int lc;

        first = TRUE;

        /* find rearest chequer */

        lc = 24;
        while (lc > 5 && !anBoard[lc])
            --lc;

        /* try to make cross over from the back */

        for (i = lc; i > 5; --i) {

            if (anBoard[i]) {

                if (isCrossOver(i, i - nDice) && (first || (i - nDice) > 5)) {

                    /* move chequers */

                    while (anBoard[i] && nd && *pnOut) {
                        --anBoard[i];
                        ++anBoard[i - nDice];
                        g_assert(checkboard(anBoard));

                        if (i - nDice < 6) {    /* we move into homeland */
                            g_assert(*pnOut > 0);
                            --*pnOut;
                        }


                        g_assert(*pnOut == chequersout(anBoard));

                        --nd;
                    }

                    if (!*pnOut || !nd)
                        break;

                    /* did we move all chequers from that point */

                    first = !anBoard[i];
                }
            }
        }

        /* move chequers from the rear */

        while (*pnOut && nd) {
            for (i = lc; i > 5; --i) {
                if (anBoard[i]) {
                    while (anBoard[i] && nd && *pnOut) {
                        --anBoard[i];
                        ++anBoard[i - nDice];
                        g_assert(checkboard(anBoard));

                        if (i - nDice < 6) {
                            g_assert(*pnOut > 0);
                            --*pnOut;
                        }

                        g_assert(*pnOut == chequersout(anBoard));

                        --nd;
                    }

                    if (!n || !*pnOut)
                        break;
                }
            }
        }
    }

    if (!*pnOut) {              /* all chequers inside home quadrant */
        while (nd) {
            unsigned int any;

            if (anBoard[nDice - 1]) {
                /* perfect bear-off */
                --anBoard[nDice - 1];
                --nd;
                g_assert(checkboard(anBoard));
                continue;
            }

            if (nd >= 2 && nDice <= 3 && anBoard[2 * nDice - 1] > 0) {
                /* bear  double 1s, 2s, and 3s off, e.g., 4/2/0 */
                --anBoard[2 * nDice - 1];
                nd -= 2;
                g_assert(checkboard(anBoard));
                continue;
            }

            if (nd >= 3 && nDice <= 2 && anBoard[3 * nDice - 1] > 0) {
                /* bear double 1s off from 3 point (3/2/1/0) or
                 * double 2s off from 6 point (6/4/2/0) */
                --anBoard[3 * nDice - 1];
                nd -= 3;
                g_assert(checkboard(anBoard));
                continue;
            }

            if (nd >= 4 && nDice <= 1 && anBoard[4 * nDice - 1] > 0) {
                /* hmmm, this should not be possible... */
                /* bear off double 1s: 4/3/2/1/0 */
                --anBoard[4 * nDice - 1];
                g_assert(checkboard(anBoard));
                nd -= 4;
            }

            any = FALSE;

            /* move chequers from rear */

            /* FIXME: fill gaps? */

            for (i = 0; nd && i < 6; i++) {
                unsigned int j = 5 - i;

                while (anBoard[j] && nd) {

                    any = TRUE;

                    --anBoard[j];
                    --nd;

                    if (j >= nDice)
                        ++anBoard[j - nDice];
                    g_assert(checkboard(anBoard));

                }

            }

            if (!any)
                /* no more chequers left */
                nd = 0;
        }
    }
}


/*
 * Find (and move) best move in one sided rollout.
 *
 * Input
 *    anBoard: the board 
 *    anDice: the roll (dice 1 is assumed to be lower than dice 2)
 *    pnOut: current number of chequers outside home quadrant
 *
 * Output:
 *    anBoard: the updated board after the move
 *    pnOut: the number of chequers outside home quadrant after move
 *
 */

static void
FindBestMoveOSR(unsigned int anBoard[25], const unsigned int anDice[2], unsigned int *pnOut)
{
    if (anDice[0] != anDice[1])
        FindBestMoveOSR2(anBoard, anDice, pnOut);
    else
        FindBestMoveOSR4(anBoard, anDice[0], pnOut);
}

/*
 * osr: one sided rollout.
 *
 * Input:
 *   anBoard: the board (reversed compared to normal convention)
 *   iGame: game#
 *   nGames: # of games.
 *   nOut: current number of chequers outside home quadrant.
 *
 * Returns: number of rolls used to get all chequers inside home
 *          quadrant.
 */

static unsigned int
osr(unsigned int anBoard[25], const unsigned int iGame, const unsigned int nGames, unsigned int nOut)
{
    unsigned int iTurn = 0;
    unsigned int anDice[2];

    /* loop until all chequers are in home quadrant */

    while (nOut) {
        /* roll dice */
        OSRQuasiRandomDice(iTurn, iGame, nGames, anDice);

        if (anDice[0] < anDice[1])
            swap_us(anDice, anDice + 1);

        /* find and move best move */
        FindBestMoveOSR(anBoard, anDice, &nOut);

        iTurn++;
    }

    return iTurn;
}


/*
 * RollOSR: perform onesided rollout
 *
 * Input:
 *   nGames: number of simulations
 *   anBoard: the board 
 *   nOut: number of chequers outside home quadrant
 *
 * Output:
 *   arProbs[ nMaxProbs ]: probabilities
 *   arGammonProbs[ nMaxGammonProbs ]: gammon probabilities
 *
 */

static void
rollOSR(const unsigned int nGames, const unsigned int anBoard[25], const unsigned int nOut,
        float arProbs[], const unsigned int nMaxProbs, float arGammonProbs[], const unsigned int nMaxGammonProbs)
{

    unsigned int an[25];
    unsigned short int anProb[32];
    unsigned int i;
    unsigned int iGame;

    int *anCounts = (int *) g_alloca(nMaxGammonProbs * sizeof(int));

    memset(anCounts, 0, sizeof(int) * nMaxGammonProbs);

    for (i = 0; i < nMaxProbs; ++i)
        arProbs[i] = 0.0f;

    /* perform rollouts */

    for (iGame = 0; iGame < nGames; ++iGame) {
        unsigned int n, m;

        memcpy(an, anBoard, sizeof(an));

        /* do actual rollout */

        n = osr(an, iGame, nGames, nOut);

        /* number of chequers in home quadrant */

        m = 0;
        for (i = 0; i < 6; ++i)
            m += an[i];

        /* update counts */

        ++anCounts[MIN(m == 15 ? n + 1 : n, nMaxGammonProbs - 1)];

        /* get prob. from bearoff1 */

        getBearoffProbs(PositionBearoff(an, pbc1->nPoints, pbc1->nChequers), anProb);

        for (i = 0; i < 32; ++i)
            arProbs[MIN(n + i, nMaxProbs - 1)] += anProb[i] / 65535.0f;

    }

    /* scale resulting probabilities */

    for (i = 0; i < (unsigned int) nMaxProbs; ++i) {
        arProbs[i] /= (float)nGames;
        /* printf ( "arProbs[%d]=%f\n", i, arProbs[ i ] ); */
    }

    /* calculate gammon probs. 
     * (prob. of getting inside home quadrant in i rolls */

    for (i = 0; i < nMaxGammonProbs; ++i) {
        arGammonProbs[i] = (float)anCounts[i] / (float)nGames;
        /* printf ( "arGammonProbs[%d]=%f\n", i, arGammonProbs[ i ] ); */
    }

}



/*
 * OSP: one sided probabilities
 *
 * Input:
 *   anBoard: one side of the board
 *   nGames: number of simulations
 *   
 * Output:
 *   an: ???
 *   arProb: array of probabilities (ar[ i ] is the prob. that player 
 *           bears off in i moves)
 *   arGammonProb: gammon probabilities
 *
 */

static unsigned int
osp(const unsigned int anBoard[25], const unsigned int nGames,
    unsigned int an[25], float arProbs[MAX_PROBS], float arGammonProbs[MAX_GAMMON_PROBS])
{

    int i;
    unsigned int nTotal, nOut;

    /* copy board into an, and find total number of chequers left,
     * and number of chequers outside home */

    nTotal = nOut = 0;

    memcpy(an, anBoard, 25 * sizeof(int));

    for (i = 0; i < 25; ++i) {
        /* total number of chequers left */
        nTotal += anBoard[i];
        if (i > 5)
            nOut += anBoard[i];
    }


    if (nOut > 0)
        /* chequers outside home: do one sided rollout */
        rollOSR(nGames, an, nOut, arProbs, MAX_PROBS, arGammonProbs, MAX_GAMMON_PROBS);
    else {
        /* chequers inside home: use BEAROFF2 */

        unsigned short int anProb[32];

        /* no gammon possible */
        for (i = 0; i < MAX_GAMMON_PROBS; ++i)
            arGammonProbs[i] = 0.0f;

        if (nTotal == 15)
            arGammonProbs[1] = 1.0f;
        else
            arGammonProbs[0] = 1.0f;

        /* get probs from BEAROFF2 */

        for (i = 0; i < MAX_PROBS; ++i)
            arProbs[i] = 0.0f;

        getBearoffProbs(PositionBearoff(anBoard, pbc1->nPoints, pbc1->nChequers), anProb);

        for (i = 0; i < 32; ++i) {
            int n = MIN(i, MAX_PROBS - 1);
            arProbs[n] += anProb[i] / 65535.0f;
            /* printf ( "arProbs[%d]=%f\n", n, arProbs[n] ); */
        }

    }

    return nTotal;

}


static float
bgProb(const unsigned int anBoard[25],
       const int fOnRoll, const unsigned int nTotal, const float arProbs[], const unsigned int nMaxProbs)
{

    unsigned int nTotPipsHome = 0;
    unsigned int i;
    float r;

    /* total pips before out of opponent's home quadrant */

    for (i = 18; i < 25; ++i)
        nTotPipsHome += anBoard[i] * (i - 17);

    r = 0.0f;

    if (nTotPipsHome) {

        /* ( nTotal + 3 ) / 4 - 1: number of rolls before opponent is off. */
        /* (nTotPipsHome + 2) / 3: numbers of rolls before I'm out of
         * opponent's home quadrant (with consecutive 2-1's) */

        if ((nTotal + 3) / 4 - 1 <= (nTotPipsHome + 2) / 3) {
            /* backgammon is possible */

            unsigned short int anProb[32];

            /* get "bear-off" prob (for getting out of opp.'s home quadr.) */

            /* FIXME: this ignores chequers on the bar */

            getBearoffProbs(PositionBearoff(anBoard + 18, pbc1->nPoints, pbc1->nChequers), anProb);

            for (i = 0; i < nMaxProbs; ++i) {

                if (arProbs[i] > 0.0f) {

                    float s = 0.0f;
                    unsigned int j;

                    for (j = i + !fOnRoll; j < 32; ++j)
                        s += anProb[j] / 65535.0f;

                    r += s * arProbs[i];

                }

            }

        }

    }

    return r;

}


/*
 * Calculate race probabilities using one sided rollouts.
 *
 * Input:
 *   anBoard: the current board 
 *            (assumed to be a race position without contact)
 *   nGames:  the number of simulations to perform
 *
 * Output:
 *   arOutput: probabilities.
 *
 */

extern void
raceProbs(const TanBoard anBoard, const unsigned int nGames, float arOutput[NUM_OUTPUTS], float arMu[2])
{

    TanBoard an;
    float aarProbs[2][MAX_PROBS];
    float aarGammonProbs[2][MAX_PROBS];
    float arG[2] = { 0.0f, 0.0f }, arBG[2] = { 0.0f, 0.0f };

    unsigned int anTotal[2];

    int i, j, k;

    float w, s;

    /* Seed set to ensure that OSR are reproducible */

    sfmt_init_gen_rand(&sfmt, 0);

    for (i = 0; i < NUM_OUTPUTS; ++i)
        arOutput[i] = 0.0f;

    for (i = 0; i < 2; ++i)
        anTotal[i] = osp(anBoard[i], nGames, an[i], aarProbs[i], aarGammonProbs[i]);

    /* calculate OUTPUT_WIN */

    w = 0;

    for (i = 0; i < MAX_PROBS; ++i) {

        /* calculate the prob. of the opponent using more than i rolls
         * to bear off */

        s = 0.0f;
        for (j = i; j < MAX_PROBS; ++j)
            s += aarProbs[0][j];

        /* winning chance is: prob. of me bearing off in i rolls times
         * prob. the opponent doesn't bear off in i rolls */

        w += aarProbs[1][i] * s;

    }

    arOutput[OUTPUT_WIN] = MIN(w, 1.0f);

    /* calculate gammon and backgammon probs */

    for (i = 0; i < 2; ++i) {

        if (anTotal[!i] == 15) {

            /* gammon and backgammon possible */

            for (j = 0; j < MAX_GAMMON_PROBS; ++j) {

                /* chance of opponent having borne all chequers of
                 * within j rolls */

                s = 0.0f;
                for (k = 0; k < j + i; ++k)
                    s += aarProbs[i][k];

                /* gammon chance */

                arG[i] += aarGammonProbs[!i][j] * s;

            }

            if (arG[i] > 0.0f)
                /* calculate backgammon probs */
                arBG[i] = bgProb(an[!i], i, anTotal[i], aarProbs[i], MAX_PROBS);

        }

    }

    arOutput[OUTPUT_WINGAMMON] = MIN(arG[1], 1.0f);
    arOutput[OUTPUT_LOSEGAMMON] = MIN(arG[0], 1.0f);
    arOutput[OUTPUT_WINBACKGAMMON] = MIN(arBG[1], 1.0f);
    arOutput[OUTPUT_LOSEBACKGAMMON] = MIN(arBG[0], 1.0f);

    /* calculate average number of rolls to bear off */

    if (arMu) {


        for (i = 0; i < 2; ++i) {
            arMu[i] = 0.0f;
            for (j = 0; j < MAX_PROBS; ++j)
                arMu[i] +=  (float)j * aarProbs[i][j];

        }

    }

}
