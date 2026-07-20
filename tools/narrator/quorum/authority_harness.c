/*
 * authority_harness.c -- amendment 2, Step 4 evidence (VERBOSE_COACHING_DESIGN
 * sec 4.3): reference-authority agreement.
 *
 * The yield harness (Steps 2-3) finds which quorum predicates rise with
 * severity -- rule CANDIDATES. Adoption (section 4.3) requires more: the
 * predicate's claim must be CONFIRMED by a higher-authority gnubg evaluation,
 * above a pre-registered threshold. This harness measures that agreement.
 *
 * For each self-play position (deterministic-noise walk), and each roll, it
 * takes the 0-ply candidate set (the verdict's own setting). Where a predicate
 * QUORUM FIRES at 0-ply (exactly one of top-5 differs on the axis), it
 * re-generates the top-5 at the reference ply and asks whether the SAME
 * predicate still fires. Agreement = P(fires at authority | fires at 0-ply):
 * is the checkable claim robust to depth, or a 0-ply artifact? Only fired
 * positions pay the expensive higher-ply pass, so cost is bounded.
 *
 * Emits, per predicate: 0-ply fire count, authority-confirmed count, agreement
 * rate -- the adoption evidence the pre-registered threshold tests against.
 * GPLv3+.
 *
 * Usage: authority_harness <weights> <authority_plies> <halfmoves>
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

/* does predicate p's quorum fire over this candidate set's top-5? */
static int quorum_fires(pred_fn f, const TanBoard pre, const movelist *ml){
    int k = ml->cMoves<5?ml->cMoves:5;
    if (k<2) return 0;
    int t=0;
    for(int i=0;i<k;i++){ TanBoard c; memcpy(c,pre,sizeof(TanBoard)); ApplyMove(c,ml->amMoves[i].anMove,FALSE); t+=f((ConstTanBoard)c); }
    return (t==1 || t==k-1);
}

int main(int argc, char **argv){
    if(argc<4){ fprintf(stderr,"usage: %s <weights> <authority_plies> <halfmoves>\n",argv[0]); return 2; }
    EvalInitialise(argv[1],NULL,0,NULL);
    int auth=atoi(argv[2]), hm=atoi(argv[3]);
    cubeinfo ci; SetCubeInfoMoney(&ci,1,0,0,FALSE,FALSE,VARIATION_STANDARD);
    movefilter aamf[MAX_FILTER_PLIES][MAX_FILTER_PLIES]; memcpy(aamf,defaultFilters,sizeof(aamf));

    evalcontext ec0 = { .fCubeful=FALSE,.nPlies=0,.fUsePrune=FALSE,.fDeterministic=TRUE,.rNoise=0.0f };
    evalcontext ecA = { .fCubeful=FALSE,.nPlies=auth,.fUsePrune=(auth>0),.fDeterministic=TRUE,.rNoise=0.0f };

    TanBoard open; memset(open,0,sizeof(open));
    int od[26]={0,-2,0,0,0,0,5,0,3,0,0,0,-5,5,0,0,0,-3,0,-5,0,0,0,0,2,0};
    for(int i=0;i<24;i++){int v=od[i+1]; if(v>0)open[0][i]=v; else if(v<0)open[1][23-i]=-v;}

    /* opening bank: 15 rolls x gnubg top-3 replies -- engine-derived, license-clean */
    int opens[15][2]={{6,5},{6,4},{6,3},{6,2},{6,1},{5,4},{5,3},{5,2},{5,1},{4,3},{4,2},{4,1},{3,2},{3,1},{2,1}};
    const int TOPN=1; TanBoard bank[15*TOPN]; int nbank=0;  /* top-1 reply: 15 lines, enough for a converging agreement rate at 2-ply */
    for(int o=0;o<15;o++){
        movelist ml;
        if(FindnSaveBestMoves(&ml,opens[o][0],opens[o][1],(ConstTanBoard)open,NULL,TRUE,0.0f,&ci,&ec0,aamf)<0) continue;
        int k=ml.cMoves<TOPN?ml.cMoves:TOPN;
        for(int i=0;i<k;i++){ TanBoard nb; memcpy(nb,open,sizeof(TanBoard)); ApplyMove(nb,ml.amMoves[i].anMove,FALSE); SwapSides(nb); memcpy(bank[nbank++],nb,sizeof(TanBoard)); }
    }
    float noises[]={0.0f}; int NN=1;  /* agreement is depth-robustness, not line-diversity; one clean config */

    long fired0[NPRED]={0}, confirmed[NPRED]={0};

    for(int linen=0; linen<nbank; linen++)
    for(int n=0;n<NN;n++){
        evalcontext ecPlay = { .fCubeful=FALSE,.nPlies=0,.fUsePrune=FALSE,.fDeterministic=TRUE,.rNoise=noises[n] };
        TanBoard b; memcpy(b,bank[linen],sizeof(TanBoard));
        for(int h=0;h<hm;h++){
            for(int d0=1;d0<=6;d0++) for(int d1=d0;d1<=6;d1++){
                movelist ml0;
                if(FindnSaveBestMoves(&ml0,d0,d1,(ConstTanBoard)b,NULL,TRUE,0.0f,&ci,&ec0,aamf)<0) continue;
                if(ml0.cMoves<2) continue;
                /* which predicates fire at 0-ply here */
                int fires[NPRED];
                int any=0;
                for(int p=0;p<NPRED;p++){ fires[p]=quorum_fires(PREDS[p].f,b,&ml0); if(fires[p]){fired0[p]++; any=1;} }
                if(!any) continue;
                /* only now pay the authority pass */
                movelist mlA;
                if(FindnSaveBestMoves(&mlA,d0,d1,(ConstTanBoard)b,NULL,TRUE,0.0f,&ci,&ecA,aamf)<0) continue;
                for(int p=0;p<NPRED;p++)
                    if(fires[p] && quorum_fires(PREDS[p].f,b,&mlA)) confirmed[p]++;
            }
            int rd0=(h%6)+1, rd1=((h/6)%6)+1, anMove[8];
            if(FindBestMove(anMove,rd0,rd1,b,&ci,&ecPlay,aamf)<0) break;
            SwapSides(b);
        }
    }

    printf("# reference-authority agreement: authority=%d-ply, %d opening lines x %d noise configs x %d halfmoves\n", auth, nbank, NN, hm);
    printf("# agreement = P(quorum fires at authority ply | fires at 0-ply) -- is the claim depth-robust?\n");
    printf("%-16s %10s %12s %10s\n","predicate","0ply_fires","auth_conf","agreement");
    for(int p=0;p<NPRED;p++){
        double a = fired0[p]? (double)confirmed[p]/fired0[p] : 0.0;
        printf("%-16s %10ld %12ld %9.1f%%\n", PREDS[p].name, fired0[p], confirmed[p], 100*a);
    }
    return 0;
}
