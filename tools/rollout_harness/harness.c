/* Rollout test harness -- host build of the port's rollout core.
 *
 * Purpose (docs/MULTICORE_ANALYSIS.md 2.10): automated confirmation that
 * the pool's handling is correct BEFORE any device is involved, and --
 * with the distribution's desktop gnubg installed beside it -- an
 * AUTOMATED GATE B: same position, same seed, same trials, our numbers
 * asserted against the reference implementation's.
 *
 * Usage: harness <gnubg-position-id> <seed> [maxCand]
 * Prints one line per rolled candidate:
 *   CAND <k> <moveText-free 8 ints> EQ <rScore> DONE <n>
 * plus MU/SIGMA lines (7 floats each) for field-level comparison, and a
 * SEED line echoing the seed actually used.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "backgammon.h"
#include "rollout.h"

extern int  gnubg_mobile_initialise(const char *weights_path);
extern int  gnubg_mobile_set_gnubg_id(const char *id);
extern int  gnubg_mobile_rollout_start(int max_n, unsigned int seed);
extern int  gnubg_mobile_rollout_status(int out[206]);

static float fbits(int v) { union { int i; float f; } u; u.i = v; return u.f; }

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "usage: %s <gnubg-id> <seed> [maxCand]\n", argv[0]);
        return 2;
    }
    const char *id = argv[1];
    unsigned int seed = (unsigned int) strtoul(argv[2], NULL, 10);
    int maxCand = argc > 3 ? atoi(argv[3]) : 3;

    /* Optional trials override for fast tests -- Gate B runs the SAME count
     * on both sides, so any matched N is a valid comparison. */
    extern rolloutcontext rcRollout;
    if (argc > 4) rcRollout.nTrials = (unsigned int) atoi(argv[4]);

    if (gnubg_mobile_initialise("gnubg-app/app/src/main/assets/gnubg.weights") != 1) {
        fprintf(stderr, "init failed\n"); return 1;
    }
    {
        int rc = gnubg_mobile_set_gnubg_id(id);
        if (rc != 0 && rc != 2) {   /* SetGNUbgID: 0 or 2 = board installed */
            fprintf(stderr, "set_gnubg_id rc=%d\n", rc); return 1;
        }
    }
    /* Mode "pos": roll the POSITION itself (gnubg_rollout core, no move
     * driver) -- the directly desktop-comparable path, since the CLI's
     * `rollout' rolls positions. maxCand < 0 selects it. */
    if (maxCand < 0) {
        extern matchstate ms;
        extern int gnubg_rollout(const TanBoard anBoard, float arOutput[7],
                                 float arStdDev[7], const cubeinfo *pci,
                                 rolloutcontext *prc);
        cubeinfo ci;
        float mu[7], sig[7];
        int nd, j;
        GetMatchStateCubeInfo(&ci, &ms);
        rcRollout.nSeed = seed;
        nd = gnubg_rollout((ConstTanBoard) ms.anBoard, mu, sig, &ci, &rcRollout);
        printf("POS SEED %u TRIALS %u DONE %d\nMU  ", seed, rcRollout.nTrials, nd);
        for (j = 0; j < 7; j++) printf(" %.6f", mu[j]);
        printf("\nSIG ");
        for (j = 0; j < 7; j++) printf(" %.6f", sig[j]);
        printf("\n");
        return nd > 0 ? 0 : 1;
    }

    int n = gnubg_mobile_rollout_start(maxCand, seed);
    if (n <= 0) { fprintf(stderr, "rollout_start=%d\n", n); return 1; }

    int st[206];
    if (gnubg_mobile_rollout_status(st) <= 0) { fprintf(stderr, "status failed\n"); return 1; }
    printf("SEED %d TRIALS %d NCAND %d\n", st[3], st[4], st[1]);
    for (int k = 0; k < st[1]; k++) {
        int base = 6 + k * 25;
        printf("CAND %d MOVE", k);
        for (int j = 0; j < 8; j++) printf(" %d", st[base + j]);
        printf(" EQ %.6f DONE %d\n", fbits(st[base + 8]), st[base + 10]);
        printf("MU   ");
        for (int j = 0; j < 7; j++) printf(" %.6f", fbits(st[base + 11 + j]));
        printf("\nSIG  ");
        for (int j = 0; j < 7; j++) printf(" %.6f", fbits(st[base + 18 + j]));
        printf("\n");
    }
    return 0;
}
