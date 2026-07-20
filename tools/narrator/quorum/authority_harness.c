/*
 * authority_harness.c -- amendment 2, Step 4 evidence (VERBOSE_COACHING_DESIGN
 * sec 4.3): equity-alignment, with a NULL baseline.
 *
 * A quorum rule ("yours alone leaves a blot") is a valid EXPLANATION only if
 * the predicate-isolated candidate systematically corresponds to equity -- it
 * is reliably the worse (or better) move. An earlier metric ("does the
 * predicate still fire at higher ply") was shown BY THE NULL BASELINE to be
 * worthless: a meaningless predicate scored 91%, because it only measured
 * candidate-set stability under depth, not explanatory validity.
 *
 * The fixed metric: for each 0-ply fire, record the EQUITY RANK of the
 * isolated candidate among the top-K (0 = best ... K-1 = worst). A real axis
 * skews the distribution (isolated candidate is mostly best, or mostly worst);
 * a null axis is uniform. We report the alignment = |P(isolated worst) -
 * P(isolated best)| and the mean rank fraction; the NULL predicates set the
 * floor these must beat. Authority ply is still used: the rank is taken at the
 * authority ply, so alignment is measured against the DEEPER truth.
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
static int p_null_parity(const TanBoard b){ int s=0; for(int i=1;i<24;i+=2) s+=b[0][i]; return (s&1); }
static int p_null_pt13(const TanBoard b){ return b[0][12]>0; }

typedef int (*pred_fn)(const TanBoard);
static struct { const char *name; pred_fn f; } PREDS[] = {
    { "blot.opp.home",  p_blot_opp_home },
    { "anchor.held",    p_anchor_held   },
    { "contact.kept",   p_contact       },
    { "home.board.4pt", p_home_point    },
    { "on.bar",         p_on_bar        },
    { "NULL.parity",    p_null_parity   },
    { "NULL.pt13",      p_null_pt13     },
};
#define NPRED ((int)(sizeof(PREDS)/sizeof(PREDS[0])))

/* If predicate f isolates exactly one of the top-k (quorum fires), return that
 * candidate's index in the (equity-ranked, best-first) list; else -1. */
static int isolated_index(pred_fn f, const TanBoard pre, const movelist *ml){
    int k = ml->cMoves<5?ml->cMoves:5;
    if (k<2) return -1;
    int vals[5], t=0;
    for(int i=0;i<k;i++){ TanBoard c; memcpy(c,pre,sizeof(TanBoard)); ApplyMove(c,ml->amMoves[i].anMove,FALSE); vals[i]=f((ConstTanBoard)c); t+=vals[i]; }
    int isolated_val;
    if (t==1) isolated_val=1;          /* one candidate HAS the property */
    else if (t==k-1) isolated_val=0;   /* one candidate LACKS it */
    else return -1;
    for(int i=0;i<k;i++) if (vals[i]==isolated_val) return i;
    return -1;
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
    int opens[15][2]={{6,5},{6,4},{6,3},{6,2},{6,1},{5,4},{5,3},{5,2},{5,1},{4,3},{4,2},{4,1},{3,2},{3,1},{2,1}};
    const int TOPN=1; TanBoard bank[15]; int nbank=0;
    for(int o=0;o<15;o++){ movelist ml;
        if(FindnSaveBestMoves(&ml,opens[o][0],opens[o][1],(ConstTanBoard)open,NULL,TRUE,0.0f,&ci,&ec0,aamf)<0) continue;
        int k=ml.cMoves<TOPN?ml.cMoves:TOPN;
        for(int i=0;i<k;i++){ TanBoard nb; memcpy(nb,open,sizeof(TanBoard)); ApplyMove(nb,ml.amMoves[i].anMove,FALSE); SwapSides(nb); memcpy(bank[nbank++],nb,sizeof(TanBoard)); }
    }

    /* per predicate: count fires, and where the isolated candidate ranks (best/worst) at AUTHORITY ply */
    long fires[NPRED]={0}, isworst[NPRED]={0}, isbest[NPRED]={0};
    double rank_frac_sum[NPRED]; memset(rank_frac_sum,0,sizeof(rank_frac_sum));

    TanBoard b; 
    for(int linen=0; linen<nbank; linen++){
        memcpy(b,bank[linen],sizeof(TanBoard));
        for(int h=0;h<hm;h++){
            for(int d0=1;d0<=6;d0++) for(int d1=d0;d1<=6;d1++){
                movelist ml0;
                if(FindnSaveBestMoves(&ml0,d0,d1,(ConstTanBoard)b,NULL,TRUE,0.0f,&ci,&ec0,aamf)<0) continue;
                if(ml0.cMoves<2) continue;
                int anyfire=0, iso[NPRED];
                for(int p=0;p<NPRED;p++){ iso[p]=isolated_index(PREDS[p].f,b,&ml0); if(iso[p]>=0) anyfire=1; }
                if(!anyfire) continue;
                /* authority pass: re-rank the SAME candidates at authority ply */
                movelist mlA;
                if(FindnSaveBestMoves(&mlA,d0,d1,(ConstTanBoard)b,NULL,TRUE,0.0f,&ci,&ecA,aamf)<0) continue;
                int kA=mlA.cMoves<5?mlA.cMoves:5;
                for(int p=0;p<NPRED;p++){
                    if(iso[p]<0) continue;
                    /* find the isolated candidate's anMove in the authority list, read its rank */
                    int *am=ml0.amMoves[iso[p]].anMove;
                    int arank=-1;
                    for(int j=0;j<kA;j++){ if(memcmp(mlA.amMoves[j].anMove,am,sizeof(int)*8)==0){arank=j;break;} }
                    if(arank<0) continue;   /* isolated candidate dropped out of authority top-5 */
                    fires[p]++;
                    rank_frac_sum[p] += (kA>1)? (double)arank/(kA-1) : 0.5;
                    if(arank==0) isbest[p]++;
                    if(arank==kA-1) isworst[p]++;
                }
            }
            int rd0=(h%6)+1, rd1=((h/6)%6)+1, anMove[8];
            evalcontext ecPlay = { .fCubeful=FALSE,.nPlies=0,.fUsePrune=FALSE,.fDeterministic=TRUE,.rNoise=0.0f };
            if(FindBestMove(anMove,rd0,rd1,b,&ci,&ecPlay,aamf)<0) break;
            SwapSides(b);
        }
    }

    printf("# equity-alignment at %d-ply authority, %d opening lines x %d halfmoves\n", auth, nbank, hm);
    printf("# alignment = |P(isolated=worst) - P(isolated=best)|; a real axis skews, a NULL axis ~0\n");
    printf("%-16s %8s %8s %8s %10s %10s\n","predicate","fires","P_best","P_worst","meanrank","ALIGN");
    for(int p=0;p<NPRED;p++){
        if(fires[p]==0){ printf("%-16s %8s\n", PREDS[p].name, "0"); continue; }
        double pb=(double)isbest[p]/fires[p], pw=(double)isworst[p]/fires[p];
        double mr=rank_frac_sum[p]/fires[p];
        printf("%-16s %8ld %7.1f%% %7.1f%% %10.2f %9.1f%%\n",
               PREDS[p].name, fires[p], 100*pb, 100*pw, mr, 100*(pw>pb?pw-pb:pb-pw));
    }
    return 0;
}
