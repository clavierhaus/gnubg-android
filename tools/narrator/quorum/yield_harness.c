/*
 * yield_harness.c -- amendment 2, Steps 2+3 (VERBOSE_COACHING_DESIGN.md
 * sec 5-6): the honesty curve, multiple predicates.
 *
 * Walks a deterministic self-play game and at each position enumerates all 21
 * rolls. For each candidate set (FindnSaveBestMoves) it measures, per severity
 * band (best-vs-2nd equity gap), and per QUORUM PREDICATE: does exactly one of
 * the top-5 differ on that predicate's structural axis? Each predicate is a
 * checkable claim over the on-screen candidates ("yours alone leaves a blot",
 * "every play but one breaks contact"...). Predicates are computed from the
 * candidate's resulting board via gnubg's own inputs and geometry -- same
 * provenance as the corpus. Emits the yield curve per predicate.
 *
 * This is the honesty curve's real instrument: yield here = how often a given
 * checkable statement is TRUE-of-exactly-one over the candidate set, banded by
 * how much the decision matters. The union over predicates is the coverage; a
 * predicate that yields near-zero everywhere is not worth a rule; one whose
 * yield rises with severity is a rule candidate. GPLv3+.
 *
 * Usage: yield_harness <weights> <plies> <halfmoves>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "eval.h"
#include "positionid.h"

/* ---- predicates: (resulting board) -> integer feature; quorum tests whether
 *      exactly one of the top-K has a DIFFERENT boolean-ized value ---- */

/* mover blots in opponent's home board (mover points 19..24 = idx 18..23) */
static int p_blot_opp_home(const TanBoard b) {
    int n=0; for (int i=18;i<24;i++) if (b[0][i]==1) n++; return n>0;
}
/* mover holds an anchor in opp home (2+ on any of idx 18..23) */
static int p_anchor_held(const TanBoard b) {
    for (int i=18;i<24;i++) if (b[0][i]>=2) return 1; return 0;
}
/* contact broken: mover's back checker has passed the opp's back checker,
 * i.e. no mover checker behind the opponent's rearmost. Approx via back chequer
 * point: mover's rearmost (highest idx with b[0]>0) vs opp rearmost. */
static int p_contact(const TanBoard b) {
    int mover_back=-1; for (int i=23;i>=0;i--) if (b[0][i]>0){mover_back=i;break;}
    int opp_back=-1;   for (int i=23;i>=0;i--) if (b[1][i]>0){opp_back=i;break;}
    /* contact exists if the two back checkers can still interact:
     * mover_back (from mover frame) and opp_back (opp frame, i.e. mover point 23-i) */
    if (mover_back<0||opp_back<0) return 0;
    int opp_back_in_mover = 23-opp_back;
    return mover_back > opp_back_in_mover; /* still passing each other */
}
/* mover made a new home-board point this move: any point idx 0..5 with 2+ */
static int p_home_point(const TanBoard b) {
    int n=0; for (int i=0;i<6;i++) if (b[0][i]>=2) n++; return n>=4; /* strong board */
}
/* a checker on the bar (mover was hit path) */
static int p_on_bar(const TanBoard b) { return b[0][24]>0; }

typedef int (*pred_fn)(const TanBoard);
static struct { const char *name; pred_fn f; } PREDS[] = {
    { "blot.opp.home",  p_blot_opp_home },
    { "anchor.held",    p_anchor_held   },
    { "contact.kept",   p_contact       },
    { "home.board.4pt", p_home_point    },
    { "on.bar",         p_on_bar        },
};
#define NPRED ((int)(sizeof(PREDS)/sizeof(PREDS[0])))

static const char *BAND[] = { "0.000-0.020","0.020-0.060","0.060-0.120","0.120+" };
static int band_of(float g){ return g<0.020f?0:g<0.060f?1:g<0.120f?2:3; }

int main(int argc, char **argv) {
    if (argc<4){ fprintf(stderr,"usage: %s <weights> <plies> <halfmoves>\n",argv[0]); return 2; }
    EvalInitialise(argv[1],NULL,0,NULL);
    int plies=atoi(argv[2]), halfmoves=atoi(argv[3]);

    evalcontext ec = { .fCubeful=FALSE,.nPlies=plies,.fUsePrune=(plies>0),
                       .fDeterministic=TRUE,.rNoise=0.0f };
    cubeinfo ci; SetCubeInfoMoney(&ci,1,0,0,FALSE,FALSE,VARIATION_STANDARD);
    movefilter aamf[MAX_FILTER_PLIES][MAX_FILTER_PLIES];
    memcpy(aamf,defaultFilters,sizeof(aamf));

    TanBoard b;
    int open[26]={0,-2,0,0,0,0,5,0,3,0,0,0,-5,5,0,0,0,-3,0,-5,0,0,0,0,2,0};
    memset(b,0,sizeof(b));
    for (int i=0;i<24;i++){int v=open[i+1]; if(v>0)b[0][i]=v; else if(v<0)b[1][23-i]=-v;}

    long seen[4]={0};
    long quorum[NPRED][4]; memset(quorum,0,sizeof(quorum));
    long measured=0;

    for (int hm=0; hm<halfmoves; hm++) {
        for (int d0=1; d0<=6; d0++) for (int d1=d0; d1<=6; d1++) {
            movelist ml;
            if (FindnSaveBestMoves(&ml,d0,d1,(ConstTanBoard)b,NULL,TRUE,0.0f,&ci,&ec,aamf)<0) continue;
            if (ml.cMoves<2) continue;
            measured++;
            float gap = ml.amMoves[0].rScore - ml.amMoves[1].rScore; if(gap<0)gap=-gap;
            int band = band_of(gap); seen[band]++;

            int k = ml.cMoves<5?ml.cMoves:5;
            TanBoard cand[5];
            for (int i=0;i<k;i++){ memcpy(cand[i],b,sizeof(TanBoard)); ApplyMove(cand[i],ml.amMoves[i].anMove,FALSE); }
            for (int p=0;p<NPRED;p++){
                int t=0; for(int i=0;i<k;i++) t+=PREDS[p].f((ConstTanBoard)cand[i]);
                if (t==1 || t==k-1) quorum[p][band]++;   /* exactly one differs */
            }
        }
        int rd0=(hm%6)+1, rd1=((hm/6)%6)+1, anMove[8];
        if (FindBestMove(anMove,rd0,rd1,b,&ci,&ec,aamf)<0) break;
        SwapSides(b);
    }

    printf("# honesty curve: yield = fraction of candidate-sets where exactly one of top-5 differs on the predicate\n");
    printf("# plies=%d halfmoves=%d candidate-sets=%ld\n", plies, halfmoves, measured);
    printf("# band totals: %s=%ld %s=%ld %s=%ld %s=%ld\n",
           BAND[0],seen[0],BAND[1],seen[1],BAND[2],seen[2],BAND[3],seen[3]);
    printf("%-16s", "predicate\\severity");
    for (int i=0;i<4;i++) printf(" %12s", BAND[i]);
    printf("\n");
    for (int p=0;p<NPRED;p++){
        printf("%-16s", PREDS[p].name);
        for (int i=0;i<4;i++){
            double y = seen[i]? (double)quorum[p][i]/seen[i] : 0.0;
            printf(" %11.1f%%", 100*y);
        }
        printf("\n");
    }
    return 0;
}
