/* Opening-book conformance harness -- host build of the port's engine.
 *
 * Question it answers: does THIS engine (engine-core as vendored, the
 * shipped weights, the shipped default MET) reproduce the numbers in the
 * BackgammonDB opening book (github.com/chalkstreamdev/bgdb-opening-book,
 * data CC-BY-4.0, ground with gnubg 1.08.003)? The book gates on the
 * `gnubg --version` string; this harness lets us gate on agreement instead,
 * per position, with gnubg as the authority on both sides.
 *
 * PORT: the book's worker (worker.py eval_checker) takes each play's
 * probabilities from a CUBELESS 3-ply hint pass and its equity from a
 * CUBEFUL 3-ply hint pass; hint's per-play evaluation is ScoreMove
 * (eval.c:5454). This harness therefore calls exactly that: GenerateMoves
 * (eval.h:407) for the legal plays of the installed position, then ScoreMove
 * twice per play with the book's two evaluation contexts (worker.py
 * eval_context: plies N, cubeful 0/1, deterministic 1, prune 1, noise 0),
 * under GetMatchStateCubeInfo (backgammon.h:500). The two evalcontexts are the
 * BOOK's published settings reproduced for the comparison -- test fixtures,
 * never an evaluation the app performs. Nothing here reads the board or
 * interprets a position.
 *
 * Usage: conformance <xgid> [plies] [met-xml-path]
 * Prints one line per legal play, gnubg's order:
 *   PLAY <anMove x8> P <win winG winBG loseG loseBG> CL <rScore2> CF <rScore>
 * anMove is gnubg's internal encoding (0-based points, 24 = bar, -1 = off).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "backgammon.h"

extern int gnubg_mobile_initialise(const char *weights_path);
extern int gnubg_mobile_set_gnubg_id(const char *id);
extern int gnubg_mobile_set_met(const char *path);

int main(int argc, char **argv) {
    const char *xgid;
    int plies;
    movelist ml;
    cubeinfo ci;
    evalcontext ecCubeless, ecCubeful;
    unsigned int i;
    int j;

    if (argc < 2) {
        fprintf(stderr, "usage: %s <xgid> [plies]\n", argv[0]);
        return 2;
    }
    xgid = argv[1];
    plies = argc > 2 ? atoi(argv[2]) : 3;

    if (gnubg_mobile_initialise("gnubg-app/app/src/main/assets/gnubg.weights") != 1) {
        fprintf(stderr, "init failed\n");
        return 1;
    }
    /* The app's default MET is Kazaross-XG2 (GameSettings.kt), which is also
     * desktop gnubg's default (gnubg.c: BuildFilename2("met",
     * "Kazaross-XG2.xml")) and therefore the book's. gnubg_mobile_initialise
     * alone leaves the built-in Zadeh table; the app installs the setting at
     * startup, so the harness must too. Match-play probabilities at ply > 0
     * depend on the table through the lookahead's reply selection, and the
     * cubeful equity through mwc2eq -- measured 2026-09-26: Zadeh vs
     * Kazaross moved probabilities by ~1e-4 and equities by ~0.010. */
    gnubg_mobile_set_met(argc > 3 ? argv[3] : "gnubg-app/app/src/main/assets/met/Kazaross-XG2.xml");
    {
        int rc = gnubg_mobile_set_gnubg_id(xgid);
        if (rc != 0 && rc != 2) {   /* SetGNUbgID: 0 or 2 = board installed */
            fprintf(stderr, "set_gnubg_id rc=%d\n", rc);
            return 1;
        }
    }
    if (ms.gs != GAME_PLAYING || !ms.anDice[0] || !ms.anDice[1]) {
        fprintf(stderr, "position has no dice rolled (cube entries are not compared here)\n");
        return 1;
    }

    /* The book's two contexts, verbatim from worker.py eval_context. */
    memset(&ecCubeless, 0, sizeof(ecCubeless));
    ecCubeless.nPlies = (unsigned int) plies;
    ecCubeless.fCubeful = 0;
    ecCubeless.fUsePrune = 1;
    ecCubeless.fDeterministic = 1;
    ecCubeless.rNoise = 0.0f;
    ecCubeful = ecCubeless;
    ecCubeful.fCubeful = 1;

    GetMatchStateCubeInfo(&ci, &ms);
    memset(&ml, 0, sizeof(ml));
    GenerateMoves(&ml, (ConstTanBoard) ms.anBoard, (int) ms.anDice[0], (int) ms.anDice[1], FALSE);
    /* GenerateMoves returns internal pointers to static data that a deeper
     * ply's move generation overwrites; FindnSaveBestMoves copies the list
     * before scoring (eval.c "Save moves", g_memdup2) and so does this. */
    if (ml.cMoves > 0)
        ml.amMoves = (move *) g_memdup2(ml.amMoves, ml.cMoves * sizeof(move));
    else
        ml.amMoves = NULL;

    printf("XGID %s PLIES %d MATCHTO %d SCORE %d %d CUBE %d OWNER %d JACOBY %d LEGAL %u\n",
           xgid, plies, ms.nMatchTo, ms.anScore[0], ms.anScore[1], ms.nCube,
           ms.fCubeOwner, ms.fJacoby, ml.cMoves);

    for (i = 0; i < ml.cMoves; i++) {
        move *pm = &ml.amMoves[i];
        float probs[5];
        float cl, cf;
        if (ScoreMove(NULL, pm, &ci, &ecCubeless, plies) < 0) {
            fprintf(stderr, "ScoreMove (cubeless) failed on play %u\n", i);
            return 1;
        }
        memcpy(probs, pm->arEvalMove, 5 * sizeof(float));
        cl = pm->rScore2;
        if (ScoreMove(NULL, pm, &ci, &ecCubeful, plies) < 0) {
            fprintf(stderr, "ScoreMove (cubeful) failed on play %u\n", i);
            return 1;
        }
        cf = pm->rScore;
        printf("PLAY");
        for (j = 0; j < 8; j++) printf(" %d", pm->anMove[j]);
        printf(" P %.6f %.6f %.6f %.6f %.6f CL %.6f CF %.6f\n",
               probs[0], probs[1], probs[2], probs[3], probs[4], cl, cf);
    }
    g_free(ml.amMoves);
    return 0;
}
