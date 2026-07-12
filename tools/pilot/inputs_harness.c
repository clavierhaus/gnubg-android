/*
 * inputs_harness.c -- §9.4 signal-discovery pilot (CORPUS_HARVEST_PLAN).
 *
 * Host-side harness over THIS repository's engine-core -- the same eval.c the
 * APK runs -- so measured signatures share provenance with the device verb
 * gnubg_mobile_position_features. Prints, for a board given in gnubg's
 * "set board simple" convention (26 signed ints: bar, points 1..24, bar;
 * positive = player on roll), gnubg's own:
 *   - PipCount            (eval.h:373)
 *   - ClassifyPosition    (eval.h:413, VARIATION_STANDARD -- same choice as
 *                          the device verb gnubg_mobile_classify)
 *   - CalculateHalfInputs (eval.h:620) for both sides, every input named
 *     with its I_* identifier from the eval.h:553 enum.
 *
 * No logic of ours: marshalling, three calls, printf. GPLv3+ like the tree.
 *
 * Build (from repo root; see tools/pilot/build.sh):
 *   gcc -DHAVE_CONFIG_H -I tools/shim -I jni-bridge -I jni-bridge/include \
 *       -I engine-core -I engine-core/lib $(pkg-config --cflags glib-2.0) \
 *       tools/pilot/inputs_harness.c tools/pilot/pilot_stubs.c \
 *       engine-core/eval.o ... -lm -o tools/pilot/inputs_harness
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "eval.h"
#include "positionid.h"

/* The I_* names, in enum order (eval.h:553). Kept adjacent to a size check so
 * an upstream enum change breaks THIS file loudly instead of shifting labels. */
static const char *INPUT_NAME[] = {
    "I_OFF1", "I_OFF2", "I_OFF3",
    "I_BREAK_CONTACT", "I_BACK_CHEQUER", "I_BACK_ANCHOR", "I_FORWARD_ANCHOR",
    "I_PIPLOSS", "I_P1", "I_P2", "I_BACKESCAPES",
    "I_ACONTAIN", "I_ACONTAIN2", "I_CONTAIN", "I_CONTAIN2",
    "I_MOBILITY", "I_MOMENT2", "I_ENTER", "I_ENTER2",
    "I_TIMING", "I_BACKBONE", "I_BACKG", "I_BACKG1",
    "I_FREEPIP", "I_BACKRESCAPES"
};

_Static_assert(sizeof(INPUT_NAME) / sizeof(INPUT_NAME[0]) == MORE_INPUTS,
               "I_* name table out of step with eval.h enum");

static const char *CLASS_NAME[] = {
    "CLASS_OVER", "CLASS_HYPERGAMMON1", "CLASS_HYPERGAMMON2",
    "CLASS_HYPERGAMMON3", "CLASS_BEAROFF2", "CLASS_BEAROFF_TS",
    "CLASS_BEAROFF1", "CLASS_BEAROFF_OS", "CLASS_RACE", "CLASS_CRASHED",
    "CLASS_CONTACT"
};

int main(int argc, char **argv)
{
    TanBoard anBoard;
    unsigned int anPips[2];
    float in0[MORE_INPUTS], in1[MORE_INPUTS];
    int simple[26];
    int i;

    if (argc != 2 + 26 && argc != 1 + 26) {
        fprintf(stderr,
            "usage: %s [label] b25 p1 .. p24 b0   (26 ints, gnubg "
            "'set board simple' order; + = player on roll)\n", argv[0]);
        return 2;
    }
    {
        int off = argc - 26;   /* optional label in argv[1] */
        for (i = 0; i < 26; i++)
            simple[i] = atoi(argv[off + i]);
        if (off == 2)
            printf("== %s ==\n", argv[1]);
    }

    /* gnubg 'simple' order is: opponent bar, points 1..24 from the player's
     * view, player bar. Split by sign into the two TanBoard halves --
     * anBoard[1] = player on roll, anBoard[0] = opponent, each from its own
     * direction. Pure marshalling, same convention set_board parses. */
    memset(anBoard, 0, sizeof(anBoard));
    for (i = 0; i < 24; i++) {
        int v = simple[1 + i];
        if (v > 0)
            anBoard[1][i] = (unsigned int) v;
        else if (v < 0)
            anBoard[0][23 - i] = (unsigned int) (-v);
    }
    anBoard[0][24] = (unsigned int) abs(simple[0]);   /* opponent on bar */
    anBoard[1][24] = (unsigned int) abs(simple[25]);  /* player on bar   */

    PipCount((ConstTanBoard) anBoard, anPips);
    printf("PipCount        player %u   opp %u\n", anPips[1], anPips[0]);

    {
        positionclass pc =
            ClassifyPosition((ConstTanBoard) anBoard, VARIATION_STANDARD);
        printf("positionclass   %d (%s)\n", (int) pc,
               (pc >= 0 && pc <= CLASS_CONTACT) ? CLASS_NAME[pc] : "?");
    }

    CalculateHalfInputs(anBoard[1], anBoard[0], in0);   /* player's inputs */
    CalculateHalfInputs(anBoard[0], anBoard[1], in1);   /* opponent's      */

    printf("%-16s %12s %12s\n", "input", "me", "opp");
    for (i = 0; i < MORE_INPUTS; i++)
        printf("%-16s %12.6f %12.6f\n", INPUT_NAME[i],
               (double) in0[i], (double) in1[i]);

    return 0;
}
