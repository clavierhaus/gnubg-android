/*
 * input_alignment.c -- a LEFTOVER turned instrument. The quorum harness's
 * machinery (host candidate generation, opening bank, self-play walk,
 * equity-alignment) answers a question the CORPUS has never had answered:
 * which of gnubg's I_* inputs actually track equity?
 *
 * The corpus signatures were authored as "a GUESS from reading enum comments,
 * not a measurement" (CORPUS_HARVEST_PLAN author_notes). This measures it: for
 * each candidate set over the opening bank, take the best and the worst of the
 * top-K; compute every I_* delta between their boards (mover side); and
 * accumulate, per input, how strongly |delta| co-moves with the equity gap
 * (mean |delta| when the gap is large vs small). Inputs whose delta rises with
 * the equity gap are REAL explanatory vocabulary; inputs flat across gaps are
 * noise the corpus should not lean on.
 *
 * This is the empirical foundation UNDER the shipped corpus, produced by the
 * tooling built for a feature that itself failed. GPLv3+.
 *
 * Usage: input_alignment <weights> <halfmoves>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "eval.h"
#include "positionid.h"

static const char *IN[] = {
 "I_OFF1","I_OFF2","I_OFF3","I_BREAK_CONTACT","I_BACK_CHEQUER","I_BACK_ANCHOR",
 "I_FORWARD_ANCHOR","I_PIPLOSS","I_P1","I_P2","I_BACKESCAPES","I_ACONTAIN",
 "I_ACONTAIN2","I_CONTAIN","I_CONTAIN2","I_MOBILITY","I_MOMENT2","I_ENTER",
 "I_ENTER2","I_TIMING","I_BACKBONE","I_BACKG","I_BACKG1","I_FREEPIP","I_BACKRESCAPES"
};

int main(int argc,char**argv){
    if(argc<3){fprintf(stderr,"usage: %s <weights> <halfmoves>\n",argv[0]);return 2;}
    EvalInitialise(argv[1],NULL,0,NULL);
    int hm=atoi(argv[2]);
    cubeinfo ci; SetCubeInfoMoney(&ci,1,0,0,FALSE,FALSE,VARIATION_STANDARD);
    movefilter aamf[MAX_FILTER_PLIES][MAX_FILTER_PLIES]; memcpy(aamf,defaultFilters,sizeof(aamf));
    evalcontext ec={.fCubeful=FALSE,.nPlies=0,.fUsePrune=FALSE,.fDeterministic=TRUE,.rNoise=0.0f};

    TanBoard open; memset(open,0,sizeof(open));
    int od[26]={0,-2,0,0,0,0,5,0,3,0,0,0,-5,5,0,0,0,-3,0,-5,0,0,0,0,2,0};
    for(int i=0;i<24;i++){int v=od[i+1]; if(v>0)open[0][i]=v; else if(v<0)open[1][23-i]=-v;}
    int opens[15][2]={{6,5},{6,4},{6,3},{6,2},{6,1},{5,4},{5,3},{5,2},{5,1},{4,3},{4,2},{4,1},{3,2},{3,1},{2,1}};
    TanBoard bank[15]; int nbank=0;
    for(int o=0;o<15;o++){movelist ml;
        if(FindnSaveBestMoves(&ml,opens[o][0],opens[o][1],(ConstTanBoard)open,NULL,TRUE,0.0f,&ci,&ec,aamf)<0)continue;
        if(ml.cMoves<1)continue;
        TanBoard nb; memcpy(nb,open,sizeof(TanBoard)); ApplyMove(nb,ml.amMoves[0].anMove,FALSE); SwapSides(nb);
        memcpy(bank[nbank++],nb,sizeof(TanBoard));
    }

    /* accumulate: for LOW-gap and HIGH-gap candidate sets separately, the mean
     * |input delta| between best and worst. If an input matters, its delta is
     * bigger when the equity gap is bigger. */
    double sum_lo[MORE_INPUTS]={0}, sum_hi[MORE_INPUTS]={0};
    long n_lo=0, n_hi=0;

    for(int L=0;L<nbank;L++){
        TanBoard b; memcpy(b,bank[L],sizeof(TanBoard));
        for(int h=0;h<hm;h++){
            for(int d0=1;d0<=6;d0++)for(int d1=d0;d1<=6;d1++){
                movelist ml;
                if(FindnSaveBestMoves(&ml,d0,d1,(ConstTanBoard)b,NULL,TRUE,0.0f,&ci,&ec,aamf)<0)continue;
                if(ml.cMoves<2)continue;
                int k=ml.cMoves<5?ml.cMoves:5;
                float gap=ml.amMoves[0].rScore-ml.amMoves[k-1].rScore; if(gap<0)gap=-gap;
                /* best and worst boards */
                TanBoard bb,wb; memcpy(bb,b,sizeof(TanBoard)); memcpy(wb,b,sizeof(TanBoard));
                ApplyMove(bb,ml.amMoves[0].anMove,FALSE);
                ApplyMove(wb,ml.amMoves[k-1].anMove,FALSE);
                float ib[MORE_INPUTS], iw[MORE_INPUTS];
                CalculateHalfInputs(bb[0], bb[1], ib);   /* mover-side inputs */
                CalculateHalfInputs(wb[0], wb[1], iw);
                int hi = gap >= 0.08f;
                for(int q=0;q<MORE_INPUTS;q++){
                    double d = ib[q]-iw[q]; if(d<0)d=-d;
                    if(hi){ sum_hi[q]+=d; } else { sum_lo[q]+=d; }
                }
                if(hi)n_hi++; else n_lo++;
            }
            int rd0=(h%6)+1,rd1=((h/6)%6)+1,anMove[8];
            if(FindBestMove(anMove,rd0,rd1,b,&ci,&ec,aamf)<0)break;
            SwapSides(b);
        }
    }

    printf("# input-equity alignment: mean |I_* delta| (best vs worst candidate), low-gap vs high-gap sets\n");
    printf("# low-gap sets=%ld  high-gap sets=%ld  (high = equity gap >= 0.08)\n", n_lo, n_hi);
    printf("# ratio hi/lo > 1 => this input's delta GROWS with the decision's stakes = real vocabulary\n");
    printf("%-16s %10s %10s %8s\n","input","mean_lo","mean_hi","ratio");
    for(int q=0;q<MORE_INPUTS;q++){
        double lo=n_lo?sum_lo[q]/n_lo:0, hi=n_hi?sum_hi[q]/n_hi:0;
        double r = lo>1e-9? hi/lo : 0;
        printf("%-16s %10.3f %10.3f %8.2f\n", IN[q], lo, hi, r);
    }
    return 0;
}
