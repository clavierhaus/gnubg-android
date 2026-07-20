/*
 * yield_harness.c -- amendment 2, Steps 2+3 (VERBOSE_COACHING_DESIGN.md
 * sec 5-6): the honesty curve, multiple predicates, REPRESENTATIVE sampling.
 *
 * Samples positions from several self-play lines (distinct opening moves) at
 * several deterministic-noise levels, aggregating the yield curve so it is not
 * one-line-anecdotal. Noise perturbs only WHICH positions self-play visits
 * (deterministic per board -> reproducible); candidate MEASUREMENT always runs
 * at clean 0-ply, so severity bands and equities stay authoritative.
 *
 * For each candidate set (FindnSaveBestMoves) it measures, per severity band
 * and per QUORUM PREDICATE: does exactly one of the top-5 differ on that axis?
 * Emits the aggregate curve plus per-config coverage, so a flat-zero predicate
 * can be read as genuinely dead (flat across configs) vs merely undersampled.
 * GPLv3+.
 *
 * Usage: yield_harness <weights> <plies_measure> <halfmoves_per_config>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "eval.h"
#include "positionid.h"

static int p_blot_opp_home(const TanBoard b){ int n=0; for(int i=18;i<24;i++) if(b[0][i]==1)n++; return n>0; }
static int p_anchor_held(const TanBoard b){ for(int i=18;i<24;i++) if(b[0][i]>=2)return 1; return 0; }
static int p_contact(const TanBoard b){
    int mb=-1; for(int i=23;i>=0;i--) if(b[0][i]>0){mb=i;break;}
    int ob=-1; for(int i=23;i>=0;i--) if(b[1][i]>0){ob=i;break;}
    if(mb<0||ob<0)return 0; return mb > (23-ob);
}
static int p_home_point(const TanBoard b){ int n=0; for(int i=0;i<6;i++) if(b[0][i]>=2)n++; return n>=4; }
static int p_on_bar(const TanBoard b){ return b[0][24]>0; }

typedef int (*pred_fn)(const TanBoard);
static struct { const char *name; pred_fn f; } PREDS[] = {
    { "blot.opp.home",  p_blot_opp_home },
    { "anchor.held",    p_anchor_held   },
    { "contact.kept",   p_contact       },
    { "home.board.4pt", p_home_point    },
    { "on.bar",         p_on_bar        },
};
#define NPRED ((int)(sizeof(PREDS)/sizeof(PREDS[0])))

static const char *BAND[]={ "0.000-0.020","0.020-0.060","0.060-0.120","0.120+" };
static int band_of(float g){ return g<0.020f?0:g<0.060f?1:g<0.120f?2:3; }

/* self-play from a given start board under a noise level; accumulate into
 * seen[] and quorum[][]. Measurement ec is always clean 0-ply. */
static void walk(const TanBoard start, float rNoise, int plies_measure, int halfmoves,
                 const cubeinfo *ci, movefilter aamf[MAX_FILTER_PLIES][MAX_FILTER_PLIES],
                 long seen[4], long quorum[NPRED][4], long *measured)
{
    evalcontext ec_play = { .fCubeful=FALSE,.nPlies=0,.fUsePrune=FALSE,
                            .fDeterministic=TRUE,.rNoise=rNoise };
    evalcontext ec_meas = { .fCubeful=FALSE,.nPlies=plies_measure,.fUsePrune=(plies_measure>0),
                            .fDeterministic=TRUE,.rNoise=0.0f };
    TanBoard b; memcpy(b,start,sizeof(TanBoard));

    for (int hm=0; hm<halfmoves; hm++){
        for (int d0=1; d0<=6; d0++) for (int d1=d0; d1<=6; d1++){
            movelist ml;
            if (FindnSaveBestMoves(&ml,d0,d1,(ConstTanBoard)b,NULL,TRUE,0.0f,ci,&ec_meas,aamf)<0) continue;
            if (ml.cMoves<2) continue;
            (*measured)++;
            float gap=ml.amMoves[0].rScore-ml.amMoves[1].rScore; if(gap<0)gap=-gap;
            int band=band_of(gap); seen[band]++;
            int k=ml.cMoves<5?ml.cMoves:5;
            TanBoard cand[5];
            for(int i=0;i<k;i++){ memcpy(cand[i],b,sizeof(TanBoard)); ApplyMove(cand[i],ml.amMoves[i].anMove,FALSE); }
            for(int p=0;p<NPRED;p++){
                int t=0; for(int i=0;i<k;i++) t+=PREDS[p].f((ConstTanBoard)cand[i]);
                if(t==1||t==k-1) quorum[p][band]++;
            }
        }
        /* advance under NOISE (varies the line), deterministic per board */
        int rd0=(hm%6)+1, rd1=((hm/6)%6)+1, anMove[8];
        if (FindBestMove(anMove,rd0,rd1,b,ci,&ec_play,aamf)<0) break;
        SwapSides(b);
    }
}

int main(int argc, char **argv){
    if(argc<4){ fprintf(stderr,"usage: %s <weights> <plies_measure> <halfmoves>\n",argv[0]); return 2; }
    EvalInitialise(argv[1],NULL,0,NULL);
    int plies=atoi(argv[2]), hm=atoi(argv[3]);
    cubeinfo ci; SetCubeInfoMoney(&ci,1,0,0,FALSE,FALSE,VARIATION_STANDARD);
    movefilter aamf[MAX_FILTER_PLIES][MAX_FILTER_PLIES]; memcpy(aamf,defaultFilters,sizeof(aamf));

    /* standard opening board */
    TanBoard open; memset(open,0,sizeof(open));
    int od[26]={0,-2,0,0,0,0,5,0,3,0,0,0,-5,5,0,0,0,-3,0,-5,0,0,0,0,2,0};
    for(int i=0;i<24;i++){int v=od[i+1]; if(v>0)open[0][i]=v; else if(v<0)open[1][23-i]=-v;}

    /* sampling configs: the opening at several noise levels. Different noise
     * levels (deterministic) send self-play down different but reproducible
     * lines -- cheap diversity without a hand-authored opening bank. */
    float noises[] = { 0.0f, 0.020f, 0.040f, 0.060f };
    int NN = (int)(sizeof(noises)/sizeof(noises[0]));

    long seen[4]={0}; long quorum[NPRED][4]; memset(quorum,0,sizeof(quorum));
    long measured=0;
    /* also track per-predicate whether it EVER fired, across all configs */
    long pred_total[NPRED]; memset(pred_total,0,sizeof(pred_total));

    for(int n=0;n<NN;n++)
        walk(open, noises[n], plies, hm, &ci, aamf, seen, quorum, &measured);

    for(int p=0;p<NPRED;p++) for(int i=0;i<4;i++) pred_total[p]+=quorum[p][i];

    printf("# honesty curve (representative): %d noise configs x %d halfmoves, measurement 0-ply clean\n", NN, hm);
    printf("# candidate-sets=%ld  band totals: %s=%ld %s=%ld %s=%ld %s=%ld\n",
           measured, BAND[0],seen[0],BAND[1],seen[1],BAND[2],seen[2],BAND[3],seen[3]);
    printf("%-16s", "predicate\\sev");
    for(int i=0;i<4;i++) printf(" %11s", BAND[i]);
    printf(" %10s\n","fired");
    for(int p=0;p<NPRED;p++){
        printf("%-16s", PREDS[p].name);
        for(int i=0;i<4;i++){ double y=seen[i]?(double)quorum[p][i]/seen[i]:0.0; printf(" %10.1f%%",100*y); }
        printf(" %10ld\n", pred_total[p]);
    }
    return 0;
}
