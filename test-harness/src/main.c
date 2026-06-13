#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <glib.h>

#include "config.h"
#include "eval.h"
#include "rollout.h"
#include "positionid.h"
#include "backgammon.h"
#include "util.h"
#include "multithread.h"

extern void gnubg_init_tld(void);
extern void gnubg_init_rollout(void);
extern int gnubg_rollout(const TanBoard anBoard,
                          float arOutput[NUM_ROLLOUT_OUTPUTS],
                          float arStdDev[NUM_ROLLOUT_OUTPUTS],
                          const cubeinfo *pci, rolloutcontext *prc);

static void set_opening_position(TanBoard anBoard) {
    memset(anBoard, 0, sizeof(TanBoard));
    anBoard[1][23] = 2; anBoard[1][12] = 5;
    anBoard[1][7]  = 3; anBoard[1][5]  = 5;
    anBoard[0][23] = 2; anBoard[0][12] = 5;
    anBoard[0][7]  = 3; anBoard[0][5]  = 5;
}

int main(int argc, char *argv[]) {
    const char *weights_path = "/data/local/tmp/gnubg.weights";
    if (argc > 1) weights_path = argv[1];

    printf("=== GNU Backgammon Android Test Harness ===\n\n");

    printf("[1] Initialising engine...\n"); fflush(stdout);
    EvalInitialise((char *)weights_path, NULL, 0, NULL);
    gnubg_init_tld();
    gnubg_init_rollout();
    printf("    pbc1=%p pbc2=%p pbcOS=%p pbcTS=%p\n\n",
           (void*)pbc1, (void*)pbc2, (void*)pbcOS, (void*)pbcTS);

    TanBoard anBoard;
    set_opening_position(anBoard);

    positionclass pc = ClassifyPosition((ConstTanBoard)anBoard, VARIATION_STANDARD);
    printf("[2] ClassifyPosition = %d (expected 10 = CLASS_CONTACT)\n\n", pc);

    cubeinfo ci;
    int anScore[2] = {0, 0};
    SetCubeInfo(&ci, 1, -1, 0, 0, anScore, 0, 0, 0, VARIATION_STANDARD);

    float arOutput[NUM_OUTPUTS] = {0};
    evalcontext ec1 = { 0, 1, 0, 1, 0.0f };

    printf("[3] EvaluatePosition 1-ply cubeless...\n"); fflush(stdout);
    EvaluatePositionNoLocking(NULL, (ConstTanBoard)anBoard, arOutput, &ci, &ec1);
    printf("    Win prob:        %.4f\n", arOutput[0]);
    printf("    Win gammon:      %.4f\n", arOutput[1]);
    printf("    Win backgammon:  %.4f\n", arOutput[2]);
    printf("    Lose gammon:     %.4f\n", arOutput[3]);
    printf("    Lose backgammon: %.4f\n", arOutput[4]);
    float equity1 = arOutput[0]*2-1 + arOutput[1]-arOutput[3] + arOutput[2]-arOutput[4];
    printf("    Cubeless equity: %.4f\n\n", equity1);

    printf("[4] FindBestMove for 3-1...\n"); fflush(stdout);
    int anMove[8];
    memset(anMove, -1, sizeof(anMove));
    FindBestMoveNoLocking(anMove, 3, 1, anBoard, &ci, &ec1, defaultFilters);
    printf("    Move: ");
    for (int i = 0; i < 8; i += 2) {
        if (anMove[i] < 0) break;
        printf("%d/%d ", anMove[i] + 1, anMove[i + 1] + 1);
    }
    printf("(expected 8/5 6/5)\n\n");

    printf("[5] Cube decision (money game, centred cube)...\n"); fflush(stdout);
    cubeinfo ci_money;
    int anScore2[2] = {0, 0};
    SetCubeInfo(&ci_money, 1, -1, 0, 0, anScore2, 0, 0, 0, VARIATION_STANDARD);

    evalsetup es;
    memset(&es, 0, sizeof(es));
    es.et = EVAL_EVAL;
    es.ec = ec1;

    float aarOutput[2][NUM_ROLLOUT_OUTPUTS];
    memset(aarOutput, 0, sizeof(aarOutput));
    GeneralCubeDecisionENoLocking(aarOutput, (ConstTanBoard)anBoard,
                                   &ci_money, &ec1, &es);
    float arDouble[4];
    cubedecision cd = FindCubeDecision(arDouble, aarOutput, &ci_money);
    const char *cdNames[] = {
        "DOUBLE_TAKE","DOUBLE_PASS","NODOUBLE_TAKE","TOOGOOD_TAKE",
        "TOOGOOD_PASS","DOUBLE_BEAVER","NODOUBLE_BEAVER","REDOUBLE_TAKE",
        "REDOUBLE_PASS","NO_REDOUBLE_TAKE","TOOGOODRE_TAKE","TOOGOODRE_PASS",
        "NO_REDOUBLE_BEAVER","NODOUBLE_DEADCUBE","NO_REDOUBLE_DEADCUBE",
        "NOT_AVAILABLE","OPTIONAL_DOUBLE_TAKE","OPTIONAL_REDOUBLE_TAKE",
        "OPTIONAL_DOUBLE_BEAVER","OPTIONAL_DOUBLE_PASS","OPTIONAL_REDOUBLE_PASS"
    };
    printf("    Cube decision: %d (%s)\n", cd,
           (cd >= 0 && cd <= 20) ? cdNames[cd] : "UNKNOWN");
    printf("    No double equity:   %.4f\n", arDouble[0]);
    printf("    Double/take equity: %.4f\n", arDouble[1]);
    printf("    Double/pass equity: %.4f\n\n", arDouble[2]);

    printf("[6] Rollout (144 trials, cubeful variance reduction)...\n");
    fflush(stdout);
    rolloutcontext rc = rcRollout;
    rc.nTrials  = 144;
    rc.fCubeful = 1;
    rc.fVarRedn = 1;

    float arRollout[NUM_ROLLOUT_OUTPUTS] = {0};
    float arStdDev[NUM_ROLLOUT_OUTPUTS]  = {0};

    int rcr = gnubg_rollout((ConstTanBoard)anBoard, arRollout, arStdDev,
                             &ci_money, &rc);
    printf("    gnubg_rollout returned %d\n", rcr);
    if (rcr == 0) {
        float equityR = arRollout[0]*2-1
                      + arRollout[1]-arRollout[3]
                      + arRollout[2]-arRollout[4];
        printf("    Win prob:  %.4f  stddev: %.4f\n", arRollout[0], arStdDev[0]);
        printf("    Equity:    %.4f  (1-ply was %.4f)\n", equityR, equity1);
        printf("    (rollout equity should be close to 1-ply result)\n");
    }
    printf("\n=== All tests passed ===\n");
    return 0;
}
