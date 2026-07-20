/*
 * quorum_harness.c -- amendment 2, Step 1 (VERBOSE_COACHING_DESIGN.md sec 5).
 *
 * Host-side candidate generator over THIS repo's engine-core -- the same
 * eval.c + weights the APK runs -- so the candidate list and its evaluations
 * share provenance with the device verb gnubg_mobile_hint_moves. Unlike the
 * §9.4 pilot (feature extraction only, evaluator stubbed), this REALLY
 * evaluates: it loads the neural net via EvalInitialise and calls gnubg's own
 * FindnSaveBestMoves. GPLv3+ like the tree.
 *
 * Usage:
 *   quorum_harness <weights_path> <d0> <d1> <board26...>
 * where board26 is gnubg "set board simple" (26 signed ints: bar, 1..24, bar;
 * positive = player on roll). Prints, best first, each candidate's anMove and
 * cubeless equity, at the reference evalcontext set below.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "eval.h"
#include "positionid.h"

/* Parse 26 signed ints (set board simple) into a TanBoard (mover = player 0). */
static void parse_board(char **argv, TanBoard anBoard)
{
    /* gnubg simple: index 0 = mover's bar, 1..24 points, 25 = opp bar.
     * TanBoard[0] = player-on-roll checkers by point, [1] = opponent. This
     * mirrors the facade's facade_unpack_board convention. */
    int raw[26];
    for (int i = 0; i < 26; i++) raw[i] = atoi(argv[i]);
    memset(anBoard, 0, sizeof(TanBoard));
    for (int i = 0; i < 24; i++) {
        int v = raw[i + 1];
        if (v > 0)      anBoard[0][i] = v;        /* mover's point i (0-based) */
        else if (v < 0) anBoard[1][23 - i] = -v;  /* opp, mirrored */
    }
    anBoard[0][24] = raw[0]  > 0 ? raw[0]  : 0;    /* mover bar  */
    anBoard[1][24] = raw[25] < 0 ? -raw[25] : 0;   /* opp bar    */
}

int main(int argc, char **argv)
{
    if (argc < 3 + 26) {
        fprintf(stderr, "usage: %s <weights> <d0> <d1> <board26...>\n", argv[0]);
        return 2;
    }
    const char *weights = argv[1];
    int d0 = atoi(argv[2]), d1 = atoi(argv[3]);

    /* Real neural net -- the whole point of this harness vs the pilot. */
    EvalInitialise((char *) weights, NULL, 0, NULL);

    TanBoard anBoard;
    parse_board(argv + 3, anBoard);   /* argv[4..29] = board26; +3 aligns [1]->d0 slot */

    /* Reference evalcontext: 0-ply, deterministic, no noise -- matches the
     * verdict's own setting (esAnalysisChequer at expert). The AUTHORITY run
     * (Step 4) will raise nPlies; Step 1 just needs candidates to exist and
     * match the device. */
    evalcontext ec = { .fCubeful = FALSE, .nPlies = 0, .fUsePrune = FALSE,
                       .fDeterministic = TRUE, .rNoise = 0.0f };
    cubeinfo ci;
    /* Money game, centred cube, mover on roll -- the neutral frame for pure
     * chequer-play candidate ranking. */
    SetCubeInfoMoney(&ci, 1, 0, 0, FALSE, FALSE, VARIATION_STANDARD);

    movelist ml;
    movefilter aamf[MAX_FILTER_PLIES][MAX_FILTER_PLIES];
    memcpy(aamf, defaultFilters, sizeof(aamf));

    if (FindnSaveBestMoves(&ml, d0, d1, (ConstTanBoard) anBoard, NULL, TRUE,
                           0.0f, &ci, &ec, aamf) < 0) {
        fprintf(stderr, "FindnSaveBestMoves failed\n");
        return 1;
    }

    printf("candidates=%u dice=%d,%d\n", ml.cMoves, d0, d1);
    for (unsigned int i = 0; i < ml.cMoves; i++) {
        move *m = &ml.amMoves[i];
        printf("rank=%u eq=%.6f anMove=", i, m->rScore);
        for (int j = 0; j < 8; j++) printf("%d%s", m->anMove[j], j < 7 ? "," : "");
        printf("\n");
    }
    return 0;
}
