/*
 * Copyright (C) 1999-2003 Gary Wong <gtw@gnu.org>
 * Copyright (C) 1999-2023 the AUTHORS
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
 */

#include "config.h"
#include "backgammon.h"
#include "randomorg.h"

#include <fcntl.h>

#if defined(HAVE_LIBGMP)
/*
 * The condition is not entirely accurate, gmp.h is still in
 * the gmp subdirectory in Solaris 11.1 but no longer in 11.4.
 * Make it work in the up to date case.
 */
#ifdef __SunOS_5_10
#include <gmp/gmp.h>
#else
#include <gmp.h>
#endif
#endif

#include <glib.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdlib.h>
#include <string.h>

#ifdef WIN32
#include <io.h>
#endif

#include "dice.h"
#include "md5.h"
#include "SFMT.h"
#include "isaac.h"
#include <glib/gstdio.h>
#include "glib-ext.h"

#if USE_GTK
#include "gtkgame.h"
#endif

const char *aszRNG[NUM_RNGS] = {
    N_("Blum, Blum and Shub"),
    "ISAAC",
    "MD5",
    N_("Mersenne Twister"),
    N_("manual dice"),
    "www.random.org",
    N_("read from file")
};

const char *aszRNGTip[NUM_RNGS] = {
    N_("Blum, Blum and Shub's verifiably strong generator"),
    N_("Bob Jenkins' Indirection, Shift, Accumulate, Add and Count " "cryptographic generator"),
    N_("A generator based on the Message Digest 5 algorithm"),
    N_("Makoto Matsumoto and Mutsuo Saito's generator"),
    N_("Enter each dice roll by hand"),
    N_("The online non-deterministic generator from random.org"),
    N_("Dice loaded from a file"),
};

rng rngCurrent = RNG_MERSENNE;
rngcontext *rngctxCurrent = NULL;

struct rngcontext {

    /* RNG_FILE */
    FILE *fDice;
    char *szDiceFilename;

    /* RNG_ISAAC */
    randctx rc;

    /* RNG_MD5 */
    md5_uint32 nMD5;            /* the current MD5 seed */

    /* RNG_MERSENNE */
    sfmt_t sfmt;

    /* RNG_BBS */

#if defined(HAVE_LIBGMP)
    mpz_t zModulus, zSeed, zZero, zOne;
    int fZInit;
#endif                          /* HAVE_LIBGMP */


    /* common */
    unsigned long c;            /* counter */
#if defined(HAVE_LIBGMP)
    mpz_t nz;                   /* seed */
#endif
    unsigned int n;             /* seed */

};


static unsigned int
 ReadDiceFile(rngcontext * rngctx);


#if defined(HAVE_LIBGMP)

static void
InitRNGBBS(rngcontext * rngctx)
{

    if (!rngctx->fZInit) {
        mpz_init(rngctx->zModulus);
        mpz_init(rngctx->zSeed);
        mpz_init_set_ui(rngctx->zZero, 0);
        mpz_init_set_ui(rngctx->zOne, 1);
        rngctx->fZInit = TRUE;
    }
}

extern int
InitRNGBBSModulus(const char *sz, rngcontext * rngctx)
{

    if (!sz)
        return -1;

    InitRNGBBS(rngctx);

    if (mpz_set_str(rngctx->zModulus, sz, 10) || mpz_sgn(rngctx->zModulus) < 1)
        return -1;

    return 0;
}

static int
BBSGood(mpz_t x)
{

    static mpz_t z19;
    static int f19;

    if (!f19) {
        mpz_init_set_ui(z19, 19);
        f19 = TRUE;
    }

    return ((mpz_get_ui(x) & 3) == 3) && mpz_cmp(x, z19) >= 0 && mpz_probab_prime_p(x, 10);
}

static int
BBSFindGood(mpz_t x)
{

    do
        mpz_add_ui(x, x, 1);
    while (!BBSGood(x));

    return 0;
}

extern int
InitRNGBBSFactors(char *sz0, char *sz1, rngcontext * rngctx)
{

    mpz_t p, q;
    char *pch;

    if (!sz0 || !sz1)
        return -1;

    if (mpz_init_set_str(p, sz0, 10) || mpz_sgn(p) < 1) {
        mpz_clear(p);
        return -1;
    }

    if (mpz_init_set_str(q, sz1, 10) || mpz_sgn(p) < 1) {
        mpz_clear(p);
        mpz_clear(q);
        return -1;
    }

    if (!BBSGood(p)) {
        BBSFindGood(p);

        pch = mpz_get_str(NULL, 10, p);
        g_print(_("%s is an invalid Blum factor, using %s instead."), sz0, pch);
        g_print("\n");
        free(pch);
    }

    if (!BBSGood(q) || !mpz_cmp(p, q)) {
        BBSFindGood(q);

        if (!mpz_cmp(p, q))
            BBSFindGood(q);

        pch = mpz_get_str(NULL, 10, q);
        g_print(_("%s is an invalid Blum factor, using %s instead."), sz1, pch);
        g_print("\n");
        free(pch);
    }

    InitRNGBBS(rngctx);

    mpz_mul(rngctx->zModulus, p, q);

    mpz_clear(p);
    mpz_clear(q);

    return 0;
}

static unsigned int
BBSGetBit(rngcontext * rngctx)
{

    mpz_powm_ui(rngctx->zSeed, rngctx->zSeed, 2, rngctx->zModulus);
    return (mpz_get_ui(rngctx->zSeed) & 1);
}

static unsigned int
BBSGetTrit(rngcontext * rngctx)
{

    /* Return a trinary digit from a uniform distribution, given binary
     * digits as inputs.  This function is perfectly distributed and
     * uses the fewest number of bits on average. */

    unsigned int state = 0;

    while (1) {
        switch (state) {
        case 0:
            state = BBSGetBit(rngctx) + 1;
            break;

        case 1:
            if (BBSGetBit(rngctx))
                state = 3;
            else
                return 0;
            break;

        case 2:
            if (BBSGetBit(rngctx))
                return 2;
            else
                state = 4;
            break;

        case 3:
            if (BBSGetBit(rngctx))
                return 1;
            else
                state = 1;
            break;

        case 4:
            if (BBSGetBit(rngctx))
                state = 2;
            else
                return 1;
            break;
        }
    }
}

static int
BBSCheck(rngcontext * rngctx)
{

    return (mpz_cmp(rngctx->zSeed, rngctx->zZero) && mpz_cmp(rngctx->zSeed, rngctx->zOne)) ? 0 : -1;
}

static int
BBSInitialSeedFailure(rngcontext * rngctx)
{
    g_print(_("Invalid seed and/or modulus for the Blum, Blum and Shub generator."));
    g_print("\n");
    g_print(_("Please reset the seed and/or modulus before continuing."));
    g_print("\n");
    mpz_set(rngctx->zSeed, rngctx->zZero);      /* so that BBSCheck will fail */

    return -1;
}

static int
BBSCheckInitialSeed(rngcontext * rngctx)
{

    mpz_t z, zCycle;
    int i, iAttempt;

    if (mpz_sgn(rngctx->zSeed) < 1)
        return BBSInitialSeedFailure(rngctx);

    for (iAttempt = 0; iAttempt < 32; iAttempt++) {
        mpz_init_set(z, rngctx->zSeed);

        for (i = 0; i < 8; i++)
            mpz_powm_ui(z, z, 2, rngctx->zModulus);

        mpz_init_set(zCycle, z);

        for (i = 0; i < 16; i++) {
            mpz_powm_ui(z, z, 2, rngctx->zModulus);
            if (!mpz_cmp(z, zCycle))
                /* short cycle detected */
                break;
        }

        if (i == 16)
            /* we found a cycle that meets the minimum length */
            break;

        mpz_add_ui(rngctx->zSeed, rngctx->zSeed, 1);
    }

    if (iAttempt == 32)
        /* we couldn't find any good seeds */
        BBSInitialSeedFailure(rngctx);

    /* FIXME print some sort of warning if we had to modify the seed */

    mpz_clear(z);
    mpz_clear(zCycle);

    return iAttempt < 32 ? 0 : -1;
}
#endif

extern void
PrintRNGCounter(const rng rngx, rngcontext * rngctx)
{

    switch (rngx) {

    case RNG_BBS:
    case RNG_ISAAC:
    case RNG_MD5:
        g_print(_("Number of calls since last seed: %lu."), rngctx->c);
        g_print("\n");

        break;

    case RNG_RANDOM_DOT_ORG:
        g_print(_("Number of dice used in current batch: %lu."), rngctx->c);
        g_print("\n");
        break;

    case RNG_FILE:
        g_print(_("Number of dice read from current file: %lu."), rngctx->c);
        g_print("\n");
        break;

    default:
        break;

    }

}


#if defined(HAVE_LIBGMP)

static void
PrintRNGSeedMP(mpz_t n)
{

    char *pch;
    pch = mpz_get_str(NULL, 10, n);
    g_print(_("The current seed is %s"), pch);
    g_print("\n");
    free(pch);

}

#else

static void
PrintRNGSeedNormal(unsigned int n)
{

    g_print(_("The current seed is %u."), n);
    g_print("\n");

}
#endif                          /* HAVE_LIBGMP */


extern void
PrintRNGSeed(const rng rngx, rngcontext * rngctx)
{

    switch (rngx) {
    case RNG_BBS:
#if defined(HAVE_LIBGMP)
        {
            char *pch0, *pch1;

            pch0 = mpz_get_str(NULL, 10, rngctx->zSeed);
            pch1 = mpz_get_str(NULL, 10, rngctx->zModulus);

            g_print(_("The current seed is %s, and the modulus is %s."), pch0, pch1);
            g_print("\n");

            free(pch0);
            free(pch1);

            return;
        }
#else
        abort();
#endif

    case RNG_MD5:
        g_print(_("The current seed is %u."), rngctx->nMD5);
        g_print("\n");
        return;

    case RNG_FILE:
        g_print(_("GNU Backgammon is reading dice from file: %s"), rngctx->szDiceFilename);
        g_print("\n");
        return;

    case RNG_ISAAC:
    case RNG_MERSENNE:
#if defined(HAVE_LIBGMP)
        PrintRNGSeedMP(rngctx->nz);
#else
        PrintRNGSeedNormal(rngctx->n);
#endif
        return;

    default:
        break;
    }
    g_printerr(_("You cannot show the seed with this random number generator."));
    g_printerr("\n");
}

extern void
InitRNGSeed(unsigned int n, const rng rngx, rngcontext * rngctx)
{

    rngctx->n = n;
    rngctx->c = 0;

    switch (rngx) {

    case RNG_BBS:
#if defined(HAVE_LIBGMP)
        g_assert(rngctx->fZInit);
        mpz_set_ui(rngctx->zSeed, (unsigned long) n);
        BBSCheckInitialSeed(rngctx);
        break;
#else
        abort();
#endif

    case RNG_ISAAC:{
            int i;

            for (i = 0; i < RANDSIZ; i++)
                rngctx->rc.randrsl[i] = (ub4) n;

            irandinit(&rngctx->rc, TRUE);

            break;
        }

    case RNG_MD5:
        rngctx->nMD5 = n;
        break;

    case RNG_MERSENNE:
        sfmt_init_gen_rand(&rngctx->sfmt, n);
        break;

    case RNG_MANUAL:
    case RNG_RANDOM_DOT_ORG:
    case RNG_FILE:
        /* no-op */
        break;

    default:
        break;

    }
}

#if defined(HAVE_LIBGMP)
static void
InitRNGSeedMP(mpz_t n, rng rng, rngcontext * rngctx)
{

    mpz_set(rngctx->nz, n);
    rngctx->c = 0;

    switch (rng) {

    case RNG_MERSENNE:{
            if (mpz_cmp_ui(n, UINT_MAX) > 0) {
                uint32_t *achState;
                uint32_t tempmtkey[SFMT_N32];
                size_t cb;
                unsigned int i;

                achState = mpz_export(NULL, &cb, -1, sizeof(uint32_t), 0, 0, n);
                for (i = 0; i < SFMT_N32 && i < cb; i++) {
                    tempmtkey[i] = achState[i];
                }
                for (; i <SFMT_N32; i++) {
                    tempmtkey[i] = 0;
                }
                sfmt_init_by_array(&rngctx->sfmt, tempmtkey, SFMT_N32);

                free(achState);
            } else {
                InitRNGSeed((unsigned int) (mpz_get_ui(n)), rng, rngctx);
            }
            break;
        }
    case RNG_MD5:
        InitRNGSeed((unsigned int) (mpz_get_ui(n) % UINT_MAX), rng, rngctx);
        break;

    case RNG_BBS:
        g_assert(rngctx->fZInit);
        mpz_set(rngctx->zSeed, n);
        BBSCheckInitialSeed(rngctx);
        break;

    case RNG_ISAAC:{
            ub4 *achState;
            size_t cb;
            unsigned int i;

            achState = mpz_export(NULL, &cb, -1, sizeof(ub4), 0, 0, n);

            for (i = 0; i < RANDSIZ && i < cb; i++)
                rngctx->rc.randrsl[i] = achState[i];

            for (; i < RANDSIZ; i++)
                rngctx->rc.randrsl[i] = 0;

            irandinit(&rngctx->rc, TRUE);

            free(achState);

            break;
        }

    case RNG_MANUAL:
    case RNG_RANDOM_DOT_ORG:
    case RNG_FILE:
        /* no-op */
        break;

    default:
        break;

    }
}

extern int
InitRNGSeedLong(char *sz, rng rng, rngcontext * rngctx)
{

    mpz_t n;

    if (mpz_init_set_str(n, sz, 10) || mpz_sgn(n) < 0) {
        mpz_clear(n);
        return -1;
    }

    InitRNGSeedMP(n, rng, rngctx);

    mpz_clear(n);

    return 0;
}
#endif

static void
CloseDiceFile(rngcontext * rngctx)
{
    if (rngctx->fDice)
        fclose(rngctx->fDice);
}

extern void
CloseRNG(const rng rngx, rngcontext * rngctx)
{


    switch (rngx) {
    case RNG_FILE:
        /* close file */
        CloseDiceFile(rngctx);
        break;

    default:
        /* no-op */
        ;

    }
}


extern int
RNGSystemSeed(const rng rngx, void *p, unsigned long *pnSeed)
{

    int f = FALSE;
    rngcontext *rngctx = (rngcontext *) p;
    unsigned int n = 0;

#if defined(HAVE_LIBGMP)
    if (!pnSeed) {
#if defined(WIN32)
        /* Can be amended to support seeds > 32 bit */
        guint32 achState;
        mpz_t mpzn;

#if GLIB_CHECK_VERSION (2,28,0)
        gint64 tv;
        tv = g_get_real_time();
        achState = (unsigned int) (((guint64)tv >> 32) ^ ((guint64)tv & 0xFFFFFFFF));
#else
        GTimeVal tv;
        g_get_current_time(&tv);
        achState = (unsigned int) tv.tv_sec ^ (unsigned int) tv.tv_usec;
#endif

        mpz_init(mpzn);
        mpz_import(mpzn, 1, -1, sizeof(guint32), 0, 0, &achState);
        InitRNGSeedMP(mpzn, rngx, rngctx);
        mpz_clear(mpzn);

        return TRUE;
#else
        /* We can use long seeds and don't have to save the seed anywhere,
         * so try 512 bits of state instead of 32. */

        int h;

        if ((h = open("/dev/urandom", O_RDONLY)) >= 0) {
            char achState[64];

            if (read(h, achState, 64) == 64) {
                mpz_t mpzn;

                close(h);

                mpz_init(mpzn);
                mpz_import(mpzn, 16, -1, 4, 0, 0, achState);
                InitRNGSeedMP(mpzn, rngx, rngctx);
                mpz_clear(mpzn);

                return TRUE;
            } else
                close(h);
        }
#endif

    }
#else
#if !defined(WIN32)
    int h;
    if ((h = open("/dev/urandom", O_RDONLY)) >= 0) {
        f = read(h, &n, sizeof n) == sizeof n;
        close(h);
    }
#endif

#endif

    if (!f) {
#if GLIB_CHECK_VERSION (2,28,0)
        gint64 tv;
        tv = g_get_real_time();
        n = (unsigned int) (((guint64)tv >> 32) ^ ((guint64)tv & 0xFFFFFFFF));
#else
        GTimeVal tv;
        g_get_current_time(&tv);
        n = (unsigned int) tv.tv_sec ^ (unsigned int) tv.tv_usec;
#endif
    }

    InitRNGSeed(n, rngx, rngctx);
#if defined(HAVE_LIBGMP)
    mpz_set_ui(rngctx->nz, (unsigned long) n);
#endif

    if (pnSeed)
        *pnSeed = (unsigned long) n;

    return f;

}

extern void
free_rngctx(rngcontext * rngctx)
{
#if defined(HAVE_LIBGMP)
    mpz_clear(rngctx->nz);
#endif
    g_free(rngctx);
}

extern void *
InitRNG(unsigned long *pnSeed, int *pfInitFrom, const int fSet, const rng rngx)
{
    int f = FALSE;
    rngcontext *rngctx = g_try_new0(rngcontext, 1);

    if (rngctx == NULL)
        return NULL;

    /* misc. initialisation */

    /* Mersenne-Twister */
    rngctx->sfmt.idx = SFMT_N32 + 1;

#if defined(HAVE_LIBGMP)
    /* BBS */
    rngctx->fZInit = FALSE;
    mpz_init(rngctx->nz);
#endif                          /* HAVE_LIBGMP */

    /* common */
    rngctx->c = 0;

    /* */

    if (fSet)
        f = RNGSystemSeed(rngx, rngctx, pnSeed);

    if (pfInitFrom)
        *pfInitFrom = f;

    return rngctx;
}

extern int
RollDice(unsigned int anDice[2], rng * prng, rngcontext * rngctx)
{
    unsigned long tmprnd;
    const unsigned long exp232_q = 715827882;
    const unsigned long exp232_l = 4294967292U;

    anDice[0] = anDice[1] = 0;

    switch (*prng) {

    case RNG_BBS:
#if defined(HAVE_LIBGMP)
        if (BBSCheck(rngctx)) {
            BBSInitialSeedFailure(rngctx);
            break;
        }
        anDice[0] = BBSGetTrit(rngctx) + BBSGetBit(rngctx) * 3 + 1;
        anDice[1] = BBSGetTrit(rngctx) + BBSGetBit(rngctx) * 3 + 1;
        rngctx->c += 2;
        break;
#else
        abort();
#endif

    case RNG_ISAAC:
        while ((tmprnd = irand(&rngctx->rc)) >= exp232_l);      /* Try again */
        anDice[0] = 1 + (unsigned int) (tmprnd / exp232_q);
        while ((tmprnd = irand(&rngctx->rc)) >= exp232_l);
        anDice[1] = 1 + (unsigned int) (tmprnd / exp232_q);
        rngctx->c += 2;
        break;

    case RNG_MANUAL:
        return GetManualDice(anDice);

    case RNG_MD5:{
            union _hash {
                char ach[16];
                md5_uint32 an[2];
            } h;

            md5_buffer((char *) &rngctx->nMD5, sizeof rngctx->nMD5, &h);
            while (h.an[0] >= exp232_l || h.an[1] >= exp232_l) {
                md5_buffer((char *) &rngctx->nMD5, sizeof rngctx->nMD5, &h);
                rngctx->nMD5++; /* useful ? indispensable ? */
            }

            anDice[0] = (unsigned int) (h.an[0] / exp232_q + 1);
            anDice[1] = (unsigned int) (h.an[1] / exp232_q + 1);

            rngctx->nMD5++;
            rngctx->c += 2;

            break;
        }

    case RNG_MERSENNE:
        while ((tmprnd = sfmt_genrand_uint32(&rngctx->sfmt)) >= exp232_l); /* Try again */
        anDice[0] = 1 + (unsigned int) (tmprnd / exp232_q);
        while ((tmprnd = sfmt_genrand_uint32(&rngctx->sfmt)) >= exp232_l);
        anDice[1] = 1 + (unsigned int) (tmprnd / exp232_q);
        rngctx->c += 2;
        break;

    case RNG_RANDOM_DOT_ORG:
#if defined(LIBCURL_PROTOCOL_HTTPS)
        anDice[0] = getDiceRandomDotOrg();
        if (anDice[0] > 0) {
            anDice[1] = getDiceRandomDotOrg();
        } else
            anDice[1] = anDice[0];
#endif
        break;

    case RNG_FILE:

        anDice[0] = ReadDiceFile(rngctx);
        anDice[1] = ReadDiceFile(rngctx);
        rngctx->c += 2;

        break;

    default:
        break;

    }
    if (anDice[0] < 1 || anDice[1] < 1 || anDice[0] > 6 || anDice[1] > 6) {
        outputerrf(_("Your dice generator isn't working. Failing back on RNG_MERSENNE"));
        SetRNG(prng, rngctx, RNG_MERSENNE, "");
        RollDice(anDice, prng, rngctx);
    }
    return 0;
}

extern FILE *
OpenDiceFile(rngcontext * rngctx, const char *sz)
{
    g_free(rngctx->szDiceFilename);     /* initialized to NULL */
    rngctx->szDiceFilename = g_strdup(sz);

    return (rngctx->fDice = g_fopen(sz, "r"));
}

static unsigned int
ReadDiceFile(rngcontext * rngctx)
{

    unsigned char uch;
    size_t n;

    if (rngctx->fDice == NULL)
        return (unsigned int) (-1);

  uglyloop:
    {

        n = fread(&uch, 1, 1, rngctx->fDice);

        if (feof(rngctx->fDice)) {
            /* end of file */
            g_print(_("Rewinding dice file (%s)"), rngctx->szDiceFilename);
            g_printf("\n");
            fseek(rngctx->fDice, 0, SEEK_SET);
        } else if (n != 1) {
            g_printerr("%s", rngctx->szDiceFilename);
            return (unsigned int) (-1);
        } else if (uch >= '1' && uch <= '6')
            return (uch - '0');

    }
    goto uglyloop;              /* This logic should be reconsidered */

}

extern char *
GetDiceFileName(rngcontext * rngctx)
{
    return rngctx->szDiceFilename;
}

rngcontext *
CopyRNGContext(rngcontext * rngctx)
{
    rngcontext *newCtx = (rngcontext *) g_malloc(sizeof(rngcontext));
    *newCtx = *rngctx;
    return newCtx;
}
