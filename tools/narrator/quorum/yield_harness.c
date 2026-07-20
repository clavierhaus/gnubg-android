/*
 * yield_harness.c -- amendment 2, Steps 2+3 (VERBOSE_COACHING_DESIGN.md
 * sec 5-6): the honesty curve, first cut.
 *
 * Walks a deterministic self-play game (FindBestMove, fixed board start), and
 * at each position enumerates all 21 distinct rolls. For each roll it takes
 * gnubg's candidate set (FindnSaveBestMoves) and measures:
 *   - severity: the equity gap best-vs-2nd (proxy for how much a plausible
 *     error costs here -- the blunder scale the honesty threshold turns on);
 *   - a QUORUM predicate over the fixed top-5: does exactly one candidate
 *     stand apart on a checkable structural axis (here: leaves a blot in the
 *     opponent's home board vs not)? This is a first, deliberately simple
 *     checkable statement -- "yours alone leaves a blot" / "every play but one
 *     is safe". Real rules come from Step 4; this proves the curve.
 *
 * Emits per severity band: positions seen, and fraction admitting the quorum
 * statement -- the honesty curve. GPLv3+ like the tree.
 *
 * Usage: yield_harness <weights> <plies_selfplay> <halfmoves>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "eval.h"
#include "positionid.h"

/* opponent home board = points 1..6 in the mover's frame = anBoard[1][0..5]
 * held by opponent; a mover blot there is anBoard[0][18..23]==1 (mover's
 * 19..24 points, i.e. deep in opp home). Count mover blots in opp home. */
static int mover_blots_in_opp_home(const TanBoard b)
{
    int n = 0;
    for (int i = 18; i < 24; i++) if (b[0][i] == 1) n++;
    return n;
}

/* Apply a gnubg anMove to a copy, return blot-in-opp-home count for it. */
static int blots_after(const TanBoard pre, const int anMove[8])
{
    TanBoard t; memcpy(t, pre, sizeof(TanBoard));
    ApplyMove(t, anMove, FALSE);
    return mover_blots_in_opp_home(t);
}

/* severity bands by best-vs-2nd equity gap */
static const char *BAND[] = { "0.000-0.020", "0.020-0.060", "0.060-0.120", "0.120+" };
static int band_of(float g) { return g < 0.020f ? 0 : g < 0.060f ? 1 : g < 0.120f ? 2 : 3; }

int main(int argc, char **argv)
{
    if (argc < 4) { fprintf(stderr, "usage: %s <weights> <plies> <halfmoves>\n", argv[0]); return 2; }
    EvalInitialise(argv[1], NULL, 0, NULL);
    int plies = atoi(argv[2]), halfmoves = atoi(argv[3]);

    evalcontext ec = { .fCubeful=FALSE, .nPlies=plies, .fUsePrune=(plies>0),
                       .fDeterministic=TRUE, .rNoise=0.0f };
    cubeinfo ci; SetCubeInfoMoney(&ci, 1, 0, 0, FALSE, FALSE, VARIATION_STANDARD);
    movefilter aamf[MAX_FILTER_PLIES][MAX_FILTER_PLIES];
    memcpy(aamf, defaultFilters, sizeof(aamf));

    /* standard opening position, mover = player 0 */
    TanBoard b;
    int open[26] = {0,-2,0,0,0,0,5,0,3,0,0,0,-5,5,0,0,0,-3,0,-5,0,0,0,0,2,0};
    memset(b,0,sizeof(b));
    for (int i=0;i<24;i++){int v=open[i+1]; if(v>0)b[0][i]=v; else if(v<0)b[1][23-i]=-v;}

    long seen[4]={0}, quorum[4]={0};
    long total_positions=0, total_measured=0;

    for (int hm=0; hm<halfmoves; hm++) {
        /* At this position, enumerate all 21 distinct rolls for density. */
        for (int d0=1; d0<=6; d0++) for (int d1=d0; d1<=6; d1++) {
            movelist ml;
            if (FindnSaveBestMoves(&ml, d0, d1, (ConstTanBoard)b, NULL, TRUE,
                                   0.0f, &ci, &ec, aamf) < 0) continue;
            if (ml.cMoves < 2) continue;
            total_measured++;

            /* severity: best - 2nd (rScore is equity, best first) */
            float gap = ml.amMoves[0].rScore - ml.amMoves[1].rScore;
            if (gap < 0) gap = -gap;
            int band = band_of(gap);
            seen[band]++;

            /* quorum over top-5: exactly one candidate leaves a blot in opp
             * home while the rest do not (or vice versa) -- a checkable claim */
            int k = ml.cMoves < 5 ? ml.cMoves : 5;
            int with_blot=0;
            for (int i=0;i<k;i++)
                if (blots_after(b, ml.amMoves[i].anMove) > 0) with_blot++;
            if (with_blot==1 || with_blot==k-1) quorum[band]++;
        }
        total_positions++;

        /* advance self-play: fixed deterministic roll sequence (hm-derived, so
         * reproducible), play best, swap sides. */
        int rd0 = (hm % 6) + 1, rd1 = ((hm/6) % 6) + 1;
        int anMove[8];
        if (FindBestMove(anMove, rd0, rd1, b, &ci, &ec, aamf) < 0) break;
        SwapSides(b);
    }

    printf("# honesty curve (first cut): quorum = 'exactly one of top-5 leaves a blot in opp home'\n");
    printf("# self-play positions=%ld  candidate-sets measured=%ld  plies=%d\n",
           total_positions, total_measured, plies);
    printf("%-14s %10s %10s %8s\n", "severity", "measured", "quorum", "yield");
    for (int i=0;i<4;i++) {
        double y = seen[i] ? (double)quorum[i]/seen[i] : 0.0;
        printf("%-14s %10ld %10ld %7.1f%%\n", BAND[i], seen[i], quorum[i], 100*y);
    }
    return 0;
}
